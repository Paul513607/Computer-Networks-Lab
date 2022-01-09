#pragma once

class Client {
private:
    int port;
    int sd;
    struct sockaddr_in server;
    socklen_t server_len = 0;
    std::string username, persist_cmd;
    int cmd_code;
public:
    void initialize(char *ip_address, char *port) { // open the socket + initialize the database
	    mkdir("repo", 777);
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

    void client_clone() {
        FileSystem fs;

        int fssize;
        char *files_c;
        std::string files_pack;
        std::vector<File> repo = fs.get_nested_files_info("./repo");

        read(sd, &fssize, sizeof(int));
        files_c = new char[fssize + 1];
        read(sd, &files_c, fssize);
        files_pack = files_c;
        delete files_c;

        std::vector<File> files = unpack(files_pack);


        fs.file_unload_multiple(files, repo);
        std::cout << "Successful clone!" << std::endl;
    }

    void client_commit() {
        FileSystem fs;
        MatchPatch mp;

        std::vector<File> local_files = fs.get_nested_files_info("./repo");
        std::string files_packed = pack(local_files);
        std::string local_hash = fs.hasher->getHashFromString(files_packed);
    
        int version, hashlen, fssize;
        char *hash_c, *oldfs_c;
        std::string hash, oldfs;

        read(sd, &version, sizeof(int));
        version++;

        read(sd, &hashlen, sizeof(int));
        hash_c = new char[hashlen + 1];
        read(sd, &hash_c, hashlen);
        hash = hash_c;
        delete hash_c;

        read(sd, &fssize, sizeof(int));
        oldfs_c = new char[fssize + 1];
        read(sd, &oldfs_c, fssize);
        oldfs = oldfs_c;
        delete oldfs_c;

        std::vector<File> old_files = unpack(oldfs);
        std::vector<File> patches = mp.make_patches(local_files, old_files);
        std::string patch_pack = pack(patches);

        write(sd, &version, sizeof(int));

        int local_hashlen = local_hash.length() + 1;
        write(sd, &local_hashlen, sizeof(int));
        write(sd, &local_hash, local_hashlen);

        int patchlen = patch_pack.size() + 1;
        write(sd, &patchlen, sizeof(int));
        write(sd, &patch_pack, patchlen);
        std::cout << "Successful commit!" << std::endl;
    }

    void client_revert() {
        std::cout << "Successful revert!" << std::endl;
    }

    void sendCommand() {    // send a command
        cmd_code = 0;
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

        std::vector<std::string> argv;
        process_command(argv, command);

        if (argv[0] == "clone" && argv.size() == 2)
            cmd_code = 1;
        else if (argv[0] == "commit" && argv.size() == 1)
            cmd_code = 2;
        else if (argv[0] == "revert" && argv.size() == 2)
            cmd_code = 3;
        command.clear();
    }

    void receiveResponse() {    // receive the response from the server for the sent command
        if (cmd_code == 0) {
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
        else {
            switch (cmd_code)
            {
            case 1:
                client_clone();
                break;
            case 2:
                client_commit();
            case 3:
                client_revert();
            default:
                break;
            }
        }
    }

    void closeClient() {
        this->username.clear();
        close(this->sd);
        std::cout << "[client] Done!" << std::endl;
    }
};