// Taller deepfakes
// Carlos Sánchez 2022114027
// Daniel Puerta 2022114046
// Dylan De Vega 2023114115

#include <stdlib.h>
#include <stdio.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/random.h>
#include <stdbool.h>

#define JUMP 1 // Rango de influencia para vecinos

// Declaraciones de funciones
int** read_file(const char*, int*, int*, int*, int*);
unsigned int sizeof_dm(int, int, size_t);
void create_index(void**, int, int, size_t);
void copyM(int**, int**, int, int);
bool neutro(int**, int, int, int, int, int);
bool exposed(int**, int, int, int, int, int);

int main(int argc, char const *argv[]){

    // Validación de argumentos
    if(argc < 2){
        printf("Send file\n");
        exit(EXIT_FAILURE);
    }

    pid_t father = getpid(); // PID del proceso padre
    int minutes, n_childs, rows, col;

    // Leer datos del archivo (minutos, hijos, filas, columnas, y matriz)
    int** tem = read_file(argv[1], &rows, &col, &minutes, &n_childs);

    // Validar número de hijos (debe ser 2 exactamente)
    if(n_childs < 2 || n_childs > 2){
        printf("Error: n_childs must be 2\n");
        exit(EXIT_FAILURE);
    }

    // Crear matriz lógica (matL) en memoria compartida
    int **matL = NULL;
    size_t sizeMatl = sizeof_dm(rows, col, sizeof(int));
    int shmIdl = shmget(IPC_PRIVATE, sizeMatl, IPC_CREAT | 0600);
    if (shmIdl == -1) {
        perror("Error in shmget");
        exit(1);
    }
    matL = shmat(shmIdl, NULL, 0);
    if (matL == NULL) {
        perror("Error in shmat");
        exit(1);
    }
    create_index((void*)matL, rows, col, sizeof(int));

    // Crear matriz temporal (matE) en memoria compartida
    int **matE = NULL;
    size_t sizeMatE = sizeof_dm(rows, col, sizeof(int));
    int shmIde = shmget(IPC_PRIVATE, sizeMatE, IPC_CREAT | 0600);
    if (shmIde == -1) {
        perror("Error in shmget");
        exit(1);
    }
    matE = shmat(shmIde, NULL, 0);
    if (matE == NULL) {
        perror("Error in shmat");
        exit(1);
    }
    create_index((void*)matE, rows, col, sizeof(int));

    // Copiar la matriz leída a memoria compartida
    copyM(tem, matL, rows, col);
    free(tem); // Liberar memoria local

    // Crear vector de sincronización en memoria compartida
    int shmidV = shmget(IPC_PRIVATE, sizeof(int)*n_childs, IPC_CREAT | 0600);
    if (shmidV == -1) {
        perror("Error in shmget");
        exit(1);
    }
    int* vec = (int*) shmat(shmidV, NULL, 0);
    if (vec == NULL) {
        perror("Error in shmat");
        exit(1);
    }

    // Inicializar vector de sincronización en 0
    for(int i = 0; i < n_childs; i++){
        vec[i] = 0;
    }

    // Crear hijos
    int idx;
    for(int i = 0; i < n_childs; i++){
        if(!fork()){
            idx = i;
            break;
        }
    }

    // Ciclo de simulación
    for(int i = 0; i < minutes; i++){
        if(getpid() == father){ // Proceso padre

            // Esperar a que ambos hijos terminen su ronda
            while(true){
                int sum = 0;
                for(int i = 0; i < n_childs; i++){
                    sum += vec[i];
                }
                if(sum == n_childs) break;
            }

            // Imprimir la matriz
            printf("Ronda n: %d\n", i + 1);
            for(int i = 0; i < rows; i++){
                for(int j = 0; j < col; j++){
                    printf("%d\t", matL[i][j]);
                }
                printf("\n");
            }

            // Actualizar matriz lógica con los valores de matE
            for(int i = 0; i < rows; i++){
                for(int j = 0; j < col; j++){
                    matL[i][j] = matE[i][j];
                }
            }

            // Reiniciar sincronización
            for(int i = 0; i < n_childs; i++){
                vec[i] = 0;
            }

        } else { // Procesos hijos

            while(vec[idx] != 0) {} // Esperar turno

            if(idx == 0){ // Hijo 0: procesa los neutros (0)
                for(int i = 0; i < rows; i++){
                    for(int j = 0; j < col; j++){
                        if(matL[i][j] == 0){
                            if(neutro(matL, rows, col, i, j, JUMP)){
                                matE[i][j] = 1;
                            } else {
                                matE[i][j] = matL[i][j];
                            }
                        } else if(matL[i][j] == 2){ // Ya verificado
                            matE[i][j] = matL[i][j];
                        }
                    }
                }

            } else { // Hijo 1: procesa los expuestos (1)
                for(int i = 0; i < rows; i++){
                    for(int j = 0; j < col; j++){
                        if(matL[i][j] == 1){
                            if(exposed(matL, rows, col, i, j, JUMP)){
                                matE[i][j] = 2;
                            } else {
                                matE[i][j] = matL[i][j];
                            }
                        } else if(matL[i][j] == 2){ // Ya verificado
                            matE[i][j] = matL[i][j];
                        }
                    }
                }
            }

            vec[idx] = 1; // Marcar que el hijo terminó su trabajo
        }
    }

    // Finalización
    if(getpid() != father){
        shmdt(matL);
        shmdt(matE);
        shmdt(vec);
        exit(0);

    } else {
        for(int i = 0; i < n_childs; i++){
            wait(NULL);
        }
        // Liberar memoria compartida
        shmdt(matL);
        shmctl(shmIdl, IPC_RMID, NULL);
        shmdt(matE);
        shmctl(shmIde, IPC_RMID, NULL);
        shmdt(vec);
        shmctl(shmidV, IPC_RMID, NULL);
    }

    return 0;
}

