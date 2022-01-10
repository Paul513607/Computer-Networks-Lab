#pragma once

std::mutex mtx;

std::vector<std::string> getUserTableLine(sqlite3 *user_db, std::string username) {
    std::vector<std::string> table_line;
    sqlite3_stmt *stmt;
    std::string select_query = "SELECT * FROM user_perm WHERE username = '" + username + "';";
    int ret = sqlite3_prepare_v2(user_db, select_query.c_str(), -1, &stmt, 0);
    if (ret)
        std::cerr << "Error at sqlite3_prepare_v2: " << sqlite3_errmsg(user_db) << std::endl;
    select_query.clear();
    const unsigned char *tmp = NULL;
    std::string tmp_str;
    
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW)
        std::cout << "No entries for this query" << std::endl;

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

// abstract class from which user specific commands classes are derived (like register, login, logout, update_perm, info_perm, remove)
class UserCommand : public Command {
protected:
    sqlite3 *user_db = nullptr;
public:
    void setUserDBptr(sqlite3 *db) {
        this->user_db = db;
    }
    virtual void Exec() = 0;
};

// Syntax: register <username> --- Lets a client register to the server and gives them only the read permission
class RegisterCommand : public UserCommand {
public:
    void Exec() override {  // register <username>
        if (user.isLogged()) {
            reply = "A user is already logged in. You can not register now!";
            return;
        }

        mtx.lock();
        std::vector<std::string> table_line;
        std::string insert_query;
        char *err_msg = const_cast<char *> ("Error at inserting into database");
        sqlite3_open("./database/users.db", &user_db);      // change paths later

        table_line = getUserTableLine(user_db, argv[1]);
        if (!table_line.empty()) {
            reply = "User " + argv[1] + " already registered!";
            sqlite3_close(user_db);
            free_string_vector_memory(table_line);
            mtx.unlock();
            return;
        }

        insert_query = "INSERT INTO user_perm VALUES('" + argv[1] + "', 'r--');";   // add user to table with only read (clone) permission
        int ret = sqlite3_exec(user_db, insert_query.c_str(), NULL, NULL, &err_msg);
        insert_query.clear();
        if (ret != SQLITE_OK) {
            std::cerr << "Error: " << err_msg;
            sqlite3_close(user_db);
            free_string_vector_memory(table_line);
            mtx.unlock();
            return;
        }
    
        reply = "Succesfully registered user: \"" + argv[1] + "\"!";
        sqlite3_close(user_db);
        free_string_vector_memory(table_line);
        mtx.unlock();
    };
};

// Syntax: login <username> --- Lets a client log in to the server with a registered username - needed in order to execute commands on the repository
class LoginCommand : public UserCommand {
public:
    void Exec() override {
        if (user.isLogged()) {
            reply = "A user is already logged in!";
            return;
        }
        mtx.lock();
        std::vector<std::string> table_line;

        sqlite3_open("./database/users.db", &user_db);   // change paths later

        table_line = getUserTableLine(user_db, argv[1]);
        if (table_line.empty()) {
            reply = "User " + argv[1] + " is not registered!";
            sqlite3_close(user_db);
            return;
        }

        user.loginUser(table_line[0].c_str());
        user.setPermissions(table_line[1]);
        free_string_vector_memory(table_line);
        sqlite3_close(user_db);
        reply = "Succesfully logged in as: \"" + table_line[0] + "\"!";
        mtx.unlock();
    };
};

