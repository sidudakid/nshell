#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>


int nh_cd(char **args){
	if (args[1]==NULL){
		perror("nsh expected a argument with cd");
	}else if (chdir(args[1])!=0){
		perror("nsh: ");
	}
	return 1;
}
void history_func(){

	const char* histfile = "~/.nshistory";
	FILE* fPtr;

	char* history = malloc(1000*sizeof(char));
        
		
	if (fPtr==NULL){
		perror("file Doesnot exist!!!");
		fPtr = fopen(histfile, "w");
		fclose(fPtr);	
	}else{
		const char* mode = (char*) 'r';
		fPtr = fopen(histfile, mode);
        	while (fgets(history, 50, fPtr)
               		!= NULL) {
            		printf("%s", history);
       		 }
	}
	free(history);

}

void shell_function(){
	#define MAX_ARGS 64
	bool isTimeout = false;
	int status;
	char* input = malloc(1024*sizeof(char));
	while(true){
		printf(" > ");
		fgets(input, sizeof(input), stdin);
		input[strcspn(input, "\n")] = '\0';
		char* token = strtok(input, " ");
		char* args[MAX_ARGS];
		int i=0;
		
		while (token != NULL){
			args[i++] = token;
			token = strtok(NULL, " ");
		}
		args[i]=NULL;
		if(args[0]==NULL) continue;
		if(strcmp(args[0], "cd")==0) nh_cd(args);
		if (strcmp(args[0], "exit")==0) break;
		if (strcmp(args[0],"exit")==0)
		{
			printf("!!! Program is exiting right now !!!");
			exit(1);
		}else if(strcmp(args[0], "history")==0){
			history_func();
		}
		pid_t pid = fork();

		if( pid < 0){
			perror("Fork Failed");
		}else if(pid==0){
			execvp(args[0], args);
			perror("exec failed");
			exit(1);
		}else{
			waitpid(pid, &status, 0);
		}

	}
	free(input);
}
int main(){
	shell_function();
}
