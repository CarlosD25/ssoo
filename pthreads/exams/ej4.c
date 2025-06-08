// Ordenamiento paralelo
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// Constante que define el tamaño de cada bucket
#define bucket_size 10

// Estructura que representa un bucket con un vector de enteros y su tamaño actual
typedef struct Bucket{
    int vec[1024];
    int tam;
} buckets;

// Variables globales
int n_pthreads;             // Número de hilos (también cantidad de buckets)
pthread_t* vec_pthread;     // Vector de identificadores de hilos

int tam_data;               // Cantidad de datos a ordenar
int* vec_data;              // Vector que contiene los datos
int* vec_pos;               // Vector de posiciones para los hilos

int n_casilleros;           // Número de casilleros (buckets)
buckets* vec_buckets;       // Arreglo de buckets

int max;                    // Valor máximo en los datos
int min;                    // Valor mínimo en los datos

// Declaraciones de funciones principales
void read_file(const char*);
void max_min();
void allocate_memory();
void free_memory();

// Declaración de la función que ejecutan los hilos
void* sort_vector(void*);

int main(int argc, char const *argv[]){
    
    // Verifica si se envió el nombre del archivo como argumento
    if(argc<2){
        perror("Send file");
        exit(EXIT_FAILURE);
    }

    // Lee los datos desde el archivo
    read_file(argv[1]);

    // Calcula el máximo y mínimo de los datos
    max_min(&max,&min);
    
    // Calcula cuántos hilos/buckets se necesitan
    n_pthreads=(max-min)/bucket_size;
    n_casilleros=(max-min)/bucket_size;

    // Reserva la memoria necesaria
    allocate_memory();
    
    // Crea los hilos y les asigna su bucket
    for(int i=0;i<n_pthreads;i++){
        vec_buckets[i].tam = 0; // Inicializa el tamaño del bucket
        vec_pos[i]=i;           // Asigna el índice al hilo
        pthread_create(&vec_pthread[i],NULL,sort_vector,&vec_pos[i]);
    }

    // Espera que todos los hilos terminen
    for(int i=0;i<n_pthreads;i++){
        pthread_join(vec_pthread[i],NULL);
    }

    // Une los buckets ordenados en el vector original
    int k=0;
    for(int i=0;i<n_casilleros;i++){
        for(int j=0;j<vec_buckets[i].tam;j++){
            vec_data[k++]=vec_buckets[i].vec[j];
        }
    }

    // Imprime el vector ordenado
    for(int i=0;i<tam_data;i++){
        printf("%d ",vec_data[i]);
    }

    // Libera la memoria reservada
    free_memory();

    return 0;
}

// Lee el archivo de entrada y llena el vector de datos
void read_file(const char* filename){
    FILE* file=fopen(filename,"r");
    if(!file){
        perror("Error in file");
        exit(EXIT_FAILURE);
    }

    // Lee la cantidad de datos
    fscanf(file,"%d",&tam_data);

    // Reserva memoria para el vector de datos
    vec_data=calloc(tam_data,sizeof(int));
    if(vec_data==NULL){
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    // Lee los datos desde el archivo
    for(int i=0;i<tam_data;i++){
        fscanf(file,"%d",&vec_data[i]);
    }
    
    fclose(file);
}

// Calcula el valor máximo y mínimo del vector de datos
void max_min(){
    max=vec_data[0];
    min=vec_data[0];

    for(int i=0;i<tam_data;i++){
        if(vec_data[i]>max){
            max=vec_data[i];
        }
        if(vec_data[i]<min){
            min=vec_data[i];
        }
    }
}

// Reserva memoria para los buckets, hilos y posiciones
void allocate_memory(){
    vec_pos=calloc(n_pthreads,sizeof(int));
    if(vec_pos==NULL){
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    vec_buckets=calloc(n_casilleros,sizeof(buckets));
    if(vec_buckets==NULL){
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    vec_pthread=calloc(n_pthreads,sizeof(pthread_t));
    if(vec_pthread==NULL){
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
}

// Función ejecutada por cada hilo para llenar y ordenar su bucket
void* sort_vector(void* arg){

    int pos=*((int*)arg); // Índice del hilo/bucket

    // Calcula el rango de valores que pertenecen a este bucket
    int ini=(pos==0)?min:(bucket_size*pos)+pos+1;
    int end=ini+bucket_size;

    vec_buckets[pos].tam=0;
    int j=0;

    // Clasifica los elementos que pertenecen a este bucket
    for(int i=0;i<tam_data;i++){
        if(vec_data[i]>=ini && vec_data[i]<=end){
            vec_buckets[pos].vec[j++]=vec_data[i];
            vec_buckets[pos].tam++;
        }
    }
    
    // Ordenamiento burbuja dentro del bucket
    for(int i = 0; i < vec_buckets[pos].tam ; i++) {
        for(int j = 0; j < vec_buckets[pos].tam - 1; j++) {
            if(vec_buckets[pos].vec[j] > vec_buckets[pos].vec[j+1]) {
                int aux = vec_buckets[pos].vec[j];
                vec_buckets[pos].vec[j] = vec_buckets[pos].vec[j+1];
                vec_buckets[pos].vec[j+1] = aux;
            }
        }
    }

    pthread_exit(0); // Termina el hilo
}

// Libera toda la memoria reservada
void free_memory(){
    free(vec_buckets);
    free(vec_data);
    free(vec_pos);
    free(vec_pthread);
}
