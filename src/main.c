#include <stdio.h>
#include <unistd.h>     // getcwd, fork, gethostname
#include <sys/types.h>  // uid_t, pid_t
#include <pwd.h>        // getpwuid
#include <string.h>     // strcpy
#include <stdlib.h>     // malloc
#include <ctype.h>      // isspace
#include <wait.h>       // waitpid
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"
#define KNOR "\x1B[0m"
#define KB "\x1B[1m"
#define KI "\x1B[3m"

#define STATE_INPUT_LEN 128
#define PIPE_READ_BUFF_SIZE 8192

// prototype printState(), used for dev & debug
void printState(int);

enum BUILD_IN_COMMAND{
    NOT_BUILD_IN_CMD = 0,
    CMD_SH_HELP,
    CMD_CD,
};
enum BUILD_IN_COMMAND isBuildInCmd(char *command){
    if(!strcmp(command,"sh-help"))
        return CMD_SH_HELP;
    if(!strcmp(command,"cd"))
        return CMD_CD;
    return NOT_BUILD_IN_CMD;
}

struct REDIRECT_INFO{
    int direction;  // 1 >, 2 <
    char *filename;
    int symbol_count;
    struct REDIRECT_INFO *next;
};
int initRedirNull(struct REDIRECT_INFO *r){
    r->direction = 0;
    r->filename = NULL;
    r->symbol_count = 0;
    r->next = NULL;
    return 4;
}
int deleteRedirAll(struct REDIRECT_INFO *r){    // will left root node
    if(r->filename != NULL){
        free(r->filename);
    }
    if(r->next!=NULL){
        deleteRedirAll(r->next);
        free(r->next);
    }
    return initRedirNull(r);
}

struct COMMAND_FRAG {
    char *binname;
    char *arg;
    struct COMMAND_FRAG *pipe_next;
    struct COMMAND_FRAG *arg_next;
    struct REDIRECT_INFO *redirect;
};
int initCmdFragNull(struct COMMAND_FRAG *frag){
    frag->binname = NULL;
    frag->arg = NULL;
    frag->pipe_next = NULL;
    frag->arg_next = NULL;
    frag->redirect = NULL;
    return 5;
}
int deleteFragAll(struct COMMAND_FRAG *frag){   // will left the root node
    if(frag->binname != NULL){
        free(frag->binname);
    }
    if(frag->arg != NULL){
        free(frag->arg);
    }
    if(frag->pipe_next != NULL){
        deleteFragAll(frag->pipe_next);
        free(frag->pipe_next);
    }
    if(frag->arg_next != NULL){
        deleteFragAll(frag->arg_next);
        free(frag->arg_next);
    }
    if(frag->redirect != NULL){
        deleteRedirAll(frag->redirect);
        free(frag->redirect);
    }
    return initCmdFragNull(frag);
}


struct STATE_STRUCT {
    // TODO need free
    char input[STATE_INPUT_LEN];
    struct passwd *current_passwd;
    struct COMMAND_FRAG *command;
} State;

void printWelcome(){
    printf("  ____             _      __            _  __         __  __        \n");
    printf(" / __ \\___  ___   | | /| / /__ ___ __  / |/ /__  ____/ /_/ /       \n");
    printf("/ /_/ / _ \\/ -_)  | |/ |/ / _ `/ // / /    / _ \\/ __/ __/ _ \\    \n");
    printf("\\____/_//_/\\__/   |__/|__/\\_,_/\\_, / /_/|_/\\___/_/  \\__/_//_/ \n");
    printf("                              /___/                                 \n");
    printf("\n");
    printf("                Powwered-by redh3tALWAYS                            \n");
    printf("\n");
    printf("\n");
}
void flushAndPrompt(){  // here requires the input
    State.current_passwd=getpwuid(getuid());
    char cwd[128];
    getcwd(cwd, 128);
    char *home_path = getenv("HOME");
    char host_name[64];
    gethostname(host_name, 64);

    printf(KGRN);
    printf("(");
    if(getuid()==0)
        printf(KRED);
    else
        printf(KBLU);
    printf(KB);
    printf(KI);
    printf("%s's %s", State.current_passwd->pw_name, host_name);
    printf(KNOR);
    printf(KGRN);
    printf(")-[");
    printf(KWHT);
    printf(KB);
    printf(KI);
    if (strlen(cwd) >= strlen(home_path) && !strncmp(cwd,home_path,strlen(home_path))){
        printf("~%s", cwd+strlen(home_path));
    }
    else{
        printf("%s", cwd);
    }
    printf(KNOR);
    printf(KGRN);
    printf("]-");
    printf(KB);
    printf(KI);
    if(getuid()==0)
        printf(KRED);
    else
        printf(KBLU);
    if(getuid()==0)
        printf("# ");
    else
        printf("$ ");
    printf(KNOR);
    printf(KWHT);

    fgets(State.input, STATE_INPUT_LEN, stdin);
    // TODO readline lib ?
}

