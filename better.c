#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


int nh_cd(char **args){
        if (args[1]==NULL){
                perror("nsh expected a argument with cd");
        }else if (chdir(args[1])!=0){
                perror("nsh: ");
        }
        return 1;
}

void history_func(){
    char histfile[1024];
    const char* home = getenv("HOME");
    if (!home) {
        perror("Could not find HOME");
        return;
    }
    snprintf(histfile, sizeof(histfile), "%s/.nshistory", home);

    FILE* fPtr = fopen(histfile, "r");
    if (fPtr == NULL){
        perror("History file does not exist, creating...");
        fPtr = fopen(histfile, "w");
        if (fPtr) fclose(fPtr);
        return;
    }
    char history[1000];
    while (fgets(history, sizeof(history), fPtr) != NULL) {
        printf("%s", history);
    }
    fclose(fPtr);
}

void shell_function(){
    #define MAX_ARGS 64
    char* input = malloc(1024*sizeof(char));
    if (!input) { perror("malloc"); exit(1); }

    char histfile[1024];
    const char* home = getenv("HOME");
    snprintf(histfile, sizeof(histfile), "%s/.nshistory", home);

    while (1) {
	if (geteuid() == 0) {
		printf("\033[0;31m root# ➜ \033[0m");
    	} else {
        	printf("\033[1;34m user$ ➜ \033[0m");
    	}
        if (!fgets(input, 1024, stdin)) break;
        input[strcspn(input, "\n")] = '\0';

        if (input[0]=='\0') continue;

        FILE *hf = fopen(histfile, "a");
        if (hf) {
            fprintf(hf, "%s\n", input);
            fclose(hf);
        }

        char* args[MAX_ARGS];
        char* token = strtok(input, " ");
        int i = 0;
        while (token && i < MAX_ARGS-1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (strcmp(args[0], "cd") == 0) {
            nh_cd(args);
            continue;
        }
        if (strcmp(args[0], "exit") == 0) {
            printf("!!! Program is exiting right now !!!\n");
            break;
        }
        if (strcmp(args[0], "history") == 0) {
            history_func();
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
        } else if (pid == 0) {
            execvp(args[0], args);
            perror("exec failed");
            exit(1);
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    }
    free(input);
}


int main(){
	shell_function();
	return 0;
}
