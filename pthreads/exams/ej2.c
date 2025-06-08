// Parcial ADN: conteo de repeticiones y búsqueda de secuencia específica usando pthreads

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

// Constante tamaño máximo para almacenar posiciones encontradas
#define TAM 100

// Estructura para guardar conteo de nucleótidos
typedef struct Repetitions {
    int a;
    int t;
    int c;
    int g;
} repetitions;

// Estructura para guardar posiciones donde se encontró una secuencia
typedef struct Sequence {
    int vec[TAM];  // posiciones donde se encontró la secuencia
    int total;     // total de ocurrencias encontradas
} sequence;

// Variables globales compartidas entre hilos
int n_pthreads;       // número total de hilos
pthread_t* vec_pthread; // vector de hilos
int* vec_pos;           // vector con índices de hilos
int tam_data;           // tamaño total del vector de datos (ADN)
char* vec_data;         // vector que contiene la secuencia de ADN
int delta;              // tamaño del bloque de datos asignado a cada hilo (menos 1 hilo)

repetitions rep;        // estructura para contar repeticiones de nucleótidos
sequence* seq;          // arreglo de secuencias encontradas por cada hilo (excepto hilo 0)

// Declaración de funciones principales
void allocate_memory();
void read_file(const char*);
void free_memory();

// Declaración de funciones para los hilos
void* n_repetitions_in_file(void*);
void* n_sequence_in_file(void*);

int main(int argc, char const *argv[]) {
    
    if(argc < 3){
        perror("Enviar: número de pthreads y archivo");
        exit(EXIT_FAILURE);
    }

    n_pthreads = atoi(argv[1]);

    // Leer la secuencia ADN desde archivo
    read_file(argv[2]);

    // Calcular tamaño de bloque para hilos que buscan secuencia (n_pthreads-1 hilos)
    delta = tam_data / (n_pthreads - 1);
    
    // Reservar memoria para vectores y estructuras
    allocate_memory();
    
    // Crear hilo 0 para contar repeticiones de nucleótidos
    pthread_create(&vec_pthread[0], NULL, n_repetitions_in_file, NULL);

    // Crear los demás hilos para buscar secuencia "GCGTGA"
    for(int i = 0; i < n_pthreads - 1; i++){
        vec_pos[i] = i;
        pthread_create(&vec_pthread[i + 1], NULL, n_sequence_in_file, &vec_pos[i]);
    }

    // Esperar que todos los hilos terminen
    for(int i = 0; i < n_pthreads; i++){
        pthread_join(vec_pthread[i], NULL);
    }

    // Mostrar resultados del conteo de nucleótidos
    printf("A: %d", rep.a);
    printf("\nT: %d", rep.t);
    printf("\nC: %d", rep.c);
    printf("\nG: %d", rep.g);

    // Mostrar resultados de posiciones donde se encontró la secuencia, por hilo
    for(int i = 0; i < n_pthreads - 1; i++){
        if(seq[i].total > 0){
            printf("\nPthread con pos: %d encontró la secuencia en posiciones:", i + 1);
            for(int j = 0; j < seq[i].total; j++){
                printf("%d ", seq[i].vec[j]);
            }
        }
    }
    printf("\n");

    // Liberar memoria usada
    free_memory();

    return 0;
}

// Reserva memoria para vectores e inicializa variables globales
void allocate_memory() {
    
    vec_pos = calloc(n_pthreads, sizeof(int));
    if(vec_pos == NULL){
        perror("Fallo en asignar memoria para vec_pos");
        exit(EXIT_FAILURE);
    }

    vec_pthread = calloc(n_pthreads, sizeof(pthread_t));
    if(vec_pthread == NULL){
        perror("Fallo en asignar memoria para vec_pthread");
        exit(EXIT_FAILURE);
    }

    seq = calloc(n_pthreads - 1, sizeof(sequence));
    if(seq == NULL){
        perror("Fallo en asignar memoria para seq");
        exit(EXIT_FAILURE);
    }
}

// Lee el archivo que contiene el tamaño y la secuencia ADN
void read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if(!file){
        perror("Error al abrir archivo");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d", &tam_data);

    // Reservar espacio para la secuencia de ADN
    vec_data = calloc(tam_data, sizeof(char));
    if(vec_data == NULL){
        perror("Fallo en asignar memoria para vec_data");
        exit(EXIT_FAILURE);
    }

    // Leer secuencia carácter por carácter
    for(int i = 0; i < tam_data; i++){
        fscanf(file, " %c", &vec_data[i]);
    }

    fclose(file);
}

// Hilo 0: cuenta las repeticiones de cada nucleótido en la secuencia completa
void* n_repetitions_in_file(void* arg) {

    // Inicializar contadores a cero
    rep.a = 0;
    rep.c = 0;
    rep.t = 0;
    rep.g = 0;

    // Recorrer toda la secuencia y contar nucleótidos
    for(int i = 0; i < tam_data; i++){
        switch (vec_data[i]) {
            case 'A':
                rep.a++;
                break;
            case 'T':
                rep.t++;
                break;
            case 'C':
                rep.c++;
                break;
            case 'G':
                rep.g++;
                break;
            default:
                break; // ignorar caracteres no válidos
        }
    }

    pthread_exit(0);
}

// Hilos 1 a n_pthreads-1: buscan la secuencia "GCGTGA" en su segmento
void* n_sequence_in_file(void* arg) {
    int pos = *((int*)arg);
    int start = pos * delta;
    int end = (pos == n_pthreads - 1) ? tam_data : delta + start;

    int j = 0;
    seq[pos].total = 0;

    const char* target = "GCGTGA";  // Secuencia a buscar
    int length = strlen(target);

    // Buscar ocurrencias de la secuencia en el rango asignado
    for(int i = start; i < end; i++){
        if(strncmp(&vec_data[i], target, length) == 0){
            seq[pos].vec[j++] = i;  // Guardar posición de ocurrencia
            seq[pos].total++;
        }
    }

    pthread_exit(0);
}

// Liberar memoria reservada para los vectores y estructuras
void free_memory() {
    free(vec_data);
    free(vec_pos);
    free(vec_pthread);
    free(seq);
}