int parseRedirect(struct COMMAND_FRAG * current_bin){
    struct COMMAND_FRAG *true_arg = current_bin;
    struct COMMAND_FRAG *arg = current_bin->arg_next;
    struct REDIRECT_INFO *r = current_bin->redirect;
    
    while(arg != NULL && arg->arg != NULL){
        /*
        >> << fd> >& 
        too hard to judge, must follow my gramma !
        */

       // support '>filename' recognization
       // if you meet a arg start with ", that's ok, the if-else still works.

        if (arg->arg[0] != '>' && arg->arg[0] != '<'){
            true_arg = arg;
            arg = arg->arg_next;
            continue;
        }

        if (current_bin->redirect == NULL){
            current_bin->redirect = (struct REDIRECT_INFO *)malloc(sizeof(struct REDIRECT_INFO));
            r = current_bin->redirect;
        }
        else{
            r->next = (struct REDIRECT_INFO *)malloc(sizeof(struct REDIRECT_INFO));
            r = r->next;
        }
        initRedirNull(r);

        if(arg->arg[0] == '>')
            r->direction = 1;
        else
            r->direction = 2;
        r->symbol_count = 1;
        if(arg->arg[1] == arg->arg[0])
            r->symbol_count = 2;
        if(strcmp(arg->arg+r->symbol_count,"")){
            // have chars after redirection symbol
            r->filename = (char *)malloc((strlen(arg->arg)+1-r->symbol_count)*sizeof(char));
            strcpy(r->filename,arg->arg+r->symbol_count);
            true_arg->arg_next = arg->arg_next;
            free(arg->arg);
            free(arg);
            arg = true_arg->arg_next;
        }
        else{
            r->filename = (char *)malloc((strlen(arg->arg_next->arg)+1)*sizeof(char));
            strcpy(r->filename,arg->arg_next->arg);
            true_arg->arg_next = arg->arg_next->arg_next;
            free(arg->arg_next->arg);
            free(arg->arg);
            free(arg);
            arg = true_arg->arg_next;
        }
    }
}
int parseCommand(){
    char temp[STATE_INPUT_LEN];
    int temp_index = 0;
    int is_binname = 2; // 2 the first, 1 after |, 0 the arg.
    struct COMMAND_FRAG *bin_head = NULL;
    struct COMMAND_FRAG *last_arg = NULL;
    int quot_count = 0;  // only support "
    // first parse redirect as params, then parse redirect info, because redirect needs higher privilege
    for(int i = 0; i < STATE_INPUT_LEN && State.input[i] != '\0'; i++){ // last is \n and then is \0
        if (State.input[i] == '"'){
            quot_count++;
            // TODO we should view '\' as a controller, 
            // rather just look back when we meet a special char, e.g. '"'
            // Though currently we don't need '\' in other places
            if(i>0)
                if(State.input[i-1] == '\\'){
                    quot_count--;
                    temp_index--;
                }
        }
        if ((!isspace(State.input[i]) && State.input[i] != '|' && quot_count%2 == 0) || quot_count%2 == 1){
            temp[temp_index] = State.input[i];
            temp_index++;
        }
        else{
            // encountered blank char or '|'
            if (temp_index == 0){
                if (State.input[i] == '|')
                    is_binname = 1;
                continue;   // ignore blank
            }
            temp[temp_index] = '\0';
            switch (is_binname) {
            case 2:
                State.command->binname = (char *)malloc((temp_index+1)*sizeof(char));
                strcpy(State.command->binname,temp);    // need \0 ?
                is_binname = 0;
                bin_head = State.command;
                last_arg = bin_head;
                break;
            case 1:
                bin_head->pipe_next = (struct COMMAND_FRAG *)malloc(sizeof(struct COMMAND_FRAG));
                bin_head = bin_head->pipe_next;
                initCmdFragNull(bin_head);
                bin_head->binname = (char *)malloc((temp_index+1)*sizeof(char));
                strcpy(bin_head->binname, temp);    // need \0 ?
                is_binname = 0;
                last_arg = bin_head;
                break;
            default:
                // is_binname = 0, default is common arg.
                last_arg->arg_next = (struct COMMAND_FRAG *)malloc(sizeof(struct COMMAND_FRAG));
                last_arg = last_arg->arg_next;
                initCmdFragNull(last_arg);
                last_arg->arg = (char *)malloc((temp_index+1)*sizeof(char));
                strcpy(last_arg->arg, temp);
                break;
            }
            temp_index =0;
        }

        if (State.input[i] == '|' && quot_count%2 == 0)
            is_binname = 1;
    }

    // for every bin parse redirect info
    struct COMMAND_FRAG *current_bin = State.command;
    while (current_bin != NULL){
        parseRedirect(current_bin);
        current_bin = current_bin->pipe_next;
    }
}

