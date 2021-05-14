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


#include "structure.h"

using std::cout;


int main(int argc, const char **argv) {
	if(argc != 6) paste("Need 6 arguements");
	int own_port = atoi(argv[1]);
	/* On init, worker needs to register with the master */
	
	// Send a CREATE message to the parent
	std::ostringstream oss;
	oss << getpid() << "," << argv[1] << ",CREATE";
	for(size_t i=2; i<6; ++i) oss << "," << argv[i];
	oss << '\0';  // Will always format correctly with the \0
	if(send_message("localhost", MASTER_WORKER_PORT, oss.str().c_str()) == -1) return 1;
	
	// Recieve a confirmation that this was accepted
	
	return 0;
}