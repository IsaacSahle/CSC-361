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

int server_connect(int socket_udp,struct sockaddr_in socket,socklen_t length);
int timeout_recvfrom (int sock, char *buf, struct sockaddr_in *connection, socklen_t * length ,int timeoutinseconds);

int main(int argc, char const *argv[])
{
	if(argc != 6){
		fprintf(stderr, "Usage: rdps <sender_ip> <sender_port> <receiver_ip> <receiver_port> <sender_file_name>\n");
		exit(EXIT_FAILURE);
	}

	int socket_udp = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	char * ptr;
	int sender_port_number = strtol(argv[2],&ptr,10);
	int reciever_port_number = strtol(argv[4],&ptr,10);
	//create socket structure
	struct sockaddr_in sender;
	struct sockaddr_in reciever;
	//set all fields
	memset(&sender, 0, sizeof sender);
	sender.sin_family = AF_INET;
	sender.sin_port = htons(sender_port_number);
	sender.sin_addr.s_addr = inet_addr(argv[1]);
	socklen_t sender_socket_length = sizeof sender;

	memset(&reciever, 0, sizeof reciever);
	reciever.sin_family = AF_INET;
	reciever.sin_port = htons(reciever_port_number);
	reciever.sin_addr.s_addr = inet_addr(argv[3]);
	socklen_t reciever_socket_length = sizeof reciever;  
	
	int option = 1;
	//set socket option 
	if (setsockopt(socket_udp,SOL_SOCKET,SO_REUSEADDR,&option,sizeof option) == -1){
		fprintf(stderr,"Set Socket Option Failed\n");	
	    close(socket_udp);
    	exit(EXIT_FAILURE);
	}
	//bind socket
	if(bind(socket_udp,(struct sockaddr *)&sender, sizeof sender) == -1){
       	//bind failed
	    fprintf(stderr,"Binding failed\n");	
	    close(socket_udp);
    	exit(EXIT_FAILURE);
	}



	char buffer[MAX_PACKET_SIZE];
	ssize_t recieved;
	//keep trying to connect to server
	while(!server_connect(socket_udp,reciever,reciever_socket_length)){}

	/*while(1){

		memset(buffer,0,MAX_PACKET_SIZE);
		recieved = recvfrom(socket_udp,(void*)buffer,MAX_PACKET_SIZE,0, (struct sockaddr*)&sender,&sender_socket_length);			

		if (recieved < 0) {
	      	    fprintf(stderr, "recvfrom failed\n");
	      	    exit(EXIT_FAILURE);
	    }
	    
	    buffer[MAX_PACKET_SIZE - 1] = '\0';	
		//handle incoming request
		socket_info my_socket;
		my_socket.sock_fdesc = socket_udp;
		my_socket.socket = sender;

		segment_handle(buffer,my_socket,SENDER);   
	
	}*/


	close(socket_udp);



	return 0;
}



int server_connect(int socket_udp,struct sockaddr_in socket,socklen_t length){
//Fire off SYN packet

//create segment
segment synchro;
char buff[MAX_PACKET_SIZE];
strcpy(synchro.magic,"CSC361");
strcpy(synchro.type,"SYN");
synchro.sequence_num = 100; //NOTE: select random sequence num > 10 (window size)
sender_sequence_number = synchro.sequence_num; //set global sequence 
synchro.ack_num = 0; //irrelevant
synchro.payload_len = 0;
synchro.window = 0;
synchro.data = (char *) calloc(1,sizeof(char));
strcpy(synchro.data,"");

char * packet = segment_to_buffer(synchro);
sendto(socket_udp,(void *)packet,(strlen(packet) + 1),0,(struct sockaddr*)&(socket),(length));
free(packet);

//listen for reply and confirm it.
//start timer
memset(buff,0,MAX_PACKET_SIZE);
if(timeout_recvfrom(socket_udp,buff,&(socket),&length,CONNECTION_TIMEOUT)){
	//buff has data verify the packet has syn and ack flag set and 
	buff[MAX_PACKET_SIZE - 1] = '\0';
	printf("%s\n",buff);
	segment * init = buffer_to_segment(buff);
	if(strcmp(init->type,"ACK") == 0 && init->sequence_num == sender_sequence_number && init->ack_num == sender_sequence_number + 1){
	//ACK:
		free(init->data);
		free(init);
		return 1;
	}

	free(init->data);
	free(init);
}

return 0;

}

//http://stackoverflow.com/questions/12713438/how-to-add-delay-to-sento-and-recvfrom-in-udp-client-server-in-c
int timeout_recvfrom (int sock, char *buf, struct sockaddr_in *connection, socklen_t * length ,int timeoutinseconds)
{
    fd_set socks;
    struct timeval t;
    FD_ZERO(&socks);
    FD_SET(sock, &socks);
    t.tv_sec = timeoutinseconds;
  	printf("%d\n",select(sock + 1, &socks, NULL, NULL, &t));
    //select(sock + 1, &socks, NULL, NULL, &t) != -1 && 
    if (recvfrom(sock,(void *)buf, MAX_PACKET_SIZE, 0, (struct sockaddr *)connection, length) != -1)
        return 1;
    else
        return 0;
}