int executeOnce(struct COMMAND_FRAG *current_bin, int read_pipe_fd, int write_pipe_fd){
    // build arg list first
    struct COMMAND_FRAG *temp = current_bin;
    int count = 1;
    while (temp->arg_next!=NULL){
        count++;
        temp = temp->arg_next;
    }
    char **arg_list = (char **)malloc((count+1)*sizeof(char *));
    arg_list[0] = current_bin->binname;
    arg_list[count] = NULL;
    temp = current_bin->arg_next;
    count = 1;
    while(temp!=NULL){
        if(temp->arg[0] == '"'){
            temp->arg[strlen(temp->arg)-1] = '\0';
            char *str = (char *)malloc(strlen(temp->arg)*sizeof(char));
            strcpy(str,temp->arg+1);
            free(temp->arg);
            temp->arg = (char *)malloc((strlen(str)+1)*sizeof(char));
            strcpy(temp->arg,str);
            free(str);
        }
        arg_list[count] = temp->arg;
        count++;
        temp = temp->arg_next;
    }

    // call fork
    pid_t pid = fork();
    if (pid == 0){      // child process
        // pipe and redirect
        if (write_pipe_fd != -1){
            dup2(write_pipe_fd,STDOUT_FILENO);
            // dup2(write_pipe_fd,STDERR_FILENO);  // pipe shouldn't handle stderr
            close(write_pipe_fd);
        }        
        if(read_pipe_fd != -1){
            dup2(read_pipe_fd,STDIN_FILENO);
            close(read_pipe_fd);
        }

        struct REDIRECT_INFO *r = current_bin->redirect;
        while (r != NULL){
            if(r->direction == 1 && write_pipe_fd == -1){
                int w_fd;
                if(r->symbol_count==1){
                    w_fd = open(r->filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                }
                else{
                    w_fd = open(r->filename, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                }
                dup2(w_fd,STDOUT_FILENO);
                close(w_fd);
            }
            else if(r->direction == 2 && read_pipe_fd == -1){
                int r_fd = open(r->filename, O_RDONLY);
                if (r_fd == -1){
                    perror("redirect");
                    exit(1);
                }
                dup2(r_fd, STDIN_FILENO);
                close(r_fd);
            }
            else{

            }
            r = r->next;
        }
        // execute        
        if (execvp(arg_list[0], arg_list) == -1){
            perror("child process: ");
            exit(1);
        }
    }
    else if (pid > 0){  // parent
        wait(NULL);
    }
    else {
        perror("fork failed.");
        exit(1);
    }

    free(arg_list);
    return 1;
}
int executeCommand(){
    if (State.command->binname == NULL)
        return -1;
    struct COMMAND_FRAG *current_bin = State.command;

    int **pipes = (int **)malloc(2*sizeof(int *));
    pipes[0] = (int *)malloc(2*sizeof(int));
    pipes[1] = (int *)malloc(2*sizeof(int));
    pipe(pipes[0]);
    pipe(pipes[1]);

    int index = 0;
    int read_pipe = -1;
    int write_pipe = 0;
    
    while(1){
        if(current_bin->pipe_next != NULL)
            write_pipe = pipes[index%2][1];
        else{
            // last command, write_pipe is useless, close and ready to quit loop
            write_pipe = -1;
            close(pipes[index%2][0]);
            close(pipes[index%2][1]);
        }

        switch (isBuildInCmd(current_bin->binname)){
        // TODO add pipe support and redirect support for build-in commands.
        case CMD_SH_HELP:
            printf("help document is not ready yet ~\n");
            break;
        case CMD_CD:
            if (current_bin->arg_next->arg==NULL){
                printf("cd: need one argument.\n");
            }
            else{
                int success;
                if (current_bin->arg_next->arg[0] == '~'){
                    success = chdir(getenv("HOME"));
                    if (current_bin->arg_next->arg[1] == '/')
                        success = chdir(current_bin->arg_next->arg+2);
                }
                else
                    success = chdir(current_bin->arg_next->arg);
                if (success == -1)
                    printf("cd: chdir() failed.");
            }
            break;
        default:
            executeOnce(current_bin,read_pipe,write_pipe);
            break;
        }

        if(current_bin->pipe_next != NULL){
            current_bin = current_bin->pipe_next;
            read_pipe = pipes[index%2][0];
            close(pipes[(index+1)%2][0]);
            close(pipes[(index+1)%2][1]);
            free(pipes[(index+1)%2]);
            pipes[(index+1)%2] = NULL;
            pipes[(index+1)%2] = (int *)malloc(2*sizeof(int));
            pipe(pipes[(index+1)%2]);
            close(pipes[index%2][1]);   // close write end to avoid stuck when read
            // should avoid use the same fd in two different pipes, so the order of the code matters!
            index++;
        }
        else{
            close(read_pipe);
            break;
        }

    }

    // all pipe fds are well closed, free() an return
    free(pipes[0]);
    free(pipes[1]);
    free(pipes);
    return 1;
}

void printState(int mode){
    switch (mode) {
    case 1:
        // print binnames and args
        struct COMMAND_FRAG * bin = State.command;
        while(bin!=NULL){
            printf("binname: %s\n",bin->binname);
            struct COMMAND_FRAG * arg = bin->arg_next;
            while(arg!=NULL){
                printf("arg: %s\n",arg->arg);
                arg = arg->arg_next;
            }

            struct REDIRECT_INFO *r = bin->redirect;
            while (r != NULL){
                printf("redirect direction: %d, filename: %s, symbol count: %d\n", r->direction,r->filename,r->symbol_count);
                r = r->next;
            }

            bin = bin->pipe_next;
        }
        break;
    default:
        printf("Go to take a nap..\n");
        break;
    }
    printf("Pressssss enter to continue...");
    getchar();
}

int main(void){
    // init
    State.command = (struct COMMAND_FRAG *)malloc(sizeof(struct COMMAND_FRAG));
    initCmdFragNull(State.command);
    printWelcome();
    while(1){
        flushAndPrompt();
        parseCommand();
        // printState(1);
        executeCommand();

        // clear
        deleteFragAll(State.command);   // clear history here, but history can be saved if there is another link table.
    }
    return 0;
}
