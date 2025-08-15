#define FUSE_USE_VERSION 30
#define _POSIX_C_SOURCE 200809L

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "fisopfs.h"

#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

char *filedisk = DEFAULT_FILE_DISK;

filesystem_t fs;

/////////// funcionalidad principal

inode_t *
crear_inode(const char *nombre, mode_t modo, int es_directorio)
{
	if (strlen(nombre) >= MAX_FILENAME_LENGTH) {
		fprintf(stderr, "Error: Nombre de archivo/directorio demasiado largo\n");
		return NULL;
	}
	inode_t *node = calloc(1, sizeof(inode_t));
	node->nombre = strdup(nombre);
	node->modo = modo;
	node->usuario_id = getuid();
	node->grupo_id = getgid();
	node->tamano = 0;
	node->acceso = time(NULL);
	node->modificacion = time(NULL);
	node->cambio_estado = time(NULL);
	node->enlaces = (nlink_t) 1;
	node->es_directorio = es_directorio;

	if (es_directorio) {
		node->hijos = NULL;
		node->cantidad_hijos = 0;
	} else {
		node->contenido = NULL;
	}

	return node;
}

inode_t *
buscar_inode(const char *path)
{
	if (strcmp(path, "/") == 0)
		return fs.raiz;

	char *ruta = strdup(path);
	char *token = strtok(ruta, "/");

	inode_t *actual = fs.raiz;
	while (token && actual && actual->es_directorio) {
		int encontrado = 0;
		for (size_t i = 0; i < actual->cantidad_hijos; i++) {
			if (strcmp(actual->hijos[i]->nombre, token) == 0) {
				actual = actual->hijos[i];
				encontrado = 1;
				break;
			}
		}

		if (!encontrado) {
			free(ruta);
			return NULL;
		}

		token = strtok(NULL, "/");
	}

	free(ruta);
	return actual;
}

inode_t *
buscar_padre(const char *path, char *nombre_out)
{
	char *path_dup = strdup(path);
	char *ultimo = strrchr(path_dup, '/');

	if (!ultimo || strcmp(path, "/") == 0) {
		free(path_dup);
		return NULL;
	}

	strcpy(nombre_out, ultimo + 1);

	if (ultimo == path_dup) {
		*ultimo = '\0';
		inode_t *padre = fs.raiz;
		free(path_dup);
		return padre;
	}

	*ultimo = '\0';
	inode_t *padre = buscar_inode(path_dup);
	free(path_dup);
	return padre;
}

void
eliminar_inode(inode_t *padre, inode_t *target)
{
	if (!padre || !padre->es_directorio)
		return;

	size_t i;
	for (i = 0; i < padre->cantidad_hijos; i++) {
		if (padre->hijos[i] == target)
			break;
	}

	if (i == padre->cantidad_hijos)
		return;

	// libera recursos del inode
	free(target->nombre);
	if (target->contenido)
		free(target->contenido);
	if (target->es_directorio && target->hijos)
		free(target->hijos);
	free(target);

	// reacomoda array de hijos
	for (size_t j = i; j < padre->cantidad_hijos - 1; j++) {
		padre->hijos[j] = padre->hijos[j + 1];
	}

	padre->cantidad_hijos--;
	padre->hijos =
	        realloc(padre->hijos, sizeof(inode_t *) * padre->cantidad_hijos);
	padre->modificacion = time(NULL);
}

int
agregar_hijo(inode_t *padre, inode_t *hijo)
{
	if (!padre || !padre->es_directorio)
		return -ENOTDIR;

	padre->hijos = realloc(padre->hijos,
	                       sizeof(inode_t *) * (padre->cantidad_hijos + 1));
	padre->hijos[padre->cantidad_hijos] = hijo;
	padre->cantidad_hijos++;
	padre->modificacion = time(NULL);

	return 0;
}

/////////// persistencia

