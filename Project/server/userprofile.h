class UserProfile
{
private:
    std::string username, permissions;
    bool is_logged = false;
public:
    void loginUser(const char* username) {
        this->username = username;
        this->is_logged = true;
    }
    void setUsername(const char* username) {
        this->username = username;
    }
    void setPermissions(std::string permissions) {
        this->permissions = permissions;
    }
    std::string getUsername() const {
        return this->username;
    }
    std::string getPermissions() const {
        return this->permissions;
    }
    void logoutUser() {
        this->username.clear();
        this->permissions.clear();
        is_logged = false;
    }
    bool isLogged() {
        return is_logged;
    }
    UserProfile& operator= (UserProfile user2) {
        this->username = user2.username;
        this->permissions = user2.permissions;
        this->is_logged = user2.is_logged;
        return (*this);
    }
    void userPrint() {
        std::cout << "User: " << username << " " << permissions << " ";
        if (is_logged == 1)
            std::cout << "1 (logged)";
        else
            std::cout << "0 (not logged)";
        std::cout << std::endl;
    }
};