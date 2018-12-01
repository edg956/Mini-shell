#define _POSIX_C_SOURCE 200112L
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <wait.h>
#include "aventura2.h"
#include <sys/types.h>
#include <signal.h>

/******************************************************************************
                            VARIABLES GLOBALES
******************************************************************************/
char *program_name = NULL;
int n_pids;
/*
    Array que guarda los diferentes estados que toman los procesos durante
    su ejecución en primer y segundo plano.
*/
static struct info_process jobs_list[ARGS_SIZE];

void main(int argc, char *argv[]) {

    program_name = malloc(COMMAND_LINE_SIZE);
    program_name = strcpy(program_name, argv[0]);

    //Declaraciones
    char *line = malloc(COMMAND_LINE_SIZE);

    //Indicar a main que escuche señales SIGCHLD, SIGINT y SIGTSTP
    signal(SIGCHLD, reaper);
    signal(SIGINT, ctrlc);
    signal(SIGTSTP, ctrlz);

    //Inicializar estructura jobs_list[]
    jobs_list[0].pid = 0;
    jobs_list[0].status = 'F';
    jobs_list[0].command_line[0] = '\0';

    print_prompt();

    while (1) {
        execute_line(read_line(line));
        print_prompt();
    }

}

/******************************************************************************
                         FUNCIONES DE LA AVENTURA 2
******************************************************************************/
/*
 * Función obtiene el comando escrito por usuario en la terminal y limpia el
 * buffer de la entrada de datos estandar stdin.
 * Guarda el comando en el puntero del argumento 'line'.
 * 
 * Parámetros:
 *          +line: Puntero al espacio en memoria a partir del cual se almacenará
 *                 el comando escrito por el usuario.
 * Devuelve:
 *          +line: Puntero al espacio donde se encuentra almacenado el comando
 *                 entrado.
 */
char *read_line(char *line) {
    char *command = fgets(line, COMMAND_LINE_SIZE, stdin);

    /*
        PREGUNTAR A PROFESORA:

        En la documentación del nivel 4 se dice que hay que comprobar en caso 
        de que: la linea leída sea NULL, y que no se haya producido un EOF.
        (¿Quiere decir que no se haya llegado al final de fichero, por tanto:
        feof(stdin) habría de ser == 0?)

        Creo que es bastante claro que es una estructura de control para en
        caso de tener una linea vacía y presionar Ctrl-D evitar un Core-Dumped
    */
    if (!command && !feof(stdin)) {
        command = line;
        command[0] = 0;
        puts("");
    }

    fflush(stdin);
    return command;
}

/**
 * Funcion que ejecuta el comando contenido en el String 'line'.
 * Implementa la función check_internal para determinar si se trata de un
 * comando interno o externo. 
 * 
 * En caso de ser interno, check_internal se encarga de realizar el llamado a
 * la función correspondiente.
 * 
 * En caso de tratarse de un comando externo, execute_line bifurca la ejecución
 * en dos procesos (padre e hijo).
 * 
 * El proceso hijo se encarga de ejecutar el comando externo indicado.
 * El proceso padre espera a que el proceso hijo acabe para recogerlo e indicar
 * el final del mismo.
 * 
 * Parámetros:
 *      +line: Puntero al String que contiene el comando.
 * Return:
 *      +0: Si la operacion se realizo sin problemas.
 *      +(-1): En caso de error.
 */
int execute_line(char *line) {
    
    //Exit cuando Ctrl+D es presionado y se devuelve un puntero NULL.
    if (line == 0) {
        puts(""); //Salto de linea al volver a Bash.
        exit(1);
    }

    //Declaraciones
    char **args = malloc(ARGS_SIZE);
    char *linecpy = malloc(COMMAND_LINE_SIZE);
    
    //Copia para procesos hijos en ejecución
    linecpy = strcpy(linecpy,line);

    int nTokens;

    nTokens = parse_args(args, line); //Nro. de Tokens obtenidos.

    //Check si el comando ha sido un comando interno o externo
    if (check_internal(args) == 0) {
        //Ejecución de comando externo
        pid_t cpid;

        //Creación de proceso hijo para ejecutar comando externo
        cpid = fork();
        
        if (cpid == -1) {   //Check errores en fork()
            fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }

        //Código para proceso hijo
        if (cpid == 0) {
            
            signal(SIGINT, SIG_IGN); //AGREGAR EN NIVEL 5
            signal(SIGCHLD, SIG_DFL);

            printf("[execute_line(): PID proceso padre: %d (%s)]\n", getppid(), program_name);
            printf("[execute_line(): PID proceso hijo: %d (%s)]\n", getpid(), args[0]);

            //Check errores en execvp(). Enviar error por stderr y realizar
            if (execvp(args[0],args) == -1) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(1);
            }
        }
        
        //Código para proceso padre:  

        //Guardar información de proceso en foreground
        jobs_list[0].pid = cpid;
        jobs_list[0].status = 'E';
        char *linecop = &jobs_list[0].command_line[0];
        linecop = strcpy(linecop, linecpy);

        //Enviar señal al enterrador de hijos
        signal(SIGCHLD, reaper);
        signal(SIGINT, ctrlc);

        while (jobs_list[0].pid != 0) {
            pause();
        }

    }

    //Libera variables locales auxiliares
    free(linecpy);
}

