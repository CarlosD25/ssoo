// Parcial diferencias finitas
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>

// Prototipo de función para el cálculo de diferencias finitas
void calcularV(int*, int*, int, int);

int main(int argc, char const *argv[]) {

    int n_hijos = atoi(argv[1]);      // Número de hijos a crear (procesos paralelos)
    pid_t padre = getpid();           // PID del proceso padre

    FILE* file = fopen(argv[2], "r"); // Apertura del archivo de entrada

    int t;      // Número de pasos temporales
    int tam;    // Tamaño del vector

    fscanf(file, "%d", &tam); // Lectura del tamaño del vector
    fscanf(file, "%d", &t);   // Lectura del número de pasos temporales

    // Validación: el número de hijos debe coincidir con el tamaño del vector
    if (tam != n_hijos) {
        printf("La cantidad de hijos tiene que ser igual al tam del vec");
        exit(EXIT_FAILURE);
    }

    // Creación de vectores en memoria compartida:
    
    // Vector actual (vecL)
    int shmidL = shmget(IPC_PRIVATE, sizeof(int)*tam, IPC_CREAT | 0600);
    int* vecL = (int*) shmat(shmidL, NULL, 0);

    // Vector siguiente estado (vecE)
    int shmidE = shmget(IPC_PRIVATE, sizeof(int)*tam, IPC_CREAT | 0600);
    int* vecE = (int*) shmat(shmidE, NULL, 0);

    // Vector contador para sincronización entre procesos
    int shmidC = shmget(IPC_PRIVATE, sizeof(int)*n_hijos, IPC_CREAT | 0600);
    int* contador = (int*) shmat(shmidC, NULL, 0);

    // Inicializar contadores a 0
    for (int i = 0; i < n_hijos; i++) {
        contador[i] = 0;
    }

    // Leer valores del vector desde el archivo
    for (int i = 0; i < tam; i++) {
        fscanf(file, "%d", &vecL[i]);
    }

    fclose(file); // Cierre del archivo

    // Variables para manejo de rangos por proceso hijo
    int idx;        // Índice del hijo
    int ini;        // Inicio del bloque que le corresponde
    int fin;        // Fin del bloque
    int delta = tam / n_hijos; // Tamaño del bloque por hijo

    // Creación de procesos hijos
    for (int i = 0; i < n_hijos; i++) {
        if (!fork()) {  // Si es proceso hijo
            idx = i;
            ini = i * delta;
            fin = (i == n_hijos - 1) ? tam : ini + delta;
            break; // Salimos del ciclo en el hijo para que solo siga ejecutando su trabajo
        }
    }

    // Bucle principal de iteración temporal
    for (int i = 0; i < t; i++) {
        if (getpid() == padre) {
            // Proceso padre: espera a que todos los hijos terminen
            while (1) {
                int sum = 0;
                for (int i = 0; i < n_hijos; i++) {
                    sum += contador[i]; // Verifica si todos los hijos terminaron
                }
                if (sum == n_hijos) break;
            }

            // Imprimir el vector en el tiempo actual
            printf("vector tiempo t: %d\n", i + 1);
            for (int i = 0; i < tam; i++) {
                printf("%d\t", vecL[i]);
            }
            printf("\n");

            // Copiar vecE a vecL para la siguiente iteración
            for (int i = 0; i < tam; i++) {
                vecL[i] = vecE[i];
            }

            // Reiniciar los contadores para la siguiente iteración
            for (int i = 0; i < n_hijos; i++) {
                contador[i] = 0;
            }

        } else {
            // Proceso hijo: espera hasta que su contador esté en 0
            while (contador[idx] != 0) {}

            // Calcula nuevas posiciones asignadas para su rango
            for (int i = ini; i < fin; i++) {
                calcularV(vecL, vecE, i, tam);
            }

            // Señala que ha terminado su trabajo
            contador[idx] = 1;
        }
    }

    return 0;
}

// Función para calcular el valor actualizado de una posición según diferencias finitas
void calcularV(int* vectorL, int* vectorE, int pos, int tam) {
    int valor = 0;

    if (pos == 0) {
        // Borde izquierdo
        valor = 0.25 * (vectorL[pos] + vectorL[pos + 1]);
        vectorE[pos] = valor;
    } else if (pos == tam - 1) {
        // Borde derecho
        valor = 0.25 * (vectorL[pos - 1] + vectorL[pos]);
        vectorE[pos] = valor;
    } else {
        // Posiciones internas
        valor = 0.25 * (vectorL[pos - 1] + vectorL[pos] + vectorL[pos + 1]);
        vectorE[pos] = valor;
    }
}
