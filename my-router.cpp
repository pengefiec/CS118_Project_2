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
#include <unistd.h>
#include <thread>

const int ports[6] = {10000, 10001, 10002, 10003, 10004, 10005};
const char ids[6] = {'A', 'B', 'C', 'D', 'F', 'E'};


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
	int index;	// 0 for A, 1 for B etc.

	packet_type type;

	char* msg;		  // For data
	routing_table dv; // For DV updates
} Packet;


void error(char* msg)
{
    perror(msg);
    exit(1);
}
/*
Print the routing table.
*/
void print_routing_table(const Router &r){
	printf("%c, %i, %i\n", r.table[0].destination, r.table[0].destination_port, r.table[0].cost);
	printf("%c, %i, %i\n", r.table[1].destination, r.table[1].destination_port, r.table[1].cost);
	printf("%c, %i, %i\n", r.table[2].destination, r.table[2].destination_port, r.table[2].cost);
	printf("%c, %i, %i\n", r.table[3].destination, r.table[3].destination_port, r.table[3].cost);
	printf("%c, %i, %i\n", r.table[4].destination, r.table[4].destination_port, r.table[4].cost);
	printf("%c, %i, %i\n", r.table[5].destination, r.table[5].destination_port, r.table[5].cost);
}

// Constructs a router and its socket
Router start_router(int port, char id, int index)
{
	Router r;
	r.port = port;
	r.id = id;
	r.index = index;

	// Initialize routing table, the destions are set to 9999 and the destination port are set to -1.
	for (int i = 0; i < 6; i++)
	{
		r.table[i].destination = ids[i];
		if (ids[i] == id)
			r.table[i].cost = 0;
		else
			r.table[i].cost = 9999;
		r.table[i].neighbor = false;
		r.table[i].outgoing_port = port;
		r.table[i].destination_port = -1;
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
	print_routing_table(r);

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

/*
Create a control packet.
*/
Packet create_ctr(const Router& r, int i){
	Packet p;
	p.type = CONTROL;
	p.outgoing_port = r.port;
	p.destination_port = r.table[i].destination_port;
	p.outgoing_id = r.id;
	p.index = r.index;
	p.destination_id = r.table[i].destination;
	for (int j=0; j<6; j++){
		p.dv[j] = r.table[j];
	}
	return p;
}
/*
Send the distance vector to r's neighbors.
*/
void send_cm(const Router& r)
{
		// Send DV to neighbors only
		for (int i = 0; i < 6; i++)
		{
			if(r.table[i].neighbor)
			{

				// Create a control packet of the router's DV and send it to neighbors
				Packet p=create_ctr(r, i);
				//Contruct the address structure.
				struct sockaddr_in remote_addr;
				socklen_t remote_addr_len = sizeof(struct sockaddr_in);
				remote_addr.sin_family = AF_INET;
				remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
				remote_addr.sin_port = htons(p.destination_port);
				if (sendto(r.socket, &p, sizeof(p), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0)
				 	error("error in sending message");
			}
		}		
}
/*
Process control message.
*/
void process_cm(Router &r, const Packet *received_packet,  struct sockaddr_in * remote_addr){

	// bool updated = false;
	routing_table old;
	// Iterate through each DV entry and check for cheaper cost and update
	for (int i = 0; i < 6; i++)
	{
		old[i] = r.table[i];
		int index = received_packet->index;
		if (r.table[i].cost > received_packet->dv[i].cost + r.table[index].cost && received_packet->dv[i].cost > 0)
		{
			r.table[i].cost = received_packet->dv[i].cost + r.table[index].cost;
			r.table[i].destination_port=ntohs(remote_addr->sin_port);
			print_routing_table(r);
			// Print old DV
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

			send_cm(r);
			// Timestamp
			fprintf(output, "%04d-%02d-%02d %02d:%02d:%02d\n",
					 	tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
					   tm->tm_hour, tm->tm_min, tm->tm_sec);

			fprintf(output, "Router %c received DV update from Router %c\n", r.id, received_packet->outgoing_id);
			fprintf(output, "Old routing table:\n");
			for (int j = 0; j < 6; j++)
				fprintf(output, "%c, %i, %i\n", old[j].destination, old[j].destination_port, old[j].cost);

			
			// Print routing table updates to file

			fprintf(output, "\nNew routing table:\n");
			for (int j = 0; j < 6; j++)
				fprintf(output, "%c, %i, %i\n", r.table[j].destination, r.table[j].destination_port, r.table[j].cost);
			fprintf(output, "\n");

		fclose(output);
		}
	}
	// Only print to file if routing table is updated
}
void period_update(const Router& r){
	while(1){
		send_cm(r);
		printf("%s\n", "I'm updating");
		std::this_thread::sleep_for (std::chrono::seconds(5));
	}
}

/*
compile using g++ -std=c++11 my-router.cpp -pthread -o myrouter
*/
int main(int argc, char *argv[])
{
	Router routers[6];
	if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    int index = atoi(argv[1]);
    int portno=ports[index];
   	char id=ids[index];
	Router r = start_router(portno, id, index);
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len=sizeof(remote_addr);
	time_t current=time(NULL);
	printf("\nRouter Start!\n");
	printf("Router %c waiting on port %d\n", r.id, r.port);
	//printf("Routing tables should converge in about 30 seconds);

	Packet* received_packet = (Packet*)malloc(sizeof(Packet));
	// Start routers to send and receive DVs
	send_cm(r);

	std:: thread t (period_update,r);
	t.detach();
	printf("%s%s\n", "this is the current value", ctime(&current));
	while(1) {
		int recvlen = recvfrom(r.socket, received_packet, sizeof(Packet), 0, (struct sockaddr *) &remote_addr, &remote_addr_len);
		if (recvlen > 0)
		{
			if (received_packet->type == CONTROL){
				process_cm(r, received_packet,&remote_addr);
				//current=time(NULL);
			}
			//else
			//	process_dm();	
		}
		
		
		
	}
}


















