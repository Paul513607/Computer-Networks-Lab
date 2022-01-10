#pragma once

class File {
public:
    std::string path, content;
    mode_t st_mode;
    bool isDirectory;
    File() {};
    File(std::string path, mode_t st_mode) {    // Directory
        this->path = path;
        this->content = "";
        this->st_mode = st_mode;
        this->isDirectory = true;
    }
    File(std::string path, std::string content, mode_t st_mode) {   // Regular file
        this->path = path;
        this->content = content;
        this->st_mode = st_mode;
        this->isDirectory = false;
    }
    void setAttributes(std::string path, std::string content, mode_t st_mode, bool isDirectory) {
        this->path = path;
        this->content = content;
        this->st_mode = st_mode;
        this->isDirectory = isDirectory;
    }
    void filePrint() {
        std::cout << "File: " << path << " " << content << " " << st_mode << " " << isDirectory << std::endl;
    }
    template <class Archive>
    void serialize(Archive & archive) {
        archive(path, content, st_mode, isDirectory);
    }
};