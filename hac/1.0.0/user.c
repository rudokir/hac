#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

// File descriptor for standard input
#define STDIN 0

// Default connection settings
#define PORT "8080" 		// Server port
#define DEFAULT_HOST "127.0.0.1"// Default to localhost
#define MAXDATASIZE 128 	// Maximum bytes for data trasfer

// State files
#define BOILER "boiler.txt" 	// Boiler state file
#define LIGHT "light.txt" 	// Light state file

// Global variables for network communication
int sockfd, numbytes, duration_in;
char buf[MAXDATASIZE], s[INET6_ADDRSTRLEN], cmd_in[10];
struct addrinfo hints, *servinfo, *p;
int rv, nbytes;

/**
 * Handles command processing and server communication
 * @param cmd_in Command string to process
 */
void data_handler(char *cmd_in)
{
	// Validate command format
	if(!strcmp(cmd_in,"b_on") || !strcmp(cmd_in,"b_off") || !strcmp(cmd_in,"sw_on")  || !strcmp(cmd_in,"sw_off")) {
		// Send command to server
		send(sockfd, cmd_in, sizeof(cmd_in), 0);
		printf("[User] Data sent to server.\n");
		sleep(0.1);

		// Receive server response
		if ((nbytes = recv(sockfd, buf, sizeof buf, 0)) <= 0) {
			if (nbytes == 0) {
				printf("Socket %d hung up\n", sockfd);
			} else {
				perror("recv");
			}
		} else {
			printf("[Server] %s\n", buf);
		}
	} else {
		printf("[User] Invalid input, try again.\n");
	}
}

/**
 * Gets the appropriate address structure for IPv4 or IPv6
 * @param sa Socket address structure
 * @return Pointer to the appropriate address structure
 */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	FILE *boiler, *light;
	char b_state[MAXDATASIZE], sw_state[MAXDATASIZE];
	char *hostname;

	// Check command line arguments
	if (argc == 1) {
		// No arguments provided, use default localhost
		hostname = DEFAULT_HOST;
	} else if (argc == 2) {
		// Custom hostname provided
		hostname = argv[1];
	} else if (argc == 3) {
		// Hostname and commmand provided
		hostname = argv[1];
	} else {
		fprintf(stderr, "[Usage] %s [hostname] [command]\n", argv[0]);
		printf("[Commands] b_on, b_off, sw_on, sw_off\n");
		exit(1);
	}

	// Initialize connection hints
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;		// Allow both IPv4 and IPv6
	hints.ai_socktype = SOCK_STREAM;	// TCP stream socket

	// Get address information
	if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// Loop through all results and connect to the first available
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// Create socket
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("[User] socket");
			continue;
		}

		// Connect to server
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("[User] connect");
			close(sockfd);
			continue;
		}

		break; // Successfully connected
	}

	// Check if connection was successful
	if (p == NULL) {
		fprintf(stderr, "[User] failed to connect\n");
		return 2;
	}

	// Display connection information
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("[User] connected to %s\n", s);

	freeaddrinfo(servinfo); // Free the linked list

	printf("[Commands] b_on, b_off, sw_on, sw_off\n");

	// Handle command if provided as argument
	if (argc == 3) {
		data_handler(argv[2]);
	}

	// Main command loop
	for(;;) {
		printf("Desired command: ");
		if (scanf("%s", cmd_in) != 1) {
			printf("[User] Error reading command\n");
			// Clear input buffer in case of error
			int c;
			while ((c = getchar()) != '\n' && c != EOF);
			continue;
		}
		data_handler(cmd_in);
	}
	close(sockfd);

	return 0;
}