void
guardar_inode(FILE *f, inode_t *node)
{
	fwrite(&node->modo, sizeof(mode_t), 1, f);
	fwrite(&node->usuario_id, sizeof(uid_t), 1, f);
	fwrite(&node->grupo_id, sizeof(gid_t), 1, f);
	fwrite(&node->tamano, sizeof(size_t), 1, f);
	fwrite(&node->acceso, sizeof(time_t), 1, f);
	fwrite(&node->modificacion, sizeof(time_t), 1, f);
	fwrite(&node->cambio_estado, sizeof(time_t), 1, f);
	fwrite(&node->enlaces, sizeof(int), 1, f);
	fwrite(&node->es_directorio, sizeof(int), 1, f);

	size_t nombre_len = strlen(node->nombre);
	fwrite(&nombre_len, sizeof(size_t), 1, f);
	fwrite(node->nombre, 1, nombre_len, f);

	if (node->es_directorio) {
		fwrite(&node->cantidad_hijos, sizeof(size_t), 1, f);
		for (size_t i = 0; i < node->cantidad_hijos; i++) {
			guardar_inode(f, node->hijos[i]);
		}
	} else {
		fwrite(node->contenido, 1, node->tamano, f);
	}
}

inode_t *
cargar_inode(FILE *f)
{
	inode_t *node = calloc(1, sizeof(inode_t));
	fread(&node->modo, sizeof(mode_t), 1, f);
	fread(&node->usuario_id, sizeof(uid_t), 1, f);
	fread(&node->grupo_id, sizeof(gid_t), 1, f);
	fread(&node->tamano, sizeof(size_t), 1, f);
	fread(&node->acceso, sizeof(time_t), 1, f);
	fread(&node->modificacion, sizeof(time_t), 1, f);
	fread(&node->cambio_estado, sizeof(time_t), 1, f);
	fread(&node->enlaces, sizeof(int), 1, f);
	fread(&node->es_directorio, sizeof(int), 1, f);

	size_t nombre_len;
	fread(&nombre_len, sizeof(size_t), 1, f);
	node->nombre = malloc(nombre_len + 1);
	fread(node->nombre, 1, nombre_len, f);
	node->nombre[nombre_len] = '\0';

	if (node->es_directorio) {
		fread(&node->cantidad_hijos, sizeof(size_t), 1, f);
		node->hijos = calloc(node->cantidad_hijos, sizeof(inode_t *));
		for (size_t i = 0; i < node->cantidad_hijos; i++) {
			node->hijos[i] = cargar_inode(f);
		}
	} else {
		node->contenido = malloc(node->tamano);
		fread(node->contenido, 1, node->tamano, f);
	}

	return node;
}

void
liberar_inode(inode_t *node)
{
	if (!node)
		return;

	free(node->nombre);

	if (node->es_directorio) {
		for (size_t i = 0; i < node->cantidad_hijos; i++) {
			liberar_inode(node->hijos[i]);
		}
		free(node->hijos);
	} else {
		free(node->contenido);
	}

	free(node);
}

void
guardar_fs()
{
	FILE *f = fopen(fs.archivo_disco, "wb");
	if (!f) {
		perror("Error abriendo archivo para guardar FS");
		return;
	}
	guardar_inode(f, fs.raiz);
	fclose(f);
}

void
cargar_fs(void)
{
	if (!fs.archivo_disco) {
		fprintf(stderr, "Error: No persistence file specified\n");
		fs.raiz = crear_inode("/", __S_IFDIR | 0755, 1);
		if (!fs.raiz) {
			fprintf(stderr, "Error: Failed to create root inode\n");
		}
		return;
	}

	FILE *f = fopen(fs.archivo_disco, "rb");
	if (!f) {
		printf("[debug] No persistence file found at %s, creating new "
		       "root\n",
		       fs.archivo_disco);
		fs.raiz = crear_inode("/", __S_IFDIR | 0755, 1);
		if (!fs.raiz) {
			fprintf(stderr, "Error: Failed to create root inode\n");
		}
		return;
	}

	fs.raiz = cargar_inode(f);
	if (!fs.raiz) {
		fprintf(stderr,
		        "Error: Failed to load inode tree from %s\n",
		        fs.archivo_disco);
		fclose(f);
		return;
	}

	if (ferror(f)) {
		fprintf(stderr,
		        "Error reading from file %s: %s\n",
		        fs.archivo_disco,
		        strerror(errno));
		liberar_inode(fs.raiz);
		fs.raiz = NULL;
		fclose(f);
		return;
	}

	if (fclose(f) != 0) {
		fprintf(stderr,
		        "Error closing file %s: %s\n",
		        fs.archivo_disco,
		        strerror(errno));
	}
}

