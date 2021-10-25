#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <utmp.h>

#include <vector>
#include <string>
#include "../include/UserProfile.h"
#include "../include/Command.h"

int glob_write_err_count = 3;

void write_len_str(int fd, int len, std::string out_txt) {
    glob_write_err_count++;
    len = out_txt.length() + 1;
    if (-1 == write(fd, &len, sizeof(int)))
        handle_error("Error at write", glob_write_err_count);
    glob_write_err_count++;
    if (-1 == write(fd, out_txt.c_str(), len))
        handle_error("Error at write", glob_write_err_count);
}

Command::~Command() {
    msg_back_str.clear();
}

std::string Command::GetMsgBack() const {
    return this->msg_back_str;
}

void HelpCommand::Execute() {
    pid_t pid_child;
    int pipe_fd_child_parent[2], len = 0;
    std::string text_parent;
    char* msg_back = nullptr;
    if (-1 == pipe(pipe_fd_child_parent))
        handle_error("Error at pipe", 1);
    pid_child = fork();
    switch (pid_child) 
    {
    case -1:
        handle_error("Error at fork", 1);
    case 0:
        close(pipe_fd_child_parent[0]);

        text_parent = "Log in/out:\n\tlogin : <username>\tLogin as <username> in the current session\n\tlogout\t\t\tLogout from current session (requires valid login)\n\tget-current-client\tGet the username of the current logged client to the server (requires valid login)\nGet information:\n\tget-logged-user\t\tGet informaton about the logged user (requires valid login)\n\tget-proc-info : <pid>\tGet information about the process with pid <pid> (requires valid login)\nMiscellaneous:\n\tquit\t\t\tQuit the current session\n\thelp\t\t\tSpecify ussage (commands)";
        
        write_len_str(pipe_fd_child_parent[1], len, text_parent);
        
        close(pipe_fd_child_parent[1]);
        text_parent.clear();

        exit(0);    
    default:
        close(pipe_fd_child_parent[1]);
        if (-1 == read(pipe_fd_child_parent[0], &len, sizeof(int)))
            handle_error("Error at read", 3);
        msg_back = new char[len + 1];
        if (-1 == read(pipe_fd_child_parent[0], msg_back, len))
            handle_error("Error at read", 4);
        this->msg_back_str = msg_back;
        delete msg_back;
        close(pipe_fd_child_parent[0]);
    }
}

LoginCommand::LoginCommand(UserProfile user, std::string usern) {
    this->user = user;
    this->username_login = usern;
}

LoginCommand::~LoginCommand() {
    this->msg_back_str.clear();
    this->username_login.clear();
}

void LoginCommand::Execute() {
    pid_t pid_child;
    int socket_fd[2], len = 0;
    std::string text_parent;
    char *msg_back = nullptr, *username = nullptr;
    bool isLoggedIn, wasSwapped = true;

    if (-1 == socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fd))
        handle_error("Error at socketpair", 1);
    pid_child = fork();

    switch (pid_child)
    {
    case -1:
        handle_error("Error at fork", 2);
    case 0:
        close(socket_fd[0]);
        isLoggedIn = this->user.isLogged;

        if (-1 == read(socket_fd[1], &len, sizeof(int)))
            handle_error("Error at read", 5);
        username = new char[len + 1];
        if (-1 == read(socket_fd[1], username, len))
            handle_error("Error at read", 6);
        
        if (!this->user.isLogged) {
            this->user.loginUser(username);
            if (this->user.isLogged) {
                text_parent = "Sucessfully logged in as user: ";
                text_parent += username;
            }
            else 
                text_parent = "Login failed. Unknown user";
        }
        else
            text_parent = "A user is already logged in"; // TODO add more users

        if (isLoggedIn == this->user.isLogged)
            wasSwapped = false;
        isLoggedIn = this->user.isLogged;

        write_len_str(socket_fd[1], len, text_parent);

        if (-1 == write(socket_fd[1], &isLoggedIn, sizeof(bool)))
            handle_error("Error at write", 1);
        if (wasSwapped) {
            len = strlen(username) + 1;
            if (-1 == write(socket_fd[1], &len, sizeof(len)))
                handle_error("Error at write", 2);
            if (-1 == write(socket_fd[1], username, len))
                handle_error("Error at write", 3);
        }
        delete username;
        close(socket_fd[1]);
        text_parent.clear();
        exit(0);
    default:
        close(socket_fd[1]);
        isLoggedIn = this->user.isLogged;

        write_len_str(socket_fd[0], len, this->username_login);

        if (-1 == read(socket_fd[0], &len, sizeof(int)))
            handle_error("Error at read", 7);
        msg_back = new char[len + 1];
        if (-1 == read(socket_fd[0], msg_back, len))
            handle_error("Error at read", 8);
        if (-1 == read(socket_fd[0], &isLoggedIn, sizeof(bool)))
            handle_error("Error at read", 9);

        if (isLoggedIn == user.isLogged)
            wasSwapped = false;
        this->user.isLogged = isLoggedIn;
        if (wasSwapped) {
            if (-1 == read(socket_fd[0], &len, sizeof(int)))
                handle_error("Error at read", 10);
            username = new char[len + 1];
            if (-1 == read(socket_fd[0], username, len))
                handle_error("Error at read", 11);
            this->user.setUsername(username);
            delete username;
        }
        this->msg_back_str = msg_back;
        delete msg_back;
        close(socket_fd[0]);
    }
}

