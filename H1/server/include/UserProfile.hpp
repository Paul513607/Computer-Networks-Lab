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
    void SetUsername(const char* username);
    std::string GetUsername() const;
    void logoutUser();
    friend std::string getProcessInformation(const char* pid_cstr);
    friend bool isKnownUser(const char* username);
};

void getKnownUsers();
std::string getProcessInformation(const char* pid);