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

int main(int argc, char const *argv[])
{
	if(argc != 4){
		fprintf(stderr, "Usage: rdpr <receiver_ip> <receiver_port> <receiver_file_name>\n");
		exit(EXIT_FAILURE);
	}

	int socket_udp = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	char * ptr;
	int port_number = strtol(argv[2],&ptr,10);
	//create socket structure
	struct sockaddr_in sockd;
	//set all fields
	memset(&sockd, 0, sizeof sockd);
	sockd.sin_family = AF_INET;
	sockd.sin_port = htons(port_number);
	sockd.sin_addr.s_addr = inet_addr(argv[1]);
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

	char buffer[MAX_PACKET_SIZE + 1];
	ssize_t recieved;

	FILE * fp;
	fp = fopen(argv[3],"ab");
	if(fp == NULL){
		fprintf(stderr, "ERROR: NULL FILE POINTER\n");
		close(socket_udp);	
		exit(EXIT_FAILURE);
	}
	
	int finish = 1;
	while(finish){

		memset(buffer,0,MAX_PACKET_SIZE + 1);
		recieved = recvfrom(socket_udp,(void*)buffer,sizeof buffer,0, (struct sockaddr*)&sockd,&socket_length);			

		if (recieved < 0) {
	      	    fprintf(stderr, "recvfrom failed\n");
	      	    exit(EXIT_FAILURE);
	    }
	    
	    buffer[MAX_PACKET_SIZE] = '\0';	
	    //handle incoming request
	    socket_info my_socket;
	    my_socket.sock_fdesc = socket_udp;
	    my_socket.socket = sockd;
	    my_socket.socket_length = socket_length;

	    finish = segment_handle(buffer,my_socket,RECIEVER,fp);   
	}

	fclose(fp);
	close(socket_udp);

	return 0;
}
