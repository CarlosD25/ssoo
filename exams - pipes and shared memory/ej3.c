// Taller carbono
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <unistd.h>
#include <sys/types.h>

// Estructura que representa un consumo energético
typedef struct Carbono {
    float kwh;              // Cantidad de energía consumida en kWh
    int ren;                // 0 si no es renovable, 1 si es renovable
    struct Carbono* sig;    // Puntero al siguiente nodo (lista enlazada)
} Carbono;

// Prototipos de funciones
void insert_list(Carbono**, float, int);
void read_file(const char*, Carbono**);
void free_list(Carbono*);

int main(int argc, char const *argv[]) {
    
    Carbono* carbono = NULL; // Lista enlazada de consumos
    int tub[2];   // Pipe para enviar datos al hijo 0
    int tub2[2];  // Pipe para recibir resultado del hijo 0
    int tub3[2];  // Pipe para enviar datos al hijo 1
    int tub4[2];  // Pipe para recibir resultado del hijo 1

    // Crear todas las tuberías
    pipe(tub);
    pipe(tub2);
    pipe(tub3);
    pipe(tub4);

    // Leer el archivo de entrada y llenar la lista
    read_file(argv[1], &carbono);

    int i = 0;
    for (; i < 2; i++) {
        if (!fork()) break; // Crear dos hijos
    }

    if (i == 2) {
        // === PROCESO PADRE ===

        // Cerrar extremos de lectura de pipes que se usan para escribir
        close(tub[0]);
        close(tub3[0]);

        // Enviar cada estructura Carbono por los pipes correspondientes
        for (Carbono* cab = carbono; cab != NULL; cab = cab->sig) {
            write(tub[1], cab, sizeof(Carbono));   // Para hijo 0
            write(tub3[1], cab, sizeof(Carbono));  // Para hijo 1
        }

        // Cerrar extremos de escritura luego de enviar
        close(tub[1]);
        close(tub3[1]);

        float totalC_childA;
        float totalC_childB;

        // Leer los resultados de ambos hijos
        read(tub2[0], &totalC_childA, sizeof(totalC_childA)); // Hijo 0
        read(tub4[0], &totalC_childB, sizeof(totalC_childB)); // Hijo 1

        // Cerrar extremos de lectura
        close(tub2[0]);
        close(tub4[0]);

        // Esperar a que los hijos terminen
        for (int j = 0; j < 2; j++) {
            wait(NULL);
        }

        // Mostrar resultados
        printf("Huella de carbono total de energia no renovable %.2f\n", totalC_childA);
        printf("Ahorro potencial toda energia renovable %.2f\n", totalC_childB);

    } else if (i == 0) {
        // === HIJO 0: Calcula huella de carbono de energía NO renovable ===

        // Cerrar pipes que no se usan
        close(tub[1]);    // No escribe
        close(tub2[0]);   // No lee
        close(tub3[0]);   // No usa
        close(tub3[1]);
        close(tub4[0]);
        close(tub4[1]);

        Carbono p;
        float total = 0;

        // Leer estructuras y sumar solo si son no renovables
        while (read(tub[0], &p, sizeof(p))) {
            if (p.ren == 0){
                total += p.kwh;
            }
        }

        float factor = 0.4 * total;  // Se asume que 0.4 es el factor de emisión
        write(tub2[1], &factor, sizeof(factor)); // Enviar resultado al padre

        // Cerrar pipes usados
        close(tub[0]);
        close(tub2[1]);

        exit(0); // Terminar hijo

    } else {
        // === HIJO 1: Calcula huella de carbono de TODO el consumo ===

        // Cerrar pipes que no se usan
        close(tub3[1]);  // No escribe
        close(tub4[0]);  // No lee
        close(tub[0]);
        close(tub[1]);
        close(tub2[0]);
        close(tub2[1]);

        Carbono p;
        float total = 0;

        // Leer toda la energía (renovable o no)
        while (read(tub3[0], &p, sizeof(p))) {
            total += p.kwh;
        }

        float factor = 0.4 * total; // Supone que todo fuera no renovable (escenario de comparación)
        write(tub4[1], &factor, sizeof(factor)); // Enviar resultado al padre

        // Cerrar pipes usados
        close(tub3[0]);
        close(tub4[1]);

        exit(0); // Terminar hijo
    }

    // Liberar la memoria de la lista
    free_list(carbono);

    return 0;
}

// Inserta un nuevo nodo al inicio de la lista
void insert_list(Carbono** cab, float kwh, int ren) {
    Carbono* new = (Carbono*)calloc(1, sizeof(Carbono));
    new->kwh = kwh;
    new->ren = ren;
    new->sig = *cab;
    *cab = new;
}

// Lee un archivo con datos y construye la lista de estructuras Carbono
void read_file(const char* filename, Carbono** cab) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("No se pudo abrir el archivo");
        exit(1);
    }

    int jobs;
    int a;        // ID o número de entrada (no se usa)
    float kwh;    // Energía consumida
    int ren;      // Indicador de si es renovable o no

    fscanf(file, "%d", &jobs);  // Número de entradas

    for (int i = 0; i < jobs; i++) {
        fscanf(file, "%d;", &a);
        fscanf(file, "%f;", &kwh);
        fscanf(file, "%d", &ren);
        insert_list(cab, kwh, ren); // Insertar en la lista
    }

    fclose(file);
}

// Libera la memoria usada por la lista enlazada
void free_list(Carbono* cab) {
    Carbono* temp;
    while (cab != NULL) {
        temp = cab;
        cab = cab->sig;
        free(temp);
    }
}
