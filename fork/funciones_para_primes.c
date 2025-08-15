#include <errno.h>
#include "funciones_para_primes.h"


//abuelo >> padre >> hijo
/*
Sugerencias: 
.podriamos tener un parametro en la imprimir_numeros_primos_con_procesos que sea el numero
del proceso que esta ejecutando esa imprimir_numeros_primos_con_procesos
.podriamos verificar valor de retorno de close()
.podriamos verificar valor de retorno en wait()
*/
void imprimir_numeros_primos_con_procesos(int fd_abuelo_padre_lectura)
{   
    int p;
    ssize_t bytes_leidos_en_pipe_abuelo_padre = read(fd_abuelo_padre_lectura,&p,sizeof(p));
    
    if (bytes_leidos_en_pipe_abuelo_padre == -1){
        //close(fd_abuelo_padre_lectura);
        perror("Error al usar read() en imprimir_numeros_primos_con_procesos()");
        return;
    }
    
    if (bytes_leidos_en_pipe_abuelo_padre == 0)
    {   
        close(fd_abuelo_padre_lectura);
        return;
    }
   

    int fd_padre_hijo[2]; //esto para el pipe hijo e hijo del hijo

    int resultado_creacion_pipe = pipe(fd_padre_hijo);

    if (resultado_creacion_pipe == -1)
    {   
        //close(fd_abuelo_padre_lectura);
        perror("Error al crear un pipe en imprimir_numeros_primos_con_procesos()");
        fprintf(stderr, "errno: %d\n", errno);
        return;
    }

    pid_t resultado_creacion_fork = fork();

    if (resultado_creacion_fork == -1)
    {   
        close(fd_abuelo_padre_lectura);
        close(fd_padre_hijo[0]);
		close(fd_padre_hijo[1]);
        perror("Error al crear proceso hijo en imprimir_numeros_primos_con_procesos()");
        return;
    }

    if (resultado_creacion_fork == 0)
    {   
        //estamos en proceso hijo
        close(fd_abuelo_padre_lectura);
        close(fd_padre_hijo[1]);
        imprimir_numeros_primos_con_procesos(fd_padre_hijo[0]);
        
    }
    else
    {   
        //estamos en proceso padre
        close(fd_padre_hijo[0]);
        
        printf("primo %d\n",p);

        int a;
        
        ssize_t bytes_escritos_en_pipe_padre_hijo;

        bytes_leidos_en_pipe_abuelo_padre = read(fd_abuelo_padre_lectura,&a,sizeof(a));
        
        while (bytes_leidos_en_pipe_abuelo_padre > 0)
        {
            if (a % p != 0)
            {
                bytes_escritos_en_pipe_padre_hijo = write(fd_padre_hijo[1],&a,sizeof(a));
                if (bytes_escritos_en_pipe_padre_hijo == -1)
                {
                    perror("Error al usar write() en imprimir_numeros_primos_con_procesos()");
                    break;
                }
            }
            bytes_leidos_en_pipe_abuelo_padre = read(fd_abuelo_padre_lectura,&a,sizeof(a));

        }
        
        if (bytes_leidos_en_pipe_abuelo_padre == -1){
            
            perror("Error al usar read() de imprimir_numeros_primos_con_procesos()");
        }
    
        close(fd_padre_hijo[1]);
        close(fd_abuelo_padre_lectura);
        wait(NULL);
        
    }
    

}

