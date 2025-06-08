//Parcial filtro de peticiones
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>

//Constan variable
#define TAM 256

typedef struct Partial{
    char vec[TAM];
    bool aprove;
}partial;


//Global variables
bool romper=false;
int n_pthreads;
pthread_t* vec_pthreads;

int n_files;
const char** files;

int tam_file;
partial* vec_cadena;

int incidentes=0;
int formadas=0;
int nocturnos=0;

int delta;


//Main declaration function
void allocate_memory();
void allocate_memory_partial();
void free_memory_partial();


//Pthreads declaration function
void* validation(void*);
void* filter(void*);
void* count(void*);

//Barriers
pthread_barrier_t barrier_1;
pthread_barrier_t barrier_2;
pthread_barrier_t barrier_3;
pthread_barrier_t barrier_5;

//Mutex
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char const *argv[]){


    n_pthreads=atoi(argv[1]);
    if(n_pthreads<3){
        n_pthreads=3;
    }

    
    if(n_pthreads%3!=0){
        perror("N pthreads tiene q ser devisible entre 3");
        exit(EXIT_FAILURE);
    }else{
        delta=n_pthreads/3;
    }

    n_files=atoi(argv[2]);

    allocate_memory();

    //Barriers
    pthread_barrier_init(&barrier_1, NULL, n_pthreads+1);
    pthread_barrier_init(&barrier_2, NULL, delta*2); //OJO CON LA CANTIDAD DE HILOS Q DESBLOQUEAN UNA BARRERA
    pthread_barrier_init(&barrier_3, NULL, delta*2);
    pthread_barrier_init(&barrier_5, NULL, n_pthreads+1);


    for(int i=0;i<n_pthreads;i++){
        int *arg = malloc(sizeof(int));
        if(i<delta){
            *arg = i;
            pthread_create(&vec_pthreads[i],NULL,validation,arg);
        }else if(i<(2*delta)){
            *arg = i - delta;
            pthread_create(&vec_pthreads[i],NULL,filter,arg);
        }else{
            *arg = i - 2*delta;
            pthread_create(&vec_pthreads[i],NULL,count,arg);
        }
    }

    for(int i=0;i<n_files;i++){
        files[i]=argv[i+3];
    }

    for(int i=0;i<n_files;i++){

        FILE* file=fopen(files[i],"r");
        while(true){

            fscanf(file,"%d",&tam_file);
            fgetc(file);
            allocate_memory_partial();

            for(int i=0;i<tam_file;i++){
                fgets(vec_cadena[i].vec,TAM,file);
                vec_cadena[i].vec[strcspn(vec_cadena[i].vec, "\n")] = '\0';
                vec_cadena[i].aprove=false;
            }
            
            pthread_barrier_wait(&barrier_1);
            pthread_barrier_wait(&barrier_5);
            
            printf("File n: %d\n",i);
            printf("Incidentes 400-402 %d\n",incidentes);
            printf("Mal formateadas %d\n",formadas);
            printf("Incidentes nocturnos  %d\n",nocturnos);

            incidentes=0;
            formadas=0;
            nocturnos=0;

            
            
            break;

        }
        
        free_memory_partial();
        fclose(file);

    }

    romper=true;
    pthread_barrier_wait(&barrier_1);
    

    for(int i=0;i<n_pthreads;i++){
        pthread_join(vec_pthreads[i],NULL);
    }

    pthread_barrier_destroy(&barrier_1);
    pthread_barrier_destroy(&barrier_2);
    pthread_barrier_destroy(&barrier_3);
    pthread_barrier_destroy(&barrier_5);


    return 0;
}

void allocate_memory(){
    vec_pthreads=calloc(n_pthreads,sizeof(pthread_t));
    if(vec_pthreads==NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }

    files=calloc(n_files,sizeof(char*));
    if(files==NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }
}

void allocate_memory_partial(){
    vec_cadena=calloc(tam_file,sizeof(partial));
    if(vec_cadena==NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }
}

void free_memory_partial(){
    free(vec_cadena);
}

void* validation(void* arg){

    int pos = *((int*)arg);
    free(arg);

    while(true){
        
        pthread_barrier_wait(&barrier_1);
        if(romper) break;

        bool range=false;
        bool format=false;

        int number;
        int HH;
        int MM;
        int deltaL=tam_file/delta;
        int start=pos*deltaL;
        int end=(pos==delta-1)?tam_file:deltaL+start;

        for(int i=start;i<end;i++){

            sscanf(vec_cadena[i].vec,"%d;%d:%d;",&number,&HH,&MM);
            if(number>=100 && number<=599){
                range=true;
            }
            if((HH>=0 && HH<=24) && (MM>=0 && MM<=59)){
                format=true;
            }
            if(range&&format){
                vec_cadena[i].aprove=true;

            }else{
                pthread_mutex_lock(&mutex1);
                formadas++;
                pthread_mutex_unlock(&mutex1);
                vec_cadena[i].aprove=false;
                

            }
            range=false;
            format=false;
            number=0;
            HH=0;
            MM=0;
            
        }

        pthread_barrier_wait(&barrier_2);
        pthread_barrier_wait(&barrier_5);


    }
    
    pthread_exit(0);


}

void* filter(void* arg){
    int pos = *((int*)arg);
    free(arg);

    while(true){
        pthread_barrier_wait(&barrier_1);
        if(romper) break;
        pthread_barrier_wait(&barrier_2);

        bool range=false;
        bool format=false;

        int number;
        int HH;
        int MM;
        int deltaL=tam_file/delta;
        int start=pos*deltaL;
        int end=(pos==delta-1)?tam_file:deltaL+start;

        for(int i=start;i<end;i++){
            if(vec_cadena[i].aprove){
                sscanf(vec_cadena[i].vec,"%d;%d:%d;",&number,&HH,&MM);
                if(number>=400 && number<=402){
                    range=true;
                }
                if((HH>=0 && HH<=5) && MM>=0 && MM<=59){
                    format=true;
                }
                if(range&&format){
                    vec_cadena[i].aprove=true;
                }else{
                    vec_cadena[i].aprove=false;
                }
                
            }

            range=false;
            format=false;
            number=0;
            HH=0;
            MM=0;
        }
        pthread_barrier_wait(&barrier_3);
        pthread_barrier_wait(&barrier_5);


    }
    pthread_exit(0);

}

void* count(void* arg){
    int pos = *((int*)arg);
    free(arg);

    while(true){
        pthread_barrier_wait(&barrier_1);
        if(romper) break;
        pthread_barrier_wait(&barrier_3);

        int number;
        int HH;
        int MM;
        int deltaL=tam_file/delta;
        int start=pos*deltaL;
        int end=(pos==delta-1)?tam_file:deltaL+start;

        for(int i=start;i<end;i++){
            if(vec_cadena[i].aprove){
                
                sscanf(vec_cadena[i].vec,"%d;%d:%d;",&number,&HH,&MM);
                if(number>=400 && number<=402){
                    pthread_mutex_lock(&mutex2);
                    incidentes++;
                    pthread_mutex_unlock(&mutex2);
                }
                if((HH>=0 && HH<=5) && MM>=0 && MM<=59){
                    pthread_mutex_lock(&mutex3);
                    nocturnos++;
                    pthread_mutex_unlock(&mutex3);
                }
                
            }
            number=0;
            HH=0;
            MM=0;
        }
        pthread_barrier_wait(&barrier_5);

    }
    pthread_exit(0);
}

