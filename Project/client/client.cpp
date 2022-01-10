#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>
#include <cstring>
#include <dirent.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <mutex>
#include <stack>
#include <sqlite3.h>

int err_counter_cli = 0;
bool openSession = true;

#define handle_error(msg, err_counter_cli) { \
            perror(msg); \
            err_counter_cli++; \
            exit(err_counter_cli); \
        } \

/* utilities */
#include "../utilities/File.h"
#include "../utilities/file-system/file-system.h"
#include "../utilities/archive-bin/Archive.h"
#include "../utilities/match-patch/Match-Patch.h"
#include "../utilities/util.h"

#include "client.h"

int main(int argc, char *argv[]) 
{
    if (argc < 3) {
        std::cout << "Syntax is: client.o <ip_address> <port>" << std::endl;
        exit(1);
    }

    Client client;
    client.initialize(argv[1], argv[2]);
    // client.readUsername();
    client.connectToServer();

    while (openSession)
    {
        client.sendCommand();
        client.receiveResponse();
    }
    
    client.closeClient();
    return 0;
}