#ifndef EFTP_CLIENT_H
#define EFTP_CLIENT_H

#include <netinet/in.h>
#include <stdbool.h>
#include <eftplib.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT_CMD 10000
#define SERVER_PORT_DATA 10001

bool isWaiting; // flag indicating if we are waiting for a server response
char fileName[256]; // to store the name of the file to upload or download
int commandSocket;  // commands & status
struct sockaddr_in serverAddressCommand; // addr struct for the server ip

/**
 * Init the client
 */
void initClient();

/**
 * Connect to the server
 * returns: 0 if successful, -1 otherwise
 */
int connectClient();

/**
 * Display the help to the user
 */
void showHelp();

/***
 * Thread function handling the reading of stdin (user commands)
 */
void *readInput(void *args);

/**
 * Thread function handling the reading of the socket (server responses)
 */
void *readCommandSocket(void *args);

/**
 * Attempt to authenticate on the server
 * returns: -1 if I/O error, -2 if credentials are invalid, 0 if credentials are valid
 */
int authenticate(int sock, string login, string password);

#endif
