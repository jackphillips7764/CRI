#include "User.h"
#include <sys/socket.h>
#include <cstring>
#include <unordered_map>
#include <iostream>
#include <string>
#include <unistd.h>

void User::sendMsg(){
	if(socket <= 0){
		std::cout << "ERROR NO SOCKET" << std::endl;
		return;
	}
	//get thing to be sent
	std::string s = this->sendQueue->front();
	this->sendQueue->pop_front();

	const char * buf = s.c_str();
	int size = strlen(buf);
	int len = send(this->socket, buf, size, 0);
	if (size <= 0){
		std::cout << "Send Error Added back" << std::endl;
		this->sendQueue->push_front(s);
	}
	else if (len < size){
		std::cout << "added back" << std::endl;
		//add back to be sent again
		this->sendQueue->push_front(s.substr(len, size-len));
	}
}

bool User::addToBuffer(std::string& stuff,
		std::unordered_map<std::string, User *>& user_map,
		std::unordered_map<std::string, Chan>& chans) {
	this->buffer += stuff;
	std::size_t found = this->buffer.find("\n");
	if(found != std::string::npos){
		std::string send = this->buffer.substr(0, found);
		bool res = this->processesComand(send, user_map, chans);
		this->buffer = this->buffer.substr(found+1);
		return res;
	}
	return false;

}

bool User::getMsg(std::unordered_map<std::string, User *>& user_map,
		std::unordered_map<std::string, Chan>& chans){
	char buf[513] = {0};
	int rc = recv(this->socket, buf, 512-this->getBufLen(), 0);
	if(rc < 0){
		if (errno != EWOULDBLOCK){
			this->delUser(user_map, chans);
			return true;
		}
	}else if(rc ==0){
		this->delUser(user_map, chans);
		return true;
	}

	std::string b(buf);
	//std::cout << b << std::endl;
	return this->addToBuffer(b, user_map, chans);


}

int getCommandType(std::string cmd){
	std::locale loc;
	for (std::string::size_type i=0; i<cmd.length(); ++i)
		cmd[i] =  std::toupper(cmd[i],loc);

	if("USER" == cmd){
		return 1;
	}
	if ("PRIVMSG" == cmd){
		return 2;
	}
	return 0;
}


void User::privMsg(std::string& comand,
		std::unordered_map<std::string, User *>& user_map,
		std::unordered_map<std::string, Chan>& chans) {

		auto channel = comand.substr(comand.find(" ") + 1);
		if(channel[0] != '#'){
			std::string user = channel.substr(0, channel.find(" "));
			std::string msg = channel.substr(channel.find(" ") + 1);
			if(user_map.count(user) > 0){
				user_map[user]->addMsg(this->username + "> " + msg + "\n");
				this->addMsg(this->username + "> " + msg + "\n");
			}else{
				this->addMsg("User not found\n");
			}
		}
}

//called with command to processes
//the list of users one is with sockets one is with
//maping to the users name
//the user dissconnects tiecoon
bool User::processesComand(std::string& comand,
		std::unordered_map<std::string, User *>& user_map,
		std::unordered_map<std::string, Chan>& chans){
	//return true when leave so it can be properly removed
	std::cout << "COMMAND: " << comand << std::endl;

	std::string cmd = comand.substr(0, comand.find(" "));

	std::cout << "CMD: " << cmd << std::endl;
	int c = getCommandType(cmd);
	std::cout << "CMD: " << c << std::endl;
	switch(c){
		case 1:
			this->username = comand.substr(comand.find(" ") + 1);
			std::cout << "USER: " << this->username << std::endl;
			user_map[this->username] = this;
			break;
		case 2:
			this->privMsg(comand, user_map, chans);
			break;
		case 0:
			addMsg("sorry nothing ready yet" + this->username + "\n");

	}
	return false;
}
void User::delUser(std::unordered_map<std::string, User *>& user_map,
		std::unordered_map<std::string, Chan>& chans){
	//TODO part from all channels with quit message

	std::cout << "User parted" << std::endl;
	//remove from lists
	user_map.erase(this->username);
	close(this->socket);
	delete this;

}

