#pragma once

class Repository {
private:
    sqlite3 *repo_db;
public:
    std::string getLatestHash();
};