/**
 * Funcion que toma el String 'line' que contiene el ultimo comando 
 * ejecutado por el usuario y lo trocea en tokens para guardarlos en 
 * el vector de tokens 'args'.
 * 
 * Parámetros:
 *      +args: Doble puntero a varios String que contendrán los tokens 
 *             obtenidos del comando.
 *      +line: Puntero al String que contiene el comando.
 * Return: 
 *      +count: Contador con el numero de tokens obtenidos.
 * 
 */
int parse_args(char **args, char *line) { 
    int count = 0;
    const char delim[] = " \t\n\r";

    *args = strtok(line, &delim[0]);

    while (*args != NULL) {
        if (*args[0] == '#') {
            *args = NULL;
        } else {
            count += 1;
            args++;
            *args = strtok(NULL, &delim[0]);
        }
    }
    return count;
    
}

/** 
 * Función que averigua si se trata de un comando interno mediante la funcion
 * strcmp(), y llama a la funcion correspondiente para tratarlo: 
 *      -internal_cd() 
 *      -internal_export()
 *      -internal_source()
 *      -internal_jobs()
 * 
 * Parámetros:
 *          +args: doble puntero que contiene tokens obtenidos del comando
 *                 introducido separado por " ". 
 * Return:
 *          +0: En caso de no tratarse de un comando interno.
 *          +1: En caso de ejecutar un comando interno.
 */
int check_internal(char **args){
     if (*args == NULL) {
        return -1;
    } else {
        if (strcmp(args[0], "cd") == 0) {
              return internal_cd(args);
        } else if (strcmp(args[0], "export") == 0) {
              return internal_export(args);
        } else if (strcmp(args[0], "source") == 0 ||
                    strcmp(args[0], ".") == 0) {
              return internal_source(args);
        } else if (strcmp(args[0], "jobs") == 0) {
              return internal_jobs(args);
        } else if (strcmp(args[0], "exit") == 0) {
              exit(0);
        } else {
            return 0;
        }
    }
}

/**
 * Función que cambia el directorio actual hacia el indicado en args.
 * Modifica los valores de la variable de entorno "PWD" cuando se
 * realiza un chdir() exitoso.
 * 
 * Cuando args[2] != NULL recompone todo el argumento dirección para verificar
 * si cumple con los formatos correctos para ficheros con espacios (" ") en su
 * nombre.
 * 
 * Si la dirección para ficheros de ese tipo no cumple con los formatos:
 * 
 *      home/user/'carpeta ejemplo'/
 *      home/user/carpeta\ ejemplo/
 * 
 * Le indica al usuario que ha introducido argumentos de más en el comando.
 * 
 * Parámetros: 
 *          +args: doble puntero que contiene tokens obtenidos del comando
 *                   introducido separado por " ".
 * Return:   
 *          +1: cd se ejecutó sin ningún tipo de errores sin tratar.
 *          +(-1): se produjo algún error en manipulación de variables de
 *                 entorno.
 */
