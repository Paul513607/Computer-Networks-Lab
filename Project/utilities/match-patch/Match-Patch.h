#pragma once

#include "./diff-match-patch-cpp-stl/diff_match_patch.h"
#include "../File.h"

class MatchPatch {
public:
    File build_patch(File newFile, File oldFile) {
        diff_match_patch<std::string> funload;
        std::string content = funload.patch_toText(funload.patch_make(newFile.content, oldFile.content));

        return File(newFile.path, content, oldFile.st_mode);
    }

    std::vector<File> build_patches(std::vector<File> newFiles, std::vector<File> oldFiles) {
        std::vector<File> files;
        auto lambda_comp = [](File x, File y) {
            return x.path < y.path;
        };
        sort(newFiles.begin(), newFiles.end(), lambda_comp);
        sort(oldFiles.begin(), oldFiles.end(), lambda_comp);

        std::vector<File>::iterator newBIt = newFiles.begin(), oldBIt = oldFiles.begin();

        while (newBIt != newFiles.end() && oldBIt != oldFiles.end()) {
            if (oldBIt->path > newBIt->path) { // No changes to old file
                newBIt++;
            }
            else if (newBIt->path > oldBIt->path) { // Deleted file
                File del_file;
                del_file.setAttributes(oldBIt->path, "", oldBIt->st_mode, true);
                files.push_back(del_file);
                oldBIt++;
            }
            else {  // File changed
                files.push_back(build_patch(*newBIt, *oldBIt));
                newBIt++;
                oldBIt++;
            }
        }

        while(oldBIt != oldFiles.end()) { // Other deleted files
            File del_file;
            del_file.setAttributes(oldBIt->path, "", oldBIt->st_mode, true);
            files.push_back(del_file);
            oldBIt++;
        }

        return files;
    }

    File patch_file(File newFile, File oldFile) {  
        diff_match_patch<std::string> funload;
        std::string content = funload.patch_apply(funload.patch_fromText(oldFile.content), newFile.content).first;

        return File(oldFile.path, content, oldFile.st_mode);
    }

    std::vector<File> patch_files(std::vector<File> newFiles, std::vector<File> oldFiles) {
        std::vector<File> files;
        static auto comparator = [](File x, File y) {
            return x.path < y.path;
        };
        sort(newFiles.begin(), newFiles.end(), comparator);
        sort(oldFiles.begin(), oldFiles.end(), comparator);

        std::vector<File>::iterator newBIt = newFiles.begin(), oldBIt= oldFiles.begin();

        while (newBIt != newFiles.end() && oldBIt != oldFiles.end()) {
            if (oldBIt->path > newBIt->path) { // No changes to old file
                newBIt++;
            }
            else if (newBIt->path > oldBIt->path) { // Deleted file
                File del_file;
                del_file.setAttributes(oldBIt->path, "", oldBIt->st_mode, true);
                files.push_back(del_file);
                oldBIt++;
            }
            else {
                files.push_back(patch_file(*newBIt, *oldBIt));
                newBIt++;
                oldBIt++;
            }
        }

        while(oldBIt != oldFiles.end()) {
            File del_file;
            del_file.setAttributes(oldBIt->path, "", oldBIt->st_mode, true);
            files.push_back(del_file);
            oldBIt++;
        }

        return files;
    }
};