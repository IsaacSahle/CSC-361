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
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>

#include "global.h"


//queue data_structure
queue_packet * queue_array[MAX];
int rear = - 1;
int front = - 1;
int w_size = 0;
int read_entire_file = 0;

int size();
void server_connection(int socket_udp,struct sockaddr_in socket,socklen_t length,int status);
void slide_window(int seq_num);
void resend_expired_packets(struct timeval * current,int socket, struct sockaddr_in sd);

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

	char buffer[MAX_PACKET_SIZE + 1];
	ssize_t recieved;
	//keep trying to connect to server
	sequence_max = MAX_PACKET_SIZE - 1;
	server_connection(socket_udp,reciever,reciever_socket_length,CONNECT);

	//open file
	fcntl(socket_udp, F_SETFL, O_NONBLOCK);

	FILE * fp;
	fp = fopen(argv[5],"rb");
	if(fp == NULL){
		fprintf(stderr, "ERROR: CANNOT OPEN FILE\n");
		close(socket_udp);	
		exit(EXIT_FAILURE);
	}
	
	while(size() > 0 || !read_entire_file){

		memset(buffer,0,MAX_PACKET_SIZE + 1);
		recieved = recvfrom(socket_udp,(void*)buffer,MAX_PACKET_SIZE,0, (struct sockaddr*)&sender,&sender_socket_length);			
		//is there data to read? 
		if(recieved > 0){
			printf("ACKNOWLEDGMENT....\n");			
			//recieved data 
			buffer[MAX_PACKET_SIZE] = '\0';	
			segment * s = buffer_to_segment(buffer);
			//go through array (sequence num < ack num)...biggg asumption that the recieved packet is ack!! add a check later			
			slide_window(s->ack_num);
			free(s->data);
			free(s);
		}
		
		int w_size = size();
		//printf("w_size: %d\n",w_size);
		//is the window size full? If not fire up packet, timestamp and toss in array
		if(w_size < WINDOW_SIZE && !read_entire_file){ //Queue is less than window size 			
			//fill window!
			int i;
			for(i = 0; i < MAX;i++){
				if(queue_array[i] == NULL && !read_entire_file){
				     queue_packet * storage  = calloc(1,sizeof(queue_packet)); 
				     //build segment
				     storage->packet = (segment *) calloc(1,sizeof(segment));
				     storage->timestamp = (struct timeval *) calloc(1,sizeof(struct timeval));
				     strcpy(storage->packet->magic,"CSC361");
				     strcpy(storage->packet->type,"DAT");
				     storage->packet->sequence_num = sender_sequence_number;
				     storage->packet->ack_num = 0;
				     storage->packet->window = 0;
				     storage->packet->data = (char *) calloc(PAYLOAD_LENGTH + 1,sizeof(char));
				     //read payloadlength bytes from the file
				      
				     char file_contents[PAYLOAD_LENGTH + 1];
			             memset(file_contents,0,PAYLOAD_LENGTH + 1);
				     int bytes_read = 0;
				     unsigned char my_char;
				      while(bytes_read < PAYLOAD_LENGTH){
						my_char = fgetc(fp);
						if(feof(fp))
						{
					   	    break;
						}						
						file_contents[bytes_read] = my_char; 						
						bytes_read++;
				      }

				     read_entire_file = (bytes_read != PAYLOAD_LENGTH)?1:0;
				     //get rid of strag char at end of message
				     file_contents[bytes_read] = '\0'; 
				     storage->packet->payload_len = bytes_read;
				     sender_sequence_number = sender_sequence_number + bytes_read;
				     strcpy(storage->packet->data,file_contents);
				     char * packet = segment_to_buffer(*(storage->packet));
				     gettimeofday(storage->timestamp,NULL);
				     printf("sending....\n");
				     
				     sendto(socket_udp,(void *)packet,(strlen(packet) + 1),0,(struct sockaddr*)&(reciever),sizeof reciever);
				     strcpy(storage->buffer,packet);
				     //free packet
				     free(packet);
				     queue_array[i] = storage;
				}
			}			
		}
		
		//remove expired packets
		struct timeval current_time;
		gettimeofday(&current_time,NULL);
		resend_expired_packets(&current_time,socket_udp,reciever); 
	
	}
	
	fclose(fp);
	server_connection(socket_udp,reciever,reciever_socket_length,TEARDOWN);
	close(socket_udp);

	return 0;
}



