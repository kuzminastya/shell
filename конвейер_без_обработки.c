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
    int vertical;
} typedef Command;
struct Conveyer {
    Command *elem;
    int size;
    int length;
} typedef Conveyer;

void Invitation ();
void InputCommand (Command *a, int *end_conv);
int InputStr (char **str, int *end_com, int *end_conv, int *vertical);
void Quotes1 (char **str, int *size, int *i);
void Quotes2 (char **str, int *size, int *i);
void ReallocWord (char **str, int *size);
void ReallocCommand (Command *a);
void ReallocConveyer (Conveyer *c);
void Output (Command a);
void Execute_com (Command a);
void Execute_conv (Conveyer c);

int main() {
    int end_conv = 0;
    while (1) { // один цикл - обработка одного конвейера
        Invitation();
        Conveyer conv;
        conv.size = 2;
        conv.length = 0;
        conv.elem = (Command *) malloc(conv.size * sizeof(Command));
        do { // один цикл - обработка одной команды
            Command com;
            com.size = 4;
            com.length = 0;
            com.vertical = 0;
            com.pstr = (char **) malloc(com.size * sizeof(char *));
            InputCommand(&com, &end_conv);
            if (com.length == 1 && strcmp("exit", com.pstr[0]) == 0) {
                return 0;
            }
            (conv.elem)[conv.length] = com;
            conv.length++;
            if (conv.length >= conv.size) {
                ReallocConveyer(&conv);
            }
            Output(com);
            //Execute_com (com);
        } while ((conv.elem)[conv.length-1].vertical);
        Execute_conv(conv);
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

void InputCommand (Command *a, int *end_conv){
    int x; // переменная длины слова
    int end_com = 0;
    do { // один цикл - чтение одной команды
        x = InputStr(&a->pstr[a->length], &end_com, end_conv, &a->vertical);
        if (x){ // для отработки случая  "слово| ..."
            a->length++;
        }
        if (a->vertical) { // если встречается |, то команда заканчивается
            break;
        }
        if (!x) { // длина слова = 0
            continue;
        }
        if (a->length >= a->size) {
            ReallocCommand(a);
        }
        if (a->length == 1 && strcmp("exit", a->pstr[0]) == 0) {
            break;
        }
    } while (end_com == 0);

    a->pstr[a->length] = NULL;
}

int InputStr (char **str, int *end_com, int *end_conv, int *vertical){
    int size = 16;
    int c = 0, i = 0;
    *str = (char*) malloc(size * sizeof(char));
    while (1) { // считывание одного слова
        c = getchar();
        if (c != EOF) {
            if (i >= size) {
                ReallocWord(str, &size);
            }
            if (c == '|') {
                *vertical = 1;
                break;
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
                *end_com = 1;
                *end_conv = 1;
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

void ReallocConveyer (Conveyer *c) {
    c->size *= 2;
    Command *tmp = (Command *) malloc(c->size * sizeof(Command));
    for (int i = 0; i < c->length; i++){
        *(tmp+i) = c->elem[i];
    }
    free (c->elem);
    c->elem = tmp;
}

void Output (Command a)
{
    for (int i = 0; i < a.length; i++){
            printf ("<%s> ", a.pstr[i]);
    }
    printf ("%d\n", a.vertical);
}

void Execute_com (Command a) {
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

void Execute_conv (Conveyer c){
    
    int fd[2];
    pipe (fd);
    pid_t child = fork();
    if (child == 0) {
        dup2(fd[1], 1);
        close(fd[1]);
        close(fd[0]);

    }
}