int internal_cd(char **args) {
    char *path = malloc(COMMAND_LINE_SIZE);

    printf("Current path: %s\n",getcwd(NULL,COMMAND_LINE_SIZE));

    /*Chequea casos posibles de entrada para comando cd:
        Casos:  "cd" sin argumentos.
                "cd ruta/a/'carpeta con espacios'" (más de un arg)
                "cd un_solo_arg"
    */
   
    //Caso "cd"
    if (args[1] == NULL) {  

        if (chdir(getenv("HOME")) != 0) {
            perror("Error");
        }

        if (setenv("PWD", getenv("HOME"), 1) == -1) {
            perror("Error");
            return -1;
        }

        printf("New path: %s\n",getcwd(NULL,COMMAND_LINE_SIZE));
        return 1;

    }

    //Caso "cd 'carpeta ejemplo'" || "cd carpeta ejemplo" (sin formato)
    if (args[2] != NULL) {   
        
        //Regeneración del argumento entero para verificar validez de formato
        int k = 1;
        char *auxpath = malloc(COMMAND_LINE_SIZE);
        
        while (args[k] != NULL) {
            auxpath = strcat(auxpath, args[k]);
            auxpath = strcat(auxpath," ");
            k++;
        }
        
        path = strncpy(path,auxpath,strlen(auxpath)-1); //Elimina espacio 
                                                     //agregado en último strcat

        //Análisis de validez de argumento. Ver check_formato()
        if (check_formato(path) == -1) {
            puts("cd: demasiados argumentos.");
            return 1;
        }

        /*
            Si path cumple con el formato de entrada, se recupera la dirección
            completa para ser tratada y eliminar caracteres no aceptados.
        */
      
        char *delim = "'\\\"";
        char *aux = strtok(path, delim);
        free (auxpath);
        auxpath = malloc(COMMAND_LINE_SIZE);
        while (aux != NULL) {
            auxpath = strcat(auxpath,aux);
            aux = strtok(NULL,delim);
        }
        path = auxpath;     //path apunta a la ruta deseada para chdir()

    } else {    //Caso 1 solo argumento "cd carpeta" || "cd carpeta_ejemplo"

        path = strcpy(path,args[1]);

    }

    //Change directory
    if (chdir(path) != 0) { //Check for errors
        
        if (errno == ENOENT) {
            puts("cd: Archivo o fichero inexistente.");
        } else {
            perror("cd: error");
        }
        return -1;

    } else {

        //Update variable de entorno
        path = getcwd(path, COMMAND_LINE_SIZE);
        if (setenv("PWD", path, 1) != 0) {
            perror("Error en setenv()");
        }

    }
    printf("New path: %s\n",getcwd(NULL,COMMAND_LINE_SIZE));
    return 1;
    
}

/**
 * Descompone en tokens el argumento NOMBRE=VALOR.
 * Notifica de la sintaxis correcta si los argumentos no son los adecuados,
 * utilizando la salida estándar de errores stderr.
 * 
 * En este nivel, muestra por pantalla mediante la función getenv() el valor
 * inicial de la variable (en niveles posteriores eliminarlo).
 * 
 * Utiliza la función setenv() para asignar el nuevo valor.
 * 
 * En este nivel, muestra por pantalla el nuevo valor mediante la función
 * getenv() para comprobar su funcionamiento.
 */
int internal_export(char **args){

    //Check existencia de argumento
    if (args[1] == NULL)  {
        fprintf(stderr, "%s\n", "Incorrect sintax. \nThe sintax is: export VAR_NAME=NEW_VALUE\n");
        return -1;
    }

    char *argument = malloc(COMMAND_LINE_SIZE);

    /*
        Check si VALUE es argumento compuesto del tipo:
            NAME="NEW VALUE";
            NAME='NEW VALUE';
            NAME=NEW\ VALUE;
    */
    if (args[2] != NULL) {
        //Reconstrucción de argumentos para comprobar validez
        char *auxarg = malloc(COMMAND_LINE_SIZE);
        for (int i = 1; args[i] != NULL; i++) {
            auxarg = strcat(auxarg, args[i]);
            auxarg = strcat(auxarg, " ");
        }
        argument = strncpy(argument,auxarg,strlen(auxarg)-1); //Eliminar ' '

        //Análisis de validez de argumento. Ver check_formato()
        if(check_formato(argument) == -1) {
            fprintf(stderr, "%s\n", "Incorrect sintax. \nThe sintax is: export VAR_NAME=NEW_VALUE\n");
            return -1;
        }

        free(auxarg);
        auxarg = malloc(COMMAND_LINE_SIZE);
        char *delim ="'\\\"";
        char *aux = strtok(argument,delim);
        //Limpiar argumento de símbolos de formato '\"' '\'' '\\'
        while (aux != NULL) {
            auxarg = strcat(auxarg,aux);
            aux = strtok(NULL,delim);
        }

        argument = auxarg;

    } else { //Caso de un solo argumento
        argument=strcpy(argument,args[1]);
    }

    //Separar nombre y valor. 
    const char delim[] = "=";
    char *nombre = strtok(argument, &delim[0]);
    char *valor = strtok(NULL,&delim[0]);

    printf("Nombre: %s\n", nombre);
    printf("Valor: %s\n", valor);

    if (valor == NULL) {
        fprintf(stderr, "%s\n", "Incorrect sintax. \nThe sintax is: export VAR_NAME=NEW_VALUE\n");
        return 1;
    }

    printf("Valor inicial de la variable %s: %s\n", nombre, getenv(nombre));
    setenv(nombre, valor, 1);
    printf("Valor final de la variable %s: %s\n", nombre, getenv(nombre));
    return 1;
    
}

