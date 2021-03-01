# 321CA Grigorie Ruxandra
--------------------------

## README
-----------

### Files:
-----------

	* subscriber.cpp
	* server.cpp
	* helpers.h 
	* Makefile
	* README

### Implementation details
---------------------------

	1) Server
	----------
	
	* The server supports two types of communication: 

		** simplex - recieve datagrams from UDP clients
		** duplex - send / recieve data to / from TCP clients 

	* The server supports multiple clients, by using multiplex socket communication.


	* Communication with UDP clients
	---------------------------------
	
		1) The server recieves datagrams (newsletters) from the UDP clients.
 
		2) If there are any subscribers to that newsletter, the server parses the data according to
		the desired display format.

		3) The server sends the formatted message to all active clients subscribed to that newsletter.

		4) If there are any inactive subscribers, the server stores the messages in a reservered queue for
		each client, and forwards the messages when they become active. 


	* Communication with TCP clients
	---------------------------------

		
		** Connection
		--------------
			
			1) The server listens for upcoming connections.
			
			2) The server accepts a socket connection from a client, and
			stores his socket in a map. The server resumes it's activity,
			following to resume the processing of the connection request 
			when it recieves an ID from the client on his socket. 

			3) If the recieved ID was not used before, the connection is accepted. 
			If the ID was used before, it accepts the connection request only if 
			the client is marked as inactive, i.e. is disconnected.

			4) The server sends a message to the client: "ID accepted"
			(if the connection request was accepted) / "ID already in use"
			(if the connection request was denied).

	
		** Clients data mannagement
		----------------------------
			
			*** The clients's data is stored as follows:

				1) clientSocket -- mapping between a client's socket and id

				2) client -- mapping between a client's id and it's info: 
				status {active/inactive}, socket and stored messages			
	
				3) newsletters -- mapping between newsletters and subscriber's ids


		** Client -> Server data flow
		------------------------------
				
			** The server accepts 3 types of messages from a TCP client:
			
				1) ID message -- used in the connection process

				2) subscribe message -- create an entry for the newsletter
				(if the subscribtion topic does not exit yet) and add the 
				client as a subscriber

				3) unsubscribe message -- remove the client as a subscriber
				from the newsletter (if the newsletter exists and the client
				is indeed a subscriber)

			** Data shared from the TCP clients is organized in the following structure:

				struct message {
					bool connect;
					char id[11];
					char type;   	
					char topic[51]; 
					char SF;
				};
		
				1) connect -- distinguishes the type of messages: connection
				or subscribe / unsubscribe

				2) id -- client ID
			
				3) type -- 's' (subscribe) or 'u' (unsubscribe)

				4) topic -- newsletter name

				** this structure has a fixed length of "MESSAGE_LEN" (65) bytes;

		
		** Server-> Client data flow
		------------------------------

			** The server sends two types of messages to a TCP client:
				
				1) connection accept/deny -- {ID accepted}/{ID already in use}

				2) newsletters messages	-- the server sends already formatted messages
				to it's clients, thus making the client "lightweight"; 
				when an "inactive" client reconnects to the server, the stored messages 
				are forwarded to the client, and then erased from memory.

			** Data shared from server is organized in the following structure:

				struct messageToClient {
					int len;
					char payload[BUFLEN];
				};

				1) len -- specifies the length of the message; it is used in the
				client for correctly parsing the recieved data.

				2) payload -- formatted message


	        ** Server shut down
		-------------------
				
			** The server deconnects all it's clients when it recieves the I/O "exit" command. 
	
	
	2) TCP clients
	--------------
	
	* The TCP clients support duplex communication with the server.

	* Accepted I/O commands:

		1) subscribe {topic} {SF}
		2) unsubscribe {topic}
		3) exit

	* Interraction with server: 
		
		** The connection process goes as described in server: after the socket connection
		is established, the clients sends his ID to the server and awaits confirmation. In 
		case of request denial, the client terminates the program.

		** The newsletter messages recieved from server are displayed as recieved, no
		further parsing is done.


	3) TCP message fragmentation/concatenation
	-------------------------------------------

	* In both server and TCP clients, a message "length" field is used for correctly interpreting
	whether all (or more than) expected data was recieved.

 	* To interpret concatenated data, the "len" field (in messageToClient structure) and "MESSAGE_LEN"  
	macro (the length of a struct message structure) are set as the reading buffer maximum length.
	This way, if more than one message arrived, they are processed one at a time, by reading at most
	it's length.

	* Fragmented data is identificated if less than "len" / MESSAGE_LEN  bytes were recieved from
	socket. Data is read from socket untill the expected quantity of data arrived.
	
	
			 

		
		


