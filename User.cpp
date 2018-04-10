#include "User.h"
#include <sys/socket.h>
#include <cstring>
#include <unordered_map>
#include <iostream>
#include <string>

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
	if (size < 0){
		std::cout << "Send Error Added back" << std::endl;
		this->sendQueue->push_front(s);
	}
	else if (len < size){
		std::cout << "added back" << std::endl;
		//add back to be sent again
		this->sendQueue->push_front(s.substr(len, size-len));
	}
}

bool User::addToBuffer(std::string& stuff, std::map<int, User *> users,
		std::unordered_map<std::string, User *>& user_map,
		std::unordered_map<std::string, Chan>& chans) {
	this->buffer += stuff;
	std::size_t found = this->buffer.find("\n");
	if(found != std::string::npos){
		std::string send = this->buffer.substr(0, found);
		bool res = this->processesComand(send, users, user_map, chans);
		this->buffer = this->buffer.substr(found+1);
		return res;
	}
	return false;

}

bool User::getMsg(std::map<int, User *> users,
		std::unordered_map<std::string, User *>& user_map,
		std::unordered_map<std::string, Chan>& chans){
	char buf[513] = {0};
	int rc = recv(this->socket, buf, 512-this->getBufLen(), 0);
	std::string b(buf);
	//std::cout << b << std::endl;
	return this->addToBuffer(b, users, user_map, chans);


}

int getCommandType(std::string cmd){
	std::locale loc;
	for (std::string::size_type i=0; i<cmd.length(); ++i)
		cmd[i] =  std::toupper(cmd[i],loc);

	if("USER" == cmd){
		return 1;
	}
	return 0;
}

//called with command to processes
//the list of users one is with sockets one is with
//maping to the users name
//only reason the user to socket map here is if
//the user dissconnects tiecoon
bool User::processesComand(std::string& comand,std::map<int, User *> users,
		std::unordered_map<std::string, User *>& user_map,
		std::unordered_map<std::string, Chan>& chans){
	//return true on user command so I can set the USER table
	std::cout << "COMMAND: " << comand << std::endl;

	std::string cmd = comand.substr(0, comand.find(" "));

	std::cout << "CMD: " << cmd << std::endl;
	int c = getCommandType(cmd);
	std::cout << "CMD: " << c << std::endl;
	switch(c){
		case 1:
			this->username = comand.substr(comand.find(" ") + 1);
			std::cout << "USER: " << this->username << std::endl;
			return true;
			break;

		case 0:
			addMsg("sorry nothing ready yet" + this->username + "\n");

	}
	return false;
}