/**
 * Funcion que abre un fichero de scripts y ejecuta cada linea desde el contexto
 * del terminal que le ejecuta.
 * 
 * Parámetros:
 *          +args: Doble puntero a Strings que contienen los tokens del comando.
 *                 Cada token representa un argumento del comando.
 * Return:
 *          +1: Si todo se ha ejecutado sin problemas.
 *          +(-1):  Si ha ocurrido un error en la ejecución del source
 */
int internal_source(char **args) {
    FILE *f;
    //Apertura de fichero
    f = fopen (args[1], "r");
    if (f == NULL){ //Check for errors
        imprime_error(NULL);
    }
    else {
        //Lectura y ejecución linea a linea hasta llegar a fin de fichero.
        char linea[COMMAND_LINE_SIZE];
        while (fgets(linea, COMMAND_LINE_SIZE, f) != NULL){
            execute_line(linea);
            fflush(f);
        }
        fclose(f);
    }
    return 1;
}

int internal_jobs(char **args) {

    return 1;
}

/**
 * Función que se encarga de 'enterrar' procesos hijos de un proceso para
 * evitar la existencia de procesos zombies en la tabla de procesos.
 * 
 * Una vez un proceso hijo que se ejecuta en primer plano acaba, el reaper se
 * encarga de 'enterrarle' y además actualiza su estado en jobs_list[0] -el
 * proceso en primer plano- de la siguiente manera:
 *      jobs_list[0].pid = 0;
 *      jobs_list[0].status = 'F'
 *      jobs_list[0].command_line: Se reemplaza su contenido por un '\0' en la
 *                                 posición 0 del array, conviertiendole en un
 *                                 String vacío.
 * 
 * Parámetros:
 *          signum: Número de la señal que recibe: SIGCHLD
 * Return:  Void method. Returns nothing.
 */
void reaper(int signum) {

    signal(SIGCHLD, reaper);

    pid_t pid;
    int wstatus;

    //Check errores en wait()
    while ((pid = waitpid(-1, &wstatus, WNOHANG)) > 0) {
        if (pid == jobs_list[0].pid) {
            jobs_list[0].pid = 0;
            jobs_list[0].status = 'F';
            jobs_list[0].command_line[0] = '\0';
        }

        //Indicar razón de finalización de proceso hijo.
        if (WIFEXITED(wstatus)) {
            printf("[reaper(): Proceso hijo %d finalizado. Estado=%d]\n", pid, WEXITSTATUS(wstatus));
        } else if (WIFSIGNALED(wstatus)) {
            printf("[reaper(): Proceso hijo %d exterminado por señal %d]\n", pid, WTERMSIG(wstatus));
        } else if (WIFSTOPPED(wstatus)) {
            printf("[reaper(): Proceso hijo %d detenido por señal %d]\n", pid, WSTOPSIG(wstatus));
        }
    }
}

/**
 * Esta función se encarga de recoger y tratar la señal SIGINT que genera la 
 * combinación de teclas Ctrl + c. Se comprueba si hay procesos en foreground 
 * y de ser así, se comprueba si el proceso que está en foreground es el
 * minishell. Si es el minishell se notifica por pantalla, sino, se "mata" el 
 * proceso correspondiente. 
 * 
 * Si no hay procesos en foreground, simplemente se notifica a través de 
 * pantalla. 
 * 
 * Parámetros: 
 *            +signum: Número de la señal que recibe: SIGINT.
 * Return: 
 *            +Método void. No devuelve ningún valor. 
 */

