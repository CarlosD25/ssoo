// Parcial: Multiplicación de matrices usando procesos y memoria compartida (Snail Matrix)

#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>

// Calcula el tamaño total necesario para una matriz dinámica (filas + espacio para datos)
unsigned int sizeof_dm(int rows, int col, size_t sizeElement);

// Crea índices para simular matriz 2D en memoria continua
void create_index(void** m, int rows, int col, size_t sizeElement);

// Determina qué proceso debe encargarse de calcular la posición (i,j)
int who_process(int posx, int posy, int rows, int col);

int main(int argc, char const *argv[]){

    int rowsA = 6, colA = 6;
    int rowsB = 6, colB = 6;

    int matA[rowsA][colA];
    int matB[rowsB][colB];

    // Inicializar matrices con 1s (ejemplo simple)
    for(int i = 0; i < rowsB; i++){
        for(int j = 0; j < colA; j++){
            matA[i][j] = 1;
            matB[i][j] = 1;
        }
    }

    if(colA != rowsB){
        printf("No se pueden multiplicar esas matrices\n");
        exit(EXIT_FAILURE);
    }

    int rows = colA;  // filas resultado
    int col = rowsB;  // columnas resultado

    // Reservar memoria compartida para matriz resultado
    size_t totalSize = sizeof_dm(rows, col, sizeof(int));
    int shmid = shmget(IPC_PRIVATE, totalSize, IPC_CREAT | 0600);
    int** matriz = shmat(shmid, NULL, 0);

    // Crear índices para simular matriz 2D
    create_index((void*)matriz, rows, col, sizeof(int));

    int idx;
    int i = 0;

    // Crear 3 procesos hijos
    for(; i < 3; i++){
        if(!fork()){
            idx = i;  // Identificador del proceso hijo
            break;
        }
    }

    if(i == 3){
        // Proceso padre espera a los hijos y luego imprime matriz resultado
        for(int i = 0; i < 3; i++){
            wait(NULL);
        }

        for(int i = 0; i < rows; i++){
            for(int j = 0; j < col; j++){
                printf("%d\t", matriz[i][j]);
            }
            printf("\n");
        }
    } else {
        // Cada hijo calcula las posiciones que le corresponden según la función who_process
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < col; j++){
                if(idx == who_process(i, j, rows, col)){
                    matriz[i][j] = idx;  
                    /* Aquí se podría calcular la multiplicación real:
                    for(int k = 0; k < colA; k++){
                        matriz[i][j] += matA[i][k] * matB[k][j];
                    }
                    */
                }
            }
        }
        exit(0);
    }

    return 0;
}

unsigned int sizeof_dm(int rows, int col, size_t sizeElement){
    // Tamaño = espacio para filas (punteros) + espacio para datos contiguos
    size_t size = rows * sizeof(void*);
    size += (rows * col * sizeElement);
    return size;
}

void create_index(void** m, int rows, int col, size_t sizeElement){
    // Crear punteros para filas en memoria contigua
    size_t sizeRow = col * sizeElement;
    m[0] = (void*)(m + rows);
    for(int i = 1; i < rows; i++){
        m[i] = (char*)m[i-1] + sizeRow;
    }
}

int who_process(int posx, int posy, int rows, int col){
    // Determina a qué "capa" o "borde" pertenece la posición (posx, posy)
    int up = posx;
    int left = posy;
    int right = col - 1 - posy;
    int down = rows - 1 - posx;

    int idx = posx;

    if(idx > left) idx = left;
    if(idx > right) idx = right;
    if(idx > down) idx = down;

    return idx;  // Devuelve el índice del proceso encargado de la posición
}