// Calcula el tamaño total de memoria dinámica para una matriz
unsigned int sizeof_dm(int rows, int cols, size_t sizeElement){
    size_t size = rows * sizeof(void *);            // Punteros a filas
    size += (cols * rows * sizeElement);            // Espacio para datos
    return size;
}

// Configura los índices para acceder a la matriz 2D en memoria compartida
void create_index(void **m, int rows, int cols, size_t sizeElement){
    int i;
    size_t sizeRow = cols * sizeElement;
    m[0] = m + rows;
    for(i = 1; i < rows; i++){
        m[i] = (m[i - 1] + sizeRow);
    }
}

// Lee el archivo de entrada y retorna una matriz local
int** read_file(const char* filename, int* rows, int* col, int* minutes, int* n_childs){
    FILE* file = fopen(filename, "r");
    if(!file){ exit(1); }

    fscanf(file, "%d", minutes);
    fscanf(file, "%d", n_childs);
    fscanf(file, "%d", rows);
    fscanf(file, "%d", col);

    int** mcopy = calloc(*rows, sizeof(int*));
    for(int i = 0; i < *rows; i++){
        mcopy[i] = calloc(*col, sizeof(int));
    }

    for(int i = 0; i < *rows; i++){
        for(int j = 0; j < *col; j++){
            fscanf(file, "%d", &mcopy[i][j]);
        }
    }

    fclose(file);
    return mcopy;
}

// Copia el contenido de una matriz a otra
void copyM(int** tem, int** matL, int rows, int col){
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < col; j++){
            matL[i][j] = tem[i][j];
        }
    }
}

// Verifica si un nodo neutro (0) se convierte en expuesto (1)
bool neutro(int** matL, int rows, int col, int posx, int posy, int jump){
    int exposed = 0;
    for(int i = posx - jump; i <= posx + jump; i++){
        for(int j = posy - jump; j <= posy + jump; j++){
            if((i >= 0 && i < rows) && (j >= 0 && j < col)){
                if(matL[i][j] == 1 || matL[i][j] == 2){
                    exposed++;
                }
            }
        }
    }
    return exposed >= 2;
}

// Verifica si un nodo expuesto (1) se convierte en verificado (2)
bool exposed(int** matL, int rows, int col, int posx, int posy, int jump){
    int n_verified = 0;
    float r = rand() / (float)RAND_MAX;

    for(int i = posx - jump; i <= posx + jump; i++){
        for(int j = posy - jump; j <= posy + jump; j++){
            if((i >= 0 && i < rows) && (j >= 0 && j < col)){
                if(matL[i][j] == 2){
                    n_verified++;
                }
            }
        }
    }

    float p = 0.15 + (n_verified * 0.05);
    return (r < p); // Probabilidad de volverse verificado
}