void ctrlc(int signum) {

    //Porque esta linea? Esta en el main ya.
    signal(SIGINT, ctrlc);

    //Declaraciones
    char *error = NULL; 
    char *line = malloc(COMMAND_LINE_SIZE);
    char *pnamecpy = malloc(COMMAND_LINE_SIZE);
    
    //Ver que haya algún proceso en foreground
    if (jobs_list[0].pid > 0) {
        /* 
            Copiar nombre de programa y la linea de comando del proceso en
            para poder modificarlos y compararles
        */
        pnamecpy = strcpy(pnamecpy, program_name); //Copia program_name
        strcat(pnamecpy, "\n");

        char *delim = " ";
        line = strcpy(line, jobs_list[0].command_line); //Copia linea de
        line = strtok(line, delim);                     //proceso en fg

        //Comparar que el proceso en foreground no sea el shell
        if (strcmp(line, pnamecpy) != 0) {
      
            kill(jobs_list[0].pid, SIGTERM);
            puts("\nMatamos el proceso en foreground.");

        } /*else {
            /*
            DISCUTIR NECESIDAD DE ESTA PARTE DE CODIGO:

            El único caso en que se puede dar esto es que desde el mini shell,
            se haya ejecutado el archivo compilado de la aventura2 ej:
            eugenio@mini_shell:ruta$ ./mini_shell.

            En este caso, tanto la linea del último comando, como el nombre del
            programa coinciden.

            SIGING llega a ambos shells, haciendo que ctrlc se ejecute dos veces:
            1 por cada shell en ejecución. En cada shell ocurre lo siguiente:

            [Contexto: desde el terminal se ejecuta ./mini_shell.
                       desde el mini shell se ejecuta ./mini_shell y
                       desde el mini_shell hijo se presiona ctrl+c]

            Shell hijo: Al no tener ningún proceso en foreground (ejecutamos
                        ctrl+c directamente) se imprime un mensaje de error
                        notificando que la señal SIGTERM no se ha enviado 
                        porque no hay proceso en foreground

            Shell padre: Al tener un proceso en foreground (./mini_shell) y
                         coincidir el argumento 0 del proceso hijo con el
                         nombre de programa (variable program_name) del shell
                         padre, se imprime por pantalla el error correspondien-
                         te a que el proceso en foreground es el shell.
            
            Problema: Se imprimen dos mensajes de error cuando uno -el del 
                      hijo- basta.

            
        /
            error = "Señal SIGTERM no enviada debido a que el proceso en foreground es el shell.";
            imprime_error(error);
        }*/

    } else {
        
        error = "\nSeñal SIGTERM no enviada debido a que no hay proceso en foreground.";
        imprime_error(error);

    }

    //Libera variables locales auxiliares
    free(line);
    free(pnamecpy);

}

/**
 * Esta función se encarga de recoger y tratar la señal SIGTSTP que genera la 
 * combinación de teclas Ctrl + z. Se comprueba si hay procesos en foreground 
 * y de ser así, se comprueba si el proceso que está en foreground es el
 * minishell. Si es el minishell se notifica por pantalla, sino, se detiene el 
 * proceso correspondiente y se modifican sus datos. 
 * 
 * Si no hay procesos en foreground, simplemente se notifica a través de 
 * pantalla. 
 * 
 * Parámetros: 
 *            +signum: Número de la señal que recibe: SIGTSTP.
 * Return: 
 *            +Método void. No devuelve ningún valor. 
 */
void ctrlz(int signum) {

    signal(SIGTSTP, ctrlz);

    //Declaraciones
    char *error = NULL;
    char *line = malloc(COMMAND_LINE_SIZE);
    char *pnamecpy = malloc(COMMAND_LINE_SIZE);

    //Ver que haya algún proceso en foreground
    if (jobs_list[0].pid > 0)   {
        /* 
            Copiar nombre de programa y la linea de comando del proceso en
            para poder modificarlos y compararles
        */
        pnamecpy = strcpy(pnamecpy, program_name); //Copia program_name
        strcat(pnamecpy, "\n");

        char *delim = " ";
        line = strcpy(line, jobs_list[0].command_line); //Copia linea de
        line = strtok(line, delim);                     //proceso en fg

        //Comparar que el proceso en foreground no sea el shell
        if (strcmp(line, pnamecpy) != 0) {

            kill(jobs_list[0].pid, SIGTSTP);
            puts("\nParamos el proceso en foreground.");
            jobs_list[0].status = 'D';

            //PENDIENTE

           /* 
           -> Añadir los datos del proceso detenido a job_list[n_pids] utilizando jobs_list_add().
           -> Resetear los datos de job_list[0] ya que el proceso ha dejado de ejecutarse en foreground.
           */

        }
        else  {
            imprime_error("\nSeñal SIGTSTP no enviada debido a que el proceso en foreground es el shell");
        }
    }
    else   {

        imprime_error("\nSeñal SIGTSTP no enviada debido a que no hay proceso en foreground");
    }

    //Libera variables locales auxiliares
    free(line);
    free(pnamecpy);

}

