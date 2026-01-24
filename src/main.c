#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_ARGS 64

int main(){
    char *line = NULL; //buffer pour getline
    size_t len = 0; //taille du buffer

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

        //builtin exit
        if (strcmp(argv[0], "exit") == 0){
            break; 
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // processus enfant
            execvp(argv[0], argv);
            perror("execvp"); // si exec échoue
            exit(1);
        } else {
            // processus parent
            wait(NULL);
        }
    
    }

    free(line); //libérer la mémoire
    return 0;
}