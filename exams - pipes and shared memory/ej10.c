/*
 * PARCIAL: Multiplicación de Matrices con Procesos e IPC (tuberías)
 *
 * Este programa multiplica dos matrices leídas desde un archivo,
 * utilizando 3 procesos hijos y comunicación mediante tuberías (pipes).
 *
 * Distribución de trabajo:
 *   - Hijo 0: calcula los elementos debajo de la diagonal principal.
 *   - Hijo 1: calcula los elementos encima de la diagonal principal.
 *   - Hijo 2: calcula los elementos sobre la diagonal principal.
 *
 * Cada hijo envía los resultados al padre a través de su tubería correspondiente.
 * El padre recolecta los valores, construye la matriz resultante y la imprime.
 */

#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <sys/types.h>
#include <unistd.h>

#define n_Hijos 3  // Número de hijos para procesar la matriz por diagonales inferiores, superiores y diagonal principal

// Estructura para representar un punto con su coordenada (x,y) y valor resultante
typedef struct point{
    int x;
    int y;
    int val;
} apoint;

// Función que lee un archivo y carga dos matrices A y B
void crearMatrices(const char*, int*, int*, int*, int*, int***, int***);

int main(int argc, char const *argv[]){
    
    // Creación de tuberías: cada hijo tiene una tubería exclusiva para comunicarse con el padre
    int** tub = calloc(n_Hijos, sizeof(int*));
    for(int i = 0; i < n_Hijos; i++){
        tub[i] = calloc(2, sizeof(int));
        pipe(tub[i]); // tub[i][0]: lectura, tub[i][1]: escritura
    }

    pid_t padre = getpid();  // PID del proceso padre

    int filasA, columnasA;
    int filasB, columnasB;
    int** matrizA;
    int** matrizB;

    // Lectura de las matrices desde el archivo
    crearMatrices(argv[1], &filasA, &columnasA, &filasB, &columnasB, &matrizA, &matrizB);

    int i = 0;
    int idx;
    for(; i < n_Hijos; i++){
        if(!fork()){  // Cada hijo rompe el bucle después de fork()
            idx = i;
            break;
        }
    }

    // Proceso padre
    if(getpid() == padre){

        // Cierra extremos de escritura de todas las tuberías
        for(int i = 0; i < n_Hijos; i++){
            close(tub[i][1]);
        }

        // Crea la matriz resultante vacía
        int** matrizResultante = calloc(filasA, sizeof(int*));
        for(int i = 0; i < filasA; i++){
            matrizResultante[i] = calloc(columnasB, sizeof(int));
        }

        apoint p;

        // Lectura de los valores enviados por cada hijo
        for(int i = 0; i < n_Hijos; i++){
            while (read(tub[i][0], &p, sizeof(p))){
                matrizResultante[p.x][p.y] = p.val;
            }
        }

        // Impresión de la matriz resultante
        for(int i = 0; i < filasA; i++){
            for(int j = 0; j < columnasB; j++){
                printf("%d\t", matrizResultante[i][j]);
            }
            printf("\n");
        }

        // Cierre de extremos de lectura de todas las tuberías
        for(int i = 0; i < n_Hijos; i++){
            close(tub[i][0]);
        }

        // Espera a que terminen todos los hijos
        for(int i = 0; i < n_Hijos; i++){
            wait(NULL);
        }

    } else {  // Procesos hijos

        // Cada hijo cierra extremos de lectura de las tuberías
        for(int i = 0; i < n_Hijos; i++){
            close(tub[i][0]);
        }

        apoint p;

        // Hijo 0: calcula valores por debajo de la diagonal (i > j)
        if(idx == 0){
            for(int i = 1; i < filasA; i++){
                for(int j = 0; j < i; j++){
                    int pro = 0;
                    for(int k = 0; k < columnasA; k++){
                        pro += matrizA[i][k] * matrizB[k][j];
                    }
                    p.val = pro;
                    p.x = i;
                    p.y = j;
                    write(tub[idx][1], &p, sizeof(p));
                }
            }

        // Hijo 1: calcula valores por encima de la diagonal (i < j)
        } else if(idx == 1){
            for(int i = 1; i < columnasA; i++){
                for(int j = 0; j < i; j++){
                    int pro = 0;
                    for(int k = 0; k < columnasA; k++){
                        pro += matrizA[j][k] * matrizB[k][i];
                    }
                    p.val = pro;
                    p.x = j;
                    p.y = i;
                    write(tub[idx][1], &p, sizeof(p));
                }
            }

        // Hijo 2: calcula los valores de la diagonal principal (i == j)
        } else {
            for(int i = 0; i < columnasA; i++){
                int pro = 0;
                for(int k = 0; k < columnasA; k++){
                    pro += matrizA[i][k] * matrizB[k][i];
                }
                p.val = pro;
                p.x = i;
                p.y = i;
                write(tub[idx][1], &p, sizeof(p));
            }
        }

        // Cierre de extremos de escritura
        for(int i = 0; i < n_Hijos; i++){
            close(tub[i][1]);
        }

        // Fin del proceso hijo
        exit(0);
    }

    return 0;
}

// Función que lee un archivo y carga dos matrices en memoria dinámica
void crearMatrices(const char* filename, int* filasA, int* columnasA, int* filasB, int* columnasB, int*** matrizA, int*** matrizB){

    FILE* file = fopen(filename, "r");

    // Lectura de matriz A
    fscanf(file, "%d", filasA);
    fscanf(file, "%d", columnasA);

    *matrizA = calloc(*filasA, sizeof(int*));
    for(int i = 0; i < *filasA; i++){
        (*matrizA)[i] = calloc(*columnasA, sizeof(int));
    }

    for(int i = 0; i < *filasA; i++){
        for(int j = 0; j < *columnasA; j++){
            fscanf(file, "%d", &(*matrizA)[i][j]);
        }
    }

    // Lectura de matriz B
    fscanf(file, "%d", filasB);
    fscanf(file, "%d", columnasB);

    *matrizB = calloc(*filasB, sizeof(int*));
    for(int i = 0; i < *filasB; i++){
        (*matrizB)[i] = calloc(*columnasB, sizeof(int));
    }

    for(int i = 0; i < *filasB; i++){
        for(int j = 0; j < *columnasB; j++){
            fscanf(file, "%d", &(*matrizB)[i][j]);
        }
    }

    fclose(file);
}
