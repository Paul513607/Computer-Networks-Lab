#pragma once

class Client {
private:
    int port;
    int sd;
    struct sockaddr_in server;
    socklen_t server_len = 0;
    std::string username;
public:
    void initialize(char *ip_address, char *port) { // open the socket
        this->port = atoi(port);

        if (-1 == (this->sd = socket(AF_INET, SOCK_STREAM, 0)))
            handle_error("Error at socket", err_counter_cli);
        
        bzero(&this->server, sizeof(struct sockaddr_in));

        // socket family
        this->server.sin_family = AF_INET;
        // server IP adress
        this->server.sin_addr.s_addr = inet_addr(ip_address);
        // port
        this->server.sin_port = htons(this->port);
    }

    void readUsername() {
        std::cout << "Set username: ";
        fflush(stdout);
        std::getline(std::cin, username);
        fflush(stdin);
    }

    std::string getUsername() const {
        return this->username;
    }

    void connectToServer() {    // connect to the server
        if (-1 == connect(this->sd, (struct sockaddr *) &this->server, sizeof(struct sockaddr))) {
            close(sd);
            handle_error("Error at connect", err_counter_cli);
        }
        std::cout << "[client] Successfully connected to the server!" << std::endl;
    }

    void sendCommand() {    // send a command
        int cmd_len = 0;
        std::string command;
        std::cout << "Input a command: ";
        fflush(stdout);
        std::getline(std::cin, command);
        fflush(stdin);

        if (command == "quit_session")
            openSession = false;

        cmd_len = command.length() + 1;
        if (write(this->sd, &cmd_len, sizeof(int)) <= 0)
            handle_error("Error at write", err_counter_cli);
        if (write(this->sd, command.c_str(), cmd_len) <= 0)
            handle_error("Error at write", err_counter_cli);
        std::cout << "[client] Successfully sent command \"" << command << "\"!" << std::endl;
        command.clear();
    }

    void receiveResponse() {    // receive the response from the server for the sent command
        int msg_len = 0;
        char *msg = nullptr;

        if (read(this->sd, &msg_len, sizeof(int)) <= 0) {
            close(this->sd);
            handle_error("Error at read", err_counter_cli);
        }
        msg = new char[msg_len];

        if (read(this->sd, msg, msg_len) <= 0) {
            close(this->sd);
            handle_error("Error at read", err_counter_cli);
        }

        std::cout << "[client] Successfully received response: \"" << msg  << "\"!" << std::endl;
        delete msg;
    }

    void closeClient() {
        this->username.clear();
        close(this->sd);
        std::cout << "[client] Done!" << std::endl;
    }
};