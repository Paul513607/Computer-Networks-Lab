#pragma once

void trim_spaces_end_start(std::string &str) {
    int index = str.find_first_not_of(" \n");
    str.substr(index);
    std::string copy = str;
    reverse(copy.begin(), copy.end());
    index = copy.find_first_not_of(" \n");
    copy.substr(index);
    reverse(copy.begin(), copy.end());
    str = copy;
    copy.clear();
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
}

std::vector<std::string> getUserTableLine(sqlite3 *user_db, std::string username) {
    std::vector<std::string> table_line;
    sqlite3_stmt *stmt;
    std::string sel_query = "SELECT * FROM user_perm WHERE username = '" + username + "';";
    sqlite3_prepare_v2(user_db, sel_query.c_str(), -1, &stmt, 0);
    const unsigned char *tmp = NULL;
    std::string tmp_str;
    
    sqlite3_step(stmt);

    tmp = sqlite3_column_text(stmt, 0);
    if (tmp == NULL) {
        sqlite3_finalize(stmt);
        return table_line;
    }
    tmp_str = (char *) tmp;
    table_line.push_back(tmp_str);
    tmp = sqlite3_column_text(stmt, 1);
    tmp_str = (char *) tmp;
    table_line.push_back(tmp_str);
    tmp_str.clear();
    sqlite3_finalize(stmt);
    return table_line;
}

// abstract class from which specific commands classes are derived
class Command {     
protected:
    std::vector<std::string> argv;
    std::string reply;
    UserProfile user;
public:
    Command() {
        argv.reserve(5);
    };
    virtual ~Command() {
        for (auto i : argv)
            i.clear();
        argv.clear();
        reply.clear();
    };
    void setArguments(std::vector<std::string> argv, UserProfile user2) {
        for (auto arg : argv) {
            this->argv.push_back(arg);
            std::cout << arg << " ";
        }
        std::cout << std::endl;
        this->user = user2;
    };
    std::string GetReply() const {
        return reply;
    }
    UserProfile GetUser() {
        return user;
    }
    virtual void Exec() = 0;
};

// Syntax: register <username> --- Lets a client register to the server and gives them only the read permission
class RegisterCommand : public Command {
private:
    sqlite3 *user_db = nullptr;
public:
    void setUserDBptr(sqlite3 *db) {
        user_db = db;
    }
    void Exec() override {  // register <username>
        if (user.isLogged()) {
            reply = "A user is already logged in. You can not register now!";
            return;
        }

        std::vector<std::string> table_line;
        std::string insert_query;
        char *err_msg = const_cast<char *> ("Error at inserting into database");
        sqlite3_open("./server/database/users.db", &user_db);      // change paths later

        table_line = getUserTableLine(user_db, argv[1]);
        if (!table_line.empty()) {
            reply = "User " + argv[1] + " already registered!";
            sqlite3_close(user_db);
            return;
        }

        insert_query = "INSERT INTO user_perm VALUES('" + argv[1] + "', 'r--');";   // add user to table with only read (clone) permission
        int ret = sqlite3_exec(user_db, insert_query.c_str(), NULL, NULL, &err_msg);
        if (ret != SQLITE_OK) {
            std::cerr << "Error: " << err_msg;
            sqlite3_close(user_db);
            return;
        }
    
        reply = "Succesfully registered user: \"" + argv[1] + "\"!";
        for (auto i : table_line)
            i.clear();
        table_line.clear(); 
        sqlite3_close(user_db);
    };
};

// Syntax: login <username> --- Lets a client log in to the server with a registered username - needed in order to execute commands on the repository
class LoginCommand : public Command {
private:
    sqlite3 *user_db = nullptr;
public:
    void setUserDBptr(sqlite3 *db) {
        user_db = db;
    }
    void Exec() override {
        if (user.isLogged()) {
            reply = "A user is already logged in!";
            return;
        }
        std::vector<std::string> table_line;

        sqlite3_open("./server/database/users.db", &user_db);   // change paths later

        table_line = getUserTableLine(user_db, argv[1]);
        if (table_line.empty()) {
            reply = "User " + argv[1] + " is not registered!";
            sqlite3_close(user_db);
            return;
        }

        user.loginUser(table_line[0].c_str());
        user.setPermissions(table_line[1]);
        for (auto i : table_line)
            i.clear();
        table_line.clear();
        sqlite3_close(user_db);
        reply = "Succesfully logged in as: \"" + table_line[0] + "\"!";
    };
};