// Syntax: logout   --- Lets a client log out of the current session (only works if logged in)
class LogoutCommand : public UserCommand {
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
class UpdatePermCommand : public UserCommand {
public:
    void Exec() override {
        mtx.lock();
        if (user.isLogged()) {
            std::vector<std::string> table_line;
            std::string perm_to_add = "";

            sqlite3_open("./database/users.db", &user_db);

            if (user.getPermissions().find('a') != std::string::npos) {
                table_line = getUserTableLine(user_db, argv[1]);
                if (table_line.empty()) {
                    sqlite3_close(user_db);
                    free_string_vector_memory(table_line);
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
                    free_string_vector_memory(table_line);
                    perm_to_add.clear();
                    return;
                }

                std::string update_query = "UPDATE user_perm SET permissions = '" + perm_to_add + "' WHERE username = '" + argv[1] + "';";
                char *err_msg = const_cast<char *> ("Error at updating the database");
                
                int ret = sqlite3_exec(user_db, update_query.c_str(), NULL, NULL, &err_msg);
                update_query.clear();
                if (ret != SQLITE_OK) {
                    reply = err_msg;
                    sqlite3_close(user_db);
                    free_string_vector_memory(table_line);
                    perm_to_add.clear();
                    return;
                }

                reply = "Successfully updated permissions for user " + argv[1] + "!";

                sqlite3_close(user_db);
                free_string_vector_memory(table_line);
                perm_to_add.clear();
            }
            else {
                reply = "Only administrators can update others' permissions!";
            }
        }
        else {
            reply = "No user logged in.";
        }
        mtx.unlock();
    }
};

// Syntax: info_perm    // Lets a client see their permissions for the database (read, write or administrator) (only works if logged in)
class InfoPermCommand : public UserCommand {
public:
    void Exec() override {
        mtx.lock();
        if (user.isLogged()) {
            int ok = 0;
            std::string perm = user.getPermissions();
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
        mtx.unlock();
    }
};

// Syntax: remove <username> // Lets a client with administrator permissions remove other users from the users database
class RemoveUserCommand : public UserCommand {
public:
    void Exec() override {
        mtx.lock();
        if (user.isLogged()) {
            if (user.getPermissions().find('a') != std::string::npos) {
                sqlite3_open("./database/users.db", &user_db);
                std::string delete_query = "DELETE FROM user_perm WHERE username = '" + argv[1] + "';";
                char *err_msg = const_cast<char *> ("Error at updating the database");

                int ret = sqlite3_exec(user_db, delete_query.c_str(), NULL, NULL, &err_msg);
                delete_query.clear();
                if (ret != SQLITE_OK) {
                    reply = err_msg;
                    sqlite3_close(user_db);
                    return;
                }

                reply = "Successfully removed user " + argv[1] + "!";
                sqlite3_close(user_db);
            }
            else {
                reply = "Only administrators can remove other users from the database!";
            }
        }
        else {
            reply = "No user logged in.";
        }
        mtx.unlock();
    }
};

// abstract class from which repo specific commands classes are derived (like clone, commit, revert) // TODO : classes
class RepoCommand : public Command {
protected:
    sqlite3 *repo_db = nullptr;
    int client;
public:
    void setClient(int client) {
        this->client = client;
    }
    void setRepoDBptr(sqlite3 *db) {
        this->repo_db = db;
    }
    int getLatestVersion() {
        mtx.lock();
        sqlite3_open("./database/file_system.db", &repo_db);

        std::string query = "SELECT version FROM repo WHERE version = (SELECT MAX(version) FROM repo);";
        sqlite3_stmt *stmt;

        int ret = sqlite3_prepare_v2(repo_db, query.c_str(), query.length() + 1, &stmt, NULL);
        if (ret)
            std::cerr << "Error at sqlite3_prepare_v2: " << sqlite3_errmsg(repo_db) << std::endl;
        
        sqlite3_step(stmt);
        int version = sqlite3_column_int(stmt, 0);

        sqlite3_finalize(stmt);
        sqlite3_close(repo_db);
        mtx.unlock();
        return version;
    }
    std::string getVersionHash(int version) {
        mtx.lock();
        sqlite3_open("./database/file_system.db", &repo_db);

        std::string query = "SELECT hash FROM repo WHERE version = " + std::to_string(version) + ';';
        sqlite3_stmt *stmt;

        std::cout << query << std::endl;

        int ret = sqlite3_prepare_v2(repo_db, query.c_str(), -1, &stmt, NULL);
        if (ret)
            std::cerr << "[1] Error at sqlite3_prepare_v2: " << sqlite3_errmsg(repo_db);
        
        sqlite3_step(stmt);
        std::string hash;
        int tmp_size = sqlite3_column_bytes(stmt, 0);
        const unsigned char* tmp = sqlite3_column_text(stmt, 0);
        hash.assign(tmp, tmp + tmp_size);

        sqlite3_finalize(stmt);
        sqlite3_close(repo_db);
        mtx.unlock();
        return hash;
    }
    std::string getVersionFS(int version) {
        mtx.lock();
        sqlite3_open("./database/file_system.db", &repo_db);

        std::string query = "SELECT filesystem FROM repo WHERE version = " + std::to_string(version) + ';';
        sqlite3_stmt *stmt;

        int ret = sqlite3_prepare_v2(repo_db, query.c_str(), -1, &stmt, NULL);
        if (ret)
            std::cerr << "[2] Error at sqlite3_prepare_v2: " << sqlite3_errmsg(repo_db) << std::endl;
        
        sqlite3_step(stmt);
        std::string fs;
        int tmp_size = sqlite3_column_bytes(stmt, 0);
        const unsigned char* tmp = (unsigned char*) sqlite3_column_blob(stmt, 0);
        fs.assign(tmp, tmp + tmp_size);

        sqlite3_finalize(stmt);
        sqlite3_close(repo_db);
        mtx.unlock();
        return fs;
    }
    virtual void Exec() = 0;
    friend class Server;
};

// Syntax: init
class InitCommand : public RepoCommand {
public:
    void Exec() override {
        int version = 0;
        char *errmsg;
        std::string hash = receive_data(client);
        std::string files_pack = receive_data(client);

        mtx.lock();
        sqlite3_open("./database/file_system.db", &repo_db);
        sqlite3_stmt *stmt;
        std::string query = "INSERT INTO repo VALUES(?, ?, ?);";
        std::cout << query << std::endl;

        sqlite3_prepare_v2(repo_db, query.c_str(), -1, &stmt, NULL);

        sqlite3_bind_int(stmt, 1, version);
        sqlite3_bind_text(stmt, 2, hash.c_str(), hash.size(), SQLITE_STATIC);
        sqlite3_bind_blob(stmt, 3, &(*files_pack.begin()), files_pack.size(), SQLITE_STATIC);

        sqlite3_step(stmt);

        sqlite3_finalize(stmt);
        sqlite3_close(repo_db);
        mtx.unlock();
    }
};

// Syntax: clone <repo_version> // Clones the repo's <repo_version> to the client's local storage
class CloneCommand : public RepoCommand {
public:
    void Exec() override {
        std::cout << "HERE6 " << std::endl;
        if (user.isLogged()) {
            if (user.getPermissions().find('r') != std::string::npos || user.getPermissions().find('a') != std::string::npos) {
                int version = atoi(argv[2].c_str());
                std::cout << "AAA " << version << std::endl;
                std::string hash = getVersionHash(version);
                std::string files = getVersionFS(version);

                send_data(files, client);
            }
            else
                reply = "Could not execute command. You need at least read permissions to execute this command";
        }
        else 
            reply = "No user logged in";
    };
};

// Syntax: commit -m <message> // Lets a client commit their local changes to to repository to the remore repository, also adds a commit message
class CommitCommand : public RepoCommand {
public:
    void update_tables(int version, std::string hash, std::string patch, std::string old_files) {
        mtx.lock();
        sqlite3_open("./database/file_system.db", &repo_db);

        std::string query = "INSERT INTO commits VALUES(" + std::to_string(version) + ", '" + hash +  "', '" + patch + "');";
        sqlite3_exec(repo_db, query.c_str(), NULL, NULL, NULL);

        MatchPatch mp;
        std::vector<File> patches, oldFiles, newFiles;
        std::string new_files;
        patches = unpack(patch);
        oldFiles = unpack(old_files);
        newFiles = mp.patch_files(patches, oldFiles);
        new_files = pack(newFiles);
        query = "INSERT INTO commits VALUES(" + std::to_string(version) + ", '" + hash +  "', '" + new_files + "');";
        sqlite3_exec(repo_db, query.c_str(), NULL, NULL, NULL);
        sqlite3_close(repo_db);
        mtx.unlock();
    }

