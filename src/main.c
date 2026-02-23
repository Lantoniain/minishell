#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_ARGS 64
#define MAX_CMDS 16

// ---------------- Redirections ----------------
int redirect_output(char **argv) {
    for (int i = 0; argv[i]; i++) {

        // >  (truncate)
        if (strcmp(argv[i], ">") == 0) {
            if (!argv[i + 1]) {
                fprintf(stderr, "No file for redirection\n");
                return -1;
            }

            int fd = open(argv[i + 1],
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);

            if (fd < 0) {
                perror("open");
                return -1;
            }

            dup2(fd, STDOUT_FILENO);
            close(fd);
            argv[i] = NULL;
            break;
        }

        // >>  (append)
        if (strcmp(argv[i], ">>") == 0) {
            if (!argv[i + 1]) {
                fprintf(stderr, "No file for redirection\n");
                return -1;
            }

            int fd = open(argv[i + 1],
                          O_WRONLY | O_CREAT | O_APPEND,
                          0644);

            if (fd < 0) {
                perror("open");
                return -1;
            }

            dup2(fd, STDOUT_FILENO);
            close(fd);
            argv[i] = NULL;
            break;
        }
    }
    return 0;
}

int redirect_input(char **argv) {
    for (int i = 0; argv[i]; i++) {
        if (strcmp(argv[i], "<") == 0) {
            if (!argv[i + 1]) { fprintf(stderr, "No file for input redirection\n"); return -1; }
            int fd = open(argv[i + 1], O_RDONLY);
            if (fd < 0) { perror("open"); return -1; }
            dup2(fd, STDIN_FILENO);
            close(fd);
            argv[i] = NULL;
            break;
        }
    }
    return 0;
}

// ---------------- Signal handler ----------------
void sigint_handler(int sig) {
    (void)sig;
    const char *prompt = "\nminishell$ ";
    write(STDOUT_FILENO, prompt, 12);
}

// ---------------- Découpe des arguments avec guillemets ----------------
void parse_args_quotes(char *line, char **argv) {
    int i = 0;
    char *p = line;
    while (*p) {
        // sauter les espaces initiaux
        while (*p == ' ') p++;
        if (*p == '\0') break;

        char *start;
        //int in_quotes = 0;

        if (*p == '"') { // argument entre guillemets
            //in_quotes = 1;
            start = ++p; // ignorer le guillemet ouvrant
            while (*p && *p != '"') p++;
        } else {
            start = p;
            while (*p && *p != ' ') p++;
        }

        // mettre fin à l’argument
        if (*p) { *p = '\0'; p++; }

        argv[i++] = start;
    }
    argv[i] = NULL;
}

// ---------------- Main ----------------
int main() {
    char *line = NULL;
    size_t len = 0;

    signal(SIGINT, sigint_handler);

    while(1) {
        printf("minishell$ ");
        fflush(stdout);

        ssize_t nread = getline(&line, &len, stdin);
        if (nread == -1) { printf("\nExit\n"); break; }
        if (line[nread - 1] == '\n') line[nread - 1] = '\0';
        if (line[0] == '\0') continue;

        // ---------------- Builtins ----------------
        // Builtin exit
        if (strncmp(line, "exit", 4) == 0) { 
            break;
        }

        // Builtin cd
        if (strncmp(line, "cd", 2) == 0) {
            char *dir = strtok(line + 3, " "); // après "cd "
            if (!dir) fprintf(stderr, "cd: missing argument\n");
            else if (chdir(dir) != 0) perror("cd");
            continue;
        }

        // ---------------- Pipes multiples ----------------
        char *cmds[MAX_CMDS];
        int n_cmds = 0;
        cmds[n_cmds] = strtok(line, "|");
        while ((cmds[++n_cmds] = strtok(NULL, "|")) != NULL);

        int pipefd[2*(n_cmds-1)]; // N-1 pipes

        for (int i = 0; i < n_cmds-1; i++) {
            pipe(pipefd + i*2);
        }

        for (int i = 0; i < n_cmds; i++) {
            char *argv[MAX_ARGS];
            parse_args_quotes(cmds[i], argv);

            if (!argv[0]) {
                continue;
            }            

            pid_t pid = fork();

            if (pid == 0) {
                // stdin ← pipe précédent
                if (i > 0) dup2(pipefd[(i-1)*2], STDIN_FILENO);

                // stdout → pipe suivant
                if (i < n_cmds-1) dup2(pipefd[i*2 + 1], STDOUT_FILENO);

                // fermer tous les pipes
                for (int j = 0; j < 2*(n_cmds-1); j++)
                    close(pipefd[j]);

                // redirections
                if (redirect_input(argv) == -1 || redirect_output(argv) == -1)
                    exit(1);

                // -------- BUILTIN ECHO (ICI) --------
                if (strcmp(argv[0], "echo") == 0) {
                    int j = 1;
                    while (argv[j]) {
                        printf("%s", argv[j]);
                        if (argv[j + 1])
                            printf(" ");
                        j++;
                    }
                    printf("\n");
                    exit(0);
                }

                // commande normale
                execvp(argv[0], argv);
                perror("execvp");
                exit(1);
            }
        }

        // fermer tous les pipes dans le parent
        for (int i = 0; i < 2*(n_cmds-1); i++) close(pipefd[i]);
        for (int i = 0; i < n_cmds; i++) wait(NULL);
    }

    free(line);
    return 0;
}