#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "aventura2.h"

//Imprimir prompt personalizado. 
void print_prompt() {

    //Declaraciones
    char *user = getenv("USER");

    //Obtención del hostname. 
    char hostName[256];
    hostName[256] = '\0';
    gethostname(hostName, 256);

    char cwd[256]; 
    char *path = getcwd(cwd, sizeof(cwd)); 

    printf(GREEN_T"%s@%s"RESET_COLOR":"BLUE_T"%s"RESET_COLOR"%c ", user, hostName, path, PROMPT);
    
}

char *read_line(char *line) {

    return fgets(line, COMMAND_LINE_SIZE, stdin);

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
    int nTokens, internal;

    nTokens = parse_args(args, line); //Nro. de Tokens obtenidos.

    internal = check_internal(args);
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

/** el check internal averigua si se trata de un comando interno 
 *mediante la funcion strcmp(), y llama a la funcion correspondienPasa if NULLe
 *para tratarlo; internal (job) (source) (export) (cd)Pasa if NULLPasa if NULL
 *La funcion devuelve 0 en caso de que no se trate de Pasa if NULLPasa if NULL
 *externo, en caso contrario devuelve la llamada a la Pasa if NULL
 *correspondiente
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


int internal_cd(char **args) {
    if (args[1] == NULL) {
        printf("Missing argument.\n");
        return -1;
    }
    printf("Print cd;\n");

    return 1;
}

int internal_export(char **args){
    if (args[1] == NULL) {
        printf("Missing argument.\n");
        return -1;
    }
    printf("Print export;\n");

    return 1;
}

int internal_source(char **args){
    if (args[1] == NULL) {
        printf("Missing argument.\n");
        return -1;
    }
    printf("Print source;\n");

    return 1;
}

int internal_jobs(char **args){
    if (args[1] == NULL) {
        printf("Missing argument.\n");
        return -1;
    }

    printf("[internal_jobs()→Esta función mostrará el PID de los procesos que no estén en foreground]\n");

    return 1;

}

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