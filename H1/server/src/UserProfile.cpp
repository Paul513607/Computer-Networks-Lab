#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <vector>
#include <string>
#include "../include/UserProfile.h"

std::vector<std::string> knownUsers;

void getKnownUsers() {
    knownUsers.clear();
    knownUsers.reserve(5);
    std::ifstream fin("../login_users.txt");
    std::string usern;
    while (std::getline(fin, usern))
        knownUsers.push_back(usern);
    fin.close();
}

bool isKnownUser(const char* username) {
    for (auto user : knownUsers)
        if (user == username) {
            return true;
        }
    return false;
}

std::string getProcessInformation(const char* pid_cstr) {
    std::string proc_status_file_path, msg_back, line_in_file, info, pid_str(pid_cstr);
    int fdr, index;
    proc_status_file_path = "/proc/" + pid_str + "/status";
    std::ifstream fin(proc_status_file_path);
    if (fin.fail()) {
        msg_back = "No status file for process pid " + pid_str;
        return msg_back;
    }
    msg_back = "Information about the process with pid " + pid_str + ":";
    while (std::getline(fin, line_in_file)) {
        if (line_in_file.find("Name:") == 0 || line_in_file.find("State:") == 0 || line_in_file.find("PPid:") == 0 || line_in_file.find("Uid:") == 0 || line_in_file.find("VmSize:") == 0) {
            msg_back += "\n";
            msg_back += line_in_file;
        }
    }
    fin.close();
    return msg_back;
}

UserProfile::UserProfile() {}

UserProfile::UserProfile(std::string username) {
    if (isKnownUser(username.c_str())) {
        this->username = username;
        this->isLogged = true;
    }
}

UserProfile::~UserProfile() {
    this->username.clear();
}

void UserProfile::loginUser(const char* username) {
    if (isKnownUser(username)) {
        this->username = username;
        this->isLogged = true;
    }
}

void UserProfile::setUsername(const char* username) {
    this->username = username;
}

std::string UserProfile::getUsername() const {
    return this->username;
}

void UserProfile::logoutUser() {
    this->username.clear();
    this->isLogged = false;
}

UserProfile& UserProfile::operator= (UserProfile user2) {
    this->username = user2.username;
    this->isLogged = user2.isLogged;
    return (*this);
}

