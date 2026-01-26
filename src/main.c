#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_ARGS 64

int redirect_output(char **argv) {
    for (int i = 0; argv[i]; i++) {
        if (strcmp(argv[i], ">") == 0) {
            if (!argv[i + 1]) {
                fprintf(stderr, "No file for redirection\n");
                return -1;
            }
            int fd = open(argv[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open");
                return -1;
            }
            dup2(fd, STDOUT_FILENO); // redirige stdout vers le fichier
            close(fd);

            argv[i] = NULL; // couper argv avant ">"
            break;
        }
    }
    return 0;
}

int redirect_input(char **argv) {
    for (int i = 0; argv[i]; i++) {
        if (strcmp(argv[i], "<") == 0) {
            if (!argv[i + 1]) {
                fprintf(stderr, "No file for input redirection\n");
                return -1;
            }
            int fd = open(argv[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("open");
                return -1;
            }
            dup2(fd, STDIN_FILENO); // redirige stdin depuis le fichier
            close(fd);

            argv[i] = NULL;
            break;
        }
    }
    return 0;
}


void sigint_handler(int sig) {
    (void)sig; // on n’utilise pas le numéro du signal
    const char *prompt = "\nminishell$ ";
    write(STDOUT_FILENO, prompt, 12); // affiche le prompt immédiatement
}

int main(){
    char *line = NULL; //buffer pour getline
    size_t len = 0; //taille du buffer

    signal(SIGINT, sigint_handler);

    while(1){ //boucle infinie
        //afficher le prompt
        printf("minishell$ ");
        fflush(stdout); //s'assurer que le prompt s'affiche immédiatement

        //lire la ligne
        ssize_t nread = getline(&line, &len, stdin);

        //gérer fin de fichier (Ctrl+D)
        if (nread == -1){
            printf("\nExit\n");
            break;
        }

        //retirer le '\n' final
        if (line[nread - 1] == '\n'){
            line[nread - 1] = '\0';
        }

        //ignorer la ligne vide
        if (line[0] == '\0'){
            continue;
        }

        //découper la lgine en arguments
        char *argv[MAX_ARGS];
        int i = 0;
        char *token = strtok(line, " ");
        while (token && i < MAX_ARGS - 1){
            argv[i++] = token;
            token = strtok(NULL, " ");
        }
        argv[i] = NULL;

        //builtins
        if (argv[0] == NULL){
            continue;
        }

        //builtin exit
        if (strcmp(argv[0], "exit") == 0){
            break; 
        }

        //builtin : cd
        if (strcmp(argv[0], "cd") == 0){
            if (argv[1] == NULL){
                fprintf(stderr, "cd: missing argument\n");
            }else if (chdir(argv[1]) != 0){
                perror("cd");
            }
            continue; //ne fork pas pour cd
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // processus enfant
            if (redirect_output(argv) == -1 || redirect_input(argv) == -1) {
                exit(1); // problème de fichier pour la redirection
            }
            execvp(argv[0], argv);
            perror("execvp"); // si exec échoue
            exit(1);
        }
        else {
            // processus parent
            wait(NULL);
        }
    }

    free(line); //libérer la mémoire
    return 0;
}