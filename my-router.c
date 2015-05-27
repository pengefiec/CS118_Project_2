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
#include <time.h>

const int ports[6] = {10000, 10001, 10002, 10003, 10005, 10004};
const char ids[6] = {'A', 'B', 'C', 'D', 'E', 'F'};

int count = 0;
time_t now;

typedef enum {CONTROL, DATA} packet_type;

struct routing_table_row
{
	char destination;
	bool neighbor;
	int cost;
	int outgoing_port;
	int destination_port;
};

typedef struct routing_table_row routing_table[6];

typedef struct
{
	int port;
	int socket;
	char id;
	int index;
	routing_table table;
} Router;

typedef struct
{
	int outgoing_port;
	int destination_port;
	int outgoing_id;
	int destination_id;
	int index;
	packet_type type;
	char* msg;
	routing_table dv;
} Packet;


void error(char *msg)
{
    perror(msg);
    exit(1);
}


// Constructs a router and its socket
Router start_router(int port, char id, int index)
{
	Router r;
	r.port = port;
	r.id = id;
	r.index = index;

	// Initialize routing table
	int i;
	for (i = 0; i < 6; i++)
	{
		r.table[i].destination = ids[i];
		if (ids[i] == id)
			r.table[i].cost = 0;
		else
			r.table[i].cost = 9999;
		r.table[i].neighbor = false;
		r.table[i].outgoing_port = port;
		r.table[i].destination_port = ports[i];
	}

	printf("Router %c created with port %d\n", id, port);

	// Open initial topology file
	FILE* f;
	char line[512];

	f = fopen("topology.txt", "rb");
	if (f == NULL)
		error("file open error");

	char* pch;

	// Parse the file and initialize routing table accordingly
	while (fgets(line, sizeof(line), f))
	{
		pch = strtok(line, ",");
		if (line[0] == id)
		{
			pch = strtok(NULL, ",");
			if (strcmp(pch, "A") == 0) {
				pch = strtok(NULL, ",");
				r.table[0].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[0].cost = atoi(pch);
				r.table[0].neighbor = true;
			}
			else if (strcmp(pch, "B") == 0) {
				pch = strtok(NULL, ",");
				r.table[1].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[1].cost = atoi(pch);
				r.table[1].neighbor = true;
			}
			else if (strcmp(pch, "C") == 0) {
				pch = strtok(NULL, ",");
				r.table[2].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[2].cost = atoi(pch);
				r.table[2].neighbor = true;
			}
			else if (strcmp(pch, "D") == 0) {
				pch = strtok(NULL, ",");
				r.table[3].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[3].cost = atoi(pch);
				r.table[3].neighbor = true;
			}
			else if (strcmp(pch, "E") == 0) {
				pch = strtok(NULL, ",");
				r.table[4].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[4].cost = atoi(pch);
				r.table[4].neighbor = true;
			}
			else if (strcmp(pch, "F") == 0) {
				pch = strtok(NULL, ",");
				r.table[5].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[5].cost = atoi(pch);
				r.table[5].neighbor = true;
			}
		}
	}

	// Print router's initial routing table given from the file
	printf("%c, %i, %i\n", r.table[0].destination, r.table[0].destination_port, r.table[0].cost);
	printf("%c, %i, %i\n", r.table[1].destination, r.table[1].destination_port, r.table[1].cost);
	printf("%c, %i, %i\n", r.table[2].destination, r.table[2].destination_port, r.table[2].cost);
	printf("%c, %i, %i\n", r.table[3].destination, r.table[3].destination_port, r.table[3].cost);
	printf("%c, %i, %i\n", r.table[4].destination, r.table[4].destination_port, r.table[4].cost);
	printf("%c, %i, %i\n", r.table[5].destination, r.table[5].destination_port, r.table[5].cost);

	fclose(f);


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

// Sets a router to start sending their DVs to their neighbors every 5 seconds
void router_send_DV(Router r)
{
	printf("Router %c waiting on port %d\n", r.id, r.port);
	while (true)
	{
		// Send DV to neighbors only
		int i;

		for (i = 0; i < 6; i++)
		{
			if(r.table[i].neighbor)
			{
				// Create a control packet of the router's DV and send it to neighbors
				Packet p;
				p.type = CONTROL;
				p.outgoing_port = r.port;
				p.destination_port = r.table[i].destination_port;
				p.outgoing_id = r.id;
				p.index = r.index;
				p.destination_id = r.table[i].destination;
				int j;
				for (j=0; j<6; j++)
					p.dv[j] = r.table[j];

				struct sockaddr_in remote_addr;
				socklen_t remote_addr_len = sizeof(struct sockaddr_in);
				remote_addr.sin_family = AF_INET;
				remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
				remote_addr.sin_port = htons(p.destination_port);

				if (sendto(r.socket, &p, sizeof(p), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0)
				 	error("error in sending message");
				else
				{

				}
			}
		}
		sleep(5);


		struct sockaddr_in remote_addr;
		socklen_t remote_addr_len = sizeof(remote_addr);

		Packet* received_packet = (Packet*)malloc(sizeof(Packet));

		int recvlen = recvfrom(r.socket, received_packet, sizeof(Packet), 0, (struct sockaddr *) &remote_addr, &remote_addr_len);
		if (recvlen > 0)
		{
			// Received control packet
			if (received_packet->type == CONTROL)
			{
				bool updated = false;
				routing_table old;
				// Iterate through each DV entry and check for cheaper cost and update
				for (i = 0; i < 6; i++)
				{
					old[i] = r.table[i];
					int index = received_packet->index;
					if (r.table[i].cost > received_packet->dv[i].cost + r.table[index].cost && received_packet->dv[i].cost > 0)
					{
						r.table[i].cost = received_packet->dv[i].cost + r.table[index].cost;
						updated = true;
					}
				}

				// Only print to file if routing table is updated
				if (updated) {
					// Print old DV
					int j;
					char filename[256] = "routing-output";
					char id = r.id;
					char idstring[2] = {id, '\0'};
					strcat(filename, idstring);
					strcat(filename, ".txt");
					FILE* output = fopen(filename, "a+");
					time_t now;
					struct tm *tm;
					now = time(0);
					tm = localtime(&now);

					// Timestamp
					fprintf(output, "%04d-%02d-%02d %02d:%02d:%02d\n",
							 	tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
							   tm->tm_hour, tm->tm_min, tm->tm_sec);

					fprintf(output, "Router %c received DV update from Router %c\n", r.id, received_packet->outgoing_id);
					fprintf(output, "Old routing table:\n");
					for (j = 0; j < 6; j++)
						fprintf(output, "%c, %i, %i\n", old[j].destination, old[j].destination_port, old[j].cost);

					
					// Print routing table updates to file

					fprintf(output, "\nNew routing table:\n");
					for (j = 0; j < 6; j++)
						fprintf(output, "%c, %i, %i\n", r.table[j].destination, r.table[j].destination_port, r.table[j].cost);
					fprintf(output, "\n");

					fclose(output);
				}
			}
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
		routers[i] = start_router(ports[i], ids[i], i);
	}

	printf("\nBegin router listening\n");
	printf("Routing tables should converge in about 30 seconds\n");
	printf("-------------------------\n");


	// Start routers to send and receive DVs
	for (i = 0; i < 6; i++)
	{
		int pid = fork();
		if (pid < 0)
			error("fork error");
		else if (pid == 0)
			router_send_DV(routers[i]);
	}


	while(true) {
	}
}