/******************************************************************************
 Nombre del proyecto: my_shell
 Grupo: Los tres mosqueteros
 Participantes/desarrolladores: 
                                + Eugenio Doñaque
                                + Álvaro Pons 
                                + Nadal Llabrés
Asignatura: 21708-Sistemas operativos (prácticas)
Professor/a: Adelaida Delgado
*******************************************************************************/

//Defines
#define _POSIX_C_SOURCE 200112L
#define USE_READLINE

//Condicionales del preprocesador
#ifdef USE_READLINE
#endif 

//Includes
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "my_shell.h"
#include <readline/readline.h>
#include <readline/history.h>

/******************************************************************************
                            VARIABLES GLOBALES
******************************************************************************/
char *program_name = NULL;
int n_pids;

/*Array que guarda los diferentes estados que toman los procesos durante
su ejecución en primer y segundo plano.*/
static struct info_process jobs_list[ARGS_SIZE];

/******************************************************************************
                                 MÉTODO MAIN
******************************************************************************/
int main(int argc, char *argv[]) {

    program_name = malloc(COMMAND_LINE_SIZE);
    program_name = strcpy(program_name, argv[0]);

    //Declaraciones
    char *line = malloc(COMMAND_LINE_SIZE);

    //Indicar a main que escuche señales SIGCHLD, SIGINT y SIGTSTP
    signal(SIGCHLD, reaper);
    signal(SIGINT, ctrlc);
    signal(SIGTSTP, ctrlz);

    //Inicializar estructura jobs_list[0]
    reset_jobs_list_fg();

    int error = 1;
      
    while (error)
    {
        error = execute_line(read_line(line));
    }
    char *ermsg = "\nHa ocurrido un error durante la ejecución del programa.\nCerrando.";
    imprime_error(ermsg);
}



/******************************************************************************
                         FUNCIONES DE LA AVENTURA 2
******************************************************************************/
/*
 * Función obtiene el comando escrito por usuario en la terminal y limpia el
 * buffer de la entrada de datos estandar stdin.
 * Guarda el comando en el puntero del argumento 'line'.
 * 
 * Dependiendo de si se hace uso de la librería readline se usará dicha librería
 * o no. 
 * 
 * Parámetros:
 *          +line: Puntero al espacio en memoria a partir del cual se almacenará
 *                 el comando escrito por el usuario.
 * Devuelve:
 *          +line: Puntero al espacio donde se encuentra almacenado el comando
 *                 entrado.
 */
