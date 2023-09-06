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
#define PIPE_READ_BUFF_SIZE 2048

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
    printf("                             redh3tALWAYS                           \n");
    printf("\n");

    // do something;
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
            // printf("temp_index: %d, is_binname: %d\n",temp_index, is_binname);
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

int executeCommand(){
    // var result_std standard output, can be passed through pipes.
    if (State.command->binname == NULL)
        return -1;
    struct COMMAND_FRAG *current_bin = State.command;
    int pipe_fd[2];
    if(pipe(pipe_fd)==-1){
        perror("pipe create failed.");
        return 1;
    }
    else {
        fprintf(stdout,"%d, %d\n",pipe_fd[0],pipe_fd[1]);
    }
    fprintf(stdout,"aaaaa\n");
    int from_pipe = 0;
    int to_pipe = 0;
    while(1){
        fprintf(stdout,"bbbbb\n");
        if(current_bin->pipe_next != NULL)
            to_pipe = 1;
        else
            to_pipe = 0;

        switch (isBuildInCmd(current_bin->binname)){
        case NULL_BINNAME:  // can't be empty
            printf("binname null.\n");
            break;
        case CMD_SH_HELP:
            printf("help document is not ready yet ~\n");
            break;
        default:
            // not build-in, build arg list first
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
            pid_t wpid;
            int status;
            if (pid == 0){      // child
                // redirect
                if (to_pipe){
                    dup2(pipe_fd[0],STDOUT_FILENO);
                    dup2(pipe_fd[0],STDERR_FILENO);
                }
                if(from_pipe){
                    dup2(pipe_fd[1],STDIN_FILENO);
                }
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                if (execvp(arg_list[0], arg_list) == -1){
                    // fprintf(stdout,"executing %s failed\n",arg_list[0]);
                    perror("child process: ");
                    exit(1);
                }
            }
            else if (pid < 0){  // failed to fork
                perror("fork failed.");
                exit(1);
            }
            else{               // parent
                do{
                    wpid = waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            }
            free(arg_list);
            break;
        }

        if(current_bin->pipe_next != NULL){
            current_bin = current_bin->pipe_next;
            from_pipe = 1;
        }
        else{
            break;
        }

    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);

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
