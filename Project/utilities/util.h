#pragma once

#define MAX_BUFFER 1024

void free_string_vector_memory(std::vector<std::string> &arr) {
    for (auto i : arr)
        i.clear();
    arr.clear();
}

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

void send_data(std::string msg, int sd)
{
    int rd, msg_size = msg.size(), split = 0, bytes_toSend;
    if ((rd = write(sd, &msg_size, sizeof(int))) < sizeof(int))
        std::cerr << "Error at write size" << std::endl;
    std::cout << "KKK " << msg_size << std::endl;
	while (split < msg_size)
	{
        if (MAX_BUFFER < msg.size() - split) 
            bytes_toSend = MAX_BUFFER;
        else 
            bytes_toSend = msg.size() - split;
		if ((rd = write(sd, &(*msg.begin()) + split, bytes_toSend)) < 0)
            std::cerr << "Error at write data" << std::endl;
        std::cout << "LLL " << rd << std::endl;
		split += rd;
	}
}

std::string receive_data(int sd)
{
	std::string received_msg = "";
	int rd, expected_bytes = 0, bytes_toReceive, ret = 0;
	char *buff;
    while (ret < sizeof(int)) {
        if ((rd = read(sd, &expected_bytes + ret, sizeof(int) - ret)) < 0)
            std::cerr << "Error at read size" << std::endl;
        ret += rd;
    }

    std::cout << "KKK2 " << expected_bytes << std::endl;

	while (received_msg.size() < expected_bytes)
	{
        if (MAX_BUFFER < expected_bytes - received_msg.size()) 
            bytes_toReceive = MAX_BUFFER;
        else 
            bytes_toReceive = expected_bytes - received_msg.size();
		buff = new char[MAX_BUFFER];
        if ((rd = read(sd, buff, bytes_toReceive)) < 0)
            std::cerr << "Error at read data" << std::endl;
        std::cout << "YYY " << rd << std::endl;
		std::string temp;
		temp.assign(buff, buff + rd);
        delete buff;
		received_msg += temp;
	}
	return received_msg;
}