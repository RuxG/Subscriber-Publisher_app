#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <set>
#include <queue>

#define BUFLEN		1600	
#define MAX_CLIENTS	5		
#define MESSAGE_LEN 65

struct message {
	bool connect;	
	char id[11];
	char type;   	
	char topic[51]; 
	char SF;
};

struct messageToClient {
	int len;
	char payload[BUFLEN];
};

struct clientInfo {
	bool active;
	int socket;
	std::queue<struct messageToClient> stored;
};

struct newsletter {
	std::set<std::string> clientsSF;		// subscribtions marked with SF = 1
	std::set<std::string> clients;			// subscribtions marked with SF = 0
};


#endif
