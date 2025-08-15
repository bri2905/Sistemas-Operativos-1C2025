#ifndef FUNCIONES_PARA_PRIMES_H
#define FUNCIONES_PARA_PRIMES_H
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>


void imprimir_numeros_primos_con_procesos(int fd_abuelo_padre_lectura);

#endif