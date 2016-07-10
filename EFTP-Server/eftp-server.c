#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <arpa/inet.h>
#include "eftp-server.h"

void initServer() {
    signal(SIGCHLD, SIG_IGN); // ignore zombie childs
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(listenSocket < 0) {
        printf("Erreur de socket listen\n");
        exit(-1);
    }
    bzero(&serverAddr,sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVER_CMD_PORT);
    if (bind(listenSocket,(struct sockaddr *)& serverAddr,sizeof(serverAddr))<0) {
        printf("Erreur de bind socket listen\n");
        exit(-1);
    }
    listen(listenSocket, MAX_CLIENTS);
    printf("serveur en attente de connexions ...\n");
}

void handleClient() {
    // Accepting a new client connection
    clientLength = sizeof(clientAddr);
    clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr,&clientLength);
    char buff[50];
    printf("connexion de %s sur le port %d\n",inet_ntop(AF_INET,&clientAddr.sin_addr,buff,sizeof(buff)),ntohs(clientAddr.sin_port));
    if(fork()==0)
    {
        bool authenticated = false;
        close(listenSocket);
        int bytes;
        char param[512];
        struct messagePacket packet;
        struct messagePacket response;
        bzero(packet.data, sizeof(packet.data));

        // User Authentification
        for(int i=0; i<MAX_LOGIN_ATTEMPTS; i++)
        {
            int r = checkAuthentification(clientSocket);
            if(r == -1) {
                printf("L'authentification a échoué suite à une erreur réseau\n");
            } else if(r == -2) {
                printf("l'authentification a échoué car l'identifiant ou le mot de passe est invalide\n");
            } else {
                printf("Authentification réussie\n");
                authenticated = true;
                break;
            }
        }
        if(!authenticated)
        {
            printf("Trop de tentatives d'authentications échoués. Fin de la connexion\n");
            close(clientSocket);
            exit(0);
        }

        // Commands processing
        while((bytes = read(clientSocket, &packet, sizeof(packet))) > 0) {
            const char *cmd = packetToString(packet);
            printf("commande du client : [%s]\n", cmd);
            if(strcmp(cmd, CMD_RLS) == 0)
            {
                if(execRemoteCommand(clientSocket, "ls", (char*[]){"-l"}, 1) != 0)
                    sendErrorMessage(clientSocket);
            }
            else if(strcmp(cmd, CMD_RPWD) == 0)
            {
                if(execRemoteCommand(clientSocket, "pwd", NULL, 0) != 0)
                    sendErrorMessage(clientSocket);
            }
            else if(startswith(packet.data, CMD_RCD))
            {
                strcpy(param, cmd+4); // +4 to ignore the command part
                if(chdir(param) == 0)
                    buildMessage(&response, PACKET_TYPE_DATA, sizeof(MSG_CD_OK), MSG_CD_OK);
                else
                    buildMessage(&response, PACKET_TYPE_DATA, sizeof(MSG_CD_FAIL), MSG_CD_FAIL);
                write(clientSocket, &response, sizeof(response));
            }
            else if(startswith(packet.data, CMD_UPLOAD))
            {
                strcpy(param, cmd+5); // +5 to ignore the command part
                if(checkWriteRights(param))
                {
                    buildMessage(&response, PACKET_TYPE_DATA, sizeof(MSG_UPLOAD_READY), MSG_UPLOAD_READY);
                    write(clientSocket, &response, sizeof(response));
                    receiveFile(SERVER_DATA_PORT, param);
                }
                else
                {
                    buildMessage(&response, PACKET_TYPE_DATA, sizeof(MSG_UPLOAD_ABANDON), MSG_UPLOAD_ABANDON);
                    write(clientSocket, &response, sizeof(response));
                }
            }
            else if(startswith(packet.data, CMD_DOWNLOAD))
            {
                strcpy(param, cmd+6); // +6 to ignore the command part
                if(checkReadRights(param))
                {
                    buildMessage(&response, PACKET_TYPE_DATA, sizeof(MSG_DOWNLOAD_READY), MSG_DOWNLOAD_READY);
                    write(clientSocket, &response, sizeof(response)); sleep(1);
                    sendFile((string)inet_ntop(AF_INET,&clientAddr.sin_addr,buff,sizeof(buff)), CLIENT_DATA_PORT, param);
                }
                else
                {
                    buildMessage(&response, PACKET_TYPE_DATA, sizeof(MSG_DOWNLOAD_ERROR), MSG_DOWNLOAD_ERROR);
                    write(clientSocket, &response, sizeof(response));
                }
            }
            else
            {
                printf("La commande [%s] n'a pas été reconnue.\n", cmd);
            }
        }
        if(bytes < 0)
            printf("Une erreur de lecture est survenue\n");

        printf("fin de connexion avec le client\n");
        close(clientSocket);
        exit(0);
    }

    close(clientSocket);
}

void sendErrorMessage(int sock) {
    struct messagePacket packet;
    packet.type = PACKET_TYPE_END;
    packet.length = sizeof("L'action a échoué\n")+1;
    strcpy(packet.data, "L'action a échoué\n");
    write(sock, &packet, sizeof(packet));
}

int checkAuthentification(int sock)
{
    int bytes;
    string msg;
    string login;
    string password;
    struct messagePacket request;
    struct messagePacket response;
    // wait for HELLO
    bytes = read(sock, &response, sizeof(response));
    if(bytes<0)
        return -1;
    msg = packetToString(response);
    if(strcmp(msg, MSG_HELLO) != 0)
        return -1;
    // send WHO
    buildMessage(&request, PACKET_TYPE_DATA, sizeof(MSG_LOGIN), MSG_LOGIN);
    write(sock, &request, sizeof(request));
    // wait for login
    bytes = read(sock, &response, sizeof(response));
    if(bytes<0)
        return -1;
    login = packetToString(response);
    // send PASSWD
    buildMessage(&request, PACKET_TYPE_DATA, sizeof(MSG_PASSWORD), MSG_PASSWORD);
    write(sock, &request, sizeof(request));
    // wait for password
    bytes = read(sock, &response, sizeof(response));
    if(bytes<0)
        return -1;
    password = packetToString(response);
    // check credentials
    if(checkCredentials(login, password))
    {
        buildMessage(&request, PACKET_TYPE_DATA, sizeof(MSG_LOGIN_OK), MSG_LOGIN_OK);
        write(sock, &request, sizeof(request));
        return 0;
    } else {
        buildMessage(&request, PACKET_TYPE_DATA, sizeof(MSG_LOGIN_FAIL), MSG_LOGIN_FAIL);
        write(sock, &request, sizeof(request));
        return -2;
    }
}

bool checkCredentials(string login, string password) {
    char buf[256];
    string tokens;
    string foundLogin;
    string foundPassword;
    FILE *cred = fopen ("credentials.txt" , "r");
    if(cred == NULL)
        return false;
    while (fgets (buf, sizeof(buf), cred)) {
        foundLogin = strtok(buf, " ");
        foundPassword = strtok(NULL, " ");
        foundPassword[strlen(foundPassword)-1] = '\0'; // remove the '\n' marker
        if(strcmp(foundLogin, login) == 0 && strcmp(foundPassword, password) == 0) {
            fclose(cred);
            return true;
        }
    }
    fclose(cred);
    return false;
}