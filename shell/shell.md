# shell

## Búsqueda en $PATH

### 1. ¿cuáles son las diferencias entre la syscall `execve(2)` y la familia de wrappers proporcionados por la librería estándar de C (libc) `exec(3)`?

- La principal diferencia es que `execve(2)` es una syscall del sistema operativo, mientras que las funciones `exec(3)` son wrappers que proporciona la libc para facilitar su uso. 

`execve(2)` es la más "básica" y flexible, porque nos deja pasar directamente la ruta absoluta del ejecutable, un array con los argumentos (`char **argv`) y otro array con las variables de entorno (`char **envp`). Es decir, tenemos control total sobre cómo se lanza el programa.

En cambio, la familia `exec(3)` incluye varias funciones como `execl()`, `execp()`, `execle()`, etc., que internamente usan `execve()`, pero nos simplifican algunas cosas. Por ejemplo, algunas buscan el ejecutable automáticamente en el PATH, otras nos dejan pasar los argumentos como una lista separada en vez de un array, y otras directamente usan el entorno del proceso actual, sin que tengamos que armarlo nosotros. Entonces, si bien todas terminan llamando a `execve()`, la diferencia está en la comodidad que nos ofrecen según lo que necesitemos hacer.

### 2. ¿Puede la llamada a `exec(3)` fallar? ¿Cómo se comporta la implementación de la shell en ese caso?

- La llamada a cualquier función de `exec(3)` puede fallar. En la implementación de nuestra shell, usamos `execvp()` que recibe el nombre del comando para buscar el ejecutable en el `PATH` y el arreglo de argumentos que acompaña al comando. 

- Nuestra shell va leyendo línea por línea en `sh.c`. La línea es pasada a la función `run_cmd()` para ser parseada. Luego se llama a `fork()` para crear un proceso hijo donde se llamará a `exec_cmd()` para ejecutar el comando correspondiente.
Si el comando es de tipo `EXEC`, se usa `execvp()` para intentar ejecutarlo. Si falla (devuelve `-1`), el proceso hijo imprime un mensaje de error y termina con `exit()` pasando el código de error. El padre espera al hijo, imprime el estado del comando y se vuelve a leer otra línea para repetir el ciclo.

---

## Flujo estándar

### 1. Investigar el significado de 2>&1, explicar cómo funciona su forma general

 **2>&1** permite redirigir el stderr al mismo lugar del stdout actual.

La forma general es:

    [origen]>&[destino]

Donde:
- [origen] es el descriptor que queremos redirigir (por ejemplo, 2 para stderr),

- [destino] es el descriptor al que se quiere redirigir (por ejemplo, 1 para stdout).

#### a. Mostrar qué sucede con la salida de cat out.txt en el ejemplo.
```bash
$ ls -C /home /noexiste >out.txt 2>&1

$ cat out.txt

ls: cannot access '/noexiste': No such file or directory
/home:

<se mostrarian los directorios de cada uno de los usuarios>
```
	
#### b. Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1).
	
```bash
$ ls -C /home /noexiste 2>&1 >out.txt
	
$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
<se mostrarian los directorios de cada uno de los usuarios>
```

- No cambió nada.

- En bash, cuando ejecutamos `ls -C /home /noexiste >out.txt 2>&1`, primero se redirige el `stdout` al archivo `out.txt`, y luego el `stderr` se redirige al mismo lugar donde está apuntando `stdout` en ese momento, que ya es `out.txt`. Por eso, tanto la salida estándar como el error se guardan en el mismo archivo.
En cambio, si usamos `ls -C /home /noexiste 2>&1 >out.txt`, el orden cambia el resultado. Primero se redirige el `stderr` al lugar donde está apuntando `stdout` (que aún es la terminal), y después `stdout` se redirige al archivo `out.txt`. Como resultado, el error se muestra en pantalla y solo la salida estándar va al archivo.
En bash, las redirecciones se van ejecutando secuencialmente, contrariamente a lo que sucede en nuestra shell, donde este orden no se respeta de la misma forma, por lo que ambos comandos terminan teniendo el mismo efecto.
---

## Tuberías múltiples

### 1. Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe
#### a. ¿Cambia en algo?
- Al ejecutar un comando con pipes, no se muestra ningun codigo de salida ya que en la funcion
`print_status_info(struct cmd *cmd)` de `printstatus.c`, cuando verificamos que el cmd recibido es de tipo PIPE, se termina la ejecucion de la misma.

#### b. ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.

