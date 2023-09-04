#include <stdio.h>
#include <unistd.h>     // getcwd
#include <sys/types.h>  // uid_t
#include <pwd.h>        // getpwuid

#define STATE_CWD_LEN 128
#define STATE_INPUT_LEN 128

struct STATE_STRUCT {
    char cwd[STATE_CWD_LEN];
    char input[STATE_INPUT_LEN];
    struct passwd *current_passwd;
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
    printf("  ____             _      __            _  __         __  __    ");
    printf(" / __ \\___  ___   | | /| / /__ ___ __  / |/ /__  ____/ /_/ /    ");
    printf("/ /_/ / _ \\/ -_)  | |/ |/ / _ `/ // / /    / _ \\/ __/ __/ _ \\   ");
    printf("\\____/_//_/\\__/   |__/|__/\\_,_/\\_, / /_/|_/\\___/_/  \\__/_//_/   ");
    printf("                              /___/                             ");
}

int main(void){
    while(1){
        updateState();
        prompt();
        printState();
    }
    return 0;
}
