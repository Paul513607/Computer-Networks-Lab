#pragma once

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