void server_connection(int socket_udp,struct sockaddr_in socket,socklen_t length,int status){
//Fire off SYN or FIN packet
//create segment
segment synchro;
char buff[MAX_PACKET_SIZE + 1];
strcpy(synchro.magic,"CSC361");

if(status == CONNECT){
	strcpy(synchro.type,"SYN");
	srand(time(NULL));   // should only be called once
	synchro.sequence_num = (rand() % 10000) + 10; //NOTE: select random sequence num > 10 (window size)
	sender_sequence_number = synchro.sequence_num; //set global sequence
	sequence_base = synchro.sequence_num;	
}else{
	strcpy(synchro.type,"FIN");
    synchro.sequence_num = sender_sequence_number;
}

synchro.ack_num = 0; //irrelevant
synchro.payload_len = 0;
synchro.window = 0;
synchro.data = (char *) calloc(1,sizeof(char));
strcpy(synchro.data,"");

char * packet = segment_to_buffer(synchro);
free(synchro.data);
printf("SENDING: %s\n",packet);
sendto(socket_udp,(void *)packet,(strlen(packet) + 1),0,(struct sockaddr*)&(socket),(length));


//listen for reply and confirm it.
fd_set socks;
struct timeval t;
t.tv_sec = CONNECTION_TIMEOUT;
t.tv_usec = 0;
memset(buff,0,MAX_PACKET_SIZE);
ssize_t recieved;
while(1){
	FD_ZERO(&socks);
	FD_SET(socket_udp, &socks);
	select(socket_udp + 1, &socks, NULL, NULL, &t);

	if (FD_ISSET(socket_udp, &socks)){
		recieved = recvfrom(socket_udp,(void *)buff, MAX_PACKET_SIZE, 0, (struct sockaddr *)&(socket),&length);
		if (recieved < 0) {
	      	    fprintf(stderr, "recvfrom failed\n");
	      	    exit(EXIT_FAILURE);
	    }
	    
	    //printf("REPLY TO SYN OR FIN: %s\n",buff);
	    segment * init = buffer_to_segment(buff);
		//check for corrupt ACK 
		if(init == NULL){
			continue;
		}else if(strcmp(init->type,"ACK") == 0 && init->ack_num == sender_sequence_number + 1){
		//ACK:
			free(init->data);
			free(init);
	    	break;
		}else{
				
			 if(strcmp(init->type,"RST") != 0){
				//not a ACK and not a RST so wtf is it...send reset flag
				//create segment
				segment reset;
				strcpy(reset.magic,"CSC361");
				strcpy(reset.type,"RST");
				reset.sequence_num = 0;
				reset.ack_num = 0;
				reset.payload_len = 0;
				reset.window = 0;
				reset.data = (char *) calloc(1,sizeof(char));
				strcpy(reset.data,"");

				char * p = segment_to_buffer(reset);
				free(reset.data);
				sendto(socket_udp,(void *)p,(strlen(p) + 1),0,(struct sockaddr*)&(socket),(length));
				free(p);
			 }

				///close
				free(init->data);
				free(init);
				free(packet);
				close(socket_udp);
				char * error;
				fprintf(stderr,"ERROR: RESET FLAG SENT DURING %s\n",error = (status == CONNECT?"CONNECTION":"TEARDOWN"));
				exit(EXIT_FAILURE);
		}

		free(init->data);
		free(init);

	}

	//resend
	sendto(socket_udp,(void *)packet,(strlen(packet) + 1),0,(struct sockaddr*)&(socket),(length));
}

free(packet);
}
 
int size(){
    int i;
    int count = 0;
    for(i = 0; i < MAX;i++){
		if(queue_array[i] != NULL){
		count++;
		}
    }
    return count;
}

void slide_window(int ack_num){
    int i;
    for(i = 0; i < MAX;i++){
	if(queue_array[i] != NULL && queue_array[i]->packet->sequence_num < ack_num){
		//remove from array!!
		free(queue_array[i]->packet->data);
		free(queue_array[i]->packet);
		free(queue_array[i]->timestamp);
		free(queue_array[i]);
		queue_array[i] = NULL;
		w_size++;		
	}
    }
}

void resend_expired_packets(struct timeval * current,int socket, struct sockaddr_in sd){
    int i;
    for(i = 0; i < MAX;i++){
	if(queue_array[i] != NULL){
		//compute time diff
		double elapsedTime;
		elapsedTime = (current->tv_sec - queue_array[i]->timestamp->tv_sec) * 1000.0;
		elapsedTime += (current->tv_usec - queue_array[i]->timestamp->tv_usec) / 1000.0;
		//printf("current: %ld.%06ld\n", current->tv_sec,current->tv_usec);
		//printf("stamp: %ld.%06ld\n",queue_array[i]->timestamp->tv_sec,queue_array[i]->timestamp->tv_usec);		
		if(elapsedTime >= PACKET_TIMEOUT){
			//resend buffer
			printf("Resending....time: %f\n",elapsedTime);			
			sendto(socket,(void *)queue_array[i]->buffer,(strlen(queue_array[i]->buffer) + 1),0,(struct sockaddr*)&sd,sizeof sd);				
			gettimeofday(queue_array[i]->timestamp,NULL);
		}
	}
    }		
}