GetLoggedUsersCommand::GetLoggedUsersCommand(UserProfile user) {
    this->user = user;
}

void GetLoggedUsersCommand::Execute() {
    pid_t pid_child;
    int pipe_fd_child_parent[2], len = 0;
    std::string text_parent;
    struct utmp *entry;
    struct tm *login_time;
    time_t timestamp;
    char* msg_back = nullptr, buffer[100];
    if (-1 == pipe(pipe_fd_child_parent))
        handle_error("Error at pipe", 2);
    pid_child = fork();
    switch (pid_child) 
    {
    case -1:
        handle_error("Error at fork", 3);
    case 0:
        close(pipe_fd_child_parent[0]);
        if (this->user.isLogged) {
            int count_users = 0;
            text_parent = "Users logged onto the operating system:\n";
            setutent();

            while ((entry = getutent()) != NULL) {
                count_users++;
                timestamp = entry->ut_tv.tv_sec;
                login_time = localtime(&timestamp);

                strftime(buffer, sizeof(buffer), "%A %d-%m-%Y %H:%M:%S %Z", login_time);

                text_parent += "\nUsername: ";
                text_parent += entry->ut_user;
                text_parent += "\nHostname for remote login: ";
                text_parent += entry->ut_host;
                text_parent += "\nTime entry was made: ";
                text_parent += buffer;
                text_parent += "\n";
            }

            endutent();
            text_parent += "\nNumber of logged users: ";
            text_parent += std::to_string(count_users);
        }
        else {
            text_parent = "Can't run this command : no user logged in";
        }

        write_len_str(pipe_fd_child_parent[1], len, text_parent);

        close(pipe_fd_child_parent[1]);
        text_parent.clear();
        exit(0);    
    default:
        close(pipe_fd_child_parent[1]);

        if (-1 == read(pipe_fd_child_parent[0], &len, sizeof(int)))
            handle_error("Error at read", 12);
        msg_back = new char[len + 1];
        if (-1 == read(pipe_fd_child_parent[0], msg_back, len))
            handle_error("Error at read", 13);
            
        this->msg_back_str = msg_back;
        delete msg_back;
        close(pipe_fd_child_parent[0]);
    }
}

GetProcInfoCommand::GetProcInfoCommand(UserProfile user, std::string pid_str) {
    this->pid_str = pid_str;
    this->user = user;
}

GetProcInfoCommand::~GetProcInfoCommand() {
    this->msg_back_str.clear();
    this->pid_str.clear();
}

void GetProcInfoCommand::Execute() {
    pid_t pid_child;
    int socket_fd[2], len = 0;
    std::string text_parent;
    char *msg_back = nullptr, *pid_cstr = nullptr;
    if (-1 == socketpair(AF_UNIX, SOCK_STREAM, 0, socket_fd))
        handle_error("Error at socketpair", 2);
    pid_child = fork();

    switch (pid_child)
    {
    case -1:
        handle_error("Error at fork", 4);
    case 0:
        close(socket_fd[0]);
        if (-1 == read(socket_fd[1], &len, sizeof(int)))
            handle_error("Error at read", 14);
        pid_cstr = new char[len + 1];
        if (-1 == read(socket_fd[1], pid_cstr, len))
            handle_error("Error at read", 15);
        
        if (user.isLogged)
            text_parent = getProcessInformation(pid_cstr);
        else 
            text_parent = "Can't run this command : no user logged in";
        delete pid_cstr;
            
        write_len_str(socket_fd[1], len, text_parent);

        close(socket_fd[1]);
        text_parent.clear();
        exit(0);
    default:
        close(socket_fd[1]);
        
        write_len_str(socket_fd[0], len, pid_str);

        if (-1 == read(socket_fd[0], &len, sizeof(int)))
            handle_error("Error at read", 16);
        msg_back = new char[len + 1];
        if (-1 == read(socket_fd[0], msg_back, len))
            handle_error("Error at read", 17);
        msg_back_str = msg_back;
        delete msg_back;
        close(socket_fd[0]);
    }
}

GetCurrentLoggedClientCommand::GetCurrentLoggedClientCommand(UserProfile user) {
    this->user = user;
}

void GetCurrentLoggedClientCommand::Execute() {
    pid_t pid_child;
    int pipe_fd_child_parent[2], len = 0;
    std::string text_parent;
    char* msg_back = nullptr;
    if (-1 == pipe(pipe_fd_child_parent))
        handle_error("Error at pipe", 3);
    pid_child = fork();
    switch (pid_child) 
    {
    case -1:
        handle_error("Error at fork", 5);
    case 0:
        close(pipe_fd_child_parent[0]);

        if (user.isLogged) {
            text_parent = "Current logged client username: " + user.getUsername();
        }
        else {
            text_parent = "No client is currently logged in to the server";
        }

        write_len_str(pipe_fd_child_parent[1], len, text_parent);
        
        close(pipe_fd_child_parent[1]);
        text_parent.clear();

        exit(0);    
    default:
        close(pipe_fd_child_parent[1]);
        if (-1 == read(pipe_fd_child_parent[0], &len, sizeof(int)))
            handle_error("Error at read", 18);
        msg_back = new char[len + 1];
        if (-1 == read(pipe_fd_child_parent[0], msg_back, len))
            handle_error("Error at read", 19);
        this->msg_back_str = msg_back;
        delete msg_back;
        close(pipe_fd_child_parent[0]);
    }
}