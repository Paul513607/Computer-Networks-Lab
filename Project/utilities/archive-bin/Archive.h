#pragma once

/* cereal */

#include "./cereal/types/vector.hpp"
#include "./cereal/types/string.hpp"
#include "./cereal/types/complex.hpp"
#include "./cereal/archives/binary.hpp"

#include "../File.h"


std::string pack(std::vector<File> files) {
    std::stringstream sbuff;
    auto lambda_comp = [] (File x, File y) {
        return x.path < y.path;
    };
    sort(files.begin(), files.end(), lambda_comp);
    {
        cereal::BinaryOutputArchive archive(sbuff);
        archive(CEREAL_NVP(files));
    }
    return sbuff.str();
}


std::vector<File> unpack(std::string buffer) {
    std::stringstream sbuff;
    std::vector<File> files;
    sbuff.str(buffer);
    {
        cereal::BinaryInputArchive archive(sbuff);
        archive(files);
    }
    return files;
}

