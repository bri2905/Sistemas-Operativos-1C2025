#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "funciones_para_xargs.h"

#ifndef NARGS
#define NARGS 4
#endif

int
main(int argc, char *argv[])
{
	// Your code here


	char *comando = argv[1];

	char *linea = NULL;
	size_t tam = 0;

	int resultado_fork;

	char *argumentos[NARGS + 2];
	argumentos[0] = comando;
	int cant_argumentos_actual;

	bool queda_lineas_por_leer_en_stdin = true;
	while (queda_lineas_por_leer_en_stdin) {
		cant_argumentos_actual = 1;  // se cuenta el nombre de comando
		for (int i = 1; i <= NARGS; i++) {
			int resultado =
			        leer_linea_de_archivo_y_guardar_en_posicion_i_de_arreglo(
			                stdin,
			                &linea,
			                &tam,
			                argumentos,
			                i,
			                &cant_argumentos_actual);

			if (resultado == -1) {
				liberar_memoria_asignada_en_arreglo(argumentos,
				                                    1,
				                                    NARGS + 1);
				free(linea);
				return -1;
			} else if (resultado == 0) {
				break;
			}
		}

		if (cant_argumentos_actual == 1) {
			queda_lineas_por_leer_en_stdin = false;
			continue;
		}

		argumentos[cant_argumentos_actual] =
		        NULL;  // agregamos NULL al final para execvp()

		resultado_fork = fork();

		if (resultado_fork == -1) {
			perror("Error al crear el proceso hijo");
			liberar_memoria_asignada_en_arreglo(argumentos,
			                                    1,
			                                    NARGS + 1);
			free(linea);
			return -1;
		}

		if (resultado_fork == 0) {
			/// estamos en proceso hijo
			if (execvp(comando, argumentos) == -1) {
				perror("Error al usar execvp()");
				liberar_memoria_asignada_en_arreglo(argumentos,
				                                    1,
				                                    NARGS + 1);
				free(linea);
				return -1;
			}

		}

		else {
			int status;
			wait(&status);
			liberar_memoria_asignada_en_arreglo(argumentos,
			                                    1,
			                                    NARGS + 1);
		}
	}

	liberar_memoria_asignada_en_arreglo(argumentos, 1, NARGS + 1);
	free(linea);
	return 0;
}
