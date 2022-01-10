#pragma once

/* cereal */

#include "./cereal/types/vector.hpp"
#include "./cereal/types/string.hpp"
#include "./cereal/archives/portable_binary.hpp"
#include "./cereal/archives/binary.hpp"

#include "../File.h"


std::string pack(std::vector<File> files) {
    std::stringstream sbuff(std::ios::binary | std::ios::out | std::ios::in);
    auto lambda_comp = [] (File x, File y) {
        return x.path < y.path;
    };
    std::cout << "PPP: \n";
    int size = 0;
    for (auto file : files) {
        std::cout << "QQQ: " << file.path << " " << file.content << " " << file.st_mode << " " << file.isDirectory << ";";
        size += file.path.size() + file.content.size() + sizeof(mode_t) + sizeof(bool);
    }
    std::cout << "Size: " << size << std::endl;
    sort(files.begin(), files.end(), lambda_comp);
    {
        cereal::BinaryOutputArchive archive(sbuff);
        archive(CEREAL_NVP(files));
    }
    return sbuff.str();
}


std::vector<File> unpack(std::string buffer) {
    std::stringstream sbuff(std::ios::binary | std::ios::out | std::ios::in);
    std::vector<File> files;
    std::cout << "OOO: " << buffer << std::endl;
    std::cout << "Size: " << buffer.size() << std::endl;
    sbuff.str(buffer);
    {
        std::cout << "HIHI3" << std::endl;
        cereal::BinaryInputArchive archive(sbuff);
        std::cout << "HIHI4" << std::endl;
        archive(files);
        std::cout << "HIHI5" << std::endl;
    }
    return files;
}

