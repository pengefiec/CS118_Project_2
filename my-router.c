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
const char ids[6] = {'A', 'B', 'C', 'D', 'E', 'F'};

typedef enum {CONTROL, DATA} packet_type;

typedef struct
{
	packet_type type;
	char* msg;
} Packet;

struct routing_table_row
{
	char destination;
	int cost;
	int outgoing_port;
	int destination_port;
};

typedef struct
{
	int port;
	int socket;
	char id;
	struct routing_table_row table[6];
} Router;

void error(char *msg)
{
    perror(msg);
    exit(1);
}


// Constructs a router and its socket
Router start_router(int port, char id)
{
	Router r;
	r.port = port;
	r.id = id;

	// Initialize routing table
	int i;
	for (i = 0; i < 6; i++)
	{
		r.table[i].destination = ids[i];
		if (ids[i] == id)
			r.table[i].cost = 0;
		else
			r.table[i].cost = 9999;
		
		r.table[i].outgoing_port = port;
		r.table[i].destination_port = ports[i];
	}


	// Start socket
	int sockfd;
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

	r.socket = sockfd;

	return r;
}

// Sets a router to start listening
void router_receive(Router r)
{
	printf("Router %c waiting on port %d\n", r.id, r.port);
	while (true)
	{
		struct sockaddr_in remote_addr;
		socklen_t remote_addr_len = sizeof(remote_addr);

		Packet* received_packet = (Packet*)malloc(sizeof(Packet));
	
		int recvlen = recvfrom(r.socket, received_packet, sizeof(Packet), 0, (struct sockaddr *) &remote_addr, &remote_addr_len);
		if (recvlen > 0)
		{
			printf("Router %c received message: '%s'\n\n", r.id, received_packet->msg);
		}
	}
}


int main(int argc, char *argv[])
{
	int i;

	Router routers[6];

	//Create six routers with ports
	for (i = 0; i < 6; i++) 
	{
		routers[i] = start_router(ports[i], ids[i]);
		printf("Router %c created with port %d\n", ids[i], ports[i]);
	}

	printf("Begin router listening\n");
	printf("-------------------------\n");

	// Start router listening
	for (i = 0; i < 6; i++)
	{
		int pid = fork();
		if (pid < 0)
			error("fork error");
		else if (pid == 0)
		{
			router_receive(routers[i]);
		}
	}


	// Send test message
	sleep(4);

	Packet p;
	p.type = DATA;
	p.msg = "test message";

	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len = sizeof(struct sockaddr_in);
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	remote_addr.sin_port = htons(10005);

	// Should send a packet from Router B to router F
	if (sendto(routers[1].socket, &p, sizeof(p), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0)
	 	error("error in sending message");
	else
	 	printf("Router %c sent message '%s' successfully\n", routers[1].id, p.msg);
}