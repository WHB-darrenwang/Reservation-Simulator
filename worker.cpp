#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include "helpers.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#include "structure.h"

using std::cout;

void parse_and_handle(const char* msg){
	paste(msg);
}

void handle_connection(int connectionfd) {
	printf("Connection to the Master on %d. Awaiting for commands.\n", connectionfd);
	
    // Initialize the buffer that the socket data is reading into
    char msg[MAX_MESSAGE_SIZE+1];
	memset(msg, 0, sizeof(msg));

    // Recieve the data into the buffer
	size_t recvd = 0;
	ssize_t rval;
    bool found_delim = false;
	do {
		rval = recv(connectionfd, msg + recvd, MAX_MESSAGE_SIZE+1 - recvd, 0);
		if (rval == -1) {
			perror("Error reading stream message");
			return;
		}
        // checking witihn the buffer if it ended (check for \0)
        for(size_t i=0; i<rval; ++i){
            if(msg[recvd+i] == '@'){
                found_delim = true;
                break;
            }
        }
		recvd += rval;
	} while (rval > 0 && recvd <= MAX_MESSAGE_SIZE && !found_delim);  // recv() returns 0 when client closes

    // Check if we found the deliminator
    if(!found_delim){
		paste("None");
        close(connectionfd);
        return;
    }
	msg[recvd-1] = '\0';
	printf("Client %d says %s\n", connectionfd, msg);
    
	parse_and_handle(msg);

	close(connectionfd);

	return;
}

void getInitialAck(int argc, const char **argv){
	/* On init, worker needs to register with the master */
	
	// Send a CREATE message to the parent
	// e.g PID,PORT,CREATE,name,long,lat,file@
	std::ostringstream oss;
	oss << getpid() << "," << argv[1] << ",CREATE";
	for(size_t i=2; i<6; ++i) oss << "," << argv[i];
	oss << '@';  
	if(send_message("localhost", MASTER_WORKER_PORT, oss.str().c_str()) == -1) return;
	paste("Sent");
}

void listen_port(int port, int queue_size, int argc, const char **argv){
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Error opening stream socket");
		return;
	}
	int yesval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
		perror("Error setting socket options");
		return;
	}
	struct sockaddr_in addr;
	if (make_server_sockaddr(&addr, port) == -1) {
		return;
	}
	if (bind(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Error binding stream socket");
		return;
	}
	port = get_port_number(sockfd);
	printf("Server listening on port %d...\n", port);

	listen(sockfd, queue_size);
	
	std::thread ack_handler(getInitialAck, argc, argv);
	ack_handler.detach();

	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		int connectionfd = accept(sockfd, 0, 0);
		if (connectionfd == -1) {
			perror("Error accepting connection");
			return;
		}
		std::thread handler(handle_connection, connectionfd);
		handler.detach();
	}
}



int main(int argc, const char **argv) {
	if(argc != 6){
		paste("Need 6 arguements");
		return 0;
	}

	int own_port;
	try{
		own_port = atoi(argv[1]);
		listen_port(own_port, 10, argc, argv);
	}catch(...){
		paste("Invalid port");
		return 0;
	}
	
	return 0;
}