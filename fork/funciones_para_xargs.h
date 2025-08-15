#ifndef FUNCIONES_PARA_XARGS_H
#define FUNCIONES_PARA_XARGS_H
#include <stdio.h>
#include <stdlib.h>
#include "string.h"

int leer_linea_de_archivo_y_guardar_en_posicion_i_de_arreglo(FILE *archivo,char **ptr_linea,size_t *tamanio_linea,
    char *arreglo[],int i,int *ptr_longitud_de_arreglo);

    void liberar_memoria_asignada_en_arreglo(char **arreglo,int pos_inicial,int pos_final);

#endif