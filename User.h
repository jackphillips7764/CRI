
#ifndef __User__
#define __User__
#include <deque>
#include <map>
#include <string.h>
#include <unordered_map>
#include <vector>

class Chan;

class User {
    int socket;
    std::string username;
    // Queue of stuff to be sent
    std::deque<std::string> *sendQueue;
    // Buffer for current command when /n will processes
    std::string buffer;
    bool isOp;

  public:
    User(int sock) {
        socket = sock;
        username = "";
        sendQueue = new std::deque<std::string>();
        buffer = "";
        isOp = false;
    };
    ~User() { delete sendQueue; };
    std::string getUsername() { return username; };
    bool addToBuffer(std::string &stuff,
                     std::unordered_map<std::string, User *> &user_map,
                     std::unordered_map<std::string, Chan> &chans);
    void addMsg(std::string msg) { sendQueue->push_back(msg); };
    bool getMsg(std::unordered_map<std::string, User *> &user_map,
                std::unordered_map<std::string, Chan> &chans);
    /* this sends when socket writable */
    void sendMsg();
    bool needToWrite() { return sendQueue->size() > 0; }
    int getSocket() { return socket; };
    int getBufLen() { return this->buffer.size(); };

    // TODO tiecoon this func is all you may need some more commands
    bool processesComand(std::string &comand,
                         std::unordered_map<std::string, User *> &user_map,
                         std::unordered_map<std::string, Chan> &chans);
    void makeOp(std::string pass, std::string actualPass);
    void delUser(std::unordered_map<std::string, User *> &user_map,
                 std::unordered_map<std::string, Chan> &chans);
    void privMsg(std::string &comand,
                 std::unordered_map<std::string, User *> &user_map,
                 std::unordered_map<std::string, Chan> &chans);
};

class Chan {
    std::vector<User *> users;

  private:
    void sendMsg();
    void kick();
    void addUser(User *user);
};

#endif
