#include <netinet/in.h>
#include <arpa/inet.h>
#include "eftplib.h"

void execCommand(string command, string* params, int paramCount) {
    if (fork() == 0) // fils
    {
        string *args = malloc((paramCount + 1) * sizeof(char *));
        args[0] = command;
        for(int i=0; i<paramCount; i++)
            args[i+1] = params[i];
        args[paramCount+1] = NULL;
        execvp(args[0], args);
    }
    else
        wait(NULL);
}

int execRemoteCommand(int sock, string command, string *params, int paramCount) {
    int bytes, outputFileFd, stdoutFd;
    char filename[256];
    struct messagePacket packet;
    sprintf(filename, "out_%d.txt", getpid());
    outputFileFd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if(outputFileFd<0)
        return -1;
    stdoutFd = dup(STDOUT_FILENO);
    dup2(outputFileFd, STDOUT_FILENO); // redirect output to file
    execCommand(command, params, paramCount);
    close(outputFileFd);
    dup2(stdoutFd, 1); // restore output to stdout
    outputFileFd = open(filename, O_RDONLY);
    if(outputFileFd<0)
        return -1;
    char buff[PACKET_DATA_SIZE];
    bzero(buff, sizeof(buff));
    while ((bytes=read(outputFileFd, buff, sizeof(buff))) > 0) {
        buildMessage(&packet, PACKET_TYPE_DATA, bytes, buff);
        write(sock, &packet, sizeof(packet));
        bzero(buff, sizeof(buff));
    }
    buildMessage(&packet, PACKET_TYPE_END, 0, NULL);
    write(sock, &packet, sizeof(packet));
    close(outputFileFd);
    remove(filename);
    return 0;
}

void buildMessage(struct messagePacket *_packet, char type, int length, char* data) {
    struct messagePacket packet;
    packet.type = type;
    packet.length = length;
    bzero(&packet.data, sizeof(packet.data));
    memcpy(packet.data, data, length);
    *_packet = packet;
}

char* packetToString(struct messagePacket _packet) {
    char *msg = malloc((_packet.length*sizeof(char))+1);
    memcpy(msg, _packet.data, _packet.length);
    msg[_packet.length] = '\0';
    return msg;
}

bool startswith(char* poss, char* chunk) {
    char* result = strstr(poss, chunk);
    return result != NULL && result == poss;
}

bool checkWriteRights(string fileName) {
    int fileFd = open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if(fileFd < 0)
        return false;
    close(fileFd);
    remove(fileName);
    return true;
}

bool checkReadRights(string fileName) {
    int fileFd = open(fileName, O_RDONLY);
    if(fileFd < 0)
        return false;
    close(fileFd);
    return true;
}

void sendFile(string serverIp, int port, string fileName) {
    printf("Envoi de fichier vers %s:%d\n", serverIp, port);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);
    int _sock;
    int _fileFd = open(fileName, O_RDONLY);
    if(_fileFd < 0)
        perror("open()");
    int _bytes;
    struct messagePacket packet;
    char _buff[PACKET_DATA_SIZE];
    bzero(_buff, sizeof(_buff));
    if((_sock = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        printf("Erreur de socket\n");
        exit(-1);
    }
    if(connect(_sock, (struct sockaddr*)&serverAddr,sizeof(serverAddr)) < 0)
        perror("connect()");
    while ((_bytes=read(_fileFd, _buff, sizeof(_buff))) > 0) {
        buildMessage(&packet, PACKET_TYPE_DATA, _bytes, _buff);
        write(_sock, &packet, sizeof(packet));
        bzero(_buff, sizeof(_buff));
    }
    if(_bytes<0)
        perror("read()");
    close(_fileFd);
    close(_sock);
}

void receiveFile(int port, string fileName) {
    int listenSocket, clientSocket;
    char buff[50];
    int bytes;
    socklen_t len;
    struct sockaddr_in _serverAddr, _clientAddr;
    struct messagePacket packet;
    bzero(packet.data, sizeof(packet.data));
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&_serverAddr,sizeof(_serverAddr));
    _serverAddr.sin_family=AF_INET;
    _serverAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    _serverAddr.sin_port=htons(port);
    if (bind(listenSocket,(struct sockaddr *)& _serverAddr,sizeof(_serverAddr))<0)
    {
        printf("Erreur de bind\n");
        perror("bind()");
        exit(-1);
    }
    listen(listenSocket, 1);
    len=sizeof(_clientAddr);
    int fileFd = open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    clientSocket = accept(listenSocket, (struct sockaddr*)&_clientAddr,&len);
    printf("connexion pour réception de fichier de %s sur le port %d\n",inet_ntop(AF_INET,&_clientAddr.sin_addr,buff,sizeof(buff)),ntohs(_clientAddr.sin_port));
    while((bytes=read(clientSocket, &packet, sizeof(packet)))>0)
    {
        write(fileFd, packet.data, packet.length);
    }
    if(bytes<0)
        perror("read()");
    close(fileFd);
    close(clientSocket);
    close(listenSocket);
}