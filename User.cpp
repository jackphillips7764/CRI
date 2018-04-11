#include "User.h"
#include <cstring>
#include <iostream>
#include <regex>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

void User::sendMsg() {
    if (socket <= 0) {
        std::cout << "ERROR NO SOCKET" << std::endl;
        return;
    }
    // get thing to be sent
    std::string s = this->sendQueue->front();
    this->sendQueue->pop_front();

    const char *buf = s.c_str();
    int size = strlen(buf);
    int len = send(this->socket, buf, size, 0);
    if (size <= 0) {
        std::cout << "Send Error Added back" << std::endl;
        this->sendQueue->push_front(s);
    } else if (len < size) {
        std::cout << "added back" << std::endl;
        // add back to be sent again
        this->sendQueue->push_front(s.substr(len, size - len));
    }
}

bool User::addToBuffer(std::string &stuff,
                       std::unordered_map<std::string, User *> &user_map,
                       std::unordered_map<std::string, Chan *> &chans,
                       std::string password) {
    this->buffer += stuff;
    std::size_t found = this->buffer.find("\n");
    while (found != std::string::npos) {
        std::string send = this->buffer.substr(0, found);
        bool res = this->processesComand(send, user_map, chans, password);
        this->buffer = this->buffer.substr(found + 1);
        if (res)
            return true;
        found = this->buffer.find("\n");
    }
    return false;
}

bool User::getMsg(std::unordered_map<std::string, User *> &user_map,
                  std::unordered_map<std::string, Chan *> &chans,
                  std::string password) {
    char buf[513] = {0};
    int rc = recv(this->socket, buf, 512 - this->getBufLen(), 0);
    if (rc < 0) {
        if (errno != EWOULDBLOCK) {
            this->delUser(user_map, chans);
            return true;
        }
    } else if (rc == 0) {
        this->delUser(user_map, chans);
        return true;
    }

    std::string b(buf);
    // std::cout << b << std::endl;
    return this->addToBuffer(b, user_map, chans, password);
}

int getCommandType(std::string cmd) {
    std::locale loc;
    for (std::string::size_type i = 0; i < cmd.length(); ++i)
        cmd[i] = std::toupper(cmd[i], loc);

    if ("USER" == cmd) {
        return 1;
    }
    if ("PRIVMSG" == cmd) {
        return 2;
    }
    if ("LIST" == cmd) {
        return 3;
    }
    if ("JOIN" == cmd) {
        return 4;
    }
    if ("PART" == cmd) {
        return 5;
    }
    if ("OPERATOR" == cmd) {
        return 6;
    }
    if ("KICK" == cmd) {
        return 7;
    }
    if ("QUIT" == cmd) {
        return 8;
    }
    return 0;
}

/* void User::privMsg(std::string &comand, */
/*                    std::unordered_map<std::string, User *> &user_map, */
/*                    std::unordered_map<std::string, Chan *> &chans) { */

/*     auto channel = comand.substr(comand.find(" ") + 1); */
/*     if (channel[0] != '#') { */
/*         std::string user = channel.substr(0, channel.find(" ")); */
/*         std::string msg = channel.substr(channel.find(" ") + 1); */
/*         if (user_map.count(user) > 0) { */
/*             user_map[user]->addMsg(this->username + "> " + msg + "\n"); */
/*             this->addMsg(this->username + "> " + msg + "\n"); */
/*         } else { */
/*             this->addMsg("User not found\n"); */
/*         } */
/*     } */
/*     // TODO channel message */
/* } */

void User::List(std::string &comand,
                std::unordered_map<std::string, User *> &user_map,
                std::unordered_map<std::string, Chan *> &chans) {
    auto channel = comand.substr(comand.find(" ") + 1);
    if (channel[0] == '#' && chans.find(channel) != chans.end()) {
        auto curchan = chans.find(channel);
        auto users = (*curchan).second->users;
        auto number = users.size();
        std::string msgusr = "There are currently ";
        std::cout << "NUMBER " << number << std::endl;
        msgusr += std::to_string(number);
        msgusr += " members.\n";
        for (auto user : users) {
            msgusr += "* ";
            msgusr += user->username;
            msgusr += "\n";
        }
        this->addMsg(msgusr);
    } else {
        std::string msg2 = "There are currently ";
        msg2 += std::to_string(chans.size());
        msg2 += " channels.\n";
        for (auto i : chans) {
            msg2 += "* ";
            msg2 += i.first;
            msg2 += "\n";
        }
        this->addMsg(msg2);
    }
}

void User::Join(std::string &comand,
                std::unordered_map<std::string, User *> &user_map,
                std::unordered_map<std::string, Chan *> &chans) {
    auto channel = comand.substr(comand.find(" ") + 1);
    std::cmatch m;
    std::regex re("#[a-zA-Z][_0-9a-zA-Z]*");
    if (std::regex_match(channel, re)) {
        if (chans.find(channel) != chans.end()) {
            auto curchan = chans.find(channel);
            (*curchan).second->addUser(this);
        } else {
            chans[channel] = new Chan();
            chans[channel]->name = channel;
            chans[channel]->addUser(this);
        }
    }
}

void User::Part(std::string &comand,
                std::unordered_map<std::string, User *> &user_map,
                std::unordered_map<std::string, Chan *> &chans) {
    if (comand.find(" ") !=
        std::string::npos) { // if there is a second arguement
        auto channel = comand.substr(comand.find(" ") + 1);
        if (chans.find(channel) != chans.end()) {
            auto curchan = chans.find(channel);
            (*curchan).second->part(this);
        } else {
            std::string msg = "You are not currently in ";
            msg += channel;
            this->addMsg(msg);
        }
    } else {
        for (auto i : chans) {
            i.second->part(this);
        }
    }
}

