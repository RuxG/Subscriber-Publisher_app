#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <netinet/tcp.h>
#include <signal.h>

#include <iostream>
#include <map>
#include <set>


int main(int argc, char* argv[]) {

	// mapping between newsletters name and clients's ids
	std::map<std::string, struct newsletter> newsletters;

	// mapping between clients's ids and status {active/inactive}, socket and queued messages
	std::map<std::string, struct clientInfo> client;

	// mapping between client's socket and id
	std::map<int, std::string> clientSocket;


	int udp_socket = -1, tcp_socket = -1, newsockfd = -1;
	int portno, ret;
	struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

 	// initialize tcp socket
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_socket == -1) {
        fprintf(stderr,"Could not create TCP socket in server.\n");
        return 1;
    }

    // deactivate Neagle algorithm
    int flag_delay = 1;
  	int tcp_no_delay = setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, &flag_delay, sizeof(int));

	// initialize udp socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_socket == -1) {
        fprintf(stderr,"Could not create UDP socket in server.\n");
        return 1;
    }

    portno = atoi(argv[1]);

    // Initialize server adress
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// bind TCP socket
	ret = bind(tcp_socket, (struct sockaddr *)(&serv_addr), socket_len);
	if(ret == -1) {
        fprintf(stderr,"Could not bind TCP socket to server.\n");
        exit(0);
    }

	/// bind UDP socket
	ret = bind(udp_socket, (struct sockaddr *)(&serv_addr), socket_len);
	if(ret == -1) {
        fprintf(stderr,"Could not bind UDP socket to server.\n");
        exit(0);
    }

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    char buffer[BUFLEN];

    // reading socket set
    fd_set read_fds;	
    // temporary socket set
	fd_set tmp_fds;		

	int fdmax;			

	// clear socket sets
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// listen TCP connections
	ret = listen(tcp_socket, MAX_CLIENTS);
	if(ret == -1) {
        fprintf(stderr,"Could not listen TCP socket.\n");
        exit(0);
    }

	// add sockets on which to await data
	FD_SET(tcp_socket, &read_fds);
	FD_SET(udp_socket, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	if(tcp_socket >= udp_socket) {
		fdmax = tcp_socket;
	} else {
		fdmax = udp_socket;
	}

	bool exit = false;

	while(!exit) {
		// clear and reinitialize temporary socket set
		FD_ZERO(&tmp_fds);
		tmp_fds = read_fds; 

		// select sockets on which data arrived	
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);

		if(ret == -1) {
        	fprintf(stderr,"Could not select socket.\n");
        	return -1;
    	}

		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
					// upcoming connection
					if (i == tcp_socket) {
					newsockfd = accept(tcp_socket, (struct sockaddr *) &cli_addr, &clilen);

					if(newsockfd == -1) {
						fprintf(stderr, "Could not accept connection.\n");
						continue;
					}
					// add new socket in set
					FD_SET(newsockfd, &read_fds);

					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					
				}  else if(i == udp_socket) {
					// upcoming datagram
					memset(buffer, 0, BUFLEN);

					int n = recvfrom(i, buffer, BUFLEN - 1, 0, (struct sockaddr *)&cli_addr, &clilen);

					if (n == 0) {
						// connection is closed
						close(i);
						FD_CLR(i, &read_fds);

					} else {
						// get subscribtion topic from datagram
						char topic[51];
						topic[50] = '\0';
						memcpy(topic, buffer, 50);

						// search topic in subscribtions table
						auto it = newsletters.find(topic);

						// if topic doesnt exit, ignore the datagram
						if(it == newsletters.end()) {
							continue;
						}

						// get subscribtion type from datagram
						u_char type = buffer[50];

						// build message to client
						struct messageToClient message;

						// variable to build the message to be sent to clients
						std::string formatted_message;

						// add udp client ip
						const char* ip = inet_ntoa(cli_addr.sin_addr);

						int len = strlen(ip);

						formatted_message.append(ip, len);

						// add udp client port number
						unsigned short port = ntohs(cli_addr.sin_port);

						formatted_message.append(":");

						formatted_message.append(std::to_string(port));
					
						const char *space_dot = " - ";

						formatted_message.append(" - ");

						// add udp client topic
						int len2 = strlen(topic);
						formatted_message.append(topic, len2);

						formatted_message.append(" - ");

						// format message for each type of data
						switch(type) {
							case 0: {

								u_char sign = buffer[51];
								unsigned int number;

								memcpy(&number, buffer + 52, 4);
								number = ntohl(number);
			
								const char* data_type = "INT";

								formatted_message.append("INT");

								formatted_message.append(" - ");

								if(sign == '0') {
									formatted_message.append(std::to_string(number));
								} else {
									formatted_message.append("-");
									formatted_message.append(std::to_string(number));
								}

								break;
							}
								
							case 1: {

								short number;
								memcpy(&number, buffer + 51, 2);
								number  = ntohs(number);

								float real = 1.0 * number / 100;

								formatted_message.append("SHORT_REAL");

								formatted_message.append(" - ");

								formatted_message.append(std::to_string(real));

								break;
							}
								
							case 2: {

								u_char sign = buffer[51];
								unsigned int number;

								memcpy(&number, buffer + 52, 4);

								number = ntohl(number);

								u_char pow = buffer[56];

								float real = number;
								for(int i = 0; i < pow; i++) {
									real /= 10;
								}

								formatted_message.append("FLOAT");

								formatted_message.append(" - ");

								if(sign == '0') {
									formatted_message.append(std::to_string(real));
								} else {
									formatted_message.append("-");
									formatted_message.append(std::to_string(real));
								}

								break;
							}
								
							case 3: {
						
								formatted_message.append("STRING");

								formatted_message.append(" - ");

								formatted_message.append(buffer + 51, strlen(buffer + 51));


								break;
							}

							default: {
								fprintf(stderr, "Invalid data type.\n");
								break;
							}
						}

						formatted_message.append("\n");
						formatted_message.append("\0");

						message.len = formatted_message.length();

						// copy data in message structure 
						const char* data = formatted_message.data();
						memcpy(message.payload, data, message.len);

						std::string t(topic);
								
						// search topic in subscribtions table
						it = newsletters.find(topic);

						// if topic exists and has clients, send message
						if(it != newsletters.end()) {

							std::set<std::string>::iterator f;
							for (f = it->second.clients.begin(); f != it->second.clients.end(); ++f) {
  								std::string id = *f;

  								auto it2 = client.find(id);

 								if(it2 != client.end() && it2->second.active) {
									int n = send(it2->second.socket, &message, message.len + 4, 0);

									int offset = n;
									int to_be_sent = message.len + 4  - n;

									while(to_be_sent > 0) {
										n = send(i, &message + offset, message.len + 4 - offset, 0);
										offset += n;
										to_be_sent -= n;
									}

								}

							}
									
							for(f = it->second.clientsSF.begin(); f != it->second.clientsSF.end(); ++f) {
 								std::string id = *f;

 								auto it2 = client.find(id);

 								if(it2 != client.end()) {
									if(it2->second.active) {
										int n = send(it2->second.socket, &message, message.len + 4 , 0);

										int offset = n;
										int to_be_sent = message.len + 4  - n;

										while(to_be_sent > 0) {
											n = send(i, &message + offset, message.len + 4 - offset, 0);
											offset += n;
											to_be_sent -= n;
										}

									} else {
										// if client is not active, store the message
										it2->second.stored.push(message);
									}
								}
							} 
															
						}
					}
				} else if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
					// data was recieved from STDIN
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					// check if command is "exit"
					if(strlen(buffer) > 5) {
						fprintf(stderr, "Command: exit\n");
					}

					buffer[4] = '\0';

					if(strcmp(buffer, "exit") == 0 && strlen(buffer) == 4) {
						exit = true;
						for(int i = 0; i < fdmax; i++) {
							if(FD_ISSET(i, &read_fds)) {
								close(i);
							}
						}
					} else {
						fprintf(stderr, "Command: exit\n");
					}

				} else { 
					// data was recieved from tcp clients
					memset(buffer, 0, BUFLEN);

					socklen_t len = sizeof(cli_addr);

					int n = recvfrom(i, buffer, MESSAGE_LEN, 0, (struct sockaddr *)&cli_addr, &len);

					// client disconnected
					if (n == 0) {

						auto ret = clientSocket.find(i);
						std::cout<<"Client "<<ret->second<<" disconnected.\n";

						close(i);
						
						FD_CLR(i, &read_fds);

						// get client id
						auto it1 = clientSocket.find(i);

						// find client
						auto it = client.find(it1->second);

						// set client status as "inactive"
						it->second.active = false;

						// delete socket
						auto itt = clientSocket.erase(i);

					} else {

						// get offset 
						int offset = n;

						// bytes left to arrive from message
						int left_to_arrive = MESSAGE_LEN - n;

						// wait untill all the bytes are delivered
						while(left_to_arrive > 0) {
							n = recv(i, buffer + offset , left_to_arrive, 0);
							if(n > 0) {
								offset += n;
								left_to_arrive -= n;
							} else {
								break;
							}			
						}

						struct message* m = (struct message*)(buffer);

						// connection request from client
						if(m->connect == true) {
								
							// search for existing client id 								
							std::string id(m->id);
							auto it = client.find(id);

							//if id already exists
							if(it != client.end()) {
								// if the client's status is "active", deny the connection request
								if(it->second.active == true) {
									const char* response = "ID already in use.";
									n = send(i, response, strlen(response), 0);

									// close and delete socket
									FD_CLR(i, &read_fds);
										close(i);

										break;
									} else {

										// send connection confirmation to client
										const char* response = "ID accepted.";
										n = send(i, response, strlen(response), 0);

										// reactivate client status
										it->second.active = true;

										// map client socket to client id
										clientSocket.insert(std::pair<int, std::string>(i, id));

										// print message
										std::cout<<"New client ("<<m->id<<") connected from "
										<<inet_ntoa(cli_addr.sin_addr)<<":"<<ntohs(cli_addr.sin_port)<<"\n";
										
										// send buffered data while client was inactive
										while(it->second.stored.size() != 0) {

											// get message
											struct messageToClient message = it->second.stored.front();
											it->second.stored.pop();

											// send data
											int to_be_sent = message.len + 4;

											int n = send(i, &message, message.len + 4, 0);

											int offset = n;
											to_be_sent = message.len + 4 - n;

											while(to_be_sent > 0) {
												n = send(i, &message + offset, message.len + 4 - offset, 0);
												offset += n;
												to_be_sent -= n;
											}
										}
									}
									
								} else {
									// new ID
									const char* response = "ID accepted.";

									// create new client entry and send confirmation
									struct clientInfo c;
									c.active = true;
									c.socket = i;
									std::string id(m->id);

									// map client socket to client id
									clientSocket.insert(std::pair<int, std::string>(i, id));

									// insert new client
									client.insert (std::pair<std::string, struct clientInfo>(id, c));

									// confirm connection
									n = send(i, response, strlen(response), 0);

									// print message
									std::cout<<"New client ("<<m->id<<") connected from "<<
									inet_ntoa(cli_addr.sin_addr)<<":"<<ntohs(cli_addr.sin_port)<<"\n";

								}
								
							} else {
								// subscribtion message 
								switch(m->type) {
									case 's': {
										std::string topic(m->topic);

										std::string id(m->id);

										// search topic in table
										auto it = newsletters.find(topic);
										
										// if topic doesnt exit, create it and add client 
										if(it == newsletters.end()) {

											struct newsletter newN;

											if(m->SF == '0') {
												auto itt2 = newN.clients.insert(id);
											} else {
												auto itt2 = newN.clientsSF.insert(id);
											}
											
											// create newsletter
											std::pair<std::string, struct newsletter> entry(topic, newN);
											auto it2 = newsletters.insert(entry);

										} else {

											// add client to newsletter
											if(m->SF == '0') {
												 auto it2 = newsletters[topic].clients.insert(id);
												
											} else {
												 auto it2 = newsletters[topic].clientsSF.insert(id);
												
											}
										}

										break;
									}
										
									case 'u': {
										std::string topic(m->topic);
										std::string id(m->id);

										// search topic in table
										auto it = newsletters.find(topic);

										// if topic exists
										if(it != newsletters.end()) {

											auto it2 = it->second.clients.find(id);
											if(it2 != it->second.clients.end()) {
												auto res = it->second.clients.erase(it2); 
											}

											auto it3 = it->second.clientsSF.find(id);
											if(it3 != it->second.clientsSF.end()) {
												auto res = it->second.clientsSF.erase(it3); 
											}

										}
										
										break;
									}
											
								}

						}
							
					}

				}
			} 
		}

	}
	return 0;
}