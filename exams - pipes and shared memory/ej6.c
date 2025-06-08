// Taller campo minado
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

// Estructura para representar la posición de una mina detectada
typedef struct desminado{
    int x;
    int y;
}desminadoP;

// Declaración de funciones
int** crearMatriz(const char*, int*, int*);
bool encontrarMina(int**, int, int, int, int);

int main(int argc, char const *argv[]){
    
    int n_Hijos = atoi(argv[1]); // Número de procesos hijos

    pid_t padre = getpid(); // PID del proceso padre

    // Creación de tuberías: cada hijo tendrá su propia tubería
    int** tub = calloc(n_Hijos, sizeof(int*));
    for (int i = 0; i < n_Hijos; i++) {
        tub[i] = calloc(2, sizeof(int));
        pipe(tub[i]); // Se crea una tubería por hijo
    }

    // Lectura de la matriz desde el archivo
    const char* nombreArchivo = argv[2];
    int filas;
    int columnas;
    int** matriz = crearMatriz(nombreArchivo, &filas, &columnas);

    // Creación de procesos hijos
    int* hijos = calloc(n_Hijos, sizeof(int));
    int ind;
    int i = 0;
    int inix, finx, iniy, finy;
    int k = (int)sqrt(n_Hijos); // Cantidad de divisiones por dimensión (k*k = n_Hijos)
    int deltax = filas / k;     // Número de filas por bloque
    int deltay = columnas / k;  // Número de columnas por bloque

    // Bifurcación de procesos hijos
    for (; i < n_Hijos; i++) {
        hijos[i] = fork();
        if (!hijos[i]) { // Proceso hijo
            ind = i;
            int row_block = i / k; // Bloque de fila asignado al hijo
            int col_block = i % k; // Bloque de columna asignado al hijo

            // Determina los límites del bloque que le corresponde al hijo
            inix = row_block * deltax;
            finx = (row_block == k - 1) ? filas : inix + deltax;

            iniy = col_block * deltay;
            finy = (col_block == k - 1) ? columnas : iniy + deltay;
            break;
        }
    }

    // Lógica del proceso padre
    if (getpid() == padre) {

        // Cierra extremos de escritura de las tuberías
        for (int i = 0; i < n_Hijos; i++) {
            close(tub[i][1]);
        }

        desminadoP p;
        // Lectura de resultados enviados por los hijos
        for (int i = 0; i < n_Hijos; i++) {
            while (read(tub[i][0], &p, sizeof(p))) {
                printf("mina en %d\t %d", p.x, p.y); // Imprime la posición de la mina encontrada
                printf("\n");
            }
        }

        // Cierra extremos de lectura
        for (int i = 0; i < n_Hijos; i++) {
            close(tub[i][0]);
        }

        // Espera que todos los hijos terminen
        for (int i = 0; i < n_Hijos; i++) {
            wait(NULL);
        }

    } else { // Lógica del proceso hijo

        // Cierra extremos de lectura de las tuberías
        for (int i = 0; i < n_Hijos; i++) {
            close(tub[i][0]);
        }

        desminadoP p;

        // Recorre el bloque asignado al hijo
        for (int k = inix; k < finx; k++) {
            for (int j = iniy; j < finy; j++) {
                if (matriz[k][j] == 1) { // Encuentra posible mina
                    p.x = k;
                    p.y = j;
                    if (encontrarMina(matriz, filas, columnas, p.x, p.y)) {
                        write(tub[ind][1], &p, sizeof(p)); // Envia la posición al padre
                    }
                }
            }
        }

        // Cierra extremos de escritura antes de salir
        for (int i = 0; i < n_Hijos; i++) {
            close(tub[i][1]);
        }

        exit(0); // Termina el proceso hijo

    }
    return 0;
}

// Función que crea y carga una matriz desde un archivo
int** crearMatriz(const char* filename, int* filas, int* columnas){

    FILE* archivo = fopen(filename, "r");

    int f, c;
    fscanf(archivo, "%d", &f);
    fscanf(archivo, "%d", &c);

    int** matriz = calloc(f, sizeof(int*));
    for (int i = 0; i < f; i++) {
        matriz[i] = calloc(c, sizeof(int));
    }

    // Llena la matriz con los datos del archivo
    for (int i = 0; i < f; i++) {
        for (int j = 0; j < c; j++) {
            fscanf(archivo, "%d", &matriz[i][j]);
        }
    }

    *filas = f;
    *columnas = c;
    fclose(archivo);
    return matriz;
}

// Función que verifica si una celda (mina) tiene una bomba cercana
bool encontrarMina(int** matriz, int filas, int columnas, int pox, int posy){

    for (int i = pox - 1; i <= pox + 1; i++) {
        for (int j = posy - 1; j <= posy + 1; j++) {
            if ((i >= 0 && i < filas) && (j >= 0 && j < columnas)) {
                if (matriz[i][j] == 2) {
                    return true; // Hay una bomba adyacente
                }
            }
        }
    }
    return false;
}
