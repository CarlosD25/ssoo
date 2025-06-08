#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>  // Para waitpid()

// Handler vacío para capturar la señal SIGUSR1 y evitar que pause() bloquee indefinidamente
void handler(int s){}

int main(int argc, char const *argv[]){

    // Asignar el manejador de la señal SIGUSR1
    signal(SIGUSR1, handler);

    pid_t vec[2], vec2[2];  // Arrays para almacenar los PIDs de los procesos hijos
    int i=0;
    int j=0;

    // Primer bucle para crear 2 procesos hijos (vec)
    for(; i<2; i++){
        vec[i] = fork();  // Se crea un hijo
        if(!vec[i]){      // Si es proceso hijo (fork devuelve 0)
            // Segundo bucle para que cada hijo cree 2 procesos hijos (vec2)
            for(; j<2; j++){
                vec2[j] = fork();  // Se crea un hijo del hijo
                if(!vec2[j]){      // Si es proceso hijo del hijo
                    break;        // Este proceso ya es nieto, sale del bucle
                }
            }
            break;  // El proceso hijo sale del primer bucle para evitar crear más procesos
        }
    }

    // Si i==2, significa que es el proceso original (padre)
    if(i == 2){
        sleep(1);   // Espera para que los hijos estén listos y en pause()
        printf("I'm father %d\n", getpid());
        // Envía señal SIGUSR1 al último hijo creado (vec[1])
        kill(vec[i-1], SIGUSR1);
        pause();    // Espera a recibir señal para continuar
        printf("I'm father %d\n", getpid());

        // Esperar a que terminen los 2 hijos directos
        for (int w = 0; w < 2; w++) {
            waitpid(vec[w], NULL, 0);
        }
        
    } else {
        // Proceso hijo o nieto, espera la señal SIGUSR1 para continuar
        pause();

        if(j == 2){  // Si j==2, es un proceso hijo que creó hijos (es decir, un hijo "intermedio")
            int k=1;
            for(; k>=0; k--){
                printf("I'm son %d\n", getpid());
                // Envía señal SIGUSR1 a cada uno de sus hijos (nietos del padre)
                kill(vec2[k], SIGUSR1);
                pause();  // Espera señal de vuelta de cada hijo
                // Esperar que el nieto termine para no dejar proceso zombie
                waitpid(vec2[k], NULL, 0);
            }
            printf("I'm son %d\n", getpid());
            // Determina a quién enviar la señal para continuar la cadena:
            // Si es el primer hijo (i==0), envía al padre; si no, al hijo anterior en vec
            int send_to = i == 0 ? getppid() : vec[i-1];
            kill(send_to, SIGUSR1);

        } else {
            // Proceso nieto (no creó más hijos)
            printf("I'm son %d\n", getpid());
            // Envía señal a su padre (proceso hijo)
            kill(getppid(), SIGUSR1);
        }
        exit(0);
    }

    
    return 0;
}
