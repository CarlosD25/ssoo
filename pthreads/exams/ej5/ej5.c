// Bucket [0-1000] - Ordenamiento paralelo usando archivos de entrada y bucket sort
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// Constantes para definir el rango global de los datos
#define min 0
#define max 1000

// Estructura de un bucket: contiene su vector dinámico, su tamaño y su rango [a, b]
typedef struct Bucket{
    int* buck;
    int size_buck;
    int a;
    int b;
} bucket;

// Variables globales
int* vec_position;            // Vector que almacena el índice de cada hilo

pthread_t* vec_pthreads;      // Vector de identificadores de hilos
int pthreads;                 // Número de hilos

int buckts;                   // Número de buckets
bucket* list_buckets;         // Lista de buckets

const char** files;           // Lista de archivos de entrada
int n_files;                  // Número de archivos

int range;                    // Tamaño del rango por bucket

// Declaraciones de funciones principales
void allocate_memory();
void free_memory();

// Declaraciones de funciones que ejecutan los hilos
void* sort_vec(void*);

// Mutex para proteger acceso a estructuras compartidas
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Barrera para sincronizar hilos antes del ordenamiento
pthread_barrier_t barrier_1;

int main(int argc, char const *argv[]){

    // Se espera que el número de hilos también defina la cantidad de buckets y archivos
    pthreads=atoi(argv[1]);
    buckts=atoi(argv[1]);
    n_files=atoi(argv[1]);

    // Calcula el rango que maneja cada bucket
    range=(max-min)/buckts;

    // Inicializa la barrera para sincronizar los hilos
    pthread_barrier_init(&barrier_1, NULL, pthreads);

    // Reserva memoria para vectores, buckets y archivos
    allocate_memory();

    // Define los rangos [a, b] de cada bucket
    for(int i=0;i<buckts;i++){
        list_buckets[i].a = (i==0) ? min : (i*range)+i;
        list_buckets[i].b = (i==buckts-1) ? max : list_buckets[i].a + range;
        list_buckets[i].buck = NULL;
        list_buckets[i].size_buck = 0;
        printf("Bucket %d: [%d - %d]\n", i, list_buckets[i].a, list_buckets[i].b);
    }

    // Asigna los nombres de archivos desde los argumentos
    for (int i = 0; i < n_files; i++) {
        files[i] = argv[i + 2];  
    }
    
    // Crea los hilos y les pasa su posición correspondiente
    for(int i=0;i<pthreads;i++){
        vec_position[i] = i;
        pthread_create(&vec_pthreads[i], NULL, sort_vec, &vec_position[i]);
    }

    // Espera que todos los hilos terminen
    for(int i=0;i<pthreads;i++){
        pthread_join(vec_pthreads[i], NULL);
    }

    // Imprime todos los valores ordenados por bucket
    for(int i=0;i<buckts;i++){
        for(int j=0;j<list_buckets[i].size_buck;j++){
            printf("%d ", list_buckets[i].buck[j]);
        }
    }

    // Libera los recursos sincronizados
    pthread_barrier_destroy(&barrier_1);
    pthread_mutex_destroy(&mutex);

    return 0;
}

// Reserva la memoria necesaria para los hilos, buckets y archivos
void allocate_memory(){
    vec_position = calloc(pthreads, sizeof(int));
    if(vec_position == NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }

    vec_pthreads = calloc(pthreads, sizeof(pthread_t));
    if(vec_pthreads == NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }

    list_buckets = calloc(buckts, sizeof(bucket));
    if(list_buckets == NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }

    files = calloc(n_files, sizeof(char*));
    if(files == NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }
}

// Función que ejecuta cada hilo
void* sort_vec(void* arg){

    int pos = *((int*)arg);                    // Índice del hilo
    FILE* file = fopen(files[pos], "r");       // Abre su archivo correspondiente
    if(file == NULL) {
        perror("Error abriendo archivo");
        pthread_exit(NULL);
    }

    int tam;
    int number;

    fscanf(file, "%d", &tam);                  // Lee cuántos números hay en el archivo

    // Lee los números y los asigna a su bucket correspondiente
    for(int j = 0; j < tam; j++){
        fscanf(file, "%d", &number);
        for(int i = 0; i < buckts; i++){
            if(number >= list_buckets[i].a && number <= list_buckets[i].b){
                pthread_mutex_lock(&mutex); // Protección concurrente
                list_buckets[i].size_buck++;
                list_buckets[i].buck = realloc(list_buckets[i].buck, list_buckets[i].size_buck * sizeof(int));
                list_buckets[i].buck[list_buckets[i].size_buck - 1] = number;
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
    }

    fclose(file);

    // Espera a que todos terminen de insertar antes de ordenar
    pthread_barrier_wait(&barrier_1);

    // Ordena su bucket correspondiente usando burbuja
    for(int i = 0; i < list_buckets[pos].size_buck; i++){
        for(int j = 0; j < list_buckets[pos].size_buck - 1; j++){
            if(list_buckets[pos].buck[j] > list_buckets[pos].buck[j+1]){
                int aux = list_buckets[pos].buck[j];
                list_buckets[pos].buck[j] = list_buckets[pos].buck[j+1];
                list_buckets[pos].buck[j+1] = aux;
            }
        }
    }
    
    pthread_exit(0); // Termina el hilo
}

// Libera toda la memoria utilizada
void free_memory(){
    for (int i = 0; i < buckts; i++) {
        free(list_buckets[i].buck);
    }
    free(list_buckets);
    free(vec_position);
    free(vec_pthreads);
    free(files);
}