- Queremos ejecutar el siguiente conjunto de comandos usando pipes donde el ultimo comando no existe:  
    `ls -l | grep Doc | comando_no_existe`

    Si lo ejecutariamos en bash, pasaria lo siguiente
    ```bash
    $ ls -l | grep Doc | comando_no_existe
    comando_no_existe: command not found
    ```
    En nuestra shell ocurriria algo similar:
    ```bash
    $ ls -l | grep "Doc" | comando_no_existente    
    Error al ejecutar comando: No such file or 
    directory
    ```




## Variables de entorno temporarias


### 1. ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?

- Es necesario establecer variables de entorno temporales después de la llamada a `fork(2)` para evitar modificar las variables de entorno del proceso padre.  
Si se cambian las variables de entorno antes de `fork()`, esas modificaciones afectarían tanto al proceso padre como al hijo. Al hacerlo después de `fork()`, aseguramos que solo el proceso hijo tenga acceso a las nuevas variables de entorno temporales, que se usarán al ejecutar el comando correspondiente, sin afectar al proceso padre.


### 2. En algunos de los wrappers de la familia de funciones de `exec(3)` (las que finalizan con la letra `e`), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar `setenv(3)` por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de `exec(3)`.

#### a. ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.

- El resultado final es el mismo, pero el procedimiento es diferente:  
Cuando ejecutamos un comando en el proceso hijo creado para esa tarea, usamos `setenv(3)` para configurar el entorno del proceso dicho, antes de ejecutar `execvp()` (de `exec(3)`).  
Si usaramos el vector de argumentos de entorno para colocarlo en alguna de las funciones de `exec(3)` (como por ejemplo, `execve()`), el entorno actual del proceso hijo no se vería afectado, sino que solo se cambiaría una vez que se ejecute la función correspondiente para cambiar la imagen del proceso.

#### b. Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

- Para lograr que el comportamiento sea el mismo al usar el arreglo de variables de entorno en vez de `setenv(3)` antes de la llamada a `exec(3)`, podemos seguir estos pasos:
  1. Antes de llamar a `exec(3)`, podemos preparar un arreglo de strings que contenga las nuevas variables de entorno que queremos pasar a la nueva imagen del proceso.
  2. Al momento de llamar a una función de `exec(3)` como `execve()`, en vez de modificar el entorno del proceso hijo con `setenv(3)`, pasamos este arreglo como el tercer argumento de la función. Este arreglo contendría todas las variables de entorno que se desean para la ejecución del comando.

## Pseudo-variables

### 1. Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).

#### `$!` : *contiene el PID del ultimo proceso ejecutado segundo plano.*

- Ejemplo en bash:

    ```bash
    $ sleep 30 &   # lanza el proceso en segundo plano con &
    [1] 7413
    $ echo $!
    7413   # ← PID del proceso 'sleep'
    ```

#### `$#` : *contiene la cantidad de argumentos pasados a un script.*
- Ejemplo en bash :
  
	Supongamos que tenemos un script llamado archivo.sh que contiene lo siguiente:

    ```bash
    #!/bin/bash
    echo "Me pasaste $# argumentos"
    ```
	
	Si ejecutamos el script pasandole argumentos, sucederia esto:
    ```bash
	$ ./archivo.sh uno dos tres
	
    Me pasaste 3 argumentos
    ```
#### `$PWD`: *contiene la ruta absoluta del directorio en el que se está ubicado actualmente.*

- Ejemplo en bash:

    ```bash
    $ echo $PWD
    <se mostraria la ruta absoluta>
    ```

---

## Comandos built-in

### 1. ¿Entre `cd` y `pwd`, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)

- Podriamos hacer que `pwd` se convierta en un comando externo. Porque solo muestra informacion de la ruta absoluta del directorio actual y no influye en la configuracion interna de la shell.
- El motivo por el cual `pwd` es un comando built-in, es que la shell contiene internamente la ruta del directorio actual. Se usa ese valor para mostrarlo por la pantalla de la shell sin la necesidad de crear un proceso nuevo, lo cual mejora el rendimiento.  
Por otro lado, el comando cd debe ser built-in, ya que modifica el estado interno de la shell (el directorio de trabajo), y si fuera un comando externo, ese cambio solo afectaría al proceso hijo, sin tener impacto en la shell principal.

---

## Procesos en segundo plano


### 1. ¿Por qué es necesario el uso de señales?

- El uso de señales es necesario porque proporcionan un mecanismo de comunicación asincrónica entre procesos. Son esenciales para gestionar la ejecución de los procesos de manera eficiente, permitiendo interrumpir, notificar o modificar el comportamiento de un proceso sin necesidad de que este se detenga explícitamente. Las señales son utilizadas para terminar procesos (como SIGKILL), manejar errores (como SIGSEGV para violación de segmentación), o coordinar la finalización de procesos hijos, como en el caso de SIGCHLD, que permite al proceso padre esperar a que el hijo termine su ejecución. 

---

### Historial

---
