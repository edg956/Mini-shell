#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE 64

#define PROMPT '$'

//Colores

#define RESET_COLOR    "\x1b[0m"
#define CYAN_T     "\x1b[1m\033[36m"
#define AMARILLO_T "\x1b[1m\033[33m"
#define ROJO_F     "\x1b[41m"

/*Se dejan los siguientes colores comentados por si son necesarios para futuras
implementaciones.

#define NEGRO_T        "\x1b[30m"
#define NEGRO_F        "\x1b[40m"
#define ROJO_T     "\x1b[31m"
#define VERDE_F        "\x1b[42m"
#define AMARILLO_F  "\x1b[43m"
#define AZUL_F      "\x1b[44m"
#define MAGENTA_T  "\x1b[1m\033[35m"
#define MAGENTA_F  "\x1b[45m"
#define CYAN_F     "\x1b[46m"
#define BLANCO_T   "\x1b[37m"
#define BLANCO_F   "\x1b[47m"
#define GREEN_T        "\x1b[1m\033[32m"
#define BLUE_T     "\x1b[1m\033[34m"
*/

/**
 * ESTRUCTURAS (STRUCT) DE MINISHELL
 */
struct info_process {
    pid_t pid;
    char status;    //'E', 'D', 'F'
    char command_line[COMMAND_LINE_SIZE];
};

/**
 * FUNCIONES DE MINISHELL
 */
char *read_line(char *line);
int execute_line(char *line);
int parse_args(char **args, char *line);
int check_internal(char **args); 
int internal_cd(char **args);
int internal_export(char **args);
int internal_source(char **args);
int internal_jobs(char **args);
int internal_bg(char **args);
int internal_fg(char **args);
void reaper(int signum);
void ctrlc(int signum);
void ctrlz(int signum);
int is_background (char **args);
int jobs_list_add(pid_t pid, char status, char *command_line);
int jobs_list_find(pid_t pid);
int jobs_list_remove(int pos);

/**
 * FUNCIONES AUXILIARES
 */
void imprime_error(char *mensaje_error);
void print_prompt();
int check_formato();
int internal_jobs2();
void reset_jobs_list_fg();
