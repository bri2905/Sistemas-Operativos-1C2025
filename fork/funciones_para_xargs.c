#include "funciones_para_xargs.h"

int leer_linea_de_archivo_y_guardar_en_posicion_i_de_arreglo(FILE *archivo,char **ptr_linea,size_t *tamanio_linea,
    char *arreglo[],int i,int *ptr_longitud_de_arreglo)
{

    size_t chars_leidos = getline(ptr_linea,tamanio_linea,archivo);
    if (chars_leidos == -1)
    {	
        if (ferror(stdin)) 
        {
            perror("Error al leer de archivo");
            return -1;
        }

        return 0;
    }

    if ((*ptr_linea)[chars_leidos - 1] == '\n')
    {
        (*ptr_linea)[chars_leidos - 1] = '\0';
    }
    char *linea_copiada = strdup(*ptr_linea);
    if (linea_copiada == NULL)
    {   
        perror("Error al copiar linea de archivo");
        return -1;
    }

    arreglo[i] = linea_copiada;
    (*ptr_longitud_de_arreglo)++;
    return 1;
}

void liberar_memoria_asignada_en_arreglo(char **arreglo,int pos_inicial,int pos_final)
{
    for (int i = pos_inicial;i<= pos_final;i++)
    {
        free(arreglo[i]);
        arreglo[i] = NULL;
    }    
}