#ifndef EFTP_SERVER_H
#define EFTP_SERVER_H

#include <netinet/in.h>
#include "eftplib.h"

#define MAX_LOGIN_ATTEMPTS 3
#define MAX_CLIENTS 10

int listenSocket; // listen socket for new clients connections
int clientSocket; // client socket
socklen_t clientLength; // client addr length
struct sockaddr_in serverAddr, clientAddr; // struct for listen & client ips

/**
 * Init the server and starts listening for connections
 */
void initServer();

/**
 * Accept a client connection and handle the commands it sends
 */
void handleClient();

/**
 * Send an error packet to the client
 */
void sendErrorMessage(int sock);

/**
 * Attempt to authenticate a client
 * returns: -1 if I/O error, -2 if credentials are invalid, 0 if credentials are valid
 */
int checkAuthentification(int sock);

/**
 * Check if a given login & password are in the credentials file
 * login: the client login
 * password: the client password
 * returns: true if the login/pass has been found, false otherwise
 */
bool checkCredentials(string login, string password);

#endif
