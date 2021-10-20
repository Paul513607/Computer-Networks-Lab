#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <errno.h>
#include <utmp.h>

#include <vector>
#include <string>
#include "../include/UserProfile.hpp"

#define FIFO_REL_PATH1 "../../client-server"
#define FIFO_REL_PATH2 "../../server-client"

#define handle_error(msg, i) {\
            fprintf(stderr, "%s %d\n", msg, i); \
            exit(i); \
        } \

void prompt_help() {

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
    for (auto cmdlet : cmdlets)
        std::cout << cmdlet << " ";
    std::cout << '\n';
}

void create_fifo(const char* fifo_name) {
    if (-1 == mkfifo(fifo_name, 0666)) {
        if (errno == EEXIST)
            fprintf(stderr, "Fifo file already exists!\n");
        else 
            handle_error("Error at mkfifo", 1);
    }
}

int main(int argc, char* argv[]) {
    UserProfile user;
    char *command, *username, *pid_cstr;
    std::string command_str, msg_back_str;
    char* msg_back;
    std::vector<std::string> cmdlets;
    std::string text_parent;
    int fd_read, fd_write;
    int len, arr_size;
    pid_t pid_child;
    int pipe_fd_parent_child[2], pipe_fd_child_parent[2], socket_fd[2];
    bool isLoggedIn, serverOpen = true;

    remove(FIFO_REL_PATH1);
    remove(FIFO_REL_PATH2);

    create_fifo(FIFO_REL_PATH1);
    create_fifo(FIFO_REL_PATH2);

    getKnownUsers();

    if (-1 == (fd_read = open(FIFO_REL_PATH1, O_RDONLY)))
        handle_error("Error at open", 1);
    if (-1 == (fd_write = open(FIFO_REL_PATH2, O_WRONLY)))
        handle_error("Error at open", 2);

    while (serverOpen) {
        if (-1 == read(fd_read, &len, sizeof(int)))
            handle_error("Error at read", 1);
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
            if (-1 == pipe(pipe_fd_child_parent))
                handle_error("Error at pipe", 1);
            pid_child = fork();
            switch (pid_child) 
            {
            case -1:
                handle_error("Error at fork", 1);
            case 0:
                close(pipe_fd_child_parent[0]);

                text_parent = "Log in/out:\n\tlogin : <username>\tLogin as <username> in the current session\n\tlogout\t\t\tLogout from current session (requires valid login)\nGet information:\n\tget-logged-user\t\tGet informaton about the logged user (requires valid login)\n\tget-proc-info : <pid>\tGet information about the process with pid <pid> (requires valid login)\nMiscellaneous:\n\tquit\t\t\tQuit the current session\n\thelp\t\t\tSpecify ussage (commands)";
                len = text_parent.length() + 1;

                if (-1 == write(pipe_fd_child_parent[1], &len, sizeof(int)))
                    handle_error("Error at write", 5);
                if (-1 == write(pipe_fd_child_parent[1], text_parent.c_str(), len))
                    handle_error("Error at write", 5);
                close(pipe_fd_child_parent[1]);
                text_parent.clear();

                return 0;    
            default:
                close(pipe_fd_child_parent[1]);
                if (-1 == read(pipe_fd_child_parent[0], &len, sizeof(int)))
                    handle_error("Error at read", 5);
                msg_back = new char[len + 1];
                if (-1 == read(pipe_fd_child_parent[0], msg_back, len))
                    handle_error("Error at read", 5);
                msg_back_str = msg_back;
                delete msg_back;
                close(pipe_fd_child_parent[0]);
            }
        }
        else if (arr_size == 3 && cmdlets[0] == "login" && cmdlets[1] == ":") {     // login : <username>
            if (-1 == socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fd))
                handle_error("Error at socketpair", 1);
            pid_child = fork();

            switch (pid_child)
            {
            case -1:
                handle_error("Error at fork", 2);
            case 0:
                close(socket_fd[0]);
                if (-1 == read(socket_fd[1], &len, sizeof(int)))
                    handle_error("Error at read", 7);
                username = new char[len + 1];
                if (-1 == read(socket_fd[1], username, len))
                    handle_error("Error at read", 7);
                
                if (!user.isLogged) {
                    user.loginUser(username);
                    if (user.isLogged) {
                        text_parent = "Sucessfully logged in as user: ";
                        text_parent += user.GetUsername();
                    }
                    else 
                        text_parent = "Login failed. Unknown user";
                }
                else
                    text_parent = "A user is already logged in"; // TODO add more users

                isLoggedIn = user.isLogged;

                len = text_parent.length() + 1;
                if (-1 == write(socket_fd[1], &len, sizeof(len)))
                    handle_error("Error at write", 7);
                if (-1 == write(socket_fd[1], text_parent.c_str(), len))
                    handle_error("Error at write", 7);
                if (-1 == write(socket_fd[1], &isLoggedIn, sizeof(bool)))
                    handle_error("Error at write", 7);
                if (isLoggedIn) {
                    len = strlen(username) + 1;
                    if (-1 == write(socket_fd[1], &len, sizeof(len)))
                        handle_error("Error at write", 7);
                    if (-1 == write(socket_fd[1], username, len))
                        handle_error("Error at write", 7);
                }
                delete username;
                close(socket_fd[1]);
                text_parent.clear();
                return 0;
            default:
                close(socket_fd[1]);
                len = cmdlets[2].length() + 1;
                if (-1 == write(socket_fd[0], &len, sizeof(len)))
                    handle_error("Error at write", 6);
                if (-1 == write(socket_fd[0], cmdlets[2].c_str(), len))
                    handle_error("Error at write", 6);

                if (-1 == read(socket_fd[0], &len, sizeof(int)))
                    handle_error("Error at read", 6);
                msg_back = new char[len + 1];
                if (-1 == read(socket_fd[0], msg_back, len))
                    handle_error("Error at read", 6);
                if (-1 == read(socket_fd[0], &isLoggedIn, sizeof(bool)))
                    handle_error("Error at read", 6);
                user.isLogged = isLoggedIn;
                if (isLoggedIn) {
                    if (-1 == read(socket_fd[0], &len, sizeof(int)))
                        handle_error("Error at read", 6);
                    username = new char[len + 1];
                    if (-1 == read(socket_fd[0], username, len))
                        handle_error("Error at read", 6);
                    user.SetUsername(username);
                    delete username;
                }
                msg_back_str = msg_back;
                delete msg_back;
                close(socket_fd[0]);
                break;
            }
        }
        else if (arr_size == 1 && cmdlets[0] == "get-logged-users") {       // get-logged-users
            if (-1 == pipe(pipe_fd_child_parent))
                handle_error("Error at pipe", 1);
            pid_child = fork();
            switch (pid_child) 
            {
            case -1:
                handle_error("Error at fork", 1);
            case 0:
                close(pipe_fd_child_parent[0]);

                if (user.isLogged)
                    text_parent = user.GetUsername();
                else 
                    text_parent = "Can't run this command : no user logged in";

                len = text_parent.length() + 1;
                if (-1 == write(pipe_fd_child_parent[1], &len, sizeof(int)))
                    handle_error("Error at write", 5);
                if (-1 == write(pipe_fd_child_parent[1], text_parent.c_str(), len))
                    handle_error("Error at write", 5);

                close(pipe_fd_child_parent[1]);
                text_parent.clear();
                return 0;    
            default:
                close(pipe_fd_child_parent[1]);

                if (-1 == read(pipe_fd_child_parent[0], &len, sizeof(int)))
                    handle_error("Error at read", 5);
                msg_back = new char[len + 1];
                if (-1 == read(pipe_fd_child_parent[0], msg_back, len))
                    handle_error("Error at read", 5);
                    
                msg_back_str = msg_back;
                delete msg_back;
                close(pipe_fd_child_parent[0]);
            }
        }
        else if (arr_size == 3 && cmdlets[0] == "get-proc-info" && cmdlets[1] == ":") {     // get-proc-info
            if (-1 == socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fd))
                handle_error("Error at socketpair", 1);
            pid_child = fork();

            switch (pid_child)
            {
            case -1:
                handle_error("Error at fork", 2);
            case 0:
                close(socket_fd[0]);
                if (-1 == read(socket_fd[1], &len, sizeof(int)))
                    handle_error("Error at read", 7);
                pid_cstr = new char[len + 1];
                if (-1 == read(socket_fd[1], pid_cstr, len))
                    handle_error("Error at read", 7);
                
                if (user.isLogged)
                    text_parent = getProcessInformation(pid_cstr);
                else 
                    text_parent = "Can't run this command : no user logged in";
                delete pid_cstr;
                    
                len = text_parent.length() + 1;
                if (-1 == write(socket_fd[1], &len, sizeof(len)))
                    handle_error("Error at write", 7);
                if (-1 == write(socket_fd[1], text_parent.c_str(), len))
                    handle_error("Error at write", 7);
                close(socket_fd[1]);
                text_parent.clear();
                return 0;
            default:
                close(socket_fd[1]);
                len = cmdlets[2].length() + 1;
                if (-1 == write(socket_fd[0], &len, sizeof(len)))
                    handle_error("Error at write", 6);
                if (-1 == write(socket_fd[0], cmdlets[2].c_str(), len))
                    handle_error("Error at write", 6);

                if (-1 == read(socket_fd[0], &len, sizeof(int)))
                    handle_error("Error at read", 6);
                msg_back = new char[len + 1];
                if (-1 == read(socket_fd[0], msg_back, len))
                    handle_error("Error at read", 6);
                msg_back_str = msg_back;
                delete msg_back;
                close(socket_fd[0]);
                break;
            }
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
        else {
            msg_back_str = "Unknown command '" + command_str + "'. Specify 'help' for usage";
        }

        len = msg_back_str.length() + 1;
        if (-1 == write(fd_write, &len, sizeof(int)))
            handle_error("Error at write", 1);
        if (-1 == write(fd_write, msg_back_str.c_str(), len))
            handle_error("Error at write", 2);
    }
    return 0;
}