char *read_line(char *line) {

    //Declaraciones
    char *command;
    char *prompt = malloc(PROMPT_SIZE);

//Uso de la librería readline (edición de comandos disponible e historial).
#ifdef USE_READLINE

   command = readline(print_prompt(prompt));

   if (command == NULL) return NULL;
   
   add_history(command);
   strcpy(line, command);
   free(command);
return line;

//Uso del propio readline (sin edición de comandos e historial).
#else
    printf("%s", print_prompt(prompt));

    command = fgets(line, COMMAND_LINE_SIZE, stdin);

    if (!command && !feof(stdin)) {
        
        command = line;
        command[0] = 0;
        puts("");
    }

    //En caso de presionar Ctrl+D con un elemento, agregar salto de linea.
    if (command && feof(stdin)) puts("");

    clearerr(stdin);    //Resetear el indicador de EOF y de errores del STDIN
    fflush(stdin);
    fflush(stdout);
       return command;

    #endif

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
    int is_bg = 0;
    char **args;
    char *linecpy;
    char *delim = "\n";
    if (!(args = malloc(ARGS_SIZE))) return 0; //Control de errores
    if (!(linecpy = malloc(COMMAND_LINE_SIZE))) return 0; //Control de errores
    
    //Copia para procesos hijos en ejecución
    linecpy = strcpy(linecpy,line);
    linecpy = strtok(linecpy,delim);    //Eliminar '\n' de la linea
    
    //Trocear line en argumentos.
    parse_args(args, line); 

    //Ver si la función a ejecutar es fg o bg
    is_bg = is_background(args);    

    //Check si el comando ha sido un comando interno o externo
    if (check_internal(args) == 0) {

        /*Ejecución de comando externo*/

        //Creación de proceso hijo para ejecutar comando externo
        pid_t cpid = fork();
        if (cpid == -1) {   //Check errores en fork()
            imprime_error(NULL);
            return 0;
        }

        //Código para proceso hijo
        if (cpid == 0) {
            signal(SIGINT, SIG_IGN);
            signal(SIGTSTP, SIG_IGN);
            signal(SIGCHLD, SIG_DFL);

            if (is_output_redirection(args)) {
                int file;
                int i = 0;

                while (strcmp(args[i], ">")) i++;

                args[i] = NULL;
                i++;
                
                if (args[i] != NULL) { 
                    file = open(args[i], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
                    dup2(file, 1);
                    close(file);
                } else {
                    imprime_error("\nRuta de fichero no especificada.");
                    return 1;
                }
            }

            //Check errores en execvp(). Enviar error por stderr y realizar
            if (execvp(args[0],args) == -1) {
                imprime_error(NULL);
                exit(1);
            }
        }

        //Enviar señal al enterrador de hijos
        signal(SIGCHLD, reaper);
        signal(SIGTSTP, ctrlz);
        signal(SIGINT, ctrlc);

        //Determinar si se trata de proceso en fg o bg
        if (is_bg) {
            //Añadir de información de proceso a lista en bg.
            if (jobs_list_add(cpid, 'E', linecpy) == -1) {
                kill(cpid, 9); //En caso de error añadiendo trabajo en jobs_list
                imprime_error("\nError añadiendo trabajo en background. Abortando.");
                return 0;
            }

        } else {
            //Guardar información de proceso en foreground.
            jobs_list[0].pid = cpid;
            jobs_list[0].status = 'E';
            strcpy(jobs_list[0].command_line, linecpy);

            //Esperar a que proceso en fg termine su ejecución.
            while (jobs_list[0].pid != 0) {
                pause();
            }   
        }
    }

    //Libera variables locales auxiliares
    free(linecpy);
    return 1;
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

    //Inicialización de args;
    *args = strtok(line, &delim[0]);

    //Llenar args de tokens.
    while (*args != NULL) {
        if (*args[0] == '#') { //Ignorar comentarios
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
int check_internal(char **args) {

    if (*args == NULL) return -1;

    //Compara comando introducido con comandos internos
    if (strcmp(args[0], "cd") == 0) {
        return internal_cd(args);

    } else if (strcmp(args[0], "export") == 0) {
        return internal_export(args);

    } else if (strcmp(args[0], "source") == 0 ||
                strcmp(args[0], ".") == 0) {
        return internal_source(args);

    } else if (strcmp(args[0], "jobs") == 0) {
        return internal_jobs(args);

    } else if (strcmp(args[0], "fg") == 0) {
        return internal_fg(args);

    } else if (strcmp(args[0], "bg") == 0) {
        return internal_bg(args);

    } else if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else { 
        return 0; //Si no es un comando interno.
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
    char *path;
    if (!(path = malloc(COMMAND_LINE_SIZE))) return -1;

    /*Chequea casos posibles de entrada para comando cd:
        Casos:  "cd" sin argumentos.
                "cd ruta/a/'carpeta con espacios'" (más de un arg)
                "cd un_solo_arg"
    */
   
    //Caso "cd"
    if (args[1] == NULL) {  
        //Cambiar de directorio
        if (chdir(getenv("HOME")) != 0) {
            perror("Error");
            return -1;
        }
        //Actualizar variable de entorno PWD
        if (setenv("PWD", getenv("HOME"), 1) == -1) {
            perror("Error");
            return -1;
        }
        return 1;

    }

    //Caso "cd 'carpeta ejemplo'" || "cd carpeta ejemplo" (sin formato)
    if (args[2] != NULL) {   
        
        //Regeneración del argumento entero para verificar validez de formato
        int k = 1;
        char *auxpath;
        if (!(auxpath = malloc(COMMAND_LINE_SIZE))) return -1; //Check for errors
        
        while (args[k] != NULL) {
            auxpath = strcat(auxpath, args[k]);
            auxpath = strcat(auxpath," ");
            k++;
        }

        //Elimina espacio agregado en último strcat
        path = strncpy(path,auxpath,strlen(auxpath)-1); 

        //Análisis de validez de argumento. Ver check_formato()
        if (check_formato(path) == -1) {
            imprime_error("cd: demasiados argumentos.");
            return 1;
        }

        /*
            Si path cumple con el formato de entrada, se recupera la dirección
            completa para ser tratada y eliminar caracteres no aceptados.
        */
      
        free (auxpath);
        if (!(auxpath = malloc(COMMAND_LINE_SIZE))) return -1;
        char *delim = "'\\\"";
        char *aux = strtok(path, delim);

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
            imprime_error("cd: Archivo o fichero inexistente.");
        } else {
            perror("cd: error");
        }
        return -1;

    } else {

        //Update variable de entorno
        path = getcwd(path, COMMAND_LINE_SIZE);
        if (setenv("PWD", path, 1) != 0) {
            perror("Error en setenv()");
            return -1;
        }

    }
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
 * 
 * Parámetros:
 *          +args: Doble puntero al comando introducido separado en tokens.
 * Devuelve:
 *          +1: Si se ejecuta correctamente.
 *          +(-1): En caso contrario.
 */
int internal_export(char **args){

    //Check existencia de argumento
    if (args[1] == NULL)  {
        imprime_error("Sintaxis incorrecta. La sintaxis esperada es: NOMBRE_VAR=VALOR_NUEVO");
        return -1;
    }

    char *argument;
    if (!(argument = malloc(COMMAND_LINE_SIZE))) return -1; //Check for errors

    /*
        Check si VALUE es argumento compuesto del tipo:
            NAME="NEW VALUE";
            NAME='NEW VALUE';
            NAME=NEW\ VALUE;
    */
    if (args[2] != NULL) {
        //Reconstrucción de argumentos para comprobar validez
        char *auxarg;
        if (!(auxarg = malloc(COMMAND_LINE_SIZE))) return -1; //Check for errors

        for (int i = 1; args[i] != NULL; i++) {
            auxarg = strcat(auxarg, args[i]);
            auxarg = strcat(auxarg, " ");
        }
        argument = strncpy(argument,auxarg,strlen(auxarg)-1); //Eliminar ' '

        //Análisis de validez de argumento. Ver check_formato()
        if(check_formato(argument) == -1) {
            imprime_error("Sintaxis incorrecta. La sintaxis esperada es: NOMBRE_VAR=VALOR_NUEVO");
            return -1;
        }

        free(auxarg);
        if (!(auxarg = malloc(COMMAND_LINE_SIZE))) return -1; //Check for errors
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

    if (valor == NULL) {
        imprime_error("Sintaxis incorrecta. La sintaxis esperada es: NOMBRE_VAR=VALOR_NUEVO");
        return 1;
    }

    setenv(nombre, valor, 1);
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
        return -1;
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
int internal_jobs(char **args) {
    //Recorre la lista de trabajos en segundo plano
    for (int i = 1; i <= n_pids; i++) {
        //Imprimir info de cada job
        printf("[%d] %d    %c    %s\n",i
        , jobs_list[i].pid,jobs_list[i].status, jobs_list[i].command_line);
    }
    return 1;
}

/**
 * El comando interno bg reanuda la ejecución de un proceso detenido
 * pero en segundo plano.
 *
 * Parámetros:
 *          +args: Doble puntero que contiene el comando introducido separado
 *                 en tokens.
 * Devuelve:
 *          +1: si todo se ha ejecutado sin problemas.
 *          +(-1): Si hubo errores.
 */
int internal_bg(char **args) {
    //Declaraciones
    int job_id = 0;
    char *command = NULL;

    /*CONDICIONES DE VERIFICACIÓN DE SINTAXIS PARA COMANDO BG*/
    if (args[1] == NULL) {
        /*Si el primer argumento de bg es vacío, entonces ejecutar último proceso
        agregado*/

        job_id = n_pids;

        //Buscar último trabajo añadido a lista que este en estado 'D'
        while (job_id > 0 && jobs_list[job_id].status != 'D') {
            job_id--;
        }

        if (job_id == 0) return 1; //No se ha conseguido un trabajo ejecutable

    /*Check excedencia de argumentos*/
    } else if (args[2] != NULL) { 

        imprime_error("bg: Demasiados argumentos.");
        return 1;

    } else {
        char *arg = malloc(sizeof(args[1]));
        arg = strcpy(arg, args[1]);

        //Check si contiene el caracter '%'
        if (strchr(arg, '%')) arg++;

        //Check si el argumento contiene solo números (incluso precedido de %)
        if (!solo_numeros(arg)) {
            imprime_error("bg: Número de trabajo inválido.");
            return 1;
        }

        //Obtener entero desde String
        job_id = atoi(arg);

        //Check si el job_id existe en el jobs_list
        if (!(job_id > 0 && job_id <= n_pids)) {
            imprime_error("bg: Número de trabajo no existe.");
            return 1;
        }

        //Retornar puntero a valor original si hace falta
        if (strchr(args[1],'%')) arg--;
        free(arg); //Free resources
    }
    /*FIN DE CONDICIONES*/

    if (jobs_list[job_id].status == 'E') {
        imprime_error("Trabajo ya se está ejecutando en segundo plano.");
        return 1;
    }

    //Actualizar información del trabajo en jobs_list y obtener su pid
    jobs_list[job_id].status = 'E';

    kill(jobs_list[job_id].pid, SIGCONT);

    //Imprimir linea de comando con & si no lo tiene ya.
    if (!strchr(jobs_list[job_id].command_line, '&')) {
        command = strcpy(command, jobs_list[job_id].command_line);
        command = strcat(command, " &");
    } else {
        command = strcpy(command, jobs_list[job_id].command_line);
    }

    printf("[%d] %d    %c    %s]\n", job_id, jobs_list[job_id].pid,
    jobs_list[job_id].status, command);

    return 1;
}

/**
 * El comando interno bg reanuda la ejecución de un proceso detenido
 * pero en primer plano.
 *
 * Parámetros:
 *          +args: Doble puntero que contiene el comando introducido separado
 *                 en tokens.
 * Devuelve:
 *          +1: si todo se ha ejecutado sin problemas.
 *          +(-1): Si hubo errores.
 */
int internal_fg(char **args) {
    //Declaraciones
    int job_id = 0;
    char *command = NULL;

    /*CONDICIONES DE VERIFICACION DE SINTAXIS COMANDO FG*/
    if (args[1] == NULL) {
        /*Si el primer argumento de fg es vacío, entonces ejecutar último proceso
        agregado*/
        job_id = n_pids;

        if (job_id == 0) return 1; //No se ha conseguido un trabajo ejecutable

    } else if (args[2] != NULL) { //Check excedencia de argumentos

        imprime_error("fg: Demasiados argumentos.");
        return 1;

    } else {
        char *arg = malloc(sizeof(args[1]));
        arg = strcpy(arg, args[1]);

        //Check si contiene el caracter '%'
        if (strchr(arg, '%')) arg++;

        //Check si el argumento contiene solo números (incluso precedido de %)
        if (!solo_numeros(arg)) {
            imprime_error("fg: Número de trabajo inválido.");
            return 1;
        }

        //Obtener entero desde String
        job_id = atoi(arg);

        //Check si el job_id existe en el jobs_list
        if (!(job_id > 0 && job_id <= n_pids)) {
            imprime_error("fg: Número de trabajo no existe.");
            return 1;
        }

        //Retornar puntero a valor original si hace falta
        if (strchr(args[1],'%')) arg--;
        free(arg); //Free resources
    }
    /*FIN CONDICIONES*/

    //Actualizar información del trabajo en jobs_list y obtener su pid
    jobs_list[job_id].status = 'E';
    pid_t pid = jobs_list[job_id].pid;
    jobs_list[0] = jobs_list[job_id];

    jobs_list_remove(job_id);

    kill(pid, SIGCONT);

    /*Modificación de la linea para quitar el & en lineas ejecutadas en bg*/    
    char *aux = strtok(jobs_list[0].command_line, " &");
    command = strcpy(command, aux);
    command = strcat(command, " ");
    aux = strtok(NULL, " &");
    command = strcat(command, aux);

    strcpy(jobs_list[0].command_line, command);

    printf("%s\n", jobs_list[0].command_line);

    free(command);

    //Esperar a que nuevo trabajo en fg termine
    while (jobs_list[0].pid != 0) {
        pause();
    }

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
            
            /*Si el job terminado es el trabajo el foreground, resetear
            jobs_list[0]*/
            reset_jobs_list_fg();
            
        } else {
            /*Si el job terminado no es el trabajo en foreground, quitar de 
            la lista*/
            int fnsh_job = jobs_list_find(pid);

            //check for errors
            if (fnsh_job == -1) { 
                imprime_error("\nError en jobs_list_find.");
            }
            printf("[%d]+ Hecho      %s\n", fnsh_job, 
            jobs_list[fnsh_job].command_line);

            jobs_list_remove(fnsh_job);
            
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

    signal(SIGINT, ctrlc);

    //Declaraciones
    char *error = NULL; 
    char *line;
    char *pnamecpy;

    //Check for errors
    if (!(line = malloc(COMMAND_LINE_SIZE))) imprime_error(NULL);
    if (!(pnamecpy = malloc(COMMAND_LINE_SIZE))) imprime_error(NULL);

    //Ver que haya algún proceso en foreground
    if (jobs_list[0].pid > 0) {
        /* 
            Copiar nombre de programa y la linea de comando del proceso en
            para poder modificarlos y compararles
        */
        pnamecpy = strcpy(pnamecpy, program_name); //Copia program_name

        char *delim = " ";
        line = strcpy(line, jobs_list[0].command_line); //Copia linea de
        line = strtok(line, delim);                     //proceso en fg

        //Comparar que el proceso en foreground no sea el shell
        if (strcmp(line, pnamecpy) != 0) {
      
            kill(jobs_list[0].pid, SIGTERM);

        } else {
            error = "Señal SIGTERM no enviada debido a que el proceso en foreground es el shell.";
            imprime_error(error);
        }

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
    char *line;
    char *pnamecpy;
    //Check for errors
    if (!(line = malloc(COMMAND_LINE_SIZE))) imprime_error(NULL);
    if (!(pnamecpy = malloc(COMMAND_LINE_SIZE))) imprime_error(NULL);

    //Ver que haya algún proceso en foreground
    if (jobs_list[0].pid > 0)   {        
        /* 
            Copiar nombre de programa y la linea de comando del proceso en
            para poder modificarlos y compararles
        */
        pnamecpy = strcpy(pnamecpy, program_name); //Copia program_name

        char *delim = " ";
        line = strcpy(line, jobs_list[0].command_line); //Copia linea de
        line = strtok(line, delim);                     //proceso en fg

        //Comparar que el proceso en foreground no sea el shell
        if (strcmp(line, pnamecpy) != 0) {

            kill(jobs_list[0].pid, SIGSTOP);

            jobs_list[0].status = 'D';

            jobs_list_add(jobs_list[0].pid, jobs_list[0].status, jobs_list[0].command_line);
            reset_jobs_list_fg();

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
int is_background(char **args) {
    int i = 0;

    //En caso de que llegue String vacío (Ej. CtrlZ || CtrlC con comando vacio)
    if (args[i] == NULL) return -1;

    while (args[i] != NULL) {   
        i++;
    }

    //Check si el último argumento contiene &
    if (strchr(args[i-1], '&')) {
        /*Si el último argumento contiene &, pero no está separado por un 
        espacio, se elimina el &*/
        char *delim = "&";
        args[i-1] = strtok(args[i-1], delim);
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
        printf("[%d]+ %d    %c    (%s)\n",n_pids, jobs_list[n_pids].pid, 
        jobs_list[n_pids].status, jobs_list[n_pids].command_line);
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
 * 
 *              + (-1) si no ha encontrado el PID.    
 */
int jobs_list_find(pid_t pid) {
    int pos = 1;
    while (pos <= n_pids && jobs_list[pos].pid != pid){
        pos++; 
    }
    if (pos > n_pids) return -1;
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

/**
 * Función booleana que recorre la lista de argumentos buscando un token ">", 
 * seguido de otro token que será un nombre de fichero.
 * 
 * Parámetros
 *              + char **args: Lista de argumentos. 
 * 
 * Return
 *              + Devuelve 0 si no hay redireccionamiento. 
 *              + Devuelve 1 si hay redireccionamiento ">". 
 */

int is_output_redirection(char **args) {
    //Declaraciones
    int i = 0;

    //En caso de que llegue String vacío (Ej. CtrlZ || CtrlC con comando vacio)
    if (args[i] == NULL) return 0;

    while (args[i] != NULL && strcmp(args[i],">")) {
        i++;
    }

    //Si no hay '>', retornar 0;
    if (args[i] == NULL) return 0;
    
    return 1;
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
    puts("");
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
char *print_prompt(char *prompt) {

    //Declaraciones
    char *user = getenv("USER");

    //Obtención del hostname. 
    char hostName[256];
    hostName[256] = '\0';
    gethostname(hostName, 256);

    char *path = getenv("PWD");
    sprintf(prompt, CYAN_T"%s@%s"RESET_COLOR":"AMARILLO_T"%s"RESET_COLOR"%c ", user, hostName, path, PROMPT);  
    return prompt;
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
                Verifica que cuando se consigue un argumento con formato                    
                ['argumento con espacios'] se cumpla la condición:
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
 * Función que resetea los valores de la posición en jobs_list reservada
 * para trabajos en foreground.
 * 
 * Parámetros:
 *          + Función no recibe parámetros.
 * Devuelve:
 *          + Función no devuelve nada.
 */
void reset_jobs_list_fg() {
    jobs_list[0].pid = 0;
    jobs_list[0].status = 'F';
    jobs_list[0].command_line[0] = '\0';
}

/**
 *  Función que revisa que el String recibido contenga únicamente caracteres
 *  numéricos
 *  
 *  Parámetros:
 *          +str: el String de C a analizar.
 *  Devuelve:
 *          +1: Si str solo contiene números.
 *          +0: En caso contrario.
 */
int solo_numeros(const char *str) {
    while (*str) {
        if (isdigit(*str++) == 0) return 0;
    }
    return 1;
}