/**
 * Comprueba si la línea de comandos contiene el símbolo "&" al final. Eso 
 * indica que dicha línea deberá ejecutarse en "background". 
 * 
 * Parámetros: 
 *              + char **args: Línea de comandos.  
 * Return: 
 *              + Devuelve 0 si no finaliza con "&". 
 *                Devuelve 1 si finaliza con "&". 
 *            
 */
int is_background (char **args) {
  int i = 0;
  while (args[i] != NULL) {
    i++;
  }
  if (strcmp(args[i-1], "&") == 0) {
    args[i-1] = NULL;
    return 1;
  }
  return 0;
}

/**
 * Encargado de añadir un nuevo trabajo en caso de que no se haya llegado al 
 * número de trabajos máximo. 
 * 
 * Parámetros: 
 *            + pid_t pid: PID del proceso que estará asociado al nuevo trabajo. 
 *            + char status: Estado del proceso. (D (Detenido), E (Ejecutando)).
 *            + char *command_line: Línea de comandos. 
 * Return: 
 *            + Devuelve 0 si el proceso se ha podido llevar a cabo. (Límite
 *              de trabajos no superado). 
 *            + Devuelve 1 si el proceso ha fallado. (Límite de trabajos 
 *              superado).
 */
int jobs_list_add(pid_t pid, char status, char *command_line){
    if (n_pids < ARGS_SIZE){
        n_pids++;
        jobs_list[n_pids].pid = pid;
        jobs_list[n_pids].status = status;
        strcpy(jobs_list[n_pids].command_line, command_line);
        return 0;
    }else{
        return -1;
    }
}

/**
 * Busca dentro del array de trabajos a través del PID. 
 * 
 * Parámetros: 
 *              + pid_t PID: PID del proceso asociado al trabajo que se desea
 *                buscar. 
 * Return:
 *              + Devuelve la posición dentro del array de trabajos del trabajo 
 *                que corresponde con el PID que se ha pasado por parámetro.     
 */
int jobs_list_find(pid_t pid) {
  int pos = 1;
  while (jobs_list[pos].pid != pid){
    pos++; 
  } 
  return pos;
}

/**
 * Elimina el trabajo especificado en la parte paramétrica. Mueve el registro
 * del último proceso de la lista a la posición del que eliminamos. 
 * Se decrementa la variable global "n_pids". 
 * 
 * Parámetros
 *              + int pos: Posición del elemento que se desea eliminar. 
 * Return: 
 *              + Devuelve 0 si el proceso ha concurrido correctamente. 
 */
int jobs_list_remove(int pos) {
    if (pos < n_pids){
        jobs_list[pos] = jobs_list[n_pids];
    }
    n_pids--;
    return 0;
}
/******************************************************************************
                             FUNCIONES DE APOYO
******************************************************************************/
/**
 * Metodo que imprime a través de la salida estandar de errores un mensaje
 * indicando:
 *      Número de error (errno): mensaje de error.
 * En caso de que el parámetro mensaje_error == NULL, mensaje de error será el
 * String devuelto por strerror(errno) -el error correspondiente a errno.
 * 
 * En caso contrario, se imprime el mensaje_error recibido.
 * 
 * Parámetros:
 *          +mensaje_error: String con el mensaje de error correspondiente.
 * 
 * Return: void method. Returns nothing.
 */
void imprime_error(char *mensaje_error) {
    if (mensaje_error == NULL) {
        fprintf(stderr, ROJO_F"Error %d: %s", errno, strerror(errno));
    
    } else {
        fprintf(stderr, ROJO_F"%s", mensaje_error);
    }
    fprintf(stderr, RESET_COLOR"%s", "");
}

