#include <stdio.h>
#include <unistd.h>     // getcwd
#include <sys/types.h>  // uid_t
#include <pwd.h>        // getpwuid

#define STATE_CWD_LEN 128
#define STATE_INPUT_LEN 128

struct COMMAND_FRAG {
    char *binname;
    char *arg;
    struct COMMAND_FRAG *pipe_next;
    struct COMMAND_FRAG *arg_next;
    // TODO redirect
};

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
    printf("cwd: %s\n", State.cwd);
    printf("input: %s\n", State.input);
}

void printWelcome(){
    printf("  ____             _      __            _  __         __  __    \n");
    printf(" / __ \\___  ___   | | /| / /__ ___ __  / |/ /__  ____/ /_/ /    \n");
    printf("/ /_/ / _ \\/ -_)  | |/ |/ / _ `/ // / /    / _ \\/ __/ __/ _ \\   \n");
    printf("\\____/_//_/\\__/   |__/|__/\\_,_/\\_, / /_/|_/\\___/_/  \\__/_//_/   \n");
    printf("                              /___/                             \n");
    printf("\n");
}

int parseCommand(){
    char temp[STATE_INPUT_LEN];
    int temp_index = 0;
    int is_binname = 1;
    for(int i = 0; i < STATE_INPUT_LEN || State.input[i] == '\0'; i++){
        if (!isspace(State.input[i]) && State.input[i] != '|'){
            temp[temp_index] = State.input[i];
            temp_index++;
        }
        else if (State.input[i] == '|'){

        }
        else if (temp_index == 0){
            // might | and blank at same time
            continue;
        }
        else{

        }
    }
}

int main(void){
    printWelcome();
    while(1){
        updateState();
        prompt();
        printState();
    }
    return 0;
}
