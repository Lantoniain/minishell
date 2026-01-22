#include<stdio.h>
#include<stdlib.h>

int main(){
    char *line = NULL; //buffer pour getline
    size_t len = 0; //taille du buffer

    while(1){ //boucle infinie
        //afficher le prompt
        printf("minishell$");
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

        //afficher la ligne pour test
        printf("Vous avez tapé: '%s'\n", line);
    }

    free(line); //libérer la mémoire
    return 0;
}