void User::Operator(std::string &comand,
                    std::unordered_map<std::string, User *> &user_map,
                    std::unordered_map<std::string, Chan *> &chans,
                    std::string password) {

    auto passwordguess = comand.substr(comand.find(" ") + 1);
    if (passwordguess == password && password != "") {
        isOp = true;
    }
}

void User::Kick(std::string &comand,
                std::unordered_map<std::string, User *> &user_map,
                std::unordered_map<std::string, Chan *> &chans) {
    auto tmp = comand.find(" ") + 1;
    auto channel = comand.substr(tmp, comand.find(" ", tmp) + 1);
    tmp = comand.find(" ") + 1;
    auto user = comand.substr(tmp);
    if (this->isOp) {
        auto curchan = chans.find(channel);
        (*curchan).second->kick(&user);
    }
}

void User::Privmsg(std::string &comand,
                   std::unordered_map<std::string, User *> &user_map,
                   std::unordered_map<std::string, Chan *> &chans) {

    std::cout << "TEST" << std::endl;
    auto tmp = comand.find(" ") + 1;
    auto opt1 = comand.substr(tmp);
    std::cout << "opt1: " << opt1 << std::endl;
    auto message = opt1.substr(opt1.find(" ") + 1);
    std::cout << "message: " << opt1 << std::endl;
    opt1 = opt1.substr(0, opt1.find(" "));
    std::cout << "opt1new: " << opt1 << std::endl;
    if (opt1[0] != '#') {
        std::cout << "sending to user" << std::endl;
        if (user_map.count(opt1) > 0) {
            user_map[opt1]->addMsg(this->username + "> " + message + "\n");
            /* this->addMsg(this->username + "> " + message + "\n"); */
        } else {
            this->addMsg("User not found\n");
        }
    } else {
        std::cout << "sending to chan " << opt1 << "   " << message
                  << std::endl;
        if (chans.find(opt1) != chans.end()) {
            auto curchan = chans.find(opt1);
            std::cout << "sending to chan " << opt1 << std::endl;
            std::string msg = username;
            msg += ": ";
            msg += message;
            (*curchan).second->sendMsg(msg);
        }
    }
}

void User::Quit(std::string &comand,
                std::unordered_map<std::string, User *> &user_map,
                std::unordered_map<std::string, Chan *> &chans) {
    for (auto i : chans) {
        i.second->part(this);
    }
    user_map.erase(this->username);
    close(this->socket);
}

// called with command to processes
// the list of users one is with sockets one is with
// maping to the users name
// the user dissconnects tiecoon
bool User::processesComand(std::string &comand,
                           std::unordered_map<std::string, User *> &user_map,
                           std::unordered_map<std::string, Chan *> &chans,
                           std::string password) {
    // return true when leave so it can be properly removed
    std::cout << "COMMAND: " << comand << std::endl;

    std::string cmd = comand.substr(0, comand.find(" "));

    std::cout << "CMD: " << cmd << std::endl;
    int c = getCommandType(cmd);
    std::cout << "CMD: " << c << std::endl;
    switch (c) {
    case 1: { // USER
        auto name = comand.substr(comand.find(" ") + 1);
        std::cmatch m;
        std::regex re("[a-zA-Z][_0-9a-zA-Z]*");
        if (std::regex_match(name, re)) {
        }
        this->username = name;
        std::cout << "USER: " << this->username << std::endl;
        user_map[this->username] = this;
        break;
    }
    case 2: // PRIVMSG
        this->Privmsg(comand, user_map, chans);
        break;
    case 3: // LIST
        this->List(comand, user_map, chans);
        break;
    case 4: // JOIN
        this->Join(comand, user_map, chans);
        break;
    case 5: // PART
        this->Part(comand, user_map, chans);
        break;
    case 6: // OPERATOR
        this->Operator(comand, user_map, chans, password);
        break;
    case 7: // KICK
        this->Kick(comand, user_map, chans);
        break;
    case 8: // QUIT
        this->Quit(comand, user_map, chans);
        break;
    case 0:
        addMsg("sorry nothing ready yet" + this->username + "\n");
    }
    return false;
}
void User::delUser(std::unordered_map<std::string, User *> &user_map,
                   std::unordered_map<std::string, Chan *> &chans) {
    // TODO part from all channels with quit message

    std::cout << "User parted" << std::endl;
    // remove from lists
    user_map.erase(this->username);
    close(this->socket);
    delete this;
}

void Chan::addUser(User *user) {
    if (std::find(users.begin(), this->users.end(), user) ==
        users.end()) { // if not in chan
        std::string msg = "";
        msg += user->username;
        msg += " joined the channel";
        sendMsg(msg);
        msg = "Joined channel ";
        msg += name;
        msg += "\n";
        user->addMsg(msg);
        users.push_back(user);
    }
}

void Chan::kick(std::string *user) {
    for (auto i : users) {
        if (i->username == *user) {
            std::string msg = "";
            msg += name;
            msg += "> ";
            msg += *user;
            msg += " has been kicked from the channel.\n";
            sendMsg(msg);
            users.erase(std::find(users.begin(), users.end(), i));
        }
    }
}

void Chan::part(User *user) {
    if (std::find(users.begin(), users.end(), user) !=
        users.end()) { // if not in chan
        std::string msg = "";
        msg += user->username;
        msg += " left the channel.";
        sendMsg(msg);
        users.erase(std::find(users.begin(), users.end(), user));
    }
}

void Chan::sendMsg(std::string message) {
    for (auto i : users) {
        std::string msg = name;
        msg += "> ";
        msg += message;
        msg += "\n";
        i->addMsg(msg);
    }
}
