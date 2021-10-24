#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <errno.h>

#include <vector>
#include <string>
#include "../include/UserProfile.h"
#include "../include/Command.h"

#define FIFO_REL_PATH1 "../../client-server"
#define FIFO_REL_PATH2 "../../server-client"

void create_fifo(const char* fifo_name, int err_no);
void server_init(int &fd_read, int &fd_write);
void process_command(std::vector<std::string> &cmdlets, std::string command);

int main(int argc, char* argv[]) {
    UserProfile user;
    Command *command_obj = nullptr;
    std::vector<std::string> cmdlets;
    std::string command_str, msg_back_str;
    char *command;
    int fd_read, fd_write, rd = 0;
    int len, arr_size;
    bool serverOpen = true;

    server_init(fd_read, fd_write);

    while (serverOpen) {
        len = rd = 0;
        while (rd <= 0) {           // hold fifo open until client starts
            if (-1 == (rd = read(fd_read, &len, sizeof(int))))
                handle_error("Error at read", 1);
        }
        command = new char[len + 1];
        if (-1 == read(fd_read, command, len))
            handle_error("Error at read", 2);
        command_str = command;
        delete command;

        for (auto cmdlet : cmdlets)
            cmdlet.clear();
        cmdlets.clear();
        msg_back_str.clear();

        process_command(cmdlets, command_str);

        arr_size = cmdlets.size();
        if (arr_size == 1 && cmdlets[0] == "help") {    // help
            command_obj = new HelpCommand();
            command_obj->Execute();
        }
        else if (arr_size == 3 && cmdlets[0] == "login" && cmdlets[1] == ":") {     // login : <username>
            command_obj = new LoginCommand(user, cmdlets[2]);
            command_obj->Execute();
            user = command_obj->user;
        }
        else if (arr_size == 1 && cmdlets[0] == "get-logged-users") {       // get-logged-users
            command_obj = new GetLoggedUsersCommand(user);
            command_obj->Execute();
        }
        else if (arr_size == 3 && cmdlets[0] == "get-proc-info" && cmdlets[1] == ":") {     // get-proc-info
            command_obj = new GetProcInfoCommand(user, cmdlets[2]);
            command_obj->Execute();
        }
        else if (arr_size == 1 && cmdlets[0] == "logout") {     // logout
            if (user.isLogged) {
                    user.logoutUser();
                    msg_back_str = "Sucessfully logged out user";
                }
                else
                    msg_back_str = "No user to log out";
        }
        else if (arr_size == 1 && cmdlets[0] == "quit") {       // quit
            if (user.isLogged)
                user.logoutUser();
            msg_back_str = "quit_current_session";
        }
        else {      // unknown command
            msg_back_str = "Unknown command '" + command_str + "'. Specify 'help' for usage";
        }

        if (command_obj != nullptr) {
            msg_back_str = command_obj->GetMsgBack();
            delete command_obj;
            command_obj = nullptr;
        }
        
        write_len_str(fd_write, len, msg_back_str);
    }
    
    close(fd_read);
    close(fd_write);
    remove(FIFO_REL_PATH1);
    remove(FIFO_REL_PATH2);
    return 0;
}

void create_fifo(const char* fifo_name, int err_no) {
    if (-1 == mkfifo(fifo_name, 0666)) {
        if (errno == EEXIST)
            fprintf(stderr, "Fifo file %d already exists!\n", err_no);
        else 
            handle_error("Error at mkfifo", err_no);
    }
}

void server_init(int &fd_read, int &fd_write) {
    create_fifo(FIFO_REL_PATH1, 1);
    create_fifo(FIFO_REL_PATH2, 2);

    getKnownUsers();

    if (-1 == (fd_read = open(FIFO_REL_PATH1, O_RDONLY)))
        handle_error("Error at open", 1);
    if (-1 == (fd_write = open(FIFO_REL_PATH2, O_WRONLY)))
        handle_error("Error at open", 2);
}

void process_command(std::vector<std::string> &cmdlets, std::string command) {
    int space_index = 0;
    while (space_index != -1) {
        space_index = command.find_first_of(' ');
        if (space_index != -1) {
            cmdlets.push_back(command.substr(0, space_index));
            command.erase(0, space_index + 1);
        }
    }
    cmdlets.push_back(command);
    fflush(stdout);
}