#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

void usage() {
	fprintf(stderr, "Usage:  {ID - max 10 characters} {server_address} {server_port}\n");
	exit(0);
}

void commandUsage() {
	fprintf(stderr, "Commands: \n{subscribe} {alphanumeric topic} {0/1}\n{unsubscribe} {alphanumeric topic}\nexit\n");
}

int main(int argc, char *argv[]) {
	
	// verify input user
	if (argc < 4 || strlen(argv[1]) > 10) {
		usage();
	}

	int sockfd, n, ret;
	struct sockaddr_in serv_addr;

	char readBuff[BUFLEN];
	char sendBuff[BUFLEN];

	if (argc < 3) {
		usage();
	}

	fd_set read_fds;	
	fd_set tmp_fds;		
	int fdmax;			

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);	
	if(sockfd < 0) {
		fprintf(stderr, "Socket initialization failed.\n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	if(ret == 0) {
		fprintf(stderr, "Ip conversion failed.\n");
		return -1;
	}


	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	if(ret == -1) {
		fprintf(stderr, "Connection to server failed.\n");
		return -1;
	}

	// send ID to server and await confirmation
	message m;
	m.connect = true;	

	memcpy(m.id, argv[1], strlen(argv[1]));
	m.id[strlen(argv[1])] = '\0';
	n = send(sockfd, &m, MESSAGE_LEN, 0);

	if(n == -1) {
		fprintf(stderr, "Could not send id to server.\n");
		return -1;
	}

	// await for ID confirmation from server
	n = recv(sockfd, readBuff, sizeof(readBuff), 0);

	fprintf(stderr, "%s\n", readBuff);
	if(strcmp(readBuff, "ID already in use.") == 0) {
		return 0;
	}

	// add I/O and server socket in set
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	int surplus = 0;
	char surplusBuf[BUFLEN];
	memset(surplusBuf, BUFLEN, 0);

	while (1) {
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		// message was recieved from STDIN
		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {

			// create a new message and set the message type to "not_conection"
  			struct message m;
  			m.connect = false;
  			// set the client id
  			memcpy(m.id, argv[1], strlen(argv[1]));
  			m.id[strlen(argv[1])] = '\0';


  			// clear the reading buffer and get a line
			memset(readBuff, 0, BUFLEN);
			fgets(readBuff, BUFLEN - 1, stdin);

			// counter for parsing
			int count = 0;

  			char* pch;
  			pch = strtok (readBuff," \n");
  			bool ok = true;

  			while (pch != NULL && ok) {	
  				count++;
  				switch(count) {
  					case 1:
  						if (strcmp(pch, "subscribe") == 0 && strlen(pch) == 9) {
  							m.type = 's';
  						} else if (strcmp(pch, "unsubscribe") == 0 && strlen(pch) == 11) {
  							m.type = 'u';
  							m.SF = -1;
  						} else if (strncmp(pch, "exit", 4) == 0 && strlen(pch) == 4) {
							close(sockfd);
							exit(0);
						} else {
							// clear remaining data from buffer
							memset(readBuff, 0, BUFLEN);	
							ok = false;
						}
  						break;

  					case 2:
  						if(strlen(pch) > 50) {
  							memset(readBuff, 0, BUFLEN);
							ok = false;
							break;
  						}

  						// set message topic
  						memcpy(m.topic, pch, strlen(pch));
  						m.topic[strlen(pch)] = '\0';

  						break;

  					case 3:
  						if(strlen(pch) > 1 || (pch[0] != '0' && pch[0] != '1')) {
  							memset(readBuff, 0, BUFLEN);
							ok = false;
							break;
  						}
  						
  						m.SF = pch[0];

  						break;
  				}

    			pch = strtok (NULL, " \n");
  			}

  			// print a message if the command was erronated
  			if((count < 3 && m.type == 's') || !ok) {
  				commandUsage();
  			} else {
  				// send the message to server

  				memcpy(sendBuff, &m, MESSAGE_LEN);

  				n = send(sockfd, sendBuff, MESSAGE_LEN, 0);

  				if(m.type == 's') {
  					std::cout<<"subscribed topic.\n";
  				} else {
  					std::cout<<"unsubscribed topic.\n";
  				}

				if(n < 0) {
					fprintf(stderr, "Could not send subscribtion to server\n");
				}
  			}
			

			
		// message was recieved from server
		} else if(FD_ISSET(sockfd, &tmp_fds)) {
			// clear the reading buffer
			memset(readBuff, 0, BUFLEN);

			// read 4 bytes corresponding to message length
			int left_to_arrive = 4;
			// set initial offset to 0
			int offset = 0;

			while(left_to_arrive > 0) {
				n = recv(sockfd, readBuff + offset , left_to_arrive, 0);
				if(n > 0) {
					offset += n;
					left_to_arrive -= n;
				} else {
					if(n == 0) {
						close(sockfd);
						return 0;
					} else {
						break;
					}
				}			
			}
			
			// message length was recieved, get start adress of message
			struct messageToClient* message = (struct messageToClient*) readBuff;

			int len = message->len;

			// get offset 
			offset = 4;

			// bytes left to arrive from message
			left_to_arrive = len;

			// wait untill all the bytes are delivered
			while(left_to_arrive > 0) {
				n = recv(sockfd, readBuff + offset , left_to_arrive, 0);
				if(n > 0) {
					offset += n;
					left_to_arrive -= n;
				} else {
			
						break;
					
				}			
			}

			// print the message
			printf("%s", message->payload);

		}
		
	}
	close(sockfd);

	return 0;
}