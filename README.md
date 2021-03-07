# Subscriber-Publisher_app

## Short description of application
This project is a small publisher-subscriber application, in which the communication is done via TCP and UDP. 

## Features

* The **TCP client** can **subscrib**e to different messaging topics and recieve the messages that are published. 

* The server supports **multiple clients**, by using multiplex socket communication.

* The server supports **two types of communication**: 
	- **simplex** - recieve datagrams from UDP clients
	- **duplex** - send / recieve data to / from TCP clients 
	
* The server implements a **store&forward** feature: while the clients are disconnected, the server can store messages and deliver them to the clients when they
reconnect.
