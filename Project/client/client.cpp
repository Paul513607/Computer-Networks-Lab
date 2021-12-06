#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <cstring>

#include <iostream>
#include <string>

int err_counter_cli = 0;
bool openSession = true;

#define handle_error(msg, err_counter_cli) { \
            perror(msg); \
            err_counter_cli++; \
            exit(err_counter_cli); \
        } \

#include "client.h"

int main(int argc, char *argv[]) 
{
    if (argc < 3) {
        std::cout << "Syntax is: client.o <ip_address> <port>\n";
        exit(1);
    }

    Client client;
    client.initialize(argv[1], argv[2]);
    client.readUsername();
    client.connectToServer();

    while (openSession)
    {
        client.sendCommand();
        client.receiveResponse();
    }
    
    client.closeClient();
    return 0;
}