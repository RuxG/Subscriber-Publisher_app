# Subscriber-Publisher_app

## Short description of application
This project is a small publisher-subscriber application, in which the communication is done via TCP and UDP. 

### Subscriber (TCP client)
This subscriber client can subscribe to different messaging topics and recieve notifications with the messages that are published. 

The TCP clients support **duplex communication** with the server.

### Publisher (UDP client) 
The udp clients send messages (**UDP datagrams**) to the server, under different topics. 

### Server
* The server acts as a **broker between the subscribers and the publisher**. It manages the TCP and UDP connections, recieves messages from the publishers and delivers them to the subscribers. 

* The server supports **two types of communication**: 

	- **simplex** - recieve datagrams from UDP clients
	- **duplex** - send / recieve data to / from TCP clients 

* The server supports **multiple clients**, by using multiplex socket communication.

* The server implements a **store&forward** feature: while the clients are disconnected, the server can store messages and deliver them to the clients when they
reconnect.

### Notes

* Running the code

	1. ./subscriber {id_user} {ip_server} {port_server} 
	2. ./server {port_server}
	3. python3 udp_client.py {ip_server} {port_server}
		
* Subscriber commands
	
	1. subscribe {topic} {SF}  	(SF = store&forward - 0/1 value that enables the feature to store messages while the clients are disconected) 
	2. unsubscribe {topic}
	3. exit
