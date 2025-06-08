// Parcial búsqueda paralela
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Variables globales
int number;             // Número a buscar
int n_pthread;          // Número de hilos
int delta;              // Tamaño del segmento de datos que procesa cada hilo

pthread_t* vec_pthread; // Vector de identificadores de hilos
int* vec_pos;           // Vector con las posiciones (índices) de cada hilo

int tam_data;           // Tamaño del arreglo de datos
int* vec_data;          // Vector que contiene los datos leídos del archivo

int repetitions = 0;    // Contador de repeticiones del número buscado

// Declaraciones de funciones principales
void read_file(const char*);
void allocate_memory();
void free_memory();

// Declaración de la función que ejecutan los hilos
void* search_number_repetitions(void*);

// Mutex para proteger la variable compartida 'repetitions'
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char const *argv[]){
    
    if(argc < 4){
        perror("Send number to search, pthreads and file");
        exit(EXIT_FAILURE);
    }

    number = atoi(argv[1]);
    n_pthread = atoi(argv[2]);

    read_file(argv[3]); // ← ahora esta función reserva correctamente vec_data

    delta = tam_data / n_pthread;
    allocate_memory();

    // Crear hilos
    for(int i = 0; i < n_pthread; i++){
        vec_pos[i] = i;
        pthread_create(&vec_pthread[i], NULL, search_number_repetitions, &vec_pos[i]);
    }

    // Esperar que todos terminen
    for(int i = 0; i < n_pthread; i++){
        pthread_join(vec_pthread[i], NULL);
    }

    printf("The number of repetitions of %d is: %d\n", number, repetitions);

    free_memory();
    return 0;
}

// Leer archivo y reservar memoria para vec_data correctamente
void read_file(const char* filename){
    FILE* file = fopen(filename, "r");
    if(!file){ 
        perror("Error in file");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d", &tam_data); // Primero se lee cuántos datos hay

    vec_data = calloc(tam_data, sizeof(int)); // Luego se reserva con ese tamaño
    if(vec_data == NULL){
        perror("Failed to allocate memory for data");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < tam_data; i++){
        fscanf(file, "%d", &vec_data[i]);
    }

    fclose(file);
}

// Reservar memoria para vectores de hilos y posiciones
void allocate_memory(){
    vec_pos = calloc(n_pthread, sizeof(int));
    if(vec_pos == NULL){
        perror("Failed to allocate memory for vec_pos");
        exit(EXIT_FAILURE);
    }

    vec_pthread = calloc(n_pthread, sizeof(pthread_t));
    if(vec_pthread == NULL){
        perror("Failed to allocate memory for vec_pthread");
        exit(EXIT_FAILURE);
    }
}

// Función que ejecuta cada hilo
void* search_number_repetitions(void* arg){
    int pos = *((int*)arg);
    int start = pos * delta;
    int end = (pos == n_pthread - 1) ? tam_data : start + delta;

    for(int i = start; i < end; i++){
        if(vec_data[i] == number){
            pthread_mutex_lock(&mutex);
            repetitions++;
            pthread_mutex_unlock(&mutex);
        }
    }

    pthread_exit(0);
}

// Liberar memoria reservada
void free_memory(){
    free(vec_data);
    free(vec_pos);
    free(vec_pthread);
}
