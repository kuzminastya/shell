#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define BOLD "\033[1m"

struct Command {
    char **pstr;
    int size; // размер буфера
    int length; // длина массива
    int vertical;
    int v; // перенаправление: 1 - ввод, 2 - перезапись, 3 - добавление в конец файла
} typedef Command;

struct Conveyer {
    Command *elem;
    int size;
    int length;
    int vert_last;
    int ampersand;
} typedef Conveyer;

struct Fon_Rezh {
    Conveyer *elem;
    int size;
    int length;
} typedef Fon_Rezh;

void Invitation ();
void InputCommand (Command *a, int *end, int *vert_last, int *ampersand);
int InputStr (char **str, int *end_com, int *end, int *vertical, int *vert_last, int *ampersand, int *v);
void Quotes1 (char **str, int *size, int *i);
void Quotes2 (char **str, int *size, int *i);
void ReallocWord (char **str, int *size);
void ReallocCommand (Command *a);
void ReallocConveyer (Conveyer *c);
void ReallocFon_Rezh (Fon_Rezh *f);
void Output (Command a);
void Cd (Command a);
void Execute_com (Command a, int fd0, int fd1, int ampersand);
void Execute_conv (Conveyer c);

int main() {
    while (1) {
        Invitation();
        Fon_Rezh fon;
        fon.length = 0;
        fon.size = 2;
        fon.elem = (Conveyer*) malloc(fon.size * sizeof(Conveyer));
        int end = 0;
        do { // один цикл - обработка одного конвейера
            Conveyer conv;
            conv.size = 2;
            conv.length = 0;
            conv.vert_last = 0;
            conv.ampersand = 0;
            conv.elem = (Command *) malloc(conv.size * sizeof(Command));
            do { // один цикл - обработка одной команды
                Command com;
                com.size = 4;
                com.length = 0;
                com.vertical = 0;
                com.v = 0;
                com.pstr = (char **) malloc(com.size * sizeof(char *));
                InputCommand(&com, &end, &conv.vert_last, &conv.ampersand);
                if (com.length == 1 && strcmp("exit", com.pstr[0]) == 0) {
                    return 0;
                }
                if (!com.length) {
                    break;
                }
                (conv.elem)[conv.length] = com;
                conv.length++;
                if (conv.length >= conv.size) {
                    ReallocConveyer(&conv);
                }
                //Output(com);
            } while ((conv.elem)[conv.length - 1].vertical);
            if (!conv.length) {
                break;
            }
            (fon.elem)[fon.length] = conv;
            fon.length++;
            if (fon.length >= fon.size) {
                ReallocFon_Rezh(&fon);
            }
        } while (!end);
        for (int i = 0; i < fon.length; i++){
            Execute_conv(fon.elem[i]);
        }
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

void InputCommand (Command *a, int *end, int *vert_last, int *ampersand){
    int x; // переменная длины слова
    int end_com = 0;
    do { // один цикл - чтение одной команды
        x = InputStr(&a->pstr[a->length], &end_com, end, &a->vertical, vert_last, ampersand, &(a->v));
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

int InputStr (char **str, int *end_com, int *end, int *vertical, int *vert_last, int *ampersand, int *v){
    int size = 16;
    int c = 0, i = 0;
    *str = (char*) malloc(size * sizeof(char));
    while (1) { // считывание одного слова
        c = getchar();
        if (c != EOF) {
            if (i >= size) {
                ReallocWord(str, &size);
            }
            if (c == '<'){
                *v = 1;
                continue;
            }
            if (c == '>'){
                if ((c = getchar()) != EOF && c == '>'){ // >>
                    *v = 3;
                    continue;
                }
                else { // >
                    *v = 2;
                    continue;
                }
            }
            if (c == '|') {
                *vert_last = 1;
                *vertical = 1;
                break;
            }
            if (c == '&'){
                *ampersand = 1;
                *end_com = 1;
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
                if (!*vert_last) {
                    *end_com = 1;
                    *end = 1;
                }
                break;
            }
            (*str)[i] = (char) c;
            *vert_last = 0;
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

void ReallocFon_Rezh (Fon_Rezh *f){
    f->size *= 2;
    Conveyer *tmp = (Conveyer*) malloc(f->size * sizeof(Conveyer));
    for (int i = 0; i < f->length; i++){
        *(tmp+i) = f->elem[i];
    }
    free (f->elem);
    f->elem = tmp;
}

void Output (Command a)
{
    for (int i = 0; i < a.length; i++){
            printf ("<%s> ", a.pstr[i]);
    }
    printf ("%d\n", a.vertical);
}

void Cd (Command a){
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
}

void Execute_com (Command a, int fd0, int fd1, int ampersand) {
    if (strcmp(a.pstr[0], "cd") != 0) {
        pid_t child = fork();
        if (child < 0) {
            puts("Error in fork()");
        } else {
            if (child == 0) {
                dup2(fd0, 0);
                dup2(fd1, 1);
                execvp(a.pstr[0], a.pstr);
                printf("%s: command not found\n", a.pstr[0]);
                exit(1);
            } else {
                if (fd1 == 1) {
                    if (!ampersand) {
                        wait(NULL);
                    }
                }
            }
        }
    }
}

void Execute_conv (Conveyer c) {
    if (strcmp("cd", c.elem[0].pstr[0]) == 0 && c.length == 1) {
        Cd(c.elem[0]);
    } else {
        pid_t son = fork();
        if (son == 0) {
            int fd[c.length - 1][2]; // массив каналов
            for (int i = 0; i < c.length; i++) {
                int fd0 = 0, fd1 = 1;
                if (i < c.length - 1) {
                    pipe(fd[i]);
                }
                if (c.elem[i].v) {
                    if (c.elem[i].v == 1) {
                        fd0 = open(c.elem[i].pstr[c.elem[i].length-1], O_RDONLY, 0666);
                        if (fd0 == -1){
                            puts("Open failed on input file");
                        }
                        c.elem[i].pstr[c.elem[i].length-1] = NULL;
                        c.elem[i].length--;
                    }
                    if (c.elem[i].v == 2) {
                        fd1 = open(c.elem[i].pstr[c.elem[i].length-1], O_WRONLY|O_TRUNC|O_CREAT, 0666);
                        c.elem[i].pstr[c.elem[i].length-1] = NULL;
                        if (fd1 == -1){
                            puts("Open failed on output file");
                        }
                        c.elem[i].length--;
                    }
                    if (c.elem[i].v == 3) {
                        fd1 = open(c.elem[i].pstr[c.elem[i].length-1], O_WRONLY|O_APPEND|O_CREAT, 0666);
                        if (fd1 == -1){
                            puts("Open failed on output file");
                        }
                        c.elem[i].pstr[c.elem[i].length-1] = NULL;
                        c.elem[i].length--;
                    }
                } else {
                    if (i == 0) {
                        if (c.length != 1) {
                            fd1 = fd[i][1];
                        }
                    } else {
                        if (i == c.length - 1) {
                            fd0 = fd[i - 1][0];
                        } else {
                            fd0 = fd[i - 1][0];
                            fd1 = fd[i][1];
                        }
                    }
                }
                Execute_com(c.elem[i], fd0, fd1, c.ampersand);
                if (c.elem[i].v){
                    if (fd0) {
                        close(fd0);
                    }
                    if (fd1 != 1){
                        close (fd1);
                    }
                }
                if (i < c.length - 1 && i != 0) {
                    close(fd[i - 1][0]);
                }
                close(fd[i][1]);
            }
            exit(1);
        } else {
            waitpid(son, NULL, 0);
        }
    }
}