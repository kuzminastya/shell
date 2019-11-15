#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define BOLD "\033[1m"

struct Command {
    char **pstr;
    int size; // размер буфера
    int length; // длина массива
} typedef Command;

void Invitation ();
int InputStr (char **str, int *end);
void Quotes1 (char **str, int *size, int *i);
void Quotes2 (char **str, int *size, int *i);
void ReallocWord (char **str, int *size);
void ReallocCommand (Command *a);
void Output (Command a);
//void my_free (Command *a);
void Execute (Command a);

int main() {
    while (1) { // один цикл - обработка одной команды
        Command com;
        com.size = 4;
        com.length = 0;
        com.pstr = (char**) malloc (com.size * sizeof(char*));
        Invitation();
        int end_com = 0;
        do { // один цикл - чтение одной команды
            if(!(InputStr(&com.pstr[com.length], &end_com))){
                continue;
            }
            com.length++;
            if (com.length >= com.size) {
                ReallocCommand(&com);
            }
            if (com.length == 1 && strcmp("exit", com.pstr[0]) == 0) {
                return 0;
            }
        } while (end_com == 0);

        com.pstr[com.length] = NULL;

        Execute(com);
    }
}

void Invitation (){
    char *the_path = (char*) malloc(255*sizeof(char));
    getlogin_r(the_path, 255);
    printf(ANSI_COLOR_MAGENTA BOLD "%s@" ANSI_COLOR_RESET , the_path);
    gethostname(the_path, 255);
    printf(ANSI_COLOR_MAGENTA BOLD "%s:" ANSI_COLOR_RESET , the_path);
    getcwd(the_path, 255);
    printf(ANSI_COLOR_YELLOW BOLD "%s" ANSI_COLOR_RESET , the_path);
    printf("$ ");
    free(the_path);
}

int InputStr (char **str, int *end){
    int size = 16;
    int c = 0, i = 0;
    *str = (char*) malloc(size * sizeof(char));
    while (1) {
        c = getchar();
        if (c != EOF) {
            if (i >= size) {
                ReallocWord(str, &size);
            }
            if (c == '\'') {
                Quotes1(str, &size, &i);
                continue;
            } else {
                if (c == '\"') {
                    Quotes2(str, &size, &i);
                    continue;
                }
            }
            if (c == ' ' || c == '\t') {
                break;
            }
            if (c == '\\' && (c = getchar()) != EOF && c == '\n') {
                continue;
            }
            if (c == '\n') {
                *end = 1;
                break;
            }
            (*str)[i] = (char) c;
            i++;
        }
    }
    (*str)[i] = '\0';
    return i;
}

void Quotes1 (char **str, int *size, int *i){
    int c;
    while ((c = getchar()) != EOF && c != '\'') {
        if (*i >= *size) {
            ReallocWord(str, size);
        }
        (*str)[*i] = (char) c;
        (*i)++;
    }
}

void Quotes2 (char **str, int *size, int *i){
    int c;
    while ((c = getchar()) != EOF && c != '\"') {
        if (*i >= *size) {
            ReallocWord(str, size);
        }
        if (c == '\\'){
            if ((c = getchar()) != EOF && c == '\n') {
                continue;
            }
            if (c != '\"' && c != '\\'){
                (*str)[*i] = '\\';
                (*i)++;
            }
        }
        (*str)[*i] = (char) c;
        (*i)++;
    }
}

void ReallocWord (char **str, int *size){
    char *tmp = (char*)malloc(2*(*size)*sizeof(char));
    for (int i = 0; i < *size; i++){
        tmp[i] = (*str)[i];
    }
    free(*str);
    *str = tmp;
    *size *= 2;
}

void ReallocCommand (Command *a){
    a->size *= 2;
    char **tmp = (char**)malloc(a->size * sizeof(char*));
    for (int i = 0; i < a->length; i++){
        *(tmp+i) = a->pstr[i];
    }
    free (a->pstr);
    a->pstr= tmp;
}

void Output (Command a)
{
    for (int i = 0; i < a.length; i++){
            printf ("<%s> ", a.pstr[i]);
    }
    printf ("\n");
}

/*void my_free(Command *a)
{
    for (int i = 0; i < a->length; i++)
    {
        free(a->pstr[i]);
    } 
    free(a->pstr);  
}*/

void Execute (Command a) {
    if (a.length != 0 && strcmp("cd", a.pstr[0]) == 0) {
        if (a.pstr[1] == NULL) {
            puts("cd: too few arguments");
        } else {
            if (a.pstr[2] != NULL) {
                puts("cd: too many arguments");
            } else {
                if ((chdir(a.pstr[1])) == -1) {
                    printf("cd: %s: no such file or directory\n", a.pstr[1]);
                }
            }
        }
    } else {
        pid_t child = fork();
        if (child < 0) {
            puts("Error in fork(), child");
        } else {
            if (child == 0) {
                    execvp(a.pstr[0], a.pstr);
                    printf("%s: command not found\n", a.pstr[0]);
                    exit(1);
            } else {
                wait(NULL);
            }
        }
    }
}