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

segment * buffer_to_segment(char * buffer){
	char * token;
	segment * seg = (segment *) malloc(sizeof(segment));
	token = strtok(buffer," ");
	strcpy(seg->magic,token);
	token = strtok(buffer," ");
	strcpy(seg->type,token);
	token = strtok(buffer," ");
	seg->sequence_num = (int) strtol(token,(char **)NULL,10);
	token = strtok(buffer," ");
	seg->ack_num = (int) strtol(token,(char **)NULL,10);
	token = strtok(buffer," ");
	seg->payload_len = (int) strtol(token,(char **)NULL,10);
	token = strtok(buffer," ");
	seg->window = (int) strtol(token,(char **)NULL,10);
	token = strtok(buffer,"");
	seg->data = (char *) malloc(seg->payload_len + 1);
	strcpy(seg->data,token);

	return seg;
}

char * segment_to_buffer(segment my_segment){

//check to see if packet is valid
char * buffer = (char *) malloc(MAX_PACKET_SIZE + 1);
memset(buffer,0,MAX_PACKET_SIZE + 1);

sprintf(buffer,"%s %s %d %d %d %d %s\n",my_segment.magic,my_segment.type,my_segment.sequence_num,my_segment.ack_num,my_segment.payload_len,my_segment.window,my_segment.data);

return buffer;
}

int segment_handle(char * buffer, socket_info my_socket, int flag){
	//convert buffer to segment: memory is allocated REMEMBER TO FREE!
	segment * my_segment = buffer_to_segment(buffer);
	//determine what kind of packet
	if(my_segment == NULL){
		return 0;
	}else if(strcmp(my_segment->type,"DAT") == 0 && flag != SENDER){
	//DAT


	}else if(strcmp(my_segment->type,"ACK") == 0 && flag != RECIEVER){
	//ACK:


	}else if(strcmp(my_segment->type,"SYN") == 0 && flag != SENDER){
	//SYN: create acknowledment segment	
	segment acknowledment_seg;
	strcpy(acknowledment_seg.magic,"CSC361");
	strcpy(acknowledment_seg.type,"ACK");
	acknowledment_seg.sequence_num = 100; //NOTE: select random sequence num
	acknowledment_seg.ack_num = my_segment->sequence_num + 1; //double check this logic is correct
	acknowledment_seg.payload_len = 0;
	acknowledment_seg.window = (MAX_PACKET_SIZE * WINDOW_SIZE); //bytes
	strcpy(acknowledment_seg.data,"");
	char * reply = segment_to_buffer(acknowledment_seg);
	//send acknowledgment	
	sendto((my_socket.sock_fdesc),(void *)reply,(strlen(reply) + 1),0,(struct sockaddr*)&(my_socket.socket),(sizeof my_socket.socket));

	//free(reply);

	}else if(strcmp(my_segment->type,"FIN") == 0 && flag != SENDER){
	//FIN:

	}else if(strcmp(my_segment->type,"RST") == 0){
	//RST

	}

	//Free segment memory
	//free(my_segment);
	return 1;
}