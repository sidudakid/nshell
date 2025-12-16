#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_ARGS 64
#define MAX_HISTORY 1000
#define MAX_LINE 1024

char **history_buffer = NULL;
int history_count = 0;
int history_capacity = 0;

void about(void);
void check_special_functions(char **buf);
void load_history_to_memory(const char *histfile);
void add_to_history_memory(const char *cmd);
char* get_input_with_history(const char *prompt, const char *histfile);
void tab_complete(char *input, int *cursor_pos);

int nh_cd(char **args){
  if (args[1]==NULL){
    perror("nsh expected a argument with cd");
  }else if (chdir(args[1])!=0){
    perror("nsh: ");
  }
  return 1;
}

char* hostname(){
 return "NONE";
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

void load_history_to_memory(const char *histfile) {
    FILE *fPtr = fopen(histfile, "r");
    if (!fPtr) return;
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fPtr)) {
        line[strcspn(line, "\n")] = '\0';
        if (line[0] != '\0') {
            add_to_history_memory(line);
        }
    }
    fclose(fPtr);
}

void add_to_history_memory(const char *cmd) {
    if (history_count >= history_capacity) {
        history_capacity = history_capacity == 0 ? 100 : history_capacity * 2;
        history_buffer = realloc(history_buffer, history_capacity * sizeof(char*));
    }
    history_buffer[history_count] = strdup(cmd);
    history_count++;
}

void tab_complete(char *input, int *cursor_pos) {
    int pos = *cursor_pos;
    
    int start = pos - 1;
    while (start >= 0 && input[start] != ' ') start--;
    start++;
    
    char partial[MAX_LINE];
    int len = pos - start;
    strncpy(partial, input + start, len);
    partial[len] = '\0';
    
    DIR *dir = opendir(".");
    if (!dir) return;
    
    struct dirent *entry;
    char matches[100][256];
    int match_count = 0;
    
    while ((entry = readdir(dir)) != NULL && match_count < 100) {
        if (strncmp(entry->d_name, partial, len) == 0 && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            strcpy(matches[match_count], entry->d_name);
            
            struct stat st;
            if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode)) {
                strcat(matches[match_count], "/");
            }
            match_count++;
        }
    }
    closedir(dir);
    
    if (match_count == 1) {
        int remaining = strlen(matches[0]);
        strcpy(input + start, matches[0]);
        *cursor_pos = start + remaining;
        input[*cursor_pos] = '\0';
        
        printf("\r\033[K%s", input);
        fflush(stdout);
    } else if (match_count > 1) {
        printf("\n");
        for (int i = 0; i < match_count; i++) {
            printf("%s  ", matches[i]);
        }
        printf("\n");
    }
}

char* get_input_with_history(const char *prompt, const char *histfile) {
    struct termios old_term, new_term;
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
    
    static char input[MAX_LINE];
    int pos = 0;
    int history_index = history_count;
    char temp_input[MAX_LINE] = {0};
    
    printf("%s", prompt);
    fflush(stdout);
    
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) break;
        
        if (c == 27) { 
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            
            if (seq[0] == '[') {
                if (seq[1] == 'A') { 
                    if (history_index > 0) {
                        if (history_index == history_count) {
                            strcpy(temp_input, input);
                        }
                        history_index--;
                        strcpy(input, history_buffer[history_index]);
                        pos = strlen(input);
                        printf("\r\033[K%s%s", prompt, input);
                        fflush(stdout);
                    }
                } else if (seq[1] == 'B') { 
                    if (history_index < history_count) {
                        history_index++;
                        if (history_index == history_count) {
                            strcpy(input, temp_input);
                        } else {
                            strcpy(input, history_buffer[history_index]);
                        }
                        pos = strlen(input);
                        printf("\r\033[K%s%s", prompt, input);
                        fflush(stdout);
                    }
                }
            }
        } else if (c == 9) { 
            tab_complete(input, &pos);
        } else if (c == 127 || c == 8) { 
            if (pos > 0) {
                pos--;
                input[pos] = '\0';
                printf("\b \b");
                fflush(stdout);
            }
        } else if (c == '\n') {
            input[pos] = '\0';
            printf("\n");
            break;
        } else if (c >= 32 && c < 127) { 
            if (pos < MAX_LINE - 1) {
                input[pos++] = c;
                input[pos] = '\0';
                printf("%c", c);
                fflush(stdout);
            }
        }
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    return input;
}

void shell_function(){
  int debug = 0;
  char* user = getenv("USER"); 
  FILE *hostfile = fopen("/etc/hostname", "r");
  char hostname[100];
  if (hostfile) {
   fgets(hostname, 100, hostfile);
   hostname[strcspn(hostname, "\n")] = '\0';
   fclose(hostfile);
  } else {
   strcpy(hostname, "localhost");
  }
 
  char histfile[1024];
  const char* home = getenv("HOME");
  snprintf(histfile, sizeof(histfile), "%s/.nshistory", home);
  
  load_history_to_memory(histfile);
  
  while (1) {
    char prompt[256];
    if (geteuid() == 0) {
        snprintf(prompt, sizeof(prompt), "\033[1;31m%s@%s #> \033[0m", user, hostname);
    } else {
        snprintf(prompt, sizeof(prompt), "\033[1;34m%s@%s > \033[0m", user, hostname);
    }
    
    char* input = get_input_with_history(prompt, histfile);
    
    if (input[0] == '\0') continue;
    
    FILE *hf = fopen(histfile, "a");
    if (hf) {
        fprintf(hf, "%s\n", input);
        fclose(hf);
    }
    add_to_history_memory(input);
    
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
  
  for (int i = 0; i < history_count; i++) {
      free(history_buffer[i]);
  }
  free(history_buffer);
}

int main(){
  const char* home = getenv("HOME");
  char fname[256];
  sprintf(fname, "%s/.nshistory", home);
  if (access(fname, F_OK) == 0) {
      shell_function();
    } else {
	   about();
    }
	return 0;
}

void about(){
  char* about="Nshell is a lightweight, customizable command-line shell written in C. It provides basic shell functionality, allowing you to execute commands, manage processes, and navigate the filesystem. Nshell is designed to be simple, efficient, and extendable, with features you can customize and build upon.";
  char* creator = "sidudakid";
  char* version = "1.0.2";
  char* last_msg = "for help with any type help command.";
  printf("%s\n Created by: %s \n Version: %s \n %s\n\n", about, creator, version, last_msg);
  shell_function();
}

void check_special_functions(char **buf){
    if (buf[0] != NULL && strlen(buf[0]) >= 3) {
        if (buf[0][0] == 27 && buf[0][1] == '[' && buf[0][2] == 'A') {
            printf("[!] Up Arrow Key Pressed\n");
        }
    }
}
