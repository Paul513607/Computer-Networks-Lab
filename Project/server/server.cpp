#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
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

int err_counter_serv = 0;
bool openServer = true;
#define PORT 3200

#define handle_error(msg, err_counter_serv) { \
            perror(msg); \
            err_counter_serv++; \
            exit(err_counter_serv); \
        } \

#define handle_error2(msg, err_counter_serv) { \
            err_counter_serv++; \
            fprintf(stderr, "Error %s number %d\n", msg, err_counter_serv);  \
        } \

#define handle_error_while(msg, err_counter_serv) { \
            err_counter_serv++; \
            fprintf(stderr, "Error %s number %d\n", msg, err_counter_serv); \
            fflush(stderr); \
        } \

/* utilities */
#include "../utilities/File.h"
#include "../utilities/file-system/file-system.h"
#include "../utilities/archive-bin/Archive.h"
#include "../utilities/match-patch/Match-Patch.h"
#include "../utilities/util.h"

#include "userprofile.h"
#include "commands.h"
#include "server.h"

int main(int argc, char *argv[])
{
    Server server;
    pid_t child_pid;
    bool handleClient;
    server.initialize();
    while(openServer) {
        if (!server.acceptConnection())
            continue;
        
        child_pid = fork();
        switch (child_pid)
        {
        case -1:
            server.closeClientSd();
            handle_error_while("Error at fork", err_counter_serv);
            continue;
        case 0:
            server.closeServerSd();
            handleClient = true;
            while(handleClient) {
                if (!server.receiveCommand()) {
                    server.closeClientSd();
                    exit(err_counter_serv);
                }

                server.runCommand();

                if (!server.sendResponse()) {
                    server.closeClientSd();
                    exit(err_counter_serv);
                }
                if (server.getCommand() == "quit_session")
                    handleClient = false;
            }
            server.logoutUser();
            server.closeClientSd();
            std::cout << "[server child] Finished with client!" << std::endl;
            exit(0);
        default:
            server.closeClientSd();
            while(waitpid(-1, NULL, WNOHANG));
            break;
        }
    }

    server.closeServer();
}