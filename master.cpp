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
#include <chrono>
#include <unordered_set>
#include <unordered_map>

#include "helpers.h"

typedef std::shared_ptr<location> loc_ptr;

// globals for book keeping
class bookkeeping{
public:
	// worker information
	std::unordered_set<int> pids;  // keep track of worker pids
	std::unordered_map<int, loc_ptr> places;  // keep track of worker ports

	bookkeeping(){}
};

bookkeeping LOG;

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

int handleCreate(std::istringstream& iss, const int worker_pid, const int worker_port){
	// First check if we are trying to create a duplicate
	// No duplicate worker_pid
	// No duplicate worker_port
	std::vector<std::string> record(NUM_CREATE_PARAM);
	size_t index = 0;
	while(index < NUM_CREATE_PARAM && std::getline(iss, record[index++], ',')){}
	if(index != NUM_CREATE_PARAM) return 0;
	
	// add the new loc to a container
	LOG.pids.insert(worker_pid);
	// https://stackoverflow.com/questions/36321153/c-smart-pointers-in-a-vector-container/36321243
	LOG.places[worker_port] = std::make_shared<location>(  // better, less cache miss
										record[0],
										(unsigned int)stoi(record[1]),
										(unsigned int)stoi(record[2]),
										record[3]);
	

	// send a confirmation to worker
	const std::string msg = std::to_string(getpid()) + "," + std::to_string(MASTER_WORKER_PORT)
											   + ",registered@";
	send_message("localhost", worker_port, msg.c_str());
	return 1;	
}

bool validWorkerPort(){
	// check if this port is not equal to any of master's ports
	// check that this port is not negative
	return true;
}

void parse_and_handle(const char* msg){
	// Parse the data into commands
    std::istringstream iss(msg);
	int worker_pid, worker_port;
    CMD command;
    std::string token;

	// First get the PID
	if(std::getline(iss, token, ',')){
		try{
			worker_pid = std::stoi(token);
			if(worker_pid < 0) throw; 
		}catch(...){
			paste("PID has to be non negative");
			return;
		}
    }else{
		perror("Invalid request format");
		return;
	}

	// Second get the worker port
	if(std::getline(iss, token, ',')){
		try{
			worker_port = std::stoi(token);
			if(!validWorkerPort()) throw; 
		}catch(...){
			paste("Port number is not valid");
			return;
		}
    }else{
		perror("Invalid request format");
		return;
	}

	// Third get the command and parse the rest data
    if(std::getline(iss, token, ',')){
        command = convert_CMD(token);
		switch (command){
		case(CREATE):
			handleCreate(iss, worker_pid, worker_port);
			break;
		case(HELP):
			break;
		}
    }else{
		perror("Invalid request format");
		return;
	}
}

// Listens to any commands and worker messages on MASTER_WORKER_PORT
int run_server(const int port, const int queue_size) {
	const int sockfd = make_tcp_conn(port, queue_size);
	if(sockfd == -1) return 1;

	while(true){
		const int connectionfd = accept(sockfd, 0, 0);
		if (connectionfd == -1) {
			perror("Error accepting connection");
			return 1;
		}
		// When recieved a task, create a thread handler to do it
		// Continue to accept tasks
		std::thread handler(recv_handle_connection, connectionfd, parse_and_handle);
		handler.detach();
	}
	return 0;
}

int main(int argc, const char **argv) {
	// ./master
	if(argc != 1){
		perror("Usage: ./master");
		return 1;
	}
	if(run_server(MASTER_WORKER_PORT, 10)) return 1;
	return 0;
}