#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <functional>

#include "helpers.h"
#include "structure.h"

typedef std::shared_ptr<location> loc_ptr;


// globals for book keeping
// probablly map pid

enum CMD{
    CREATE,     // Create and register a new location
    REMOVE,     // Remove a location from book keeping
    RESERVE,    // Make a reservation for that location
    STREAM,     // Start to stream reservations frequencies
    LIST,       // Send a list of locations and surface information on it
	HELP,		// Default
};

// retruns the enum from CMD given a string
CMD convert_CMD(const std::string& command){
    if	   (command == "CREATE") return CREATE;
    else if(command == "REMOVE") return REMOVE;
    else if(command == "RESERVE") return RESERVE;
    else if(command == "STREAM") return STREAM;
    else if(command == "LIST") return LIST;
	else return HELP;
}

int handleCreate(std::istringstream& iss, const int worker_port){
	std::vector<std::string> record(NUM_CREATE_PARAM);
	size_t index = 0;
	while(index < NUM_CREATE_PARAM && std::getline(iss, record[index++], ',')){}
	if(index != NUM_CREATE_PARAM) return 0;

	loc_ptr loc = loc_ptr(new location(record[0],
									   (unsigned int)stoi(record[1]),
									   (unsigned int)stoi(record[2]),
									   record[3]));
	
	// add the new loc to a container
	// send a confirmation to worker
	return 1;
	
}

void parse_and_handle(const char* msg){
	// Parse the data into commands
    std::istringstream iss(msg);
	int pid, worker_port;
    CMD command;
    std::string token;

	// First get the PID
	if(std::getline(iss, token, ',')){
		try{
			pid = std::stoi(token);
			if(pid < 0) throw; 
		}catch(...){
			paste("PID has to be non negative");
		}
    }

	// Second get the worker port
	if(std::getline(iss, token, ',')){
		try{
			worker_port = std::stoi(token);
			if(worker_port < 0 || worker_port == MASTER_WORKER_PORT) throw; 
		}catch(...){
			paste("Port number is not valid");
		}
    }

	// Third get the command and parse the rest data
    if(std::getline(iss, token, ',')){
        command = convert_CMD(token);
		switch (command){
		case(CREATE):
			handleCreate(iss, worker_port);
			break;
		default:
			std::cout << "Unkown request\n";
			break;
		}
    }
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
		rval = recv(connectionfd, msg + recvd, MAX_MESSAGE_SIZE - recvd, 0);
		if (rval == -1) {
			perror("Error reading stream message");
			return;
		}
        // checking witihn the buffer if it ended (check for \0)
        for(size_t i=0; i<rval; ++i){
            if(msg[recvd+i] == '\0'){
                found_delim = true;
                break;
            }
        }
		recvd += rval;
	} while (rval > 0 && recvd < MAX_MESSAGE_SIZE && !found_delim);  // recv() returns 0 when client closes

    // Check if we found the deliminator
    if(!found_delim){
		paste("None");
        close(connectionfd);
        return;
    }
	printf("Client %d says %s\n", connectionfd, msg);
    
	parse_and_handle(msg);

    

	close(connectionfd);

	return;
}

/**
 * Endlessly runs a server that listens for connections and serves
 * them _synchronously_.
 *
 * Parameters:
 *		port: 		The port on which to listen for incoming connections.
 *		queue_size: 	Size of the listen() queue
 * Returns:
 *		-1 on failure, does not return on success.
 */
int run_server(int port, int queue_size) {
	// (1) Create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Error opening stream socket");
		return -1;
	}

	// (2) Set the "reuse port" socket option
	int yesval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
		perror("Error setting socket options");
		return -1;
	}

	// (3) Create a sockaddr_in struct for the proper port and bind() to it.
	struct sockaddr_in addr;
	if (make_server_sockaddr(&addr, port) == -1) {
		return -1;
	}

	// (3b) Bind to the port.
	if (bind(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Error binding stream socket");
		return -1;
	}

	// (3c) Detect which port was chosen.
	port = get_port_number(sockfd);
	printf("Server listening on port %d...\n", port);

	// (4) Begin listening for incoming connections.
	listen(sockfd, queue_size);

	// (5) Serve incoming connections one by one forever.
	while (true) {
		int connectionfd = accept(sockfd, 0, 0);
		if (connectionfd == -1) {
			perror("Error accepting connection");
			return -1;
		}
		std::thread handler(std::bind(handle_connection, connectionfd));
		handler.detach();  // resources are freed when the thread finishes
	}
}

int main(int argc, const char **argv) {
	if (argc != 1) return 1;
	if (run_server(MASTER_WORKER_PORT, 10) == -1) return 1;
	return 0;
}