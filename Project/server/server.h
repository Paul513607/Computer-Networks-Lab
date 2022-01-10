#pragma once

class Server {
private:
    int port = PORT;
    int sd;
    int client;
    struct sockaddr_in server;
    struct sockaddr_in from;
    socklen_t from_len = 0;
    std::string command_store, reply_msg;
    int cmd_code;

    UserProfile user;
    Command *cmd = nullptr;
    sqlite3 *user_db = nullptr, *repo_db = nullptr;
public:
    ~Server() {
        command_store.clear();
        reply_msg.clear();
    }

    void initialize() {    // open the server socket, bind it to it's family, address and port and start listening for connecting clients
        if (-1 == (sd = socket(AF_INET, SOCK_STREAM, 0)))
            handle_error("Error at socket", err_counter_serv);

        bzero(&server, sizeof(struct sockaddr_in));
        bzero(&from, sizeof(struct sockaddr_in));

        // socket family
        server.sin_family = AF_INET;
        // accept any adress
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        // port
        server.sin_port = htons(port);

        if (-1 == bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)))
            handle_error("Error at bind", err_counter_serv);

        if (-1 == listen(sd, 5))    // accept a maximim of 5 pending connections simultaneously
            handle_error("Error at listen", err_counter_serv);
    }

    std::string getCommand() const {
        return this->command_store;
    }

    bool acceptConnection() {   // accept an incoming connection from a client (afterwards create a child process that takes care of the client and keep listening)
        std::cout << "[server] Waiting for connections on port " << port << std::endl;
        from_len = sizeof(from);
        if (-1 == (client = accept(sd, (struct sockaddr *) &from, &from_len))) {
            close(sd);
            handle_error_while("Error at accept", err_counter_serv);
            return false;
        }

        std::cout << "[server] User successfully connected!" << std::endl;
        return true;
    }

    bool receiveCommand() {     // reads the command sent by the client from the socket --- child
        int cmd_len = 0;
        char *command = nullptr;
        int rd = 0;

        while (rd <= 0)
            rd = read(client, &cmd_len, sizeof(int));
        command = new char[cmd_len];

        if (read(client, command, cmd_len) <= 0) {
            close(client);
            close(sd);
            delete command;
            handle_error_while("Error at read", err_counter_serv);
            return false;
        }
        
        command_store = command;
        delete command;
        std::cout << "[server child] Successfully received command \"" << command_store << "\"!" << std::endl;

        return true;
    }

    bool processCmdline() {     // do necessary checks on the command --- child
        std::vector<std::string> argv;
        process_command(argv, command_store);
        reply_msg.clear();
        
        if (argv[0] == "init" && argv.size() == 1) {
            cmd_code = 3;
            cmd = new InitCommand();
            cmd->setArguments(argv, user);
            (static_cast<InitCommand *> (cmd))->setRepoDBptr(repo_db);
            (static_cast<InitCommand *> (cmd))->setClient(client);
        }
        else if (argv[0] == "register" && argv.size() == 2) {
            cmd = new RegisterCommand();
            cmd->setArguments(argv, user);
            (static_cast<RegisterCommand *> (cmd))->setUserDBptr(user_db);
        }
        else if (argv[0] == "login" && argv.size() == 2) {
            cmd = new LoginCommand();
            cmd->setArguments(argv, user);
            (static_cast<LoginCommand *> (cmd))->setUserDBptr(user_db);
        }
        else if (argv[0] == "logout" && argv.size() == 1) {
            cmd = new LogoutCommand();
            cmd->setArguments(argv, user);
        }
        else if (argv[0] == "update_perm" && argv.size() == 3) {
            cmd = new UpdatePermCommand();
            cmd->setArguments(argv, user);
            (static_cast<UpdatePermCommand *> (cmd))->setUserDBptr(user_db);
        }   
        else if (argv[0] == "info_perm" && argv.size() == 1) {
            cmd = new InfoPermCommand();
            cmd->setArguments(argv, user);
        }
        else if (argv[0] == "remove" && argv.size() == 2) {
            cmd = new RemoveUserCommand();
            cmd->setArguments(argv, user);
            (static_cast<RemoveUserCommand *> (cmd))->setUserDBptr(user_db);
        }
        else if (argv[0] == "clone" && argv.size() == 2) {
            cmd_code = 1;
            cmd = new CloneCommand();
            cmd->setArguments(argv, user);
            (static_cast<CloneCommand *> (cmd))->setRepoDBptr(repo_db);
            (static_cast<CloneCommand *> (cmd))->setClient(client);
        }
        else if (argv[0] == "commit" && argv.size() == 1) {
            cmd_code = 2;
            cmd = new CommitCommand();
            cmd->setArguments(argv, user);
            (static_cast<CommitCommand *> (cmd))->setRepoDBptr(repo_db);
            (static_cast<CommitCommand *> (cmd))->setClient(client);
        }
        else if (argv[0] == "revert" && argv.size() == 2) {
            cmd = new RevertCommand();
            cmd->setArguments(argv, user);
            (static_cast<RevertCommand *> (cmd))->setRepoDBptr(repo_db);
            (static_cast<RevertCommand *> (cmd))->setClient(client);
        }
        else if (argv[0] == "verhashlog" && argv.size() == 1) {
            cmd = new VersionHashLogCommand();
            cmd->setArguments(argv, user);
            (static_cast<VersionHashLogCommand *> (cmd))->setRepoDBptr(repo_db);
        }
        else {
            reply_msg = "Unknown command";
            free_string_vector_memory(argv);
            return false;
        }
        free_string_vector_memory(argv);
        return true;
    }

    void runCommand() {     // run the command --- child
        cmd_code = 0;
        std::cout << "BBB " << cmd_code << std::endl;
        if (processCmdline()) {
            std::cout << "CCC " << cmd_code << std::endl;
            if (cmd_code == 0) {
                cmd->Exec();
                reply_msg = cmd->GetReply();
                user = cmd->GetUser();
                delete cmd;
            }
        }
    }

    bool sendResponse() {   // send a response back to a client  --- child
        if (cmd_code == 0) {
            int msg_len = 0;
            std::string msg;

            msg = reply_msg;
            msg_len = msg.length() + 1;
            if (write(client, &msg_len, sizeof(int)) <= 0) {
                msg.clear();
                close(client);
                close(sd);
                handle_error_while("Error at write", err_counter_serv);
                return false;
            }
            if (write(client, msg.c_str(), msg_len) <= 0) {
                msg.clear();
                close(client);
                close(sd);
                handle_error_while("Error at write", err_counter_serv);
                return false;
            }
            std::cout << "[server child] Successfully sent response \"" << msg << "\"!" << std::endl;
            msg.clear();
        }
        else if (cmd_code == 1) {
            bool ok = true;
            if (!user.isLogged())
                ok = false;
            std::cout << "HERE1 " << ok << std::endl;
            user.userPrint();
            if (user.getPermissions().find('r') == std::string::npos)
                ok = false;
            std::cout << "HERE2 " << ok << std::endl;
            write(client, &ok, sizeof(bool));
            std::cout << "HERE3 " << ok << std::endl;

            if (ok) {
                std::cout << "HERE4 " << ok << std::endl;
                cmd->Exec();
                user = cmd->GetUser();
                delete cmd;
            }
        }
        else if (cmd_code == 2) {
            bool ok = true;
            if (!user.isLogged())
                ok = false;
            if (user.getPermissions().find('w') == std::string::npos)
                ok = false;
            write(client, &ok, sizeof(bool));

            if (ok) {
                cmd->Exec();
                user = cmd->GetUser();
                delete cmd;
            }
        }
        else if (cmd_code == 3) {
            cmd->Exec();
            user = cmd->GetUser();
            delete cmd;
        }
        return true;
    }

    void logoutUser() {
        user.logoutUser();
    }

    void closeClientSd() {
        close(client);
    }

    void closeServerSd() {
        close(sd);
    }

    void closeServer() {
        command_store.clear();
        close(sd);
    }
};