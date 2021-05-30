#include <arpa/inet.h>		// htons(), ntohs()
#include <netdb.h>		// gethostbyname(), struct hostent
#include <netinet/in.h>		// struct sockaddr_in
#include <stdio.h>		// perror(), fprintf()
#include <string.h>		// memcpy()
#include <sys/socket.h>		// getsockname()
#include <unistd.h>		// stderr

#include "structure.h"

int make_server_sockaddr(struct sockaddr_in *addr, int port) {
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = INADDR_ANY;
	addr->sin_port = htons(port);
	return 0;
}

int make_client_sockaddr(struct sockaddr_in *addr, const char *hostname, int port) {
	addr->sin_family = AF_INET;
	struct hostent *host = gethostbyname(hostname);
	if (host == nullptr) {
		fprintf(stderr, "%s: unknown host\n", hostname);
		return -1;
	}
	memcpy(&(addr->sin_addr), host->h_addr, host->h_length);
	addr->sin_port = htons(port);
	return 0;
}

 int get_port_number(int sockfd) {
 	struct sockaddr_in addr;
	socklen_t length = sizeof(addr);
	if (getsockname(sockfd, (sockaddr *) &addr, &length) == -1) {
		perror("Error getting port of socket");
		return -1;
	}
	return ntohs(addr.sin_port);
 }


int send_message_tcp(const char *hostname, int port, const char *message) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("Failure on opening socket for sending UDP message");
		return -1;
	}
	struct sockaddr_in addr;
	if (make_client_sockaddr(&addr, hostname, port) == -1) {
		return -1;
	}
	if (connect(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Error connecting stream socket");
		return -1;
	}
	if (send(sockfd, message, strlen(message), 0) == -1) {
		perror("Error sending on stream socket");
		return -1;
	}
	close(sockfd);
	return 0;
}

int send_message_udp(const char *hostname, int port, const char *message) {
	paste("here");
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		perror("Failure on opening socket for sending UDP message");
		return -1;
	}
	struct sockaddr_in addr;
	if (make_client_sockaddr(&addr, hostname, port) == -1) {
		return -1;
	}
	if (connect(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Error connecting stream socket");
		return -1;
	}
	if (send(sockfd, message, strlen(message), 0) == -1) {
		perror("Error sending on stream socket");
		return -1;
	}
	close(sockfd);
	return 0;
}

// Recieves message and handles it
void recv_handle_connection(int connectionfd, void (*handle_func)(const char* msg)) {	
    // Initialize the buffer that the socket data is reading into
    char msg[MAX_MESSAGE_SIZE+1];  // +1 to allow space for accepting the delimiter
	memset(msg, 0, sizeof(msg));

    // Recieve the data into the buffer
	size_t recvd = 0;
	ssize_t rval;
    bool found_delim = false;
	do{
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
	}while(rval > 0 && recvd <= MAX_MESSAGE_SIZE && !found_delim);  // recv() returns 0 when client closes

    // Check if we found the deliminator
    if(!found_delim){
		perror("No delimiter found");
        close(connectionfd);
        return;
    }

	msg[recvd-1] = '\0';
    
	handle_func(msg);

	close(connectionfd);

	return;
}

// Creates a TCP connection on the port passed in having that queue_size
int make_tcp_conn(int port, const int queue_size){
	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) {
		perror("Error opening TCP stream socket");
		return -1;
	}
	int yesval = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
		perror("Error setting TCP socket options");
		return -1;
	}
	struct sockaddr_in addr;
	if(make_server_sockaddr(&addr, port) == -1) {
		return -1;
	}
	if(bind(sockfd, (sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Error binding stream socket");
		return -1;
	}
	port = get_port_number(sockfd);
	printf("TCP Server listening on port %d...\n", port);
	listen(sockfd, queue_size);
	return sockfd;
}