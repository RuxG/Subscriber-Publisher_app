# Subscriber-Publisher_app

## Short description of application
This project is a small publisher-subscriber application, in which the communication is done via TCP and UDP. 

The **three main parts** of the application are:

### Subscriber (TCP client)
This subscriber client can subscribe to different messaging topics and recieve notifications with the messages that are published. 

* The TCP clients support **duplex communication** with the server.

	* Accepted I/O commands:

		1. subscribe {topic} {SF}
		2. unsubscribe {topic}
		3. exit

	* Interraction with server: 
		
		1. The connection process goes as described in server: after the socket connection
		is established, the clients sends his ID to the server and awaits confirmation. In 
		case of request denial, the client terminates the program.

		2. The newsletter messages recieved from server are displayed as recieved, no
		further parsing is done.


### Publisher (UDP client) 
The publisher sends messages (corelated to different topics) to the server.


### Server

* The server acts as a **broker between the subscribers and the publisher**. It manages the TCP and UDP connections, recieves messages from the publishers and delivers them to the subscribers. 

* The server supports **two types of communication**: 

	- **simplex** - recieve datagrams from UDP clients
	- **duplex** - send / recieve data to / from TCP clients 

* The server supports **multiple clients**, by using multiplex socket communication.

* **Communication with UDP clients**
	
	1. The server recieves datagrams (newsletters) from the UDP clients.
 
	2. If there are any subscribers to that newsletter, the server parses the data according to the desired display format.

	3. The server sends the formatted message to all active clients subscribed to that newsletter.

	4. If there are any inactive subscribers, the server stores the messages in a reservered queue for each client, and forwards 
	the messages when they become active. 


* **Communication with TCP clients**

		
	1. **Connection**
			
		1. The server listens for upcoming connections.
			
		2. The server accepts a socket connection from a client, and
		stores his socket in a map. The server resumes it's activity,
		following to resume the processing of the connection request 
		when it recieves an ID from the client on his socket. 

		3. If the recieved ID was not used before, the connection is accepted. 
		If the ID was used before, it accepts the connection request only if 
		the client is marked as inactive, i.e. is disconnected.

		4. The server sends a message to the client: "ID accepted" (if the connection request was accepted) 
		/ "ID already in use" (if the connection request was denied).

	2. The server accepts **3 types of messages** from a TCP client:
			
		1. ID message - used in the connection process

		2. subscribe message - create an entry for the newsletter
		(if the subscribtion topic does not exit yet) and add the 
		client as a subscriber

		3. unsubscribe message - remove the client as a subscriber
		from the newsletter (if the newsletter exists and the client
		is indeed a subscriber)


* **Server shut down**
	
	* The server deconnects all it's clients when it recieves the I/O "exit" command. 

### Notes

	**Running the code**

		* ./subscriber {id_user} {ip_server} {port_server} 
		* ./server {port_server}
		* python3 udp_client.py {ip_server} {port_server}
		
	**Subscriber commands:
		* subscribe {topic} {SF}  	(SF = store&forward - 0/1 value that enables the feature to store messages while the clients are disconected) 
		* unsubscribe {topic}
		* exit


