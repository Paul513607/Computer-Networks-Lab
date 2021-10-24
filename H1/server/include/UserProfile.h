#pragma once
class UserProfile
{
private:
    std::string username;
public:
    bool isLogged = false;

    UserProfile();
    UserProfile(std::string username);
    ~UserProfile();
    void loginUser(const char* username);
    void setUsername(const char* username);
    std::string getUsername() const;
    void logoutUser();
    UserProfile& operator= (UserProfile user2);
    friend std::string getProcessInformation(const char* pid_cstr);
    friend bool isKnownUser(const char* username);
};

void getKnownUsers();
std::string getProcessInformation(const char* pid);