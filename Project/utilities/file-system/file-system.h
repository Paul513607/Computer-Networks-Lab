#pragma once

#include "../File.h"
#include "./hashlib2plus/trunk/src/hashlibpp.h"

class FileSystem {
public:
    hashwrapper *hasher;
    FileSystem() {
        hasher = new sha256wrapper;
    }
    ~FileSystem() {
        delete hasher;
    }
    File file_read(std::string file_path) {
        File file;

        struct stat st;
        if (-1 == stat(file_path.c_str(), &st))
            std::cerr << "Error at stat: " << errno;

        int fsize = st.st_size;
        if (S_ISDIR(st.st_mode)) {
            file.setAttributes(file_path, "", st.st_mode, true);
        }
        else if (S_ISREG(st.st_mode)) {
            std::ifstream in(file_path.c_str());
            std::stringstream stream_buffer;
            std::string buffer;
            stream_buffer << in.rdbuf();
            buffer = stream_buffer.str();
            in.close();
            if (buffer.size() < fsize) {
                std::string err_msg = "Error at reading file with path " + file_path;
                std::cerr << err_msg << std::endl;
            }
            file.setAttributes(file_path, buffer, st.st_mode, false);
        }
        else {
            std::cerr << "Error: Unknown file type for file with path " << file_path << std::endl;
        }
        return file;
    }

    void file_unload(File file) {
        if (file.isDirectory) {
            mkdir(file.path.c_str(), 0777);
        }
        else {
            std::ofstream out(file.path);
            out << file.content;
            out.close();
        }
    }

    void traverse_directory(std::string path, std::vector<std::string> &files) {
        DIR *dptr;
        struct stat fst;
        struct dirent *in_dir;
        std::stack<std::string> st;
        files.push_back(path);
        st.push(path);

        while (!st.empty()) {
            std::string top_path = st.top();
            st.pop();
            stat(top_path.c_str(), &fst);
            if (S_ISREG(fst.st_mode))
                continue;

            dptr = opendir(top_path.c_str());
            if (dptr == NULL) {
                std::cerr << "Error at opendir: " << errno;;
                return;
            }

            while ((in_dir = readdir(dptr)) != NULL) {
                if (strcmp(in_dir->d_name, ".") != 0 && strcmp(in_dir->d_name, "..") != 0) { // Avoid the current and parent directories
                    std::string child_path = top_path + "/" + in_dir->d_name;
                    files.push_back(child_path);
                    st.push(child_path);
                }
            }
        }
    }

    std::vector<File> get_nested_files_info(std::string og_path) {
        std::vector<File> files;
        std::vector<std::string> paths;

        traverse_directory(og_path, paths);

        for (auto path : paths) {
            files.push_back(file_read(path));
        }

        return files;
    }

    void remove_file(File file) {
        if (!file.isDirectory)
            remove(file.path.c_str());
        else {
            std::vector<std::string> to_delete_path;
            traverse_directory(file.path, to_delete_path);

            for (auto del_it = to_delete_path.end(); del_it != to_delete_path.begin(); del_it--) {
                struct stat st;
                if (-1 == stat(del_it->c_str(), &st))
                    std::cerr << "Error at stat: " << errno;
                
                if (S_ISREG(st.st_mode))
                    remove(del_it->c_str());
                else if (S_ISDIR(st.st_mode))
                    rmdir(del_it->c_str());
            }
        }
    }

    void file_unload_multiple(std::vector<File> newFiles, std::vector<File> oldFiles) {
        // We sort the files alphabetically by path to make sure the directory comes before regular files
        auto lambda_comp = [&](File x, File y) {
            return x.path < y.path;
        };
        sort(newFiles.begin(), newFiles.end(), lambda_comp);
        sort(oldFiles.begin(), oldFiles.end(), lambda_comp);

        std::vector<File>::iterator newBIt = newFiles.begin(), oldBIt = oldFiles.begin();

        // Merge the two file-hierarchys, add/delete necessary new/old files
        while (newBIt != newFiles.end() && oldBIt != oldFiles.end()) {
            if (oldBIt->path > newBIt->path) {
                file_unload(*newBIt);
                newBIt++;
            }
            else if (newBIt->path > oldBIt->path) {
                remove_file(*oldBIt);
                oldBIt++;
            }
            else { // Same file, change content
                file_unload(*newBIt);
                newBIt++;
                oldBIt++;
            }
        }
        
        while(newBIt != newFiles.end()) {
            file_unload(*newBIt);
            newBIt++;
        }

        while(oldBIt != oldFiles.end()) {
            remove_file(*oldBIt);
            oldBIt++;
        }
    }
};