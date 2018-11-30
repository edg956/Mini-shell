#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "aventura2.h"


void main() {

    //Declaraciones
    char *line = malloc(COMMAND_LINE_SIZE);

    print_prompt();
    int i = 0; 

    while (i==0) {

        execute_line(read_line(line));
        print_prompt();

    }

}

/******************************************************************************
                             FUNCIONES DE APOYO
******************************************************************************/
int cuenta_elementos(char **args) {
    int numargs;
    numargs = 0;
    while (args[numargs] != NULL) {
        numargs++;
    }
    return numargs;
}

void imprime_error(char *mensaje_error) {
    if (mensaje_error == NULL) {
      fprintf(stderr, ROJO_F"Error %d: %s\n", errno, strerror(errno));
      fprintf(stderr, COLOR_RESET);
    }  else{
      fprintf(stderr, ROJO_F"%s\n", mensaje_error);
      fprintf(stderr, COLOR_RESET);
    }
}

/**
 * 
 * Funcion que imprime el prompt al usuario por consola, indicando
 * la ruta del directorio donde se encuentra al usuario, y recoje el
 * comando introducido guardandolo en 'line'.
 * 
 * Parametros:
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

    printf(GREEN_T"%s@%s"RESET_COLOR":"BLUE_T"%s"RESET_COLOR"%c ", user, 
    hostName, path, PROMPT);
    
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
 * Parametros:
 *          +argument:  Puntero al String que conforma el argumento a ser
 *                      validado.
 * 
 * Devuelve:
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
    fflush(stdin);

    return command;
}

/**
 * Funcion que ejecuta el comando contenido en el String 'line'
 * 
 * Parametros:
 *      +line: Puntero al String que contiene el comando.
 * Return:
 *      +0: Si la operacion se realizo sin problemas.
 *      +(-1): En caso de error.
 */
int execute_line(char *line) {

    char **args = malloc(ARGS_SIZE);
    int nTokens;

    nTokens = parse_args(args, line); //Nro. de Tokens obtenidos.

    if (check_internal(args) == 0) {
        printf("Comando no es interno\n"); //No es parte del proyecto
    }
}

/**
 * Funcion que toma el String 'line' que contiene el ultimo comando 
 * ejecutado por el usuario y lo trocea en tokens para guardarlos en 
 * el vector de tokens 'args'.
 * 
 * Parametros:
 *      +args: Puntero a varios String que contendrán los tokens 
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
 * Parametros:
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
        } else if (strcmp(args[0], "source") == 0) {
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
            puts("Archivo o fichero inexistente.");
        } else {
            perror("Error");
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
        fprintf(stderr, "%s", "Incorrect sintax. \nThe sintax is: export VAR_NAME=NEW_VALUE\n");
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
            fprintf(stderr, "%s", "Incorrect sintax. \nThe sintax is: export VAR_NAME=NEW_VALUE\n");
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

    if (nombre == NULL || valor == NULL) {
        puts("Error de sintaxis. Uso: export NOMBRE=VALOR.");
        return 1;
    }

    printf("\nValor inicial de la variable %s: %s\n", nombre, getenv(nombre));
    setenv(nombre, valor, 1);
    printf("\nValor final de la variable %s: %s\n", nombre, getenv(nombre));
    return 1;
    
}

int internal_source(char **args) {
    FILE *f;
    f = fopen (args[1], "r");
    if (f == NULL){
        imprime_error(NULL);
    }
    else{
        char linea[COMMAND_LINE_SIZE];
        while (fgets(linea, COMMAND_LINE_SIZE, f) != NULL){
            execute_line(linea);
        }
        fclose(f);
    }
    return 1;
}

int internal_jobs(char **args) {

    return 1;
}

