#include "funciones_para_primes.h"
#include "stdlib.h"
#include "sys/types.h"

int
main(int argc, char *argv[])
{
	// Your code here
	int n = atoi(argv[1]);

	int fd_padre_hijo[2];

	int resultado_creacion_pipe = pipe(fd_padre_hijo);

	if (resultado_creacion_pipe == -1) {
		perror("Error al crear el Pipe en main()");
		return -1;
	}

	pid_t resultado_creacion_fork = fork();

	if (resultado_creacion_fork == -1) {
		close(fd_padre_hijo[0]);
		close(fd_padre_hijo[1]);
		perror("Error al crear el proceso hijo en main()");
		return -1;
	}

	if (resultado_creacion_fork == 0) {
		// estamos en proceso hijo
		close(fd_padre_hijo[1]);
		imprimir_numeros_primos_con_procesos(fd_padre_hijo[0]);

	} else {
		// estamos en proceso padre
		close(fd_padre_hijo[0]);  // aca cerramos el fd de lectura del proceso padre

		ssize_t bytes_escritos_en_pipe;

		for (int i = 2; i < n; i++) {
			bytes_escritos_en_pipe =
			        write(fd_padre_hijo[1], &i, sizeof(i));

			if (bytes_escritos_en_pipe == -1) {
				perror("Error al usar write() en main()");
				break;
			}
		}

		close(fd_padre_hijo[1]);
		int status;
		wait(&status);
	}

	return 0;
}