// Syntax: logout   --- Lets a client log out of the current session (only works if logged in)
class LogoutCommand : public Command {
public:
    void Exec() {
        if (user.isLogged()) {
            reply = "Successfully logged out user " + user.getUsername() + "!";
            user.logoutUser();
        }
        else {
            reply = "No user logged in.";
        }
    }
};

// Syntax: update_perm <username> <+/-><r|w|a>  --- Lets a client with administrator permissions update the permissions for other users (only works if logged in)
class UpdatePermCommand : public Command {
private:
    sqlite3 *user_db = nullptr;
public:
    void setUserDBptr(sqlite3 *db) {
        user_db = db;
    }
    void Exec() override {
        if (user.isLogged()) {
            std::vector<std::string> table_line;
            std::string perm_to_add = "";

            sqlite3_open("./server/database/users.db", &user_db);

            if (user.getPermissions().find('a') != std::string::npos) {
                table_line = getUserTableLine(user_db, argv[1]);
                if (table_line.empty()) {
                    sqlite3_close(user_db);
                    for (auto i : table_line)
                        i.clear();
                    table_line.clear();
                    reply = "User " + argv[1] + " is not registered.";
                    return;
                }

                if (argv[2].find('r') != std::string::npos) 
                    perm_to_add += 'r';
                else
                    perm_to_add += '-';
                if (argv[2].find('w') != std::string::npos)
                    perm_to_add += 'w';
                else 
                    perm_to_add += '-';
                if (argv[2].find('a') != std::string::npos) {
                    if (argv[2][0] == '+') perm_to_add = "rwa";
                    else if (argv[2][0] == '-') perm_to_add += 'a';
                }
                else
                    perm_to_add+='-';

                if (argv[2][0] == '+') {
                    for (long unsigned int i = 0; i < perm_to_add.size(); ++i)
                        if (perm_to_add[i] == '-') perm_to_add[i] = table_line[1][i];
                }
                else if (argv[2][0] == '-') {
                    if ((perm_to_add[0] != '-' || perm_to_add[1] != '-') && table_line[1][2] == 'a')    // if user has admin perm and read/write perm gets removed, it also removes admin perm
                        perm_to_add[2] = 'a';
                    for (long unsigned int i = 0; i < perm_to_add.size(); ++i)
                        if (perm_to_add[i] == table_line[1][i]) table_line[1][i] = '-';
                    perm_to_add = table_line[1];
                }
                else {
                    reply = "Wrong format for this command. Syntax is: update_perm <username> <+/-><r|w|a>";
                    sqlite3_close(user_db);
                    for (auto i : table_line)
                        i.clear();
                    table_line.clear();
                    perm_to_add.clear();
                    return;
                }

                std::string update_query = "UPDATE user_perm SET permissions = '" + perm_to_add + "' WHERE username = '" + argv[1] + "';";
                char *err_msg = const_cast<char *> ("Error at updating the database");
                
                int ret = sqlite3_exec(user_db, update_query.c_str(), NULL, NULL, &err_msg);
                if (ret != SQLITE_OK) {
                    std::cerr << "Error: " << err_msg;
                }

                reply = "Successfully updated permissions for user " + argv[1] + "!";

                sqlite3_close(user_db);
                for (auto i : table_line)
                    i.clear();
                table_line.clear();
                perm_to_add.clear();
            }
            else {
                reply = "Only admins can update others' permissions!";
            }
        }
        else {
            reply = "No user logged in.";
        }
    }
};

// Syntax: info_perm    // Lets a client see their permissions for the database (read, write or administrator) (only works if logged in)
class InfoPermCommand : public Command {
public:
    void Exec() override {
        if (user.isLogged()) {
            int ok = 0;
            std::string perm = user.getPermissions();
            std::cout << "HAHA "; user.userPrint();
            reply = "Permissions for current user: ";
            if (perm.find('r') != std::string::npos) {
                ok = 1;
                reply += " r (read/clone permission),";
            }
            if (perm.find('w') != std::string::npos) {
                ok = 1;
                reply += " w (write/commit(push) permission),";
            }
            if (perm.find('a') != std::string::npos) {
                ok = 1;
                reply += " a (admin - update others' permissions, revert permission)";
            }
            perm.clear();

            if (!ok) 
                reply += "no permissions";
            else
                if (reply[reply.length()] == ',')
                    reply[reply.length()] = '\0';
        }
        else {
            reply = "No user logged in.";
        }
    }
};

class DeleteUserCommand : public Command {  // TODO --- must be admin
private:
    sqlite3 *user_db;
public:
    void setUserDBptr(sqlite3 *db) {
        user_db = db;
    }
    void Exec() override {
    
    }
};