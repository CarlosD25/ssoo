// Parcial: Relajación de Jacobi en paralelo usando memoria compartida
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>

// Prototipos
int** read_file(const char*, int*, int*);
unsigned int sizeof_dm(int, int, size_t);
void create_index(void**, int, int, size_t);
int value(int**, int, int);
void copyM(int**, int**, int, int);

int main(int argc, char const *argv[]) {
    if (argc < 4) {
        perror("Uso: ./programa N_HIJOS ITERACIONES archivo.txt\n");
        return EXIT_FAILURE;
    }

    int n_childs = atoi(argv[1]);   // Número de hijos
    int t = atoi(argv[2]);          // Iteraciones de Jacobi
    pid_t father = getpid();        // PID del proceso padre

    int rows, col;
    int** tem = read_file(argv[3], &rows, &col); // Leer matriz del archivo

    // Crear memoria compartida para la matriz actual (matL)
    int **matL = NULL;
    size_t sizeMatl = sizeof_dm(rows, col, sizeof(int));
    int shmIdl = shmget(IPC_PRIVATE, sizeMatl, IPC_CREAT | 0600);
    if (shmIdl == -1) {
        perror("Error en shmget (matL)");
        exit(1);
    }
    matL = shmat(shmIdl, NULL, 0);
    if (matL == (void *)-1) {
        perror("Error en shmat (matL)");
        exit(1);
    }
    create_index((void*)matL, rows, col, sizeof(int)); // Índices 2D

    // Crear memoria compartida para la matriz de siguiente estado (matE)
    int** matE = NULL;
    size_t sizeMatE = sizeof_dm(rows, col, sizeof(int));
    int shmIde = shmget(IPC_PRIVATE, sizeMatE, IPC_CREAT | 0600);
    if (shmIde == -1) {
        perror("Error en shmget (matE)");
        exit(1);
    }
    matE = shmat(shmIde, NULL, 0);
    if (matE == (void *)-1) {
        perror("Error en shmat (matE)");
        exit(1);
    }
    create_index((void*)matE, rows, col, sizeof(int)); // Índices 2D

    // Copiar matriz leída a matL
    copyM(tem, matL, rows, col);
    free(tem); // Liberar matriz original

    // Crear memoria compartida para vector de sincronización
    int shmidV = shmget(IPC_PRIVATE, sizeof(int) * n_childs, IPC_CREAT | 0600);
    if (shmidV == -1) {
        perror("Error en shmget (vector)");
        exit(1);
    }
    int* vec = (int*) shmat(shmidV, NULL, 0);
    if (vec == (void *)-1) {
        perror("Error en shmat (vector)");
        exit(1);
    }

    // Inicializar vector de sincronización en 0
    for (int i = 0; i < n_childs; i++) {
        vec[i] = 0;
    }

    // Crear procesos hijos
    int idx;
    int start, end;
    int half = rows / n_childs;
    for (int i = 0; i < n_childs; i++) {
        if (!fork()) {
            idx = i;
            start = i * half;
            end = (i == n_childs - 1) ? rows : start + half;
            break;
        }
    }

    // Bucle principal: iteraciones de Jacobi
    for (int i = 0; i < t; i++) {
        if (getpid() == father) { // Proceso padre
            // Esperar que todos los hijos terminen
            while (1) {
                int sum = 0;
                for (int i = 0; i < n_childs; i++) {
                    sum += vec[i];
                }
                if (sum == n_childs) break; // Todos terminaron
            }

            // Imprimir matriz actual
            printf("Iteración %d:\n", i + 1);
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < col; j++) {
                    printf("%d\t", matL[i][j]);
                }
                printf("\n");
            }

            // Actualizar matL con valores de matE (excepto bordes)
            for (int i = 1; i < rows - 1; i++) {
                for (int j = 1; j < col - 1; j++) {
                    matL[i][j] = matE[i][j];
                }
            }

            // Resetear vector de sincronización
            for (int i = 0; i < n_childs; i++) {
                vec[i] = 0;
            }

        } else { // Proceso hijo
            // Esperar que el padre reinicie el vector
            while (vec[idx] != 0) {}

            // Calcular nuevos valores en la zona asignada (sin tocar bordes)
            for (int i = start; i < end; i++) {
                for (int j = 0; j < col; j++) {
                    if ((i == 0 || i == rows - 1) || (j == 0 || j == col - 1)) {
                        continue;
                    } else {
                        matE[i][j] = value(matL, i, j);
                    }
                }
            }
            vec[idx] = 1; // Señalar que el hijo terminó
        }
    }

    // Terminación y limpieza
    if (getpid() == father) {
        // Esperar a los hijos
        for (int i = 0; i < n_childs; i++) {
            wait(NULL);
        }

        // Liberar recursos compartidos
        shmdt(matL);
        shmctl(shmIdl, IPC_RMID, NULL);
        shmdt(matE);
        shmctl(shmIde, IPC_RMID, NULL);
        shmdt(vec);
        shmctl(shmidV, IPC_RMID, NULL);

    } else {
        // Hijos sólo desconectan la memoria
        shmdt(matL);
        shmdt(matE);
        shmdt(vec);
        exit(0);
    }

    return 0;
}

// Leer matriz desde archivo y devolver matriz dinámica
int** read_file(const char* filename, int* rows, int* col) {
    FILE* file = fopen(filename, "r");
    if (!file) exit(1);

    fscanf(file, "%d", rows);
    fscanf(file, "%d", col);

    int** mcopy = calloc(*rows, sizeof(int*));
    for (int i = 0; i < *rows; i++) {
        mcopy[i] = calloc(*col, sizeof(int));
    }

    for (int i = 0; i < *rows; i++) {
        for (int j = 0; j < *col; j++) {
            fscanf(file, "%d", &mcopy[i][j]);
        }
    }
    fclose(file);
    return mcopy;
}

// Calcular tamaño total necesario para una matriz dinámica en memoria compartida
unsigned int sizeof_dm(int rows, int cols, size_t sizeElement) {
    size_t size;
    size = rows * sizeof(void *);                // Punteros de fila
    size += (cols * rows * sizeElement);         // Datos reales
    return size;
}

// Crear índices para una matriz en memoria compartida
void create_index(void **m, int rows, int cols, size_t sizeElement) {
    int i;
    size_t sizeRow = cols * sizeElement;
    m[0] = m + rows;
    for (i = 1; i < rows; i++) {
        m[i] = (m[i - 1] + sizeRow);
    }
}

// Calcular nuevo valor de una celda (promedio de vecinos ortogonales)
int value(int** matL, int posx, int posy) {
    int top = matL[posx - 1][posy];
    int left = matL[posx][posy - 1];
    int right = matL[posx][posy + 1];
    int bottom = matL[posx + 1][posy];

    return (0.25) * (top + left + right + bottom);
}

// Copiar matriz tem a matL
void copyM(int** tem, int** matL, int rows, int col) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < col; j++) {
            matL[i][j] = tem[i][j];
        }
    }
}
