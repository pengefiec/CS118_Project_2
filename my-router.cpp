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
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream> 
#include <mutex>
#include <iostream>
using namespace std;
const int ports[6] = {10000, 10001, 10002, 10003, 10004, 10005};
const char ids[6] = {'A', 'B', 'C', 'D', 'E', 'F'};
unordered_map<int, int> neighbors;
//ADM for administration message.
typedef enum {CONTROL, DATA, ADMIN} packet_type;

struct routing_table_row
{
	char destination;
	bool neighbor;
	int cost;
	int outgoing_port;
	int destination_port;
	time_t last_update_time;
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
	int outgoing_port;//unuseful
	int destination_port;
	char outgoing_id;
	char destination_id;
	int index;	// 0 for A, 1 for B etc.

	packet_type type;

	string msg;		  // For data
	routing_table dv; // For DV updates
} Packet;


void error(char* msg)
{
    perror(msg);
    exit(1);
}
Router r;
mutex table_lock;
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
/*
Constructs a router and its socket
*/
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
				neighbors.insert(make_pair(r.table[0].destination_port, r.table[0].cost));
			}
			else if (strcmp(pch, "B") == 0) {
				pch = strtok(NULL, ",");
				r.table[1].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[1].cost = atoi(pch);
				r.table[1].neighbor = true;
				neighbors.insert(make_pair(r.table[1].destination_port, r.table[1].cost));
			}
			else if (strcmp(pch, "C") == 0) {
				pch = strtok(NULL, ",");
				r.table[2].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[2].cost = atoi(pch);
				r.table[2].neighbor = true;
				neighbors.insert(make_pair(r.table[2].destination_port, r.table[2].cost));
			}
			else if (strcmp(pch, "D") == 0) {
				pch = strtok(NULL, ",");
				r.table[3].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[3].cost = atoi(pch);
				r.table[3].neighbor = true;
				neighbors.insert(make_pair(r.table[3].destination_port, r.table[3].cost));
			}
			else if (strcmp(pch, "E") == 0) {
				pch = strtok(NULL, ",");
				r.table[4].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[4].cost = atoi(pch);
				r.table[4].neighbor = true;
				neighbors.insert(make_pair(r.table[4].destination_port, r.table[4].cost));
			}
			else if (strcmp(pch, "F") == 0) {
				pch = strtok(NULL, ",");
				r.table[5].destination_port = atoi(pch);
				pch = strtok(NULL, ",");
				r.table[5].cost = atoi(pch);
				r.table[5].neighbor = true;
				neighbors.insert(make_pair(r.table[5].destination_port, r.table[5].cost));
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
	if (::bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0)
		error("error on binding");

	r.socket = sockfd;

	return r;
}
/*
Get all routers' outgoing port which are reached via this router.
*/
vector<int> get_via_here(const Router & r, int index){
	vector<int> via_here;
	for(int i=0; i<6;i++){
		int j=distance(ids, find(ids, ids + 6, r.table[i].destination));
		if(r.table[i].destination_port==r.table[index].destination_port && j!=i){
			via_here.push_back(r.table[i].outgoing_port);
		}
	}
	return via_here;
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
				//Poison reverse.
				vector<int> via_here=get_via_here(r, i);
				for (vector<int>::iterator it = via_here.begin() ; it != via_here.end(); ++it){
					int x=distance(ports, find(ports, ports + 6, *it));
					p.dv[x].cost=9999;
				}
				if (sendto(r.socket, &p, sizeof(p), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0)
				 	error("error in sending message");
			}
		}		
}
/*
Update routing table when new dv comes.
*/
bool update_routing_table(Router &r, const Packet *received_packet, struct sockaddr_in * remote_addr){
	bool updated = false;
	time_t now=time(NULL);
	// Iterate through each DV entry and check for cheaper cost and update
	for (int i = 0; i < 6; i++)
	{
		r.table[i].last_update_time=now;
		int index = received_packet->index;
		if (r.table[i].cost > neighbors[ports[index]] + received_packet->dv[i].cost&& received_packet->dv[i].cost > 0)
		{
			updated=true;
			r.table[i].cost = neighbors[ports[index]] + received_packet->dv[i].cost;
			r.table[i].destination_port=ntohs(remote_addr->sin_port);
			print_routing_table(r);
			send_cm(r);
			
		}
	}
		return updated;	
}

