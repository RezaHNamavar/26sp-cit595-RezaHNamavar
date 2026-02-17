/* include these header files to use their functions */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tokenizer.h" // adding tokenizer to it
#include <fcntl.h>

/* Macro to universaly define the size of the input.
 *
 * The compiler will replace all instances of this macro with the value.
 *
 * This helps avoid "magic numbers", which you should avoid in your code.
 */
#define INPUT_SIZE 1024

/* This is declared outside of main, so it is a global variable.
 *
 * This is the ONLY global variable you are allowed to use.
 *
 * You must ensure that this value is updated when appropriate.
 *
 * In general, avoid the use of global variables in any code you write.
 */
pid_t childPid = 0;

/* In C, you must declare the function before
 * it is used elsewhere in your program.
 *
 * By defining them at the top of the program, you avoid
 * implied declaration warnings (the compiler will guess that the return value is an int).
 *
 * This is normally implemented as a header (.h) file.
 *
 * You may choose to refactor this into a header file,
 * as long as you update your makefile orrectly.
 */
//void alarmHandler(int sig); // remove
void executeShell(void);
char *getCommandFromInput();
void killChildProcess();
void registerSignalHandlers();
void sigintHandler(int sig);
void writeToStdout(char *text);

/* This is the main function.
 *
 * It will register the signal handlers via function call,
 * check for a timeout (for project1b),
 * and call `executeShell` in a loop indefinitely.
 *
 * DO NOT modify this function, it is correctly implemented for you.
 */
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    registerSignalHandlers();

    //int timeout = 0;
    //if (argc == 2) {
    //    timeout = atoi(argv[1]);
    //}

    //if (timeout < 0) {
    //    writeToStdout("Invalid input detected. Ignoring timeout value.\n");
    //    timeout = 0;
    //}

    while (1) {
        executeShell();
    }

    return 0;
}

/* Sends SIGKILL signal to the child process identified by `childPid`.
 *
 * It will check for system call errors and exit penn-shredder program if
 * there is an error.
 *
 * DO NOT modify this function, it is correctly implemented for you.
 */
void killChildProcess() {
    if (kill(childPid, SIGKILL) == -1) {
        perror("Error in kill");
        exit(EXIT_FAILURE);
    }
}

/* Signal handler for SIGALRM.
 *
 * It should kill the child process.
 *
 * It should then print out penn-shredder's catchphrase to standard output.
 *
 * If no child process currently exists, it should take no action.
 *
 * TODO: implement in project1b.
 */
/*void alarmHandler(int sig) { //remove this whole thing?
    (void)sig;

    if (childPid != 0) {
        killChildProcess();
        const char msg[] = "Bwahaha ... tonight I dine on turtle soup\n"; //writeToStdout("Bwahaha ... tonight I dine on turtle soup\n");
        write(STDOUT_FILENO, msg, sizeof(msg) - 1); // update
    }

}*/


/* Signal handler for SIGINT.
 *
 * Kills the child process if childPid is non-zero.
 *
 * Takes no action if the child process does not exist.
 *
 * Takes no action on  parent process and its execution.
 *
 * When a user enters Control+C, this sends the SIGINT signal to the program.
 * If this function is registered to run on SIGINT, 
 * it will run the function body instead of the default behaviour.
 * Read the manual pages for signal (section 7) to see the default behaviour of all signals.
 *
 * DO NOT modify this function, it is correctly implemented for you.
 */
void sigintHandler(int sig) {
    if (childPid != 0) {
        killChildProcess();
    }
}


/* Registers SIGALRM and SIGINT handlers with corresponding functions.
 *
 * Checks for system call failure and exits program if
 * there is an error.
 *
 * TODO: implement the SIGARLM portion in project1b.
 *
 * The SIGINT portion is implemented correctly for you.
 */
