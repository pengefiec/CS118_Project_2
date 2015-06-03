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

const int ports[6] = {10000, 10001, 10002, 10003, 10004, 10005};
const char ids[6] = {'A', 'B', 'C', 'D', 'F', 'E'};


typedef enum {CONTROL, DATA, ADM} packet_type;

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
	int outgoing_port;
	int destination_port;
	int outgoing_id;
	int destination_id;
	int index;	// 0 for A, 1 for B etc.

	packet_type type;

	char* msg;		  // For data
	routing_table dv; // For DV updates
} Packet;

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void generate_traffic(Packet p, int port){
	int sockfd;
	//create traffic sending socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	struct sockaddr_in addr;
	bzero((char *) &addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(22222);
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0)
		error("error on binding");
	//send traffic to first hop router
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len = sizeof(struct sockaddr_in);
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	remote_addr.sin_port = htons(port);
	if (sendto(sockfd, &p, sizeof(p), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0)
	 	error("error in sending message");
	else
	{
		printf("%s\n", p.msg);

	}
}

void make_data_packet(char outgoing_router, char destination_router, char* msg ){
	// printf("%s\n%s\n",outgoing_router, destination_router);
	Packet p;
	p.type = DATA;
	p.destination_id = destination_router;
	int port;
	for(int i = 0; i < 6; i++){
		if(ids[i] == outgoing_router){
			port = ports[i];	
		}
	}
	p.msg = msg;
	generate_traffic(p, port);
}

Packet make_ADM_packet(char target_router, char* msg){
	Packet p;
	p.type = ADM;
	int port;
	for(int i = 0; i < 6; i++){
		if(ids[i] == target_router){
			port = ports[i];	
		}
	p.msg  = msg;
	}
	generate_traffic(p, port);
}


int main(int argc, char *argv[])
{
	/* code */
	if(argc < 2){
		error("Please define the outgoing_router and destination_router");
	}
	// for(int i=0; i < argc; i ++){
	// 	printf("%s\n",argv[i] );
	// }
	char* type = argv[1];
	if(strcmp(type, "0") == 0 ){
		if(argc != 4){
			error("Please provide the outgoing_router and destination_router");
		}
		char temp[500]; 
		scanf("%s", &temp);
		char* msg = temp;
		make_data_packet(argv[2][0], argv[3][0],msg);
	}
	if(strcmp(type, "1") == 0){
		if(argc != 3){
			error("Please provide the target destination_router");
		}
		char temp[100]; 
		scanf("%s", &temp);
		char* msg = temp;
		make_ADM_packet(argv[2][0], msg);
	}
	return 0;
}