/*
Output routing table to a file.
*/
void output_routing_table(const Router &r, char id, char* filename, routing_table old){
	
	FILE* output = fopen(filename, "a+");
	// Timestamp
	time_t now;
	struct tm *tm;
	now = time(0);
	tm = localtime(&now);

	//Print the time stamp.
	fprintf(output, "%04d-%02d-%02d %02d:%02d:%02d\n",
				 	tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
				   tm->tm_hour, tm->tm_min, tm->tm_sec);
	fprintf(output, "Router %c received DV update from Router %c\n", r.id, id);
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
/*
Output a data file send from a source router.
*/
void output_data_message(const Packet* received_packet, char* filename){
	
	FILE* output = fopen(filename, "a+");
	// Timestamp
	time_t now;
	struct tm *tm;
	now = time(0);
	tm = localtime(&now);
	//Print the time stamp.
	fprintf(output, "%04d-%02d-%02d %02d:%02d:%02d\n",
				 	tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
				   tm->tm_hour, tm->tm_min, tm->tm_sec);
	//Print out the header.
	fprintf(output, "Mesage type: %s\n", "DATA");
	fprintf(output, "Outgoing port on source router: %d\n", received_packet->outgoing_port);
	fprintf(output, "Data packet send to: %c\n", received_packet->destination_id);
	//Print out the txet phrase.
	fprintf(output, "Payload: %s\n", received_packet->msg.c_str());
	fprintf(output, "\n");
	fclose(output);
}
/*
Process control message. Output the old routing table and new routing table into a file if changes happen.
*/
void process_cm(Router &r, const Packet *received_packet,  struct sockaddr_in * remote_addr, char* filename){
	// Get old DV
	routing_table old;
	for(int i=0;i<6; i++){
		old[i] = r.table[i];
	}
	// Only print to file if routing table is updated	
	if(update_routing_table(r, received_packet, remote_addr)){
		output_routing_table(r, received_packet->outgoing_id, filename, old);
	}	
}
/*
Process data message. Relay to another router.
*/
void process_dm(const Router &r, const Packet *received_packet, char*filename){

	//Must repack it, don't know why.
	Packet p;
	p.type = DATA;
	p.destination_id = received_packet->destination_id;
	p.msg=received_packet->msg;
	//printf("%s\n", p.msg.c_str());
	output_data_message(received_packet, filename);
	int x=distance(ids, find(ids, ids + 6, received_packet->destination_id));
	if(x!=r.index&&r.table[x].cost<9999){
		struct sockaddr_in remote_addr;
		socklen_t remote_addr_len = sizeof(struct sockaddr_in);
		remote_addr.sin_family = AF_INET;
		remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		remote_addr.sin_port = htons(ports[x]);

		
		if (sendto(r.socket, &p, sizeof(p), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0)
		 	error("error in sending message");
	}
	else{

	}
}
/*
Get all neighbors index.
*/
vector<int> get_all_neighbor_index(Router& r){
	vector<int> all_neighbor;
	for(int i=0;i<6;i++){
		if(r.table[i].neighbor)
			all_neighbor.push_back(distance(ports, find(ports, ports+6, r.table[i].destination_port)));
	}
}

/*
Process admin message. Output the old routing table and new routing table into a file if changes happen,
repack and resend this packet to the related neighbor, since the network is undirectional, we need to chage in 
both side.
*/
void process_admin(Router &r, const Packet *received_packet, char* filename){
	// Get old DV
	routing_table old;
	for(int i=0;i<6; i++){
		old[i] = r.table[i];
	}
	istringstream ss(received_packet->msg);
	string id_s;
	string cost_s;

	getline(ss, id_s, ',');
	printf("%s\n", id_s.c_str());
	getline(ss, cost_s,',');
	printf("%s\n", cost_s.c_str());
	char id=id_s.at(0);
	int x=distance(ids, find(ids, ids + 6, id));
	int new_cost=stoi(cost_s);
	if(neighbors[ports[x]]!=new_cost){
	 	neighbors[ports[x]]=new_cost;
	 	r.table[x].cost=new_cost;
	 	output_routing_table(r, 'H', filename, old);
	 }
	 printf("outgoing port is %dr.port is %d", received_packet->outgoing_port, r.port);
	if(received_packet->outgoing_port==r.port){
		Packet p;
		p.type = ADMIN;
		p.destination_id = id;
		string message=string(1,r.id)+','+cost_s;
		p.msg=message;
		struct sockaddr_in remote_addr;
		socklen_t remote_addr_len = sizeof(struct sockaddr_in);
		remote_addr.sin_family = AF_INET;
		remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		remote_addr.sin_port = htons(ports[x]);
		if (sendto(r.socket, &p, sizeof(p), 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0)
		 	error("error in sending message");
	}
	
}

/*
Check the routing table, mark the expored record.
*/
void check_expire(Router &r){
	time_t now=time(NULL);
	for(int i=0;i<6;i++){
		double elapse=difftime(now, r.table[i].last_update_time);
		if(elapse>15.0){
			r.table[i].cost=9999;
			//update_routing_table(r);
		}
	}
	
}
/*
Periodically send up date info, using in a separated thread.
*/
void period_update(){
	while(1){
		//check_expire(r);
		table_lock.lock();
		if(r.id=='B'){
			print_routing_table(r);
		}
		send_cm(r);
		table_lock.unlock();
		printf("%s\n", "I'm sending update");
		std::this_thread::sleep_for (std::chrono::seconds(5));
	}
}


/*
Compile using g++ -std=c++11 my-router.cpp -pthread -o myrouter
Run with ./my router [router index]
Each router should be run in a separated terminal.
*/
int main(int argc, char *argv[])
{
	//Router routers[6];
	if (argc < 2) {
        fprintf(stderr,"ERROR, no router index provided\n");
        exit(1);
    }
    int index = atoi(argv[1]);
    int portno=ports[index];
   	char id=ids[index];
	r = start_router(portno, id, index);
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len=sizeof(remote_addr);
	time_t current=time(NULL);

	//Construct update file.
	char filename[256] = "routing-output";
	char idstring[2] = {id, '\0'};
	strcat(filename, idstring);
	strcat(filename, ".txt");
	

	printf("\nRouter Start!\n");
	printf("Router %c waiting on port %d\n", r.id, r.port);
	//printf("Routing tables should converge in about 30 seconds);

	Packet* received_packet = (Packet*)malloc(sizeof(Packet));
	// Start routers to send and receive DVs
	send_cm(r);

	std::thread t (period_update);
	t.detach();
	printf("%s%s\n", "this is the current value", ctime(&current));
	while(1) {
		int recvlen = recvfrom(r.socket, received_packet, sizeof(Packet), 0, (struct sockaddr *) &remote_addr, &remote_addr_len);
		if (recvlen > 0)
		{
			if (received_packet->type == CONTROL){
				printf("I received control packet from %c\n", received_packet->outgoing_id);
				process_cm(r, received_packet,&remote_addr, filename);
			}
			else if(received_packet->type==DATA){
				printf("%s%s\n", "I received data packet, the payload is: ",received_packet->msg.c_str());
				
				process_dm(r, received_packet,filename);
				
			}
			else if(received_packet->type==ADMIN){
				printf("%s%s\n", "I received a admin packet, the payload is: ",received_packet->msg.c_str());
				process_admin(r, received_packet, filename);
			}
			else{
				error("Wrong data type.");
			}	
		}
	}
	
}