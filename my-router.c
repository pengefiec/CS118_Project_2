#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */

const int ports[6] = {10000, 10001, 10002, 10003, 10004, 10005};

void error(char *msg)
{
    perror(msg);
    exit(1);
}

struct packet
{
	int source_port;
	int dest_port;
	char* msg;
};


int start_router(int port)
{
	signal(SIGPIPE, SIG_IGN);
	int sockfd;
	char buf[1024] = "hi";
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	struct sockaddr_in addr;
	bzero((char *) &addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0)
		error("error on binding");


	printf("Router waiting on port %d\n", port);

	// Send message test
	// if (port == 10001)
	// {
	// 	struct packet p;
	// 	p.source_port = port;
	// 	p.dest_port = 10000;
	// 	p.msg = "hello test";

	// 	struct sockaddr_in remote_addr;
	// 	socklen_t remote_addr_len = sizeof(struct sockaddr_in);
	// 	remote_addr.sin_family = AF_INET;
	// 	remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	// 	remote_addr.sin_port = htons(10000);

	// 	if (sendto(sockfd, &p, sizeof(p), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0)
	// 		error("error in sending message");
	// 	else
	// 		printf("Message: '%s' sent successfully!\n", p.msg);
	// }


	// Listening to receive message
	while (true)
	{
		struct sockaddr_in remote_addr;
		socklen_t remote_addr_len = sizeof(remote_addr);

		struct packet* received_packet = (struct packet*)malloc(sizeof(struct packet));
	
		int recvlen = recvfrom(sockfd, received_packet, sizeof(struct packet), 0, (struct sockaddr *) &remote_addr, &remote_addr_len);
		if (recvlen > 0)
		{
			buf[recvlen] = 0;
			printf("Message received!: %s\n", received_packet->msg);
		}
	}

}

int main(int argc, char *argv[])
{
	int i;

	//Create six routers with ports
	for (i = 0; i < 6; i++) 
	{
		int pid = fork();
		if (pid < 0)
			error("forking error");
		else if (pid == 0)
		{
			start_router(ports[i]);
		}
	}
	//start_router(10000);
}