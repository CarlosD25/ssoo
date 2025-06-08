#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>  // Para waitpid()

// Manejador vacío para la señal SIGUSR1, usado solo para interrumpir pause()
void handler(int s){}

int main(int argc, char const *argv[]){
    
    // Registrar el manejador para la señal SIGUSR1
    signal(SIGUSR1, handler);

    pid_t vec[5], padre;  // vec guarda los PIDs de los 5 hijos directos, padre guarda el PID del proceso padre original
    int h=0, i=0;

    // Guardar el PID del proceso padre original
    padre = getpid();

    // Bucle para crear 5 hijos directos
    for (; i < 5; i++) {
        vec[i] = fork();

        if (!vec[i]) {  // Si es proceso hijo (fork retorna 0)
            if (i == 2) {
                // El hijo con índice 2 creará hasta 4 hijos (nietos del padre original)
                for (int j = 0; j < 4; j++) {
                    h = fork();
                    if (h) break;  // El proceso padre dentro del bucle sale para no crear más hijos
                    // El proceso hijo creado continúa y no crea más hijos dentro de este bucle
                }
            }
            // Un hijo (o nieto) sale del bucle principal de creación de hijos
            break;
        }
    }

    // Bloque que ejecuta solo el proceso padre original (el que no hizo fork en el for)
    if (i == 5) {
        sleep(1);  // Espera para asegurarse que los hijos estén listos

        printf("I'm father %d\n", getpid());

        // Enviar señal SIGUSR1 al último hijo creado (vec[4]) para iniciar la cadena de señales
        kill(vec[i-1], SIGUSR1);

        // Esperar señal para continuar (procede de la cadena que regresa de hijos/nietos)
        pause();

        printf("I'm father %d\n", getpid());

        // Esperar a que terminen los 5 hijos directos para evitar procesos zombis
        for (int w = 0; w < 5; w++) {
            waitpid(vec[w], NULL, 0);
        }

    } else {
        // Bloque que ejecutan hijos y nietos

        // Espera señal SIGUSR1 para sincronizar la ejecución
        pause();

        if (h) {
            // Este bloque es para procesos que han creado hijos dentro del ciclo (h != 0)
            // Aquí el hijo 2 que creó nietos entra aquí (porque h contiene el PID de uno de sus hijos)
            printf("I'm son %d\n", getpid());

            if (padre == getppid()) {
                // Si el padre del proceso actual es el proceso padre original
                // Esto sucede para el hijo 2 que creó nietos, que es hijo directo del padre original

                kill(h, SIGUSR1);   // Enviar señal al hijo creado (nieto) para que siga ejecución
                pause();            // Esperar señal de regreso del nieto tras finalizar su parte

                printf("I'm son %d\n", getpid());

                kill(vec[i-1], SIGUSR1); 
                // Enviar señal al hermano anterior (vec[i-1]), para pasar la cadena hacia atrás

            } else {
                // Si el padre no es el padre original (es un nieto, por ejemplo)
                kill(h, SIGUSR1);    // Enviar señal al hijo creado (nieto)
                pause();             // Esperar señal de regreso

                printf("I'm son %d\n", getpid());

                kill(getppid(), SIGUSR1); 
                // Enviar señal a su propio padre (para continuar cadena hacia arriba)

            }

            // Esperar a que termine el hijo creado para evitar zombies
            waitpid(h, NULL, 0);

        } else {
            // Este bloque es para procesos que no crearon hijos (h == 0)
            if (i == 2) {
                // Para el hijo 2 que no creó más hijos (es un nieto)
                printf("I'm son %d\n", getpid());

                kill(getppid(), SIGUSR1); 
                // Notifica a su padre (hijo 2) que ha terminado su parte

            } else {
                // Para los otros hijos que no crean nietos
                printf("I'm son %d\n", getpid());

                // Define a quién enviar la señal:
                // Si es el primer hijo (i == 0), envía señal al padre original
                // Si no, envía señal a su hermano anterior (vec[i-1])
                int send_to = (i == 0) ? getppid() : vec[i-1];

                kill(send_to, SIGUSR1);  // Envía la señal al proceso definido

            }
        }
        exit(0);
    }

    return 0;
}
