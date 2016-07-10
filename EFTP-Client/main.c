#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "eftp-client.h"

int main (int argc, const char * argv[]) {
    pthread_t threadInput;
    pthread_t threadSocket;
    bool authenticated = false;
    string login = malloc(128);
    string pass = malloc(128);

    printf("lancement du client ...\n");

    // Connecting to the server
    initClient();
    if(connectClient() < 0)
    {
        printf("impossible de se connecter au serveur\n");
        return -1;
    }

    printf("client connecté au serveur !\n");

    // Attempt to authenticate on the server
    while(!authenticated)
    {
        printf("identifiant :\n");
        scanf("%s", login);
        printf("mot de passe :\n");
        scanf("%s", pass);
        int r = authenticate(commandSocket, login, pass);
        if(r == -1) {
            printf("L'authentification a échoué suite à une erreur réseau ou trop de tentatives.\n");
            close(commandSocket);
            exit(0);
        }
        else if(r == -2)
            printf("L'authentification a échoué car l'identifiant ou le mot de passe est invalide.\n\n");
        else {
            printf("Authentification réussie ! \n");
            authenticated = true;
        }
    }


    // Launching threads to listen for keyboard & server data
    if(pthread_create(&threadInput, NULL, readInput, NULL)) {

        printf("Erreur à la création du thread d'écoute du clavier");
        return -1;
    }
    if(pthread_create(&threadSocket, NULL, readCommandSocket, NULL)) {

        printf("Erreur à la création du thread d'écoute du clavier");
        return -1;
    }

    pthread_join(threadInput, NULL);
}