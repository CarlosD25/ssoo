// Parcial ADN - análisis de secuencias de ADN usando procesos hijos, pipes y memoria compartida

#include <stdlib.h>
#include <stdio.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>

// Estructura para guardar estadísticas de bases nitrogenadas
typedef struct estadistica{
    int a; // número de bases A
    int t; // número de bases T
    int c; // número de bases C
    int g; // número de bases G
} adnE;

// Estructura para guardar la posición de una secuencia encontrada y el índice del proceso
typedef struct secuencia{
    int idx; // índice del proceso hijo que encontró la secuencia
    int pos; // posición donde se encontró la secuencia en el ADN
} adnS;

// Función que calcula la cantidad de cada base nitrogenada en un arreglo de caracteres
void estadisticas(char*, int, int*, int*, int*, int*);

int main(int argc, char const *argv[]){
    
    int n_hijos = atoi(argv[1]); // número de procesos hijos a crear
    int cantidadBases;            // cantidad total de bases en el archivo
    pid_t padre = getpid();       // PID del proceso padre

    // Abrir archivo con la secuencia de ADN
    FILE* file = fopen(argv[2], "r");
    if (!file) {
        perror("Error en el archivo");
        exit(EXIT_FAILURE);
    }

    // Leer cantidad de bases del archivo
    fscanf(file, "%d", &cantidadBases);

    // Crear tuberías para comunicación entre padre e hijos
    int** tub = calloc(n_hijos, sizeof(int*));
    if (tub == NULL) {
        perror("No hay memoria");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n_hijos; i++) {
        tub[i] = calloc(2, sizeof(int));
        pipe(tub[i]); // crear tubería para cada hijo
    }

    // Crear memoria compartida para almacenar la secuencia completa de ADN
    int shmidV = shmget(IPC_PRIVATE, sizeof(char) * cantidadBases, IPC_CREAT | 0600);
    if (shmidV == -1) {
        perror("Error en shmget");
        exit(1);
    }
    char* vec = (char*) shmat(shmidV, NULL, 0);
    if (vec == (void *) -1) {
        perror("Error en shmat");
        exit(1);
    }

    // Leer la secuencia de ADN del archivo en la memoria compartida
    for (int i = 0; i < cantidadBases; i++) {
        fscanf(file, " %c", &vec[i]);
    }
    fclose(file);

    int idx;
    // Crear n_hijos procesos hijos
    for (int i = 0; i < n_hijos; i++) {
        if (!fork()) {
            idx = i; // guardar índice del proceso hijo
            break;
        }
    }

    if (getpid() == padre) {
        // Código del proceso padre

        // Cerrar los extremos de escritura de las tuberías, sólo usará lectura
        for (int i = 0; i < n_hijos; i++) {
            close(tub[i][1]);
        }

        // Leer y mostrar estadísticas desde el primer hijo (índice 0)
        adnE p;
        read(tub[0][0], &p, sizeof(p));
        printf("Estadisticas:\n");
        printf("A:\t%d veces\n", p.a);
        printf("T:\t%d veces\n", p.t);
        printf("C:\t%d veces\n", p.c);
        printf("G:\t%d veces\n", p.g);
        close(tub[0][0]);
        
        printf("\nSecuencia:\n");
        adnS ps;
        // Leer y mostrar las posiciones donde se encontraron secuencias de los otros hijos
        for (int i = 1; i < n_hijos; i++) {
            while (read(tub[i][0], &ps, sizeof(ps))) {
                printf("Proceso: %d\tPosicion: %d\n", ps.idx, ps.pos);
            }
            close(tub[i][0]);
        }

        // Esperar a que terminen todos los hijos
        for (int i = 0; i < n_hijos; i++) {
            wait(NULL);
        }
        
        // Desvincular y eliminar memoria compartida
        shmdt(vec);
        shmctl(shmidV, IPC_RMID, NULL);

    } else {
        // Código de los procesos hijos

        // Cerrar los extremos de lectura de las tuberías, sólo usarán escritura
        for (int i = 0; i < n_hijos; i++) {
            close(tub[i][0]);
        }

        if (idx == 0) {
            // Primer hijo calcula estadísticas de toda la secuencia
            int a, t, c, g;
            estadisticas(vec, cantidadBases, &a, &t, &c, &g);
            adnE p;
            p.a = a;
            p.t = t;
            p.c = c;
            p.g = g;

            // Enviar estadísticas al padre por tubería
            write(tub[idx][1], &p, sizeof(p));
            close(tub[idx][1]);
            exit(0);
            
        } else {
            // Los otros hijos buscan la secuencia objetivo en un segmento específico

            int ini;
            int fin;
            int delta = cantidadBases / (n_hijos - 1);
            
            ini = (idx - 1) * delta;
            fin = (idx == (n_hijos - 1)) ? cantidadBases : ini + delta;

            adnS p;
            int size_secuence = 6;
            const char *target = "GCGTGA";

            // Buscar la secuencia "GCGTGA" dentro del segmento asignado
            for (int i = ini; i < fin; i++) {
                if (strncmp(&vec[i], target, size_secuence) == 0) {
                    p.idx = idx;
                    p.pos = i;
                    // Enviar posición encontrada al padre por tubería
                    write(tub[idx][1], &p, sizeof(p));
                }
            }

            close(tub[idx][1]);
            shmdt(vec);
            exit(0);
        }
    }

    return 0;
}

// Función que cuenta las bases A, T, C, G en la secuencia
void estadisticas(char* vec, int cantidadB, int* a, int* t, int* c, int* g) {

    *a = *t = *c = *g = 0;

    for (int i = 0; i < cantidadB; i++) {
        switch (vec[i]) {
            case 'A':
                (*a)++;
                break;
            case 'T':
                (*t)++;
                break;
            case 'C':
                (*c)++;
                break;
            case 'G':
                (*g)++;
                break;
            default:
                break;
        }
    }
}
