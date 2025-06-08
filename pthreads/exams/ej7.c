//Sopa de letras
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>

//Struct
typedef struct Word{
    char word_e[100];
    int x;
    int y;
    bool found;
}word;

//Global variables
int rows;
int col;
char** mat;
char** words;

int* vec_pos;
int n_pthreads;
pthread_t* vec_pthread;

word* vec_word;

//Main declaration functions
void read_file(const char*);
void allocate_memory();
void free_memory();

//Pthreads declaration function
void* search_word(void*);
bool search_top(char*,int,int,int);
bool search_left(char*,int,int,int);
bool search_bottom(char*,int,int,int);
bool search_right(char*,int,int,int);


int main(int argc, char const *argv[]){

    read_file(argv[1]);
    allocate_memory();

    for(int i=0;i<n_pthreads;i++){
        vec_pos[i]=i;
        pthread_create(&vec_pthread[i],NULL,search_word,&vec_pos[i]);
    }

    for(int i=0;i<n_pthreads;i++){
        pthread_join(vec_pthread[i],NULL);
    }

    for(int i=0;i<n_pthreads;i++){
        if(vec_word[i].found){
            printf("word found: %s ",vec_word[i].word_e);
            printf("position: %d %d ",vec_word[i].x,vec_word[i].y);
            printf("\n");
        }
    }

    free_memory();
    
    return 0;
}

void read_file(const char* filename){

    FILE* file= fopen(filename,"r");
    if(!file){perror("Error in file");exit(EXIT_FAILURE);}

    fscanf(file,"%d",&rows);
    fscanf(file,"%d",&col);
    fscanf(file,"%d",&n_pthreads);
    //Important
    fgetc(file);

    mat=calloc(rows,sizeof(char*));
    if(mat==NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }
    for(int i=0;i<rows;i++){
        mat[i]=calloc(col,sizeof(char));
    }

    for(int i=0;i<rows;i++){
        for(int j=0;j<col;j++){
            fscanf(file," %c",&mat[i][j]);
            //printf("%c ",mat[i][j]);
        }
    }
    //Important
    fgetc(file);

    words=calloc(n_pthreads,sizeof(char*));
    if(words==NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }

    for(int i=0;i<n_pthreads;i++){
        words[i]=calloc(100,sizeof(char));
    }

    for(int i=0;i<n_pthreads;i++){
        fgets(words[i],100,file);
        //Important
        words[i][strcspn(words[i], "\n")] = '\0';
    }

    fclose(file);
}

void allocate_memory(){

    vec_pthread=calloc(n_pthreads,sizeof(pthread_t));
    if(vec_pthread==NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }

    vec_word=calloc(n_pthreads,sizeof(word));
    if(vec_word==NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }

    vec_pos=calloc(n_pthreads,sizeof(int));
    if(vec_pos==NULL){
        perror("There isn't memory");
        exit(EXIT_FAILURE);
    }

}

void* search_word(void* arg){
    int pos=*((int*)arg);
    char* p=words[pos];
    bool must_break=false;

    for(int i=0;i<rows;i++){
        for(int j=0;j<col;j++){
            if(mat[i][j]==*p){
                if(search_top(p,pos,i,j)){
                    must_break=true;
                    break;
                }
                if(search_bottom(p,pos,i,j)){
                    must_break=true;
                    break;
                }
                if(search_left(p,pos,i,j)){
                    must_break=true;
                    break;
                }
                if(search_right(p,pos,i,j)){
                    must_break=true;
                    break;
                }
            }
        }

        if(must_break) break;
    }

    pthread_exit(0);

}

bool search_top(char* p,int pos,int i,int j){
    
    int len = strlen(p);

    if (i - len + 1 < 0) return false; // no cabe hacia arriba

    for (int k = 0; k < len; k++) {
        if (mat[i - k][j] != p[k]) {
            return false;
        }
    }

    vec_word[pos].found = true;
    vec_word[pos].x = i;
    vec_word[pos].y = j;
    strcpy(vec_word[pos].word_e, p);
    return true;

}

bool search_bottom(char* p,int pos,int i,int j){

    int len = strlen(p);

    if (i + len > rows) return false; // no cabe hacia abajo

    for (int k = 0; k < len; k++) {
        if (mat[i + k][j] != p[k]) {
            return false;
        }
    }

    vec_word[pos].found = true;
    vec_word[pos].x = i;
    vec_word[pos].y = j;
    strcpy(vec_word[pos].word_e, p);
    return true;

}

bool search_right(char* p,int pos,int i,int j){

    int len = strlen(p);

    if (j + len > col) return false; // no cabe hacia la derecha

    for (int k = 0; k < len; k++) {
        if (mat[i][j + k] != p[k]) {
            return false;
        }
    }

    vec_word[pos].found = true;
    vec_word[pos].x = i;
    vec_word[pos].y = j;
    strcpy(vec_word[pos].word_e, p);
    return true;

}

bool search_left(char* p,int pos,int i,int j){

    int len = strlen(p);

    if (j - len + 1 < 0) return false; // no cabe hacia la izquierda

    for (int k = 0; k < len; k++) {
        if (mat[i][j - k] != p[k]) {
            return false;
        }
    }

    vec_word[pos].found = true;
    vec_word[pos].x = i;
    vec_word[pos].y = j;
    strcpy(vec_word[pos].word_e, p);
    return true;

}

void free_memory(){
    free(vec_pos);
    free(vec_pthread);
    free(vec_word);
}