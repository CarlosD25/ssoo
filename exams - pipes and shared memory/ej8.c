// Parcial Iterative Box Blur con comentarios
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <sys/types.h>

#define SALTO 1 // Radio de la vecindad a considerar (box blur de 3x3)

typedef struct punto {
    int x;      // Coordenada x (fila)
    int y;      // Coordenada y (columna)
    int valor;  // Valor calculado del nuevo píxel
} point;

// Prototipos de funciones
int** crear_matriz(char*, int*, int*);
int matriz_blur(int**, int, int, int, int, int);

int main(int argc, char const *argv[]) {
    
    int n_Hijos = atoi(argv[1]);       // Número de hijos a crear
    char *nombreFile = argv[2];        // Nombre del archivo con la matriz

    pid_t padre = getpid();            // Guardamos el PID del proceso padre

    // Crear las tuberías de comunicación para cada hijo
    int** tub = calloc(n_Hijos, sizeof(int*));
    for (int i = 0; i < n_Hijos; i++) {
        tub[i] = calloc(2, sizeof(int));
        pipe(tub[i]); // Se crea una tubería por hijo
    }

    // Variables para dimensiones de la matriz
    int filas = 0;
    int columnas = 0;

    // Crear matriz leyendo desde archivo
    int** matriz = crear_matriz(nombreFile, &filas, &columnas);

    // Crear arreglo de PIDs para los hijos
    pid_t* hijos = calloc(n_Hijos, sizeof(pid_t));

    // Dividir las filas entre los hijos
    int delta = filas / n_Hijos;
    int ini, fin;

    int i = 0;
    int idxh = -1; // Índice del hijo actual

    // Crear los procesos hijos
    for (; i < n_Hijos; i++) {
        hijos[i] = fork();
        if (!hijos[i]) { // Estamos en un hijo
            idxh = i;
            ini = i * delta;
            fin = (i == n_Hijos - 1) ? filas : ini + delta;
            break;
        }
    }

    if (getpid() == padre) {
        // Proceso padre

        // Cerrar extremos de escritura de todas las tuberías
        for (int i = 0; i < n_Hijos; i++) {
            close(tub[i][1]);
        }

        // Crear matriz final donde se almacenarán los valores procesados
        int** matrizFinal = calloc(filas, sizeof(int*));
        for (int j = 0; j < filas; j++) {
            matrizFinal[j] = calloc(columnas, sizeof(int));
        }

        // Leer los datos enviados por los hijos
        point p;
        for (int i = 0; i < n_Hijos; i++) {
            while (read(tub[i][0], &p, sizeof(p))) {
                matrizFinal[p.x][p.y] = p.valor;
            }
        }

        // Cerrar extremos de lectura
        for (int i = 0; i < n_Hijos; i++) {
            close(tub[i][0]);
        }

        // Imprimir la matriz final ya desenfocada (blur)
        for (int i = 0; i < filas; i++) {
            for (int j = 0; j < columnas; j++) {
                printf("%d\t", matrizFinal[i][j]);
            }
            printf("\n");
        }

        // Esperar que todos los hijos terminen
        for (int i = 0; i < n_Hijos; i++) {
            wait(NULL);
        }

    } else {
        // Proceso hijo

        // Cerrar extremos de lectura de todas las tuberías
        for (int i = 0; i < n_Hijos; i++) {
            close(tub[i][0]);
        }

        // Enviar a través de tubería los píxeles blur calculados
        point p;
        for (int k = ini; k < fin; k++) {
            for (int j = 0; j < columnas; j++) {
                p.x = k;
                p.y = j;
                p.valor = matriz_blur(matriz, filas, columnas, k, j, SALTO);
                write(tub[idxh][1], &p, sizeof(p));
            }
        }

        // Cerrar extremos de escritura
        for (int i = 0; i < n_Hijos; i++) {
            close(tub[i][1]);
        }

        exit(0); // Terminar el proceso hijo
    }
}

// Leer matriz desde archivo
int** crear_matriz(char* filename, int* filas, int* columnas) {
    int** m;
    int f = 0;
    int c = 0;

    FILE* archivo = fopen(filename, "r");
    fscanf(archivo, "%d", &f); // Leer cantidad de filas
    fscanf(archivo, "%d", &c); // Leer cantidad de columnas

    m = calloc(f, sizeof(int*));
    for (int i = 0; i < f; i++) {
        m[i] = calloc(c, sizeof(int));
    }

    // Leer cada valor de la matriz
    for (int i = 0; i < f; i++) {
        for (int j = 0; j < c; j++) {
            fscanf(archivo, "%d", &m[i][j]);
        }
    }

    *filas = f;
    *columnas = c;
    fclose(archivo);
    return m;
}

// Aplica un box blur centrado en (posx, posy) con un radio "salto"
int matriz_blur(int** matriz, int filas, int columnas, int posx, int posy, int salto) {
    int sum = 0;
    int count = 0;

    // Recorrer la vecindad (salto=1 => vecindad de 3x3)
    for (int i = posx - salto; i <= posx + salto; i++) {
        for (int j = posy - salto; j <= posy + salto; j++) {
            if ((i >= 0 && i < filas) && (j >= 0 && j < columnas)) {
                sum += matriz[i][j];
                count++;
            }
        }
    }

    if (count == 0) return 0; // Evita división por cero

    return sum / count; // Promedio
}
