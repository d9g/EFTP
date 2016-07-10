#include <strings.h>
#include <arpa/inet.h>
#include <printf.h>
#include <stdlib.h>
#include "eftp-client.h"

void initClient() {
    bzero(&serverAddressCommand,sizeof(serverAddressCommand));
    serverAddressCommand.sin_family = AF_INET;
    serverAddressCommand.sin_port = htons(SERVER_PORT_CMD);
    inet_pton(AF_INET, SERVER_IP, &serverAddressCommand.sin_addr);
    if((commandSocket = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        printf("Erreur de socket command\n");
        exit(-1);
    }
    isWaiting = false;
}

int connectClient() {
    return connect(commandSocket, (struct sockaddr*)&serverAddressCommand,sizeof(serverAddressCommand));
}

void showHelp() {
    printf("- Liste des commandes locales \n");
    printf("help : affiche cette aide \n");
    printf("ls : liste les fichiers du dossier courant\n");
    printf("pwd : donne le nom et le chemin du dossier courant\n");
    printf("cd : change le dossier courant\n");
    printf("rm : supprime le fichier\n");
    printf("- Liste des commandes distantes \n");
    printf("rls : liste les fichiers du dossier courant sur le serveur\n");
    printf("rpwd : donne le nom et le chemin du dossier courant sur le serveur\n");
    printf("rcd : change le dossier courant sur le serveur\n");
    printf("upld : envoi un fichier au serveur\n");
    printf("downl : récupère un fichier sur le serveur\n");
}

void *readInput(void *args) {
    bool keepRunning = true;
    char buffer[512]; // command
    char buffer2[512]; // additional parameter (for rm, cd, etc.)
    struct messagePacket commandPacket;
    printf("\n== Saisir une commande (help pour la liste des commandes) ==\n");
    while(keepRunning) {
        if(!isWaiting)
        {
            // Read input on STDIN
            bzero(buffer, sizeof(buffer));
            bzero(commandPacket.data, sizeof(commandPacket.data));
            printf("client > ");
            scanf("%s", buffer);
            // Process input
            if(strcmp(buffer, CMD_QUIT) == 0)
            {
                keepRunning = false;
            }
            else if(strcmp(buffer, CMD_HELP) == 0)
            {
                showHelp();
            }
            else if(strcmp(buffer, CMD_LS) == 0)
            {
                execCommand(CMD_LS, (char*[]){"-l"}, 1);
            }
            else if (strcmp(buffer, CMD_PWD) == 0)
            {
                execCommand(CMD_PWD, NULL, 0);
            }
            else if (strcmp(buffer, CMD_CD) == 0)
            {
                printf("quel est le chemin ?\n");
                scanf("%s", buffer);
                chdir(buffer);
            }
            else if(strcmp(buffer, CMD_RM) == 0)
            {
                printf("quel est le chemin ?\n");
                scanf("%s", buffer);
                execCommand(CMD_RM, (char*[]){buffer}, 1);
            }
            else if(strcmp(buffer, CMD_RLS) == 0 || strcmp(buffer, CMD_RPWD) == 0)
            {
                isWaiting = true;
                buildMessage(&commandPacket, PACKET_TYPE_DATA, strlen(buffer)+1, buffer);
                write(commandSocket, &commandPacket, sizeof(commandPacket));
            }
            else if(strcmp(buffer, CMD_RCD) == 0)
            {
                isWaiting = true;
                printf("quel est le chemin ?\n");
                scanf("%s", buffer2);
                sprintf(buffer, "rcd %s", buffer2);
                buildMessage(&commandPacket, PACKET_TYPE_DATA, strlen(buffer)+1, buffer);
                write(commandSocket, &commandPacket, sizeof(commandPacket));
            }
            else if(strcmp(buffer, CMD_UPLOAD) == 0)
            {
                isWaiting = true;
                printf("nom du fichier ?\n");
                scanf("%s", fileName);
                sprintf(buffer, "upld %s", fileName);
                buildMessage(&commandPacket, PACKET_TYPE_DATA, strlen(buffer)+1, buffer);
                write(commandSocket, &commandPacket, sizeof(commandPacket));
            }
            else if(strcmp(buffer, CMD_DOWNLOAD) == 0)
            {
                isWaiting = true;
                printf("nom du fichier ?\n");
                scanf("%s", fileName);
                sprintf(buffer, "downl %s", fileName);
                buildMessage(&commandPacket, PACKET_TYPE_DATA, strlen(buffer)+1, buffer);
                write(commandSocket, &commandPacket, sizeof(commandPacket));
            }
            else
            {
                printf("La commande n'a pas été reconnue.\n");
            }
        }
    }
    return NULL;
}

void *readCommandSocket(void *args)
{
    int bytes;
    char *msg;
    struct messagePacket packet;
    bzero(packet.data, sizeof(packet.data));
    while(1) {
        while((bytes=read(commandSocket, &packet, sizeof(packet)))>0)
        {
            if(packet.type == PACKET_TYPE_DATA) {
                msg = packetToString(packet);
                if(strcmp(msg, MSG_DOWNLOAD_READY) == 0)
                {
                    printf("le serveur a accepté la demande de téléchargement.\n");
                    receiveFile(CLIENT_DATA_PORT, fileName);
                    printf("transfert terminé.\n");
                    isWaiting = false;
                }
                else if(strcmp(msg, MSG_DOWNLOAD_ERROR) == 0)
                {
                    printf("échec du téléchargement.\n");
                    isWaiting = false;
                }
                else if(strcmp(msg, MSG_UPLOAD_ABANDON) == 0)
                {
                    printf("le serveur a refusé la demande d'upload.\n");
                    isWaiting = false;
                }
                else if(strcmp(msg, MSG_UPLOAD_READY) == 0)
                {
                    printf("serveur prêt pour l'upload.\n");
                    sendFile(SERVER_IP, SERVER_PORT_DATA, fileName);
                    printf("transfert terminé.\n");
                    isWaiting = false;
                }
                else if(strcmp(msg, MSG_CD_OK) == 0)
                {
                    printf("commande cd effectuée avec succès.\n");
                    isWaiting = false;
                }
                else if(strcmp(msg, MSG_CD_FAIL) == 0)
                {
                    printf("commande cd échoué.\n");
                    isWaiting = false;
                }
                else {
                    printf("%s", msg);
                }
                free(msg);
            }
            if(packet.type == PACKET_TYPE_END) {
                if(packet.length > 0) {
                    msg = packetToString(packet);
                    printf("%s", msg);
                }
                isWaiting = false;
            }
            bzero(&packet.data, sizeof(packet.data));
        }
        printf("Le serveur a fermé la connexion.\n");
        break;
    }
    return NULL;
}

int authenticate(int sock, string login, string password)
{
    int bytes;
    char *msg;
    struct messagePacket request;
    struct messagePacket response;
    // send HELLO
    buildMessage(&request, PACKET_TYPE_DATA, sizeof(MSG_HELLO), MSG_HELLO);
    write(sock, &request, sizeof(request));
    // wait for WHO
    bytes = read(sock, &response, sizeof(response));
    if(bytes<0)
        return -1;
    msg = packetToString(response);
    if(strcmp(msg, MSG_LOGIN) != 0)
        return -1;
    // send login
    buildMessage(&request, PACKET_TYPE_DATA, strlen(login), login);
    write(sock, &request, sizeof(request));
    // wait for PASSWD
    bytes = read(sock, &response, sizeof(response));
    if(bytes<0)
        return -1;
    msg = packetToString(response);
    if(strcmp(msg, MSG_PASSWORD) != 0)
        return -1;
    // send password
    buildMessage(&request, PACKET_TYPE_DATA, strlen(password), password);
    write(sock, &request, sizeof(request));
    // wait for result
    bytes = read(sock, &response, sizeof(response));
    if(bytes<0)
        return -1;
    msg = packetToString(response);
    if(strcmp(msg, MSG_LOGIN_FAIL) == 0)
        return -2;
    if(strcmp(msg, MSG_LOGIN_OK) != 0)
        return -1;
    else
        return 0;
}