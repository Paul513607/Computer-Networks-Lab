#pragma once
#define handle_error(msg, i) { \
            fprintf(stderr, "%s %d\n", msg, i); \
        } \

void write_len_str(int fd, int len, std::string out_txt);

class Command
{
protected:
    std::string msg_back_str;
public:
    UserProfile user;
    virtual ~Command();
    virtual void Execute() = 0;
    std::string GetMsgBack() const;
};

class HelpCommand : public Command
{
public:
    HelpCommand() {};
    void Execute() override;
};

class LoginCommand : public Command
{
private:
    std::string username_login;
public:
    LoginCommand() {};
    LoginCommand(UserProfile user, std::string usern);
    ~LoginCommand();
    void Execute() override;
};

class GetLoggedUsersCommand : public Command
{
public:
    GetLoggedUsersCommand() {};
    GetLoggedUsersCommand(UserProfile user);
    void Execute() override;
};

class GetProcInfoCommand : public Command
{
private:
    std::string pid_str;
public:
    GetProcInfoCommand() {};
    GetProcInfoCommand(UserProfile user, std::string pid_str);
    void Execute() override;
};

class GetCurrentLoggedClientCommand : public Command
{
public:
    GetCurrentLoggedClientCommand() {};
    GetCurrentLoggedClientCommand(UserProfile user);
    void Execute() override;
};