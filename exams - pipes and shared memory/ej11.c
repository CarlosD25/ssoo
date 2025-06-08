// Parcial: Producto Escalar entre Vectores con procesos e IPC (pipes)

#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <sys/types.h>
#include <unistd.h>

void crearVectores(char*, int**, int**, int*);
int productoEscalar(int*, int*, int, int);

int main(int argc, char const *argv[]){
    int n_Hijos = atoi(argv[1]);
    pid_t padre = getpid();
    pid_t* hijos = calloc(n_Hijos, sizeof(pid_t));

    // Crear tuberías para comunicación hijos -> padre
    int** tub = calloc(n_Hijos, sizeof(int*));
    for(int i = 0; i < n_Hijos; i++){
        tub[i] = calloc(2, sizeof(int));
        pipe(tub[i]);
    }

    char* nameFile = (char*)argv[2];
    int* vec1;
    int* vec2;
    int tam;

    crearVectores(nameFile, &vec1, &vec2, &tam);

    int delta = tam / n_Hijos;
    int ini, fin;
    int idx = -1; // Índice para el hijo

    // Crear procesos hijos
    for(int i = 0; i < n_Hijos; i++){
        hijos[i] = fork();
        if (!hijos[i]) {
            idx = i;
            ini = i * delta;
            fin = (i == n_Hijos - 1) ? tam : ini + delta;
            break;
        }
    }

    if(getpid() == padre){
        // Padre cierra escritura y lee resultados parciales de hijos
        for(int i = 0; i < n_Hijos; i++){
            close(tub[i][1]);
        }

        int valor, sum = 0;
        for(int i = 0; i < n_Hijos; i++){
            while(read(tub[i][0], &valor, sizeof(int))){
                sum += valor;
            }
        }

        printf("%d\n", sum);

        for(int i = 0; i < n_Hijos; i++){
            close(tub[i][0]);
            wait(NULL);
        }
    } else {
        // Hijo cierra lectura, calcula y envía su resultado
        for(int i = 0; i < n_Hijos; i++){
            close(tub[i][0]);
        }

        int valor = productoEscalar(vec1, vec2, ini, fin);
        write(tub[idx][1], &valor, sizeof(int));  // Usar idx aquí

        for(int i = 0; i < n_Hijos; i++){
            close(tub[i][1]);
        }

        exit(0);
    }

    return 0;
}

void crearVectores(char* filename, int** vec1, int** vec2, int* tam){
    FILE* archivo = fopen(filename, "r");
    fscanf(archivo, "%d", tam);

    *vec1 = calloc(*tam, sizeof(int));
    *vec2 = calloc(*tam, sizeof(int));

    for(int i = 0; i < *tam; i++){
        fscanf(archivo, "%d", &((*vec1)[i]));
        (*vec2)[i] = (*vec1)[i];
    }
    fclose(archivo);
}

int productoEscalar(int* vec1, int* vec2, int ini, int fin){
    int sum = 0;
    for(int i = ini; i < fin; i++){
        sum += vec1[i] * vec2[i];
    }
    return sum;
}