/**
 * 
 * Funcion que imprime el prompt al usuario por consola, indicando
 * la ruta del directorio donde se encuentra al usuario, y recoje el
 * comando introducido guardandolo en 'line'.
 * 
 * Parámetros:
 *      +line: Puntero al String donde guardar la linea del comando
 * Return:
 *      +pline: Puntero al String que contiene el comando
 */
void print_prompt() {

    //Declaraciones
    char *user = getenv("USER");

    //Obtención del hostname. 
    char hostName[256];
    hostName[256] = '\0';
    gethostname(hostName, 256);

    char cwd[256]; 
    //char *path = getcwd(cwd, sizeof(cwd));
    char *path = getenv("PWD");

    printf(CYAN_T"%s@%s"RESET_COLOR":"AMARILLO_T"%s"RESET_COLOR"%c ", user, hostName, path, PROMPT);
}

/**
 * Función que verifica que el String 'argument' cumple con el formato aceptado
 * por bash para argumentos compuestos de palabras separados por espacios.
 * 
 * Los formatos aceptados son:
 * 
 *      "argumento separado por espacios".   Ej: cd "carpeta con espacios"
 *      'argumento separado por espacios'.   Ej: export VAR='Valor con espacios'
 *      argumento\ separado\ por\ espacios.  Ej: cd carpeta\ con\ espacios/
 * 
 * Cuando se ejecuta un comando de un solo parámetro y se detectan más paráme-
 * tros en la linea de comando, se ha de recuperar el argumento completo a
 * partir de **args e invocar a check_formato.
 * 
 * Parámetros:
 *          +argument:  Puntero al String que conforma el argumento a ser
 *                      validado.
 * 
 * Return:
 *          +1: Si el argumento cumple con el formato para argumentos compuestos
 *          +(-1): Si el argumento es inválido.
 */
int check_formato(char *argument) {
    if (strchr(argument,'\'') || strchr(argument,'\\') || 
            strchr(argument,'\"')) {
        for (int i = 0; argument[i] != '\0'; i++) {
            /*
                Verifica que cuando se consigue un argumento con formato                    ['argumento con espacios'] se cumpla la condición:
                "No puede haber un argumento que comience por el
                caracter '\'' y no termine por el caracter '\''."

                Es decir, al terminar el argumento, antes de '\0' tiene que 
                existir el caracter '\''.

            */
            if (argument[i] == '\'') {
                while (argument[i] != '\0' && argument[i] != '/') {
                        i++;
                    }
                if (argument[i-1] != '\'') {
                    return -1;
                }
            }

            /*
                Verifica que cuando se consigue un argumento con formato
                ["argumento con espacios"] se cumpla la condición:

                "No puede haber un argumento que comience por el
                caracter '\"' y no termine por el caracter '\"'."

                Es decir, al terminar el argumento, antes de '\0' tiene que 
                existir el caracter '\"'.

            */
            if (argument[i] == '\"') {
                while (argument[i] != '\0' && argument[i] != '/') {
                        i++;
                    }
                if (argument[i-1] != '\"') {
                    return -1;
                }
            }

            /*
                Verifica que cuando se consigue un argumento con formato
                [argumento\ con\ espacios] se cumpla la condición:

                "No puede haber un espacio ' ' sin un caracter '\' antes."
            */
            if (argument[i] == ' ' && argument[i-1]!= '\\') {
                return -1;
            }
        }
        return 1;
    } else {
        return -1;
    }
}

/**
 * Función que imprime por pantalla información relacionada con el proceso. 
 * Mostrará por pantalla: 
 * 
 *          - Identificador de trabajo.
 *          - PID del proceso.
 *          - Línea de comandos. 
 *          - Estado (D (Detenido), E (Ejecutando)).
 * 
 * No necesita ningún parámetro de entrada para funcionar. No devuelve ningún
 * valor.
 */

//Discutir el nombre de la función
int internal_jobs2() {

  //Falta poner el identificador de trabajo. 
  printf("[internal_jobs(): Identificador de trabajo: \n");

  //Imprimir PID.
  printf("[internal_jobs(): PID: %d\n", jobs_list[0].pid);

  //Imprimir  línea de comandos.
  printf("[internal_jobs(): Línea de comandos: %s\n", jobs_list[0].command_line); 

  //Imprimir estado del proceso. 
  printf("[internal_jobs(): Estado: %d\n", jobs_list[0].status); 

}





