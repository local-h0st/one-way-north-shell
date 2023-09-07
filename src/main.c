#include <stdio.h>
#include <unistd.h>     // getcwd, fork
#include <sys/types.h>  // uid_t, pid_t
#include <pwd.h>        // getpwuid
#include <string.h>     // strcpy
#include <stdlib.h>     // malloc
#include <ctype.h>      // isspace
#include <wait.h>       // waitpid

#define STATE_CWD_LEN 128
#define STATE_INPUT_LEN 128
#define PIPE_READ_BUFF_SIZE 8192

enum BUILD_IN_COMMAND{
    NOT_BUILD_IN_CMD = 0,
    NULL_BINNAME,
    CMD_SH_HELP,
};

struct COMMAND_FRAG {
    char *binname;
    char *arg;
    struct COMMAND_FRAG *pipe_next;
    struct COMMAND_FRAG *arg_next;
    // TODO redirect
};

int initCmdFragNull(struct COMMAND_FRAG *frag){
    frag->binname = NULL;
    frag->arg = NULL;
    frag->pipe_next = NULL;
    frag->arg_next = NULL;
    return 4;
}

int deleteFragAll(struct COMMAND_FRAG *frag){
    // will left the root node
    if(frag->binname!=NULL)
        free(frag->binname);
    if(frag->arg!=NULL)
        free(frag->arg);
    if(frag->pipe_next!=NULL){
        deleteFragAll(frag->pipe_next);
        free(frag->pipe_next);
    }
    if(frag->arg_next!=NULL){
        deleteFragAll(frag->arg_next);
        free(frag->arg_next);
    }
    return initCmdFragNull(frag);
}

struct STATE_STRUCT {
    char cwd[STATE_CWD_LEN];
    char input[STATE_INPUT_LEN];
    struct passwd *current_passwd;
    struct COMMAND_FRAG *command;
} State;


void updateState(){
    getcwd(State.cwd, STATE_CWD_LEN);           // cwd
    State.current_passwd=getpwuid(getuid());    // current passwd file
}

void prompt(){
    // to do with 'State'
    // here requires the input
    printf("(%s) might be a prompt > ", State.current_passwd->pw_name);
    fgets(State.input, STATE_INPUT_LEN, stdin);
}


void printState(){
    // for debug
    struct COMMAND_FRAG * bin = State.command;
    while(bin!=NULL){
        printf("binname: %s\n",bin->binname);
        struct COMMAND_FRAG * arg = bin->arg_next;
        while(arg!=NULL){
            printf("arg: %s\n",arg->arg);
            arg = arg->arg_next;
        }
        bin = bin->pipe_next;
    }
}

void printWelcome(){
    printf("  ____             _      __            _  __         __  __        \n");
    printf(" / __ \\___  ___   | | /| / /__ ___ __  / |/ /__  ____/ /_/ /       \n");
    printf("/ /_/ / _ \\/ -_)  | |/ |/ / _ `/ // / /    / _ \\/ __/ __/ _ \\    \n");
    printf("\\____/_//_/\\__/   |__/|__/\\_,_/\\_, / /_/|_/\\___/_/  \\__/_//_/ \n");
    printf("                              /___/                                 \n");
    printf("\n");
    printf("                      Powwered-by redh3tALWAYS                      \n");
    printf("\n");

    // do some init
    State.command = (struct COMMAND_FRAG *)malloc(sizeof(struct COMMAND_FRAG));
    initCmdFragNull(State.command);
}

int parseCommand(){
    deleteFragAll(State.command);
    char temp[STATE_INPUT_LEN];
    int temp_index = 0;
    int is_binname = 2; // 2 the first, 1 after |, 0 the arg.
    struct COMMAND_FRAG *bin_head = NULL;
    struct COMMAND_FRAG *last_arg = NULL;
    for(int i = 0; i < STATE_INPUT_LEN && State.input[i] != '\0'; i++){ // last is \n and then is \0
        if (!isspace(State.input[i]) && State.input[i] != '|'){
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

        if (State.input[i] == '|')
            is_binname = 1;
    }
}

enum BUILD_IN_COMMAND isBuildInCmd(char *command){
    if (command==NULL)
        return NULL_BINNAME;
    if(!strcmp(command,"sh-help"))
        return CMD_SH_HELP;

    return NOT_BUILD_IN_CMD;
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
        arg_list[count] = temp->arg;
        count++;
        temp = temp->arg_next;
    }

    // call fork
    pid_t pid = fork();
    if (pid == 0){      // child process
        // redirect
        if (write_pipe_fd != -1){
            dup2(write_pipe_fd,STDOUT_FILENO);
            dup2(write_pipe_fd,STDERR_FILENO);
            close(write_pipe_fd);
        }
        if(read_pipe_fd != -1){
            dup2(read_pipe_fd,STDIN_FILENO);
            close(read_pipe_fd);
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
        // TODO add pipe support for build-in commands.
        case CMD_SH_HELP:
            printf("help document is not ready yet ~\n");
            break;
        case NOT_BUILD_IN_CMD:
            executeOnce(current_bin,read_pipe,write_pipe);
            break;
        default:
            perror("unknown command.");
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


int main(void){
    printWelcome();
    while(1){
        updateState();
        prompt();
        parseCommand();
        executeCommand();
        // printState();
    }
    return 0;
}
