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
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define STDIN 0  // file descriptor for standard input

#define PORT "9034" // the port client will be connecting to 

#define MAXDATASIZE 50 // max number of bytes we can get at once
#define BOILER "boiler.txt" // current state
#define LIGHT "light.txt" // current state

int sockfd, numbytes, duration_in;  
char buf[MAXDATASIZE], s[INET6_ADDRSTRLEN], cmd_in[10];
struct addrinfo hints, *servinfo, *p;
int rv, nbytes;

void data_handler(char *cmd_in)
{
	if(!strcmp(cmd_in,"b_on") || !strcmp(cmd_in,"b_off") || !strcmp(cmd_in,"sw_on")  || !strcmp(cmd_in,"sw_off"))
	{	
		send(sockfd, cmd_in, sizeof(cmd_in), 0);
		printf("[User] Data sent to server.\n");
		sleep(0.1);
		if ((nbytes = recv(sockfd, buf, sizeof buf, 0)) <= 0)
		{
			if (nbytes == 0) 
				{
					// connection closed
					printf("Socket %d hung up\n", sockfd);
				} else
				{
					perror("recv");
				}
		}
		else
		{
			printf("[Server] %s\n", buf);
		}
	}
	else
	{
		printf("[User] Invalid input, try again.\n");
	}	
}

// get sockaddr, IPv4 or IPv6:
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

	if (!(argc == 2 || argc == 3)) {
	    fprintf(stderr,"[Usage] <exe> <hostname> opt<command>\n");
		printf("[Commands] b_on , b_off , sw_on , sw_off\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("[User] socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("[User] connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "[User] failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("[User] connected to %s\n", s);

	freeaddrinfo(servinfo); // done 

	printf("[Commands] b_on, b_off, sw_on, sw_off\n");

	if (argc == 3) {
		data_handler(argv[2]);
	}
	
	for(;;)
	{
		//wait for inputs from client
		printf("Desired command: ");
		scanf("%s", cmd_in);
		data_handler(cmd_in);	
	}
	close(sockfd);

	return 0;
}

