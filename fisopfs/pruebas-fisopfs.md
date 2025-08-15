## TP3: FileSystem FUSE - Pruebas
### Grupo: 36
### Alumnos: Almada, Celano Minig, Fabregat, Fernández
#### Fecha de entrega: 15 de junio de 2025

## Estado inicial del FileSystem

<div align="center">
<img width="80%" src="./pruebas-fisopfs/estadoInicial.png">
</div>

## Archivos

- Se verifico si en nuestro *fs* se podían crear archivos usando: *touch*, *redirigiendo la salida del echo (ya sea a un archivo "común" o un archivo binario)*

<div align="center">
<img width="80%" src="./pruebas-fisopfs/crearArchivos.png">
</div>

- Se verifico si en nuestro *fs* se podían leer los archivos anteriormente creados.

<div align="center">
<img width="80%" src="./pruebas-fisopfs/lecturaArchivos.png">
</div>

- Se verifico si en nuestro *fs* se podía escribir sobre nuestros archivos, ya sea redirigiendo la salida de *echo* y truncar el archivo utilizando *>>*.

<div align="center">
<img width="80%" src="./pruebas-fisopfs/escrituraArchivos.png">
</div>

- Se verifico si en nuestro *fs* se podían borrar nuestros archivos usando el comando *rm* o *unlink*.

<div align="center">
<img width="80%" src="./pruebas-fisopfs/borradoArchivos.png">
</div>

## Directorios

- Se verifico si en nuestro *fs* se podían crear directorios y/o subdirectorios.

<div align="center">
<img width="80%" src="./pruebas-fisopfs/directoriosCreacion.png">
</div>

- Se verifico si en nuestro *fs* se podían leer estos directorios y/o subdirectorios.

<div align="center">
<img width="80%" src="./pruebas-fisopfs/borradoArchivos.png">
</div>

- Se verifico si en nuestro *fs* se podían borrar estos directorios (vacios).

<div align="center">
<img width="80%" src="./pruebas-fisopfs/eliminacionDirectorios.png">
</div>

- Se verifico si en nuestro *fs* se podían borrar estos directorios (con archivo dentro).

<div align="center">
<img width="80%" src="./pruebas-fisopfs/directoriosEliminarNoVacios.png">
</div>

## Estadisticas y fechas

- Se verifico si en nuestro *fs* si se podia.

<div align="center">
<img width="80%" src="./pruebas-fisopfs/estadisticasYfechas.png">
</div>

## Usuarios y grupos

- Se verifico si en nuestro *fs* se podían distinguir los usuarios y los grupos de estos.

<div align="center">
<img width="80%" src="./pruebas-fisopfs/usuariosYgrupos.png">
</div>