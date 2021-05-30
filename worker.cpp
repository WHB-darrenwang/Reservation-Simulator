#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#include "helpers.h"

using std::cout;

void run_heartbeat_udp(const int port, const int queue_size){
	// every 5 seconds send a heatbeat
	while(true){
		send_message_udp("localhost", 8083, "0,7777,heartbeat");
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));  // sleep for 5 seconds
	}
}

void parse_and_handle(const char* msg){
	// if we got a registered, start a UDP for heartbeat
	paste(msg);
	std::thread heartbeat_hander(run_heartbeat_udp, 8000, 10);
	heartbeat_hander.detach();
}

void request_registration(int argc, const char **argv){
	/* On init, worker needs to register with the master */
	
	// Send a CREATE message to the parent
	// e.g PID,PORT,CREATE,name,long,lat,file@
	std::ostringstream oss;
	oss << getpid() << "," << argv[1] << ",CREATE";
	for(size_t i=2; i<6; ++i) oss << "," << argv[i];
	oss << '@';  
	if(send_message_tcp("localhost", MASTER_WORKER_PORT, oss.str().c_str()) == -1){
		perror("Worker could not send a registration message to master");
		return;
	}
}

int listen_port(int port, int queue_size, int argc, const char **argv){
	const int sockfd = make_tcp_conn(port, queue_size);
	if(sockfd == -1) return 1;
	
	// After establishing a listening port, send a registration message
	std::thread registeration(request_registration, argc, argv);
	registeration.detach();

	while(true){
		const int connectionfd = accept(sockfd, 0, 0);
		if (connectionfd == -1) {
			perror("Error accepting connection");
			return 1;
		}
		std::thread handler(recv_handle_connection, connectionfd, parse_and_handle);
		handler.detach();
	}
	return 0;
}



int main(int argc, const char **argv) {
	// ./worker WORKER_PORT name longitude latitude filename
	if(argc != 6){
		perror("Usage: ./worker WORKER_PORT name longitude latitude filename");
		return 0;
	}
	int own_port;
	try{
		own_port = atoi(argv[1]);
		if(listen_port(own_port, 10, argc, argv)) return 1;
	}catch(...){
		perror("Invalid worker port");
		return 1;
	}
	return 0;
}