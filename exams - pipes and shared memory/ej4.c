// Taller covid 24
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdbool.h>

#define salto 1  // Rango de vecindad (distancia de exploración para infección o recuperación)

// Declaración de funciones auxiliares
unsigned int sizeof_dm(int, size_t);
void create_index(void**, int, size_t);
bool recuperado(int**, int, int, int, int);
bool infectado(int**, int, int, int, int);

int main(int argc, char const *argv[]) {
    
    // Abrimos archivo que contiene configuración inicial
    FILE* file = fopen(argv[1], "r");
    int dias;      // Cantidad de días de simulación
    int n_hijos;   // Número de procesos hijos (división por filas)
    int tam;       // Tamaño de la matriz

    pid_t padre = getpid(); // ID del proceso padre (actual)

    // Leemos parámetros del archivo
    fscanf(file, "%d", &dias);
    fscanf(file, "%d", &n_hijos);
    fscanf(file, "%d", &tam);

    // Arreglo para guardar los PID de los hijos
    pid_t* hijos = calloc(n_hijos, sizeof(pid_t));

    // === Crear matrices en memoria compartida ===
    int** matrizL = NULL;  // Matriz de lectura (estado actual)
    size_t totalSizeA = sizeof_dm(tam, sizeof(int));
    int shmidA = shmget(IPC_PRIVATE, totalSizeA, IPC_CREAT | 0600);
    matrizL = shmat(shmidA, NULL, 0);
    create_index((void*)matrizL, tam, sizeof(int));

    int** matrizE = NULL;  // Matriz de escritura (estado siguiente)
    size_t totalSizeB = sizeof_dm(tam, sizeof(int));
    int shmidB = shmget(IPC_PRIVATE, totalSizeB, IPC_CREAT | 0600);
    matrizE = shmat(shmidB, NULL, 0);
    create_index((void*)matrizE, tam, sizeof(int));
    // ============================================

    // === Vector contador para sincronización en memoria compartida ===
    int shmidV = shmget(IPC_PRIVATE, sizeof(int) * n_hijos, IPC_CREAT | 0600);
    int* contador = (int*)shmat(shmidV, NULL, 0);
    
    for (int i = 0; i < n_hijos; i++) {
        contador[i] = 0;  // Inicializamos a 0
    }

    // === Leer los valores iniciales de la matriz desde el archivo ===
    for (int i = 0; i < tam; i++) {
        for (int j = 0; j < tam; j++) {
            fscanf(file, "%d", &matrizL[i][j]);
        }
    }

    fclose(file);

    // === Crear procesos hijos ===
    int ini;  // Índice de inicio de fila
    int fin;  // Índice final de fila
    int delta = tam / n_hijos;  // División de filas
    int idx;  // Índice del hijo

    for (int i = 0; i < n_hijos; i++) {
        hijos[i] = fork();
        if (!hijos[i]) {
            idx = i;
            ini = i * delta;
            fin = (i == n_hijos - 1) ? tam : ini + delta;
            break;  // El hijo sale del ciclo
        }
    }

    // === Comienza simulación por días ===
    for (int i = 0; i < dias; i++) {
        if (getpid() == padre) {
            // === PROCESO PADRE ===

            // Esperar a que todos los hijos terminen su cálculo del día
            while (1) {
                int sum = 0;
                for (int i = 0; i < n_hijos; i++) {
                    sum += contador[i];
                }
                if (sum == n_hijos) break;  // Todos han terminado
            }

            // Imprimir la matriz actual
            printf("Poblacion dia n: %d \n", i + 1);
            for (int i = 0; i < tam; i++) {
                for (int j = 0; j < tam; j++) {
                    printf("%d\t", matrizL[i][j]);
                }
                printf("\n");
            }

            // Copiar el estado calculado a la matriz de lectura
            for (int i = 0; i < tam; i++) {
                for (int j = 0; j < tam; j++) {
                    matrizL[i][j] = matrizE[i][j];
                }
            }

            // Reiniciar contadores para el próximo día
            for (int i = 0; i < n_hijos; i++) {
                contador[i] = 0;
            }

        } else {
            // === PROCESO HIJO ===

            // Esperar a que el padre reinicie el contador
            while (contador[idx] != 0) {}

            // Procesar su sección de la matriz
            for (int i = ini; i < fin; i++) {
                for (int j = 0; j < tam; j++) {
                    if (matrizL[i][j] == 0) {
                        // Si está sano y se infecta
                        if (infectado(matrizL, tam, i, j, salto)) {
                            matrizE[i][j] = 1;
                        } else {
                            matrizE[i][j] = matrizL[i][j];
                        }
                    } else if (matrizL[i][j] == 1) {
                        // Si está infectado y puede recuperarse
                        if (recuperado(matrizL, tam, i, j, salto)) {
                            matrizE[i][j] = 2;
                        } else {
                            matrizE[i][j] = matrizL[i][j];
                        }
                    } else {
                        // Si ya está recuperado
                        matrizE[i][j] = matrizL[i][j];
                    }
                }
            }

            // Marcar que ya terminó su parte
            contador[idx] = 1;
        }
    }

    return 0;
}

// === Calcular tamaño total de memoria para matriz dinámica ===
unsigned int sizeof_dm(int tam, size_t sizeElement) {
    size_t size = tam * sizeof(void*);           // Punteros a filas
    size += (tam * tam * sizeElement);           // Datos reales
    return size;
}

// === Crear índices de matriz (arreglo de punteros a filas) ===
void create_index(void** m, int tam, size_t sizeElement) {
    int i;
    size_t sizeRow = tam * sizeElement;
    m[0] = m + tam;  // Primer puntero apunta al bloque de datos
    for (i = 1; i < tam; i++) {
        m[i] = m[i - 1] + sizeRow;  // Cada fila salta una línea completa
    }
}

// === Señal vacía (sin uso en este código) ===
void handler(int s) {}

// === Verifica si un individuo debe infectarse ===
bool infectado(int** matrizL, int tam, int posx, int posy, int saltar) {
    int infectado = 0;
    for (int i = posx - saltar; i <= posx + saltar; i++) {
        for (int j = posy - saltar; j <= posy + saltar; j++) {
            if ((i >= 0 && i < tam) && (j >= 0 && j < tam)) {
                if (matrizL[i][j] == 1) {
                    infectado++;
                }
            }
        }
    }
    return infectado >= 2;  // Se infecta si tiene al menos 2 vecinos infectados
}

// === Verifica si un individuo se recupera con cierta probabilidad ===
bool recuperado(int** matrizL, int tam, int posx, int posy, int saltar) {
    int recuperado = (float)rand() / (RAND_MAX);  // Número aleatorio [0,1]
    float vecinosR = 0;
    float probabilidadR = 0;

    // Contar vecinos recuperados
    for (int i = posx - saltar; i <= posx + saltar; i++) {
        for (int j = posy - saltar; j <= posy + saltar; j++) {
            if ((i >= 0 && i < tam) && (j >= 0 && j < tam)) {
                if (matrizL[i][j] == 2) {
                    vecinosR++;
                }
            }
        }
    }

    // Calcular probabilidad de recuperación
    probabilidadR = 0.2 + (vecinosR * 0.5);
    return (recuperado < probabilidadR);  // Se recupera si el número aleatorio es menor
}
