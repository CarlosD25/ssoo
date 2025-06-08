//Parcial busqueda de repeticiones de un numero
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>

typedef struct repetitions{
    int pos;
    int idx;
} rep;

void read_file(const char*,int**,int*);

int main(int argc, char const *argv[]){

    if(argc<4){
        perror("send file and N childs\n");
        return EXIT_FAILURE;
    }
    
    int number=atoi(argv[1]);
    int n_childs=atoi(argv[2]);
    pid_t father=getpid();
    int tam;
    int* copy;

    //shared memory
    read_file(argv[3],&copy,&tam);

    if(tam%n_childs!=0){
        perror("El tamaño del vector tiene que ser multiplo del numero de hijos");
        return EXIT_FAILURE;
    }

    int shmidV = shmget(IPC_PRIVATE, sizeof(int)*tam, IPC_CREAT | 0600);
    if (shmidV == -1) {
        perror("Error in shmget");
        exit(1);
    }
    int* vec = (int*) shmat(shmidV, NULL, 0);
    if (vec == (void *)-1) {
        perror("Error in shmat");
        exit(1);
    }

    memcpy(vec, copy, sizeof(int) * tam);
    free(copy);

    //Pipes
    int** tub=calloc(n_childs,sizeof(int*));
    for(int i=0;i<n_childs;i++){
        tub[i]=calloc(2,sizeof(int));
        pipe(tub[i]);
    }

    //Childs
    int idx;
    int start;
    int end;
    int half=tam/n_childs;
    for(int i=0;i<n_childs;i++){
        if(!fork()){
            idx=i;
            start=i*half;
            end=(i==n_childs-1)?tam:start+half;
            break;
        }
    }

    if(getpid()==father){ //Parent process

        for(int i=0;i<n_childs;i++){
            close(tub[i][1]);
        }

        rep p;
        int sum=0;

        for(int i=0;i<n_childs;i++){
            while(read(tub[i][0],&p,sizeof(p))){
                printf("Process %d find number in %d position\n ",p.idx,p.pos);
                sum++;
            }
        }

        for(int i=0;i<n_childs;i++){
            close(tub[i][0]);
        }

        for(int i=0;i<n_childs;i++){
            wait(NULL);
        }

        printf("The number of repetitions of the number %d is %d\n",number,sum);

        shmdt(vec);
        shmctl(shmidV, IPC_RMID, NULL);
        

    }else{//Childs process

        for(int i=0;i<n_childs;i++){
            close(tub[i][0]);
        }

        rep p;

        for(int i=start;i<end;i++){
            if(vec[i]==number){
                p.pos=i;
                p.idx=idx;
                write(tub[idx][1],&p,sizeof(p));
            }
        }

        for(int i=0;i<n_childs;i++){
            close(tub[i][1]);
        }

        shmdt(vec);
        exit(0);
    }

    return 0;
}

void read_file(const char* filename, int** copy, int* tam){

    FILE* file=fopen(filename,"r");
    if(!file){exit(1);}

    fscanf(file,"%d",tam);

    *copy=calloc(*tam,sizeof(int));
    if(!copy){exit(1);}

    for(int i=0;i<*tam;i++){
        fscanf(file,"%d",&(*copy)[i]);
    }

    fclose(file);

}