/////////// operaciones de fuse

static void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] init called\n");

	fs.archivo_disco = strdup(filedisk);
	if (!fs.archivo_disco) {
		perror("Error al asignar memoria para archivo de disco");
		return NULL;
	}
	cargar_fs();

	return NULL;
}

static int
fisopfs_getattr(const char *path, struct stat *stbuf)
{
	printf("[debug] getattr called for path: %s\n", path);
	memset(stbuf, 0, sizeof(struct stat));
	inode_t *node = buscar_inode(path);
	if (!node)
		return -ENOENT;

	stbuf->st_mode = node->modo;
	stbuf->st_uid = node->usuario_id;
	stbuf->st_gid = node->grupo_id;
	stbuf->st_size = node->tamano;
	stbuf->st_nlink = node->enlaces;
	stbuf->st_atime = node->acceso;
	stbuf->st_mtime = node->modificacion;
	stbuf->st_ctime = node->cambio_estado;

	return 0;
}

static int
fisopfs_readdir(const char *path,
                void *buf,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] readdir called for path: %s\n", path);
	inode_t *dir = buscar_inode(path);
	if (!dir || !dir->es_directorio)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for (size_t i = 0; i < dir->cantidad_hijos; i++) {
		filler(buf, dir->hijos[i]->nombre, NULL, 0);
	}

	return 0;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] mkdir called for path: %s\n", path);
	char nombre[MAX_FILENAME_LENGTH];
	inode_t *padre = buscar_padre(path, nombre);
	if (!padre || !padre->es_directorio)
		return -ENOENT;

	if (buscar_inode(path))
		return -EEXIST;

	inode_t *nuevo = crear_inode(strdup(nombre), __S_IFDIR | mode, 1);
	guardar_fs();
	return agregar_hijo(padre, nuevo);
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] create called for path: %s\n", path);
	char nombre[MAX_FILENAME_LENGTH];
	inode_t *padre = buscar_padre(path, nombre);
	if (!padre || !padre->es_directorio)
		return -ENOENT;

	if (buscar_inode(path))
		return -EEXIST;

	inode_t *nuevo =
	        crear_inode(strdup(nombre), __S_IFREG | (mode & 0777), 0);
	guardar_fs();
	return agregar_hijo(padre, nuevo);
}

static int
fisopfs_read(const char *path,
             char *buf,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] read called for path: %s, size: %lu, offset: %lu\n",
	       path,
	       size,
	       offset);
	inode_t *node = buscar_inode(path);
	if (!node || node->es_directorio)
		return -EISDIR;

	if (offset < node->tamano) {
		if (offset + size > node->tamano)
			size = node->tamano - offset;
		memcpy(buf, node->contenido + offset, size);
	} else {
		size = 0;
	}

	node->acceso = time(NULL);
	return size;
}

static int
fisopfs_write(const char *path,
              const char *buf,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf("[debug] write called for path: %s, size: %lu, offset: %lu\n",
	       path,
	       size,
	       offset);
	inode_t *node = buscar_inode(path);
	if (!node || node->es_directorio)
		return -EISDIR;

	size_t new_size = offset + size;
	if (new_size > node->tamano) {
		node->contenido = realloc(node->contenido, new_size);
		node->tamano = new_size;
	}

	memcpy(node->contenido + offset, buf, size);
	node->modificacion = time(NULL);
	node->cambio_estado = time(NULL);

	guardar_fs();
	return size;
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] unlink called for path: %s\n", path);
	inode_t *node = buscar_inode(path);
	if (!node || node->es_directorio)
		return -EISDIR;

	char nombre[MAX_FILENAME_LENGTH];
	inode_t *padre = buscar_padre(path, nombre);
	eliminar_inode(padre, node);
	guardar_fs();
	return 0;
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] rmdir called for path: %s\n", path);
	inode_t *node = buscar_inode(path);
	if (!node || !node->es_directorio || node->cantidad_hijos > 0)
		return -ENOTEMPTY;

	char nombre[MAX_FILENAME_LENGTH];
	inode_t *padre = buscar_padre(path, nombre);
	eliminar_inode(padre, node);
	guardar_fs();
	return 0;
}

static void
fisopfs_destroy(void *private_data)
{
	printf("[debug] destroy called\n");
	guardar_fs();
	liberar_inode(fs.raiz);
	free(fs.archivo_disco);
}

static int
fisopfs_open(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] open called for path: %s, flags: %d\n", path, fi->flags);

	inode_t *node = buscar_inode(path);
	if (!node) {
		return -ENOENT;  // Archivo no existe
	}
	if (node->es_directorio) {
		return -EISDIR;  // No se puede abrir un directorio
	}

	if ((fi->flags & O_ACCMODE) == O_WRONLY ||
	    (fi->flags & O_ACCMODE) == O_RDWR) {
		if (!(node->modo & S_IWUSR)) {
			return -EACCES;  // Sin permiso de escritura
		}
	}
	if ((fi->flags & O_ACCMODE) == O_RDONLY) {
		if (!(node->modo & S_IRUSR)) {
			return -EACCES;  // Sin permiso de lectura
		}
	}

	if (fi->flags & O_TRUNC) {
		if (node->contenido) {
			free(node->contenido);
			node->contenido = NULL;
		}
		node->tamano = 0;
		node->modificacion = time(NULL);
		node->cambio_estado = time(NULL);
		guardar_fs();
	}

	return 0;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug] truncate called for path: %s, size: %ld\n", path, size);

	inode_t *node = buscar_inode(path);
	if (!node) {
		return -ENOENT;  // Archivo no existe
	}
	if (node->es_directorio) {
		return -EISDIR;  // No se puede truncar un directorio
	}

	if (size == 0) {
		if (node->contenido) {
			free(node->contenido);
			node->contenido = NULL;
		}
		node->tamano = 0;
	} else {
		node->contenido = realloc(node->contenido, size);
		if (!node->contenido && size > 0) {
			return -ENOMEM;  // Error de memoria
		}
		if (size > node->tamano) {
			memset(node->contenido + node->tamano,
			       0,
			       size - node->tamano);
		}
		node->tamano = size;
	}

	node->modificacion = time(NULL);
	node->cambio_estado = time(NULL);
	guardar_fs();
	return 0;
}

static int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	printf("[debug] utimens called for path: %s\n", path);
	inode_t *node = buscar_inode(path);
	if (!node) {
		return -ENOENT;  // Archivo no existe
	}

	// tv[0] es el tiempo de acceso (atime), tv[1] es el tiempo de modificación (mtime)
	node->acceso = tv[0].tv_sec;
	node->modificacion = tv[1].tv_sec;
	node->cambio_estado = time(NULL);  // Actualizar ctime

	guardar_fs();
	return 0;
}

static struct fuse_operations operations = { .init = fisopfs_init,
	                                     .getattr = fisopfs_getattr,
	                                     .readdir = fisopfs_readdir,
	                                     .mkdir = fisopfs_mkdir,
	                                     .rmdir = fisopfs_rmdir,
	                                     .create = fisopfs_create,
	                                     .read = fisopfs_read,
	                                     .write = fisopfs_write,
	                                     .unlink = fisopfs_unlink,
	                                     .destroy = fisopfs_destroy,
	                                     .open = fisopfs_open,
	                                     .truncate = fisopfs_truncate,
	                                     .utimens = fisopfs_utimens };

int
main(int argc, char *argv[])
{
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "--filedisk") == 0) {
			filedisk = argv[i + 1];

			// We remove the argument so that fuse doesn't use our
			// argument or name as folder.
			// Equivalent to a pop.
			for (int j = i; j < argc - 1; j++) {
				argv[j] = argv[j + 2];
			}

			argc = argc - 2;
			break;
		}
	}

	return fuse_main(argc, argv, &operations, NULL);
}