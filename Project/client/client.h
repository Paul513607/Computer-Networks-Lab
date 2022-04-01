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
	    DIR *dptr = opendir("repo");
        if (dptr)
            closedir(dptr);
        else
            mkdir("repo", 0777);
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

    void client_init() {
        FileSystem fs;
        MatchPatch mp;

        File main_dir;
        char buff[512];
        char *ch_ptr = getcwd(buff, 512);
        std::string file_path = buff;
        file_path += "/";
        file_path += "repo";
        main_dir.setAttributes(file_path, "", S_IFDIR, true);
        std::vector<File> local_files;
        local_files.push_back(main_dir);
        std::string files_packed = pack(local_files);
        std::string local_hash = fs.hasher->getHashFromString(files_packed);
    
        send_data(local_hash, sd);
        send_data(files_packed, sd);

        std::cout << "Successful init!" << std::endl;
    }

    void client_clone() {
        FileSystem fs;

        std::string files_pack;
        
        char buff[512];
        char *ch_ptr = getcwd(buff, 512);
        std::string file_path = buff;
        file_path += "/";
        file_path += "repo";

        std::vector<File> repo = fs.get_nested_files_info(file_path);

        files_pack = receive_data(sd);

        std::vector<File> files = unpack(files_pack);

        fs.file_unload_multiple(files, repo);
        std::cout << "Successful clone!" << std::endl;
    }

    void client_commit() {
        FileSystem fs;
        MatchPatch mp;

        char buff[512];
        char *ch_ptr = getcwd(buff, 512);
        std::string file_path = buff;
        file_path += "/";
        file_path += "repo";

        std::vector<File> local_files = fs.get_nested_files_info(file_path);
        
        std::string files_packed = pack(local_files);
        std::string local_hash = fs.hasher->getHashFromString(files_packed);
    
        std::cout << "Starting to receive data..." << std::endl;
        int version;
        std::string ver, hash, oldfs;

        ver = receive_data(sd);
        version = atoi(ver.c_str());
        version++;

        hash = receive_data(sd);
        oldfs = receive_data(sd);


        std::vector<File> old_files = unpack(oldfs);
        std::vector<File> patches = mp.build_patches(old_files, local_files);
        std::string patch_pack = pack(patches);

        write(sd, &version, sizeof(int));
    
        send_data(local_hash, sd);
        send_data(patch_pack, sd);

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
        else if (argv[0] == "init" && argv.size() == 1)
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
            if (cmd_code == 1) {
                bool ok, ver_ok;
                read(sd, &ok, sizeof(bool));
                if (ok) {
                    read(sd, &ver_ok, sizeof(bool));
                    if (ver_ok) {
                        client_clone();
                    }
                    else {
                        std::cout << "Wrong version" << std::endl;
                    }
                }
                else
                    std::cout << "Either you are not logged in, you don't have read permissions, or the version you are trying to clone is not available\n";
            }
            else if (cmd_code == 2) {
                bool ok;
                read(sd, &ok, sizeof(bool));

                if (ok) {
                        client_commit();
                }
                else {
                    std::cout << "Either you are not logged in, or you don't have write permissions" << std::endl;
                }
            }
            else if (cmd_code == 3) {
                client_init();
            }
        }
    }

    void closeClient() {
        this->username.clear();
        close(this->sd);
        std::cout << "[client] Done!" << std::endl;
    }
};