#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>

#include "global.h"

void server_connect(const char * ip_address,int port,socket_info dest_sockd);

int main(int argc, char const *argv[])
{
	if(argc != 6){
		fprintf(stderr, "Usage: rdps <sender_ip> <sender_port> <receiver_ip> <receiver_port> <sender_file_name>\n");
		exit(EXIT_FAILURE);
	}

	int socket_udp = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	char * ptr_1;
	char * ptr_2;
	int send_port_number = strtol(argv[2],&ptr_1,10);
	int dest_port_number = strtol(argv[4],&ptr_2,10);
	//create socket structure
	struct sockaddr_in sockd;
	//set all fields
	memset(&sockd, 0, sizeof sockd);
	sockd.sin_family = AF_INET;
	sockd.sin_port = htons(send_port_number);
	sockd.sin_addr.s_addr = htonl(INADDR_ANY);
	socklen_t socket_length = sizeof sockd; 
	
	int option = 1;
	//set socket option 
	if (setsockopt(socket_udp,SOL_SOCKET,SO_REUSEADDR,&option,sizeof option) == -1){
		fprintf(stderr,"Set Socket Option Failed\n");	
	    close(socket_udp);
    	exit(EXIT_FAILURE);
	}
	//bind socket
	if(bind(socket_udp,(struct sockaddr *)&sockd, sizeof sockd) == -1){
       	//bind failed
	    fprintf(stderr,"Binding failed\n");	
	    close(socket_udp);
    	exit(EXIT_FAILURE);
	}



	char buffer[MAX_PACKET_SIZE];
	ssize_t recieved;
	socket_info info;
	info.socket = sockd;
	info.sock_fdesc = socket_udp;
	server_connect(argv[3],dest_port_number,info);

	while(1){

		memset(buffer,0,MAX_PACKET_SIZE);
		recieved = recvfrom(socket_udp,(void*)buffer,MAX_PACKET_SIZE,0, (struct sockaddr*)&sockd,&socket_length);			

		if (recieved < 0) {
	      	    fprintf(stderr, "recvfrom failed\n");
	      	    exit(EXIT_FAILURE);
	    }
	    
	    buffer[MAX_PACKET_SIZE - 1] = '\0';	
		//handle incoming request
		socket_info my_socket;
		my_socket.sock_fdesc = socket_udp;
		my_socket.socket = sockd;

		segment_handle(buffer,my_socket,SENDER);   
	
	}


	close(socket_udp);



	return 0;
}



void server_connect(const char * ip_address,int port, socket_info dest_sockd){
//Fire off SYN packet

//Setup socket
dest_sockd.socket.sin_port = htons(port);
inet_aton(ip_address,&dest_sockd.socket.sin_addr);

//create segment
segment synchro;
strcpy(synchro.magic,"CSC361");
strcpy(synchro.type,"SYN");
synchro.sequence_num = 100; //NOTE: select random sequence num
synchro.ack_num = 0; //irrelevant
synchro.payload_len = 0;
synchro.window = (MAX_PACKET_SIZE * WINDOW_SIZE); //bytes
strcpy(synchro.data,"");
char * buffer = segment_to_buffer(synchro);

sendto((dest_sockd.sock_fdesc),(void *)buffer,(strlen(buffer) + 1),0,(struct sockaddr*)&(dest_sockd.socket),(sizeof dest_sockd.socket));

//listen for reply and confirm it.

}