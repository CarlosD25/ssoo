#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>  // Para wait()

// Manejador vacío para SIGUSR1 (solo interrumpe pause())
void handler(int s){}

int main(int argc, char const *argv[]){
    
    signal(SIGUSR1, handler);  // Registrar manejador para SIGUSR1

    pid_t vec[2], vec2[2];     // vec guarda PIDs hijos directos, vec2 guarda PIDs nietos
    pid_t padre = getpid();    // Guardar PID del proceso padre original
    int i = 0, j = 0;

    // Crear 2 hijos directos
    for (; i < 2; i++) {
        vec[i] = fork();
        if (!vec[i]) {  // Si es proceso hijo
            // Cada hijo crea 2 hijos (nietos)
            for (; j < 2; j++) {
                vec2[j] = fork();
                if (!vec2[j]) break;  // Nieto termina de crear descendencia
            }
            break; // El hijo sale del bucle principal (de i)
        }
    }

    if (i == 2) {
        // Este bloque solo ejecuta el padre original (que no hizo fork en el for)
        sleep(2);  // Esperar a que hijos y nietos estén listos
        printf("I'm father %d\n", getpid());

        // Iniciar la cadena enviando señal SIGUSR1 al último hijo creado
        kill(vec[i-1], SIGUSR1);

        pause();  // Esperar señal para continuar la ejecución

        printf("I'm father %d\n", getpid());

        // Esperar a que terminen los 2 hijos directos para evitar zombies
        for (int k = 0; k < 2; k++) {
            wait(NULL);
        }

    } else {
        // Este bloque ejecutan hijos y nietos

        pause();  // Esperar señal SIGUSR1 para sincronizar

        if (j == 2) {
            // Este proceso es un hijo que creó sus propios hijos (nietos)
            printf("I'm son %d\n", getpid());

            // Enviar señal SIGUSR1 al último nieto creado para continuar la cadena
            kill(vec2[j-1], SIGUSR1);

            pause();  // Esperar señal de regreso desde nieto

            printf("I'm son %d\n", getpid());

            // Decidir a quién enviar la señal para continuar la cadena
            pid_t send_to = (i == 0) ? getppid() : vec[i-1];
            kill(send_to, SIGUSR1);

            // Esperar a que terminen sus hijos (nietos)
            for (int k = 0; k < 2; k++) {
                wait(NULL);
            }

        } else {
            // Este bloque es para procesos nietos (no crean más hijos)
            printf("I'm son %d\n", getpid());

            // Enviar señal a su padre (hijo) para continuar la cadena
            pid_t send_to = (j == 0) ? getppid() : vec2[j-1];
            kill(send_to, SIGUSR1);
        }

        exit(0);  // Terminar proceso hijo o nieto
    }

    return 0;
}
