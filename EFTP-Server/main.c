#include <stdio.h>
#include "eftp-server.h"


int main (int argc, const char * argv[]) {
    // Launching the server
    printf("lancement du serveur sur le port : %d\n", SERVER_CMD_PORT);
    initServer();

    while(1)
    {
        handleClient();
    }
    return 0;
}