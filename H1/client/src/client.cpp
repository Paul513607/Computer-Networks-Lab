#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

#include <string>

#define FIFO_REL_PATH1 "../../client-server"
#define FIFO_REL_PATH2 "../../server-client"

#define handle_error(msg, i) {\
            perror(msg); \
            exit(i); \
        } \

int main(int argc, char* argv[]) {
    bool isOpenSession = true;
    int fd_write, fd_read;
    std::string command;
    char* cmd_output;

    if (-1 == (fd_write = open(FIFO_REL_PATH1, O_WRONLY)))
        handle_error("Error at open", 1);
    if (-1 == (fd_read = open(FIFO_REL_PATH2, O_RDONLY)))
        handle_error("Error at open", 2);

    std::cout << "\nWelcome!";

    while (isOpenSession) {
        std::cout << "\nInput a command: ";
        std::getline(std::cin, command);

        int len = command.length() + 1;

        if (-1 == write(fd_write, &len, sizeof(int)))
            handle_error("Error at write", 1);
        if (-1 == write(fd_write, command.c_str(), len))
            handle_error("Error at write", 2);

        if (-1 == read(fd_read, &len, sizeof(int)))
            handle_error("Error at read", 1);
        cmd_output = new char[len + 1];
        if (-1 == read(fd_read, cmd_output, len))
            handle_error("Error at read", 2);

        if (strcmp(cmd_output, "quit_current_session") == 0)
            isOpenSession = false;
        else
            std::cout << '\n' << cmd_output << '\n';
        delete cmd_output;
    }
    close(fd_read);
    close(fd_write);
    std::cout << "Done!\n";
    return 0;
}