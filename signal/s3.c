#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>  // Para waitpid()

// Handler vacío para capturar SIGUSR1 y que pause() no bloquee indefinidamente
void handler(int s){}

int main(int argc, char const *argv[]){
    // Registrar el manejador para SIGUSR1
    signal(SIGUSR1, handler);

    pid_t vec[3];         // Array para guardar PIDs de los 3 hijos directos
    int padre = getpid(); // Guardar PID del proceso padre original
    int h = 0, i = 0;     // h para PID de hijo creado dentro del hijo, i para el bucle

    // Bucle para crear 3 hijos directos
    for(; i < 3; i++){
        vec[i] = fork();

        if (!vec[i]) {  // Si es proceso hijo
            // Cada hijo crea hasta 3 hijos (nietos del padre original)
            for(int j = 0; j < 3; j++){
                h = fork();
                if (h) break;  // El proceso padre dentro del bucle sale para no crear más hijos
                // El proceso hijo creado sigue y no crea más hijos en este bucle
            }
            break; // Hijo o nieto sale del bucle principal
        }
    }

    // Código que ejecuta solo el padre original (el que no hizo fork)
    if (i == 3) {
        sleep(1);  // Esperar para que todos los hijos estén listos y en pause()
        printf("I'm father %d\n", getpid());

        // Iniciar la cadena enviando SIGUSR1 al último hijo creado (vec[2])
        kill(vec[i - 1], SIGUSR1);

        // Esperar señal para continuar (cadena que regresa de hijos/nietos)
        pause();

        printf("I'm father %d\n", getpid());

        // Esperar a que terminen los 3 hijos directos para evitar zombies
        for (int w = 0; w < 3; w++) {
            waitpid(vec[w], NULL, 0);
        }

    } else {
        // Código para hijos y nietos

        // Espera la señal SIGUSR1 para sincronizar ejecución
        pause();

        if (h) {
            // Este bloque es para procesos que crearon hijos (h != 0)
            printf("I'm son %d\n", getpid());

            // Enviar señal al hijo creado para continuar cadena
            kill(h, SIGUSR1);

            // Esperar señal de regreso de hijo creado
            pause();

            // Ahora decide a quién enviar la señal de retorno
            if (getppid() != padre) {
                // Si el padre de este proceso NO es el padre original (es un nieto)
                printf("I'm son %d\n", getpid());

                // Enviar señal a su propio padre para continuar la cadena hacia arriba
                kill(getppid(), SIGUSR1);

            } else {
                // Si el padre es el proceso original (hijo directo del padre)
                if (i == 0) {
                    // Si es el primer hijo directo, envía señal al padre original
                    printf("I'm son %d\n", getpid());
                    kill(getppid(), SIGUSR1);
                } else {
                    // Si no es el primer hijo, envía señal a su hermano anterior (vec[i-1])
                    printf("I'm son %d\n", getpid());
                    kill(vec[i - 1], SIGUSR1);
                }
            }

            // Esperar a que el hijo creado termine para evitar zombie
            waitpid(h, NULL, 0);

        } else {
            // Este bloque es para procesos que no crearon hijos (h == 0), es decir, nietos terminales
            printf("I'm son %d\n", getpid());

            // Envía señal a su padre para indicarle que terminó
            kill(getppid(), SIGUSR1);
        }

        exit(0);
    }

    return 0;
}
