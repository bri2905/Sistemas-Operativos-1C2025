#ifndef FISOPFS_H
#define FISOPFS_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_INODES 128
#define MAX_BLOCKS 1024
#define BLOCK_SIZE 512
#define MAX_FILENAME_LENGTH 64

typedef struct inode {
	char *nombre;
	mode_t modo;
	uid_t usuario_id;
	gid_t grupo_id;
	size_t tamano;
	time_t acceso;
	time_t modificacion;
	time_t cambio_estado;
	nlink_t enlaces;
	char *contenido;        // Para archivos (NULL si es directorio)
	struct inode **hijos;   // Para directorios (NULL si es archivo)
	size_t cantidad_hijos;  // Para directorios (0 si es archivo)
	int es_directorio;
} inode_t;

typedef struct filesystem {
	inode_t *raiz;
	char *archivo_disco;
} filesystem_t;

extern filesystem_t fs;

#endif