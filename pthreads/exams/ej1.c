// Parcial relajación de Jacobi usando pthreads y barreras

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// Constante usada para iterar en la matriz
#define JUMP 1

// Variables globales para las matrices y dimensiones
int** mat_l;   // Matriz actual (lectura)
int** mat_e;   // Matriz siguiente (escritura)
int rows;      // Número de filas
int col;       // Número de columnas

// Variables para manejo de hilos
int n_pthreads;        // Número de hilos
pthread_t* vec_pthreads; // Vector de hilos
int* vec_pos;          // Vector con índices de hilos

// Variables para particionar el trabajo
int start;
int delta;
int end;

int tmp;  // Número de iteraciones (tiempo)

// Declaraciones de funciones
void read_file(const char*);
void allocate_memory();
void free_memory();

// Funciones de hilos y cálculo
void* operation(void*);
int value(int,int);

// Barreras para sincronización entre iteraciones
pthread_barrier_t barrier_1;
pthread_barrier_t barrier_2;

int main(int argc, char const *argv[]){

    if(argc < 4){
        printf("Enviar: número de iteraciones, número de pthreads y archivo\n");
        exit(EXIT_FAILURE);
    }

    tmp = atoi(argv[1]);
    if(tmp <= 0){
        perror("El número de iteraciones debe ser mayor que 0");
        exit(EXIT_FAILURE);
    }

    n_pthreads = atoi(argv[2]);

    // Inicializar barreras con n_pthreads + 1 (incluye hilo principal)
    pthread_barrier_init(&barrier_1, NULL, n_pthreads + 1);
    pthread_barrier_init(&barrier_2, NULL, n_pthreads + 1);

    // Leer matriz inicial desde archivo
    read_file(argv[3]);

    // Reservar memoria para estructuras necesarias
    allocate_memory();

    // Crear hilos y asignarles índice
    for(int i = 0; i < n_pthreads; i++){
        vec_pos[i] = i;
        pthread_create(&vec_pthreads[i], NULL, operation, &vec_pos[i]);
    }

    // Ciclo principal de iteraciones
    for(int j = 0; j < tmp; j++){
        printf("Matriz en iteración: %d\n", j + 1);
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < col; j++){
                printf("%d\t", mat_l[i][j]);
            }
            printf("\n");
        }

        // Esperar a que todos los hilos calculen nueva matriz (barrier_1)
        pthread_barrier_wait(&barrier_1);

        // Esperar a que todos terminen actualización de matriz siguiente (barrier_2)
        pthread_barrier_wait(&barrier_2);

        // Copiar los valores calculados en mat_e a mat_l para la próxima iteración
        for(int i = 1; i < rows - 1; i++){
            for(int j = 1; j < col - 1; j++){
                mat_l[i][j] = mat_e[i][j];
            }
        }
    }

    // Esperar a que todos los hilos terminen
    for(int i = 0; i < n_pthreads; i++){
        pthread_join(vec_pthreads[i], NULL);
    }

    // Destruir barreras
    pthread_barrier_destroy(&barrier_1);
    pthread_barrier_destroy(&barrier_2);

    // Liberar memoria
    free_memory();

    return 0;
}

// Lee la matriz inicial desde archivo
void read_file(const char* filename){
    FILE* file = fopen(filename, "r");
    if(!file){
        perror("Error al abrir archivo");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d", &rows);
    fscanf(file, "%d", &col);

    // Reservar memoria para mat_l
    mat_l = calloc(rows, sizeof(int*));
    if(mat_l == NULL){
        perror("No hay memoria para mat_l");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < rows; i++){
        mat_l[i] = calloc(col, sizeof(int));
        if(mat_l[i] == NULL){
            perror("No hay memoria para mat_l[i]");
            exit(EXIT_FAILURE);
        }
    }

    // Leer valores de la matriz
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < col; j++){
            fscanf(file, "%d", &mat_l[i][j]);
        }
    }

    fclose(file);
}

// Reservar memoria para hilos y matriz de resultado
void allocate_memory(){
    vec_pthreads = calloc(n_pthreads, sizeof(pthread_t));
    if(vec_pthreads == NULL){
        perror("No hay memoria para vec_pthreads");
        exit(EXIT_FAILURE);
    }

    vec_pos = calloc(n_pthreads, sizeof(int));
    if(vec_pos == NULL){
        perror("No hay memoria para vec_pos");
        exit(EXIT_FAILURE);
    }

    mat_e = calloc(rows, sizeof(int*));
    if(mat_e == NULL){
        perror("No hay memoria para mat_e");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < rows; i++){
        mat_e[i] = calloc(col, sizeof(int));
        if(mat_e[i] == NULL){
            perror("No hay memoria para mat_e[i]");
            exit(EXIT_FAILURE);
        }
    }
}

// Función que ejecuta cada hilo: calcula valores de Jacobi para su segmento
void* operation(void* arg){
    int pos = *((int*)arg);

    // Dividir filas entre hilos
    delta = rows / n_pthreads;
    start = pos * delta;
    end = (pos == n_pthreads - 1) ? rows : start + delta;

    for(int t = 0; t < tmp; t++){
        // Espera a que el hilo principal dé luz verde para comenzar cálculo
        pthread_barrier_wait(&barrier_1);

        // Recorrer toda la matriz (puedes optimizar para sólo su segmento)
        for(int i = 0; i < rows; i++){
            for(int j = 0; j < col; j++){
                // Evitar bordes, calcular sólo para posiciones internas
                if((i != 0 && i < rows - 1) && (j != 0 && j < col - 1)){
                    mat_e[i][j] = value(i, j);
                }
            }
        }

        // Esperar a que todos los hilos terminen de calcular
        pthread_barrier_wait(&barrier_2);
    }

    pthread_exit(0);
}

// Calcula el valor nuevo para la posición (posx, posy) como promedio de vecinos
int value(int posx, int posy){
    int top = mat_l[posx - 1][posy];
    int right = mat_l[posx][posy + 1];
    int bottom = mat_l[posx + 1][posy];
    int left = mat_l[posx][posy - 1];
    return 0.25 * (top + right + bottom + left);
}

// Libera toda la memoria reservada
void free_memory(){
    free(vec_pos);
    free(vec_pthreads);

    for(int i = 0; i < rows; i++){
        free(mat_l[i]);
        free(mat_e[i]);
    }

    free(mat_l);
    free(mat_e);
}