    void Exec() override {
        if (user.isLogged()) {
            if (user.getPermissions().find('w') != std::string::npos || user.getPermissions().find('a') != std::string::npos) {

                int latest_version = getLatestVersion();
                std::string hash = getVersionHash(latest_version);
                std::string old_files = getVersionFS(latest_version);

                send_data(std::to_string(latest_version), client);
                send_data(hash, client);
                send_data(old_files, client);

                std::string patch_pack, new_hash;

                read(client, &latest_version, sizeof(int));
                
                new_hash = receive_data(client);
                patch_pack = receive_data(client);

                reply = "Command ok";

                update_tables(latest_version, hash, patch_pack, old_files);
            }
            else {
                reply = "Could not execute command. You need at least write permissions to execute this command";

            }
        }
        else 
            reply = "No user logged in";
    };
};

// Syntax: revert <hash> // Lets a client (with admin permissions) revert the remote repository to a previous version
class RevertCommand : public RepoCommand {
public:
    void Exec() override {
        if (user.isLogged()) {
            if (user.getPermissions().find('a') != std::string::npos) {
                mtx.lock();
                sqlite3_open("./database/file_system.db", &repo_db);
                sqlite3_stmt *stmt;

                std::string query = "DELETE FROM repo WHERE version > " + argv[2] + ";";
                sqlite3_exec(repo_db, query.c_str(), NULL, NULL, NULL);

                query = "DELETE FROM commits WHERE version > " + argv[2] + ";";
                sqlite3_exec(repo_db, query.c_str(), NULL, NULL, NULL);
                
                sqlite3_close(repo_db);
                reply = "Successfully reverted the repository";
                mtx.unlock();
            }
            else
                reply = "Could not execute command. You need administrator permissions to execute this command";
        }
        else 
            reply = "No user logged in";
    };
};


// Syntax: verhashlog // Lets a client see all commit hashes along with their commit messages
class VersionHashLogCommand : public RepoCommand {
public:
    void Exec() override {
        if (!user.isLogged()) {
            reply = "No user logged in";
            return;
        }

        mtx.lock();
        sqlite3_open("./database/file_system.db", &repo_db);

        std::string query = "SELECT version, hash FROM repo;";
        sqlite3_stmt *stmt;

        int ret = sqlite3_prepare_v2(repo_db, query.c_str(), -1, &stmt, NULL);
        if (ret)
            std::cerr << "Error at sqlite3_prepare_v2: " << sqlite3_errmsg(repo_db) << std::endl;
        
        reply = "The repository's versions and hashes are:\n";
        while (true) {
            int ret = sqlite3_step(stmt);
            if (ret == SQLITE_DONE)
                break;
            if (ret != SQLITE_ROW) {
                std::cerr << "Error at sqlite_step: " << sqlite3_errmsg(repo_db);
                break;
            }
            int version = sqlite3_column_int(stmt, 0);
            std::string hash;
            int tmp_size = sqlite3_column_bytes(stmt, 1);
            const unsigned char* tmp = sqlite3_column_text(stmt, 1);
            hash.assign(tmp, tmp + tmp_size);

            std::cout << version << " " << hash << std::endl;

            reply += "Version: ";
            reply += std::to_string(version);
            reply += ", Hash: ";
            reply += hash;
            reply += '\n';
        }

        sqlite3_finalize(stmt);
        sqlite3_close(repo_db);
        mtx.unlock();
    };
};