#include "User.h"
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <dns_sd.h>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unordered_map>
#include <map>
#include <sys/ioctl.h>
#include <string.h>
#include <iostream>

void Accept(int master_socket, std::map<int, User*>& users) {
	struct sockaddr_in addr;
	int addr_len = sizeof(addr);
	int new_socket = -1;
	do{
		new_socket = accept(master_socket, (struct sockaddr *)&addr, (socklen_t *)&addr_len);
		if (new_socket < 0){
			if (errno != EWOULDBLOCK) {
                perror("  accept() failed");
				exit(1);
			}break;
        }
		printf("new connection\n");
		User * user = new User(new_socket);
		users[new_socket] = user;
		int s = users[new_socket]->getSocket();
		printf("User %d\n", s);
	}while(new_socket != -1);

}

int main(int argc, char ** argv){
	//TODO add the shit for pass

	//all user and chan stuff
	std::unordered_map<std::string, User*> users_map;
	std::unordered_map<std::string, Chan> chans_map;
	std::map<int, User*> users; /* maps form socket to user */

	//make new socket
	struct sockaddr_in6 addr;
	int master_socket = -1;
	master_socket = socket(AF_INET6, SOCK_STREAM, 0);
	if(master_socket <= 0){
		perror("socket Failed\n");
		return EXIT_FAILURE;
	}
	//TODO make nonblocking chec err
	int opt = 1;
	ioctl(master_socket, FIONBIO, (char *)&opt);

	opt = 0;
	//set to not only ipv6 :)
    if (setsockopt(master_socket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) < 0) {
		perror("setsockopt Failed\n");
		return EXIT_FAILURE;
	}
	opt = 1;
	//allow mult connections
	if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0){
		perror("set socket opt Failed\n");
		return EXIT_FAILURE;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_addr = in6addr_any;
	addr.sin6_port = 0;

	//bind
	if(bind(master_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		perror("Bind Failed\n");
		return EXIT_FAILURE;
	}

	//listen
	if(listen(master_socket, 2) < 0){
		perror("Listen Failed\n");
		return EXIT_FAILURE;

	}
	//get info about the port
	int addr_len = sizeof(addr);
	getsockname(master_socket, (struct sockaddr *)&addr,(socklen_t *) &addr_len);

	int port = ntohs(addr.sin6_port);

	printf("PORT: %d\n", port);

	int max_sd, sd;
	fd_set readfds;
	fd_set writefds; /* only wait if something needs to be written */
	while(1){
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(master_socket, &readfds);
		max_sd= master_socket;
		//set the write and read for select
		for(auto it=users.begin(); it!=users.end(); ++it){
			if (it->second->needToWrite()){
				FD_SET(it->first, &writefds);
			}
			FD_SET(it->first, &readfds);
			if (it->first > max_sd) max_sd = it->first;
		}

		//call select
		int activity = select(max_sd + 1, &readfds, &writefds, 0, 0); /* TODO: might want to put actual timeout */
		if((activity < 0) && (errno!=EINTR)){
			printf("Error\n");
		}

		if(FD_ISSET(master_socket, &readfds)){
			//accept new connection
			Accept(master_socket, users);
		}
		for(auto it=users.begin(); it!=users.end();){
			sd = it->first;
			if (FD_ISSET(sd, &readfds)) {
				//processes msg if they set user add to table
				if(it->second->getMsg(users_map, chans_map)){
					it = users.erase(it);
					std::cout << users.size() << " " << users_map.size() <<
						std::endl;
					continue;
				}
			}
			if (FD_ISSET(sd, &writefds)) {
				it->second->sendMsg();
			}
			++it;
		}



	}

}
