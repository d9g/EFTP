#ifndef eftplib_h
#define eftplib_h

#include <ntsid.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <printf.h>
#include <fcntl.h>
#include <stdbool.h>

// Ports

#define SERVER_CMD_PORT 10000 // server main's port (listen & commands)
#define SERVER_DATA_PORT 10001 // server file upload port
#define CLIENT_DATA_PORT 10002 // client file upload port

// Client Commands
#define CMD_QUIT "quit"
#define CMD_HELP "help"
#define CMD_LS "ls"
#define CMD_PWD "pwd"
#define CMD_CD "cd"
#define CMD_RM "rm"

// Remote Commands
#define CMD_RLS "rls"
#define CMD_RPWD "rpwd"
#define CMD_RCD "rcd"
#define CMD_UPLOAD "upld"
#define CMD_DOWNLOAD "downl"

// Auth Messages
#define MSG_HELLO "BONJ"
#define MSG_LOGIN "WHO"
#define MSG_PASSWORD "PASSWD"
#define MSG_LOGIN_OK "WELC"
#define MSG_LOGIN_FAIL "BYE"

// "cd" Command Messages
#define MSG_CD_OK "CDOK"
#define MSG_CD_FAIL "NOCD"

// "upld" Command Messages
#define MSG_UPLOAD_READY "RDYUP"
#define MSG_UPLOAD_ABANDON "FBDN"

// "downl" Command Messages
#define MSG_DOWNLOAD_READY "RDYDOWN"
#define MSG_DOWNLOAD_ERROR "UNKWN"

// packet definitions
#define PACKET_TYPE_DATA 0x01 // flag for packet containing data
#define PACKET_TYPE_END 0x02 // flag indicating that the data transfer is done
#define PACKET_DATA_SIZE 1024
struct messagePacket {
    char type;
    int length;
    char data[PACKET_DATA_SIZE];
};

// Other
typedef char* string;

// Functions

/**
 * execute a command
 * command: the name of the command
 * params: array containing the parameters of the command (can be NULL)
 * paramCount: amount of parameters (can be 0)
 */
void execCommand(string command, string *params, int paramCount);

/**
 * execute a command, store the result in a file and returns it on a socket
 * sock: a socket to send the command's result
 * command: the name of the command
 * params: array containing the parameters of the command (can be NULL)
 * paramCount: amount of parameters (can be 0)
 * returns: 0 if OK, -1 if error
 */
int execRemoteCommand(int sock, string command, string *params, int paramCount);

/**
 * build a messagePacket structure
 * packet: a pointer to a messagePacket structure
 * type: either PACKET_TYPE_DATA if the packet contains data or PACKET_TYPE_END if the packet is the last
 * length: length of the data array (max defined by PACKET_DATA_SIZE)
 * data: array of bytes
 */
void buildMessage(struct messagePacket *_packet, char type, int lentgh, char* data);

/**
 * extract the string contained in a messagePacket
 * _packet: the messagePacket structure to extract data from
 * returns: a string extracted from the packet
 */
char* packetToString(struct messagePacket _packet);

/**
 * check if a string starts with the given prefix
 * poss: the string to check
 * chunk: the prefix to look for
 * returns: true if the prefix is found, false otherwise
 */
bool startswith(char* poss, char* chunk);

/**
 * check if it possible to write a file
 * returns: true if possible, false otherwise
 */
bool checkWriteRights(string fileName);

/**
 * check if it possible to read a file
 * returns: true if possible, false otherwise
 */
bool checkReadRights(string fileName);

/**
 * Send a file
 */
void sendFile(string serverIp, int port, string fileName);

/**
 * Receive a file
 */
void receiveFile(int port, string fileName);
#endif