void registerSignalHandlers() {
    if (signal(SIGINT, sigintHandler) == SIG_ERR) { // remove this?
        perror("invalid signal");
        exit(EXIT_FAILURE);
    }

    //if (signal(SIGALRM, alarmHandler) == SIG_ERR) { // remove this?
    //    perror("Error in signal");
    //    exit(EXIT_FAILURE);
    //}
}

static void freeTokens(char **tokens, int count);

static void freeTokens(char **tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i]);
    }

}


/* Prints the shell prompt and waits for input from user.
 *
 * It then creates a child process which executes the command.
 *
 * The parent process waits for the child and checks that
 * it was either signalled or exited.
 *
 * TODO: you may modify this function for project1a and project1b.
 *
 * TODO: implement the alarm portion in project1b.
 * Use the timeout argument to start an alarm of that timeout period.
 * */
void executeShell(void) {
    //alarm (0); //added
    char *input_line;
    int status;
    char minishell[] = "penn-shredder# ";
    writeToStdout(minishell);

    input_line = getCommandFromInput();

    if (input_line == NULL) {
        return;
    }

    TOKENIZER *tokenizer = init_tokenizer(input_line);
    if (tokenizer == NULL) {
        perror("tokenizer null");
        free(input_line);
        return;
    }

    char *argv[INPUT_SIZE];
    int argc = 0;

    char *current_token; 
    while ((current_token = get_next_token(tokenizer)) != NULL) {
        argv[argc++] = current_token;
        if (argc >= INPUT_SIZE - 1) break;                      
    }
    argv[argc] = NULL;

    if (argc == 0) {
        freeTokens(argv, argc);
        free_tokenizer(tokenizer); 
        free(input_line);
        return;
    }

    //the redirect part
    char *input_file = NULL;
    char *output_file = NULL;

    char *exec_argv[INPUT_SIZE];
    int execution_count = 0;
    

    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "|") == 0 ) {
            writeToStdout("Invalid\n");
            freeTokens(argv, argc);
            free_tokenizer(tokenizer);
            free(input_line);
            return;
        }

        if (strcmp(argv[i], "<") == 0) {
            if (argv[i + 1] == NULL || strcmp(argv[i + 1], "<") == 0 || strcmp(argv[i + 1], ">") == 0) {
                writeToStdout("Invalid\n");
                freeTokens(argv, argc);
                free_tokenizer(tokenizer);
                free(input_line);
                return;
            }

            if (input_file != NULL) {
                writeToStdout("Invalid standard input redirect");
                freeTokens(argv, argc);
                free_tokenizer(tokenizer);
                free(input_line);
                return;
            }
            input_file = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], ">") == 0) {
            if (argv[i + 1] == NULL || strcmp(argv[i + 1], "<") == 0 || strcmp(argv[i + 1], ">") == 0) {
                writeToStdout("Invalid multiple standard output");
                freeTokens(argv, argc);
                free_tokenizer(tokenizer);
                free(input_line);
                return; 
            }

            if (output_file != NULL) {
                writeToStdout("Invalid  multiple standard output");
                freeTokens(argv, argc);
                free_tokenizer(tokenizer);
                free(input_line);
                return;
            }
            output_file = argv[i + 1];
            i++;

        } else {
            exec_argv[execution_count++] = argv[i];
            if (execution_count >= INPUT_SIZE - 1) break;

        }
    }

    exec_argv[execution_count] = NULL;

    if (execution_count == 0) {
        freeTokens(argv, argc);
        free_tokenizer(tokenizer);
        free(input_line);
        return;
    }

    childPid = fork();

    if (childPid < 0) {
        perror("Error in creating child process");
        freeTokens(argv, argc);
        free_tokenizer(tokenizer);
        free(input_line);
        return;
    }

    if (childPid == 0) {
            
        if (signal(SIGINT, SIG_DFL) == SIG_ERR) { //updated 2a
            perror("Error in signal");
            freeTokens(argv, argc);
            free_tokenizer(tokenizer); 
            free(input_line);
            exit (EXIT_FAILURE);
        }

        if (input_file != NULL) {
            int filedesc_in = open(input_file, O_RDONLY);
            if (filedesc_in < 0) {
                perror("Invalid standard input redirect");
                freeTokens(argv, argc);
                free_tokenizer(tokenizer); 
                free(input_line);
                exit(EXIT_FAILURE);
            }
            if (dup2(filedesc_in, STDIN_FILENO) < 0) {
                perror("invalid dup2");
                close(filedesc_in); 
                freeTokens(argv, argc);
                free_tokenizer(tokenizer); 
                free(input_line);
                exit(EXIT_FAILURE);
            }
            close(filedesc_in);
        }

        if (output_file != NULL) {
            int filedesc_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644) ;
            if (filedesc_out < 0) {
                perror("Invalid standard output redirect");
                freeTokens(argv, argc);
                free_tokenizer(tokenizer); 
                free(input_line);
                exit(EXIT_FAILURE);
            }
            if (dup2(filedesc_out, STDOUT_FILENO) < 0 ) {
                perror("invalid dup2"); 
                close(filedesc_out);
                freeTokens(argv, argc);
                free_tokenizer(tokenizer); 
                free(input_line);
                exit(EXIT_FAILURE);
            }
            close(filedesc_out);
        }

        execvp(exec_argv[0], exec_argv);
        perror("invalid execvp");
        freeTokens(argv, argc);
        free_tokenizer(tokenizer); 
        free(input_line);
        exit(EXIT_FAILURE);
    }
        

        
    do {
        if (waitpid(childPid, &status, 0) == -1 ) { 
            perror("invalid wait");
            break;
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    
    //alarm(0);
    childPid = 0;
    freeTokens(argv, argc);
    free_tokenizer(tokenizer);
    free(input_line);
        
    
}

/* Writes particular text to standard output.
 *
 * Exits penn-shredder if the system call `write` fails.
 *
 * DO NOT modify this function, it is correctly implemnted for you.
 */
void writeToStdout(char *text) {
    if (write(STDOUT_FILENO, text, strlen(text)) == -1) {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}

/* Reads input from standard input using `read` system call.
 * You are not permitted to use other functions like `fgets`.
 *
 * If the user enters Control+D from the keyboard,
 * penn-shredder exits immediately.
 *
 * If the user enters text followed by Control+D,
 * penn-shredder does not exit and does not report an error.
 *
 * Note specifically that Control+D represents End-Of-File (EOF).
 * This is not a character that can be read.  It just tells `read` that
 * there is no more data to read from the keyboard.
 *
 * The leading and trailing spaces must be removed from the user input.
 *
 * The string must be NULL-terminated.
 *
 * Note that the starter code is hardcoded to return "/bin/ls",
 * which will cause an infinite loop as provided.
 *
 * TODO: implement this function for project1a.
 */
char *getCommandFromInput() {
    char buffer[INPUT_SIZE];
    ssize_t total = 0;

    while (total < INPUT_SIZE - 1) {
        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);

        if (n == -1) {
            perror("Error in read");
            exit(EXIT_FAILURE);
        }
        
        if (n == 0 ) {
            if (total == 0 ) {
                exit(EXIT_SUCCESS);
            }
            break;
        }
        if (ch == '\n') {
            break;
        }

        buffer[total++] = ch;
    }

    buffer[total] = '\0';

    ssize_t start = 0;
    while (start < total && buffer[start] == ' ') {
        start++;
    }

    ssize_t end = total - 1;
    while (end >= start && buffer[end] == ' ') {
        end--;
    }

    if (start > end) {
        return NULL;
    }

    ssize_t len = end - start + 1;
    char *input_line = (char *)malloc((size_t)len + 1);
    if (input_line == NULL) {
        perror("Error in malloc");
        exit(EXIT_FAILURE);
    }

    memcpy(input_line, buffer + start, (size_t)len);
    input_line[len] = '\0';

    return input_line;

}


