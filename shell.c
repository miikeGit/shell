#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>

#define BUFSIZE 1024

#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void throw_err(char* description) {
	fprintf(stderr, description);
	exit(EXIT_FAILURE);
}

char *builtin_str[] = {
	"clear",
	"cd",
	"help",
  	"exit"
};

int sh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int sh_clear(char **args) {
  printf("\033[H\033[2J"); // Moves cursor to the top left corner of the screen and clears it
  return 1;
}

void sh_print_logo() {
    printf(ANSI_COLOR_GREEN " ____  _          _ _ \n");
    printf(ANSI_COLOR_GREEN "/ ___|| |__   ___| | |\n");
    printf(ANSI_COLOR_GREEN "\\___ \\| '_ \\ / _ \\ | |\n");
    printf(ANSI_COLOR_GREEN " ___) | | | |  __/ | |\n");
    printf(ANSI_COLOR_GREEN "|____/|_| |_|\\___|_|_|\n" ANSI_COLOR_RESET);
}

int sh_help(char** args) {
	sh_clear(args);
	sh_print_logo();
	printf("Enter commands and their arguments, then press Enter.\n");
    printf(ANSI_COLOR_YELLOW "Available built-in commands:\n" ANSI_COLOR_RESET);
    
    for (int i = 0; i < sh_num_builtins(); i++) {
        printf(" - " ANSI_COLOR_GREEN "%s\n" ANSI_COLOR_RESET, builtin_str[i]);
    }

    printf("Any other command will be treated as an external program.\n");

	return 1;
}

int sh_cd(char **args) {
	if (args[1] == NULL) fprintf(stderr, "cd: expected argument, none provided.");
	else if (chdir(args[1])) perror("sh_cd");
	return 1;
}

int sh_exit(char** args) {
	return 0;
}

int (*builtin_func[]) (char **) = {
	&sh_clear,
  	&sh_cd,
  	&sh_help,
  	&sh_exit
};

char* sh_read_line() {	
	char *line = NULL;
	size_t bufsize = 0;

	if (getline(&line, &bufsize, stdin) == -1) {
		if (feof(stdin)) {
			exit(EXIT_SUCCESS);
		} else {
			perror("READ_LINE_ERROR");
			exit(EXIT_FAILURE);
		}
	}

	return line;
}

char** sh_split_line(char* line) {
	int bufsize = BUFSIZE;
	char **tokens = malloc(bufsize);
	if (!tokens) throw_err("SPLIT_LINE_ALLOC_ERROR_1");

	char* token = NULL;
	char* delim = " \t\n";
	token = strtok(line, delim);

	for (int i = 0; token != NULL; i++) {
		tokens[i] = token;

		// Reallocate memory for tokens array if it's not enough
		if (i >= bufsize) {
			bufsize += BUFSIZE;
			tokens = realloc(tokens, bufsize);

			if (!tokens) throw_err("SPLIT_LINE_ALLOC_ERROR_2");
		}
		token = strtok(NULL, delim);
	}
	return tokens;
}

int sh_launch(char** args) {

	pid_t pid, wpid;
	int status;
	
	pid = fork();

	if (!pid) { // Child
		// Expects program name and array of arguments
		if (execvp(args[0], args) == -1) {
			perror("sh_execute");
		}
		exit(EXIT_FAILURE);
	} else if (pid == -1) {
		// Error forking
		perror("sh_execute");
	} else { // Parent
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSTOPPED(status)); // Run until child is either exited or killed
	}

	return 1;
}

int sh_execute(char **args) {
	if (!args[0]) return 1;
	// Execute built-in
	for (int i = 0; i < sh_num_builtins(); i++) {
		if (!strcmp(args[0], builtin_str[i])) return (*builtin_func[i])(args); 
	}
	// Else run program;
	return sh_launch(args);
}

void sh_loop() {
	char* line;
	char** args;
	int status;

	do {
		printf("> ");
		// Read line from terminal
		line = sh_read_line();
		// Split into arguments
		args = sh_split_line(line);
		// Execute
		status = sh_execute(args);
	} while(status); 
}

int main() {

	sh_help(NULL);
	sh_loop();

	return 0;
}
