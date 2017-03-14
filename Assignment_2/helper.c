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
	//printf("BUFFER: %s\n",buffer);
	/*if(strlen(buffer) < MAX_PACKET_SIZE)
		return NULL;*/
	
	char * token;
	segment * seg = (segment *) malloc(sizeof(segment));
	token = strtok(buffer," ");
	//printf("Token1: %s\n",token);
	strcpy(seg->magic,token);
	token = strtok(NULL," ");
	//printf("Token2: %s\n",token);
	strcpy(seg->type,token);
	token = strtok(NULL," ");
	//printf("Token3: %s\n",token);
	seg->sequence_num = (int) strtol(token,(char **)NULL,10);
	token = strtok(NULL," ");
	//printf("Token4: %s\n",token);
	seg->ack_num = (int) strtol(token,(char **)NULL,10);
	token = strtok(NULL," ");
	//printf("Token5: %s\n",token);
	seg->payload_len = (int) strtol(token,(char **)NULL,10);
	token = strtok(NULL," ");
	//printf("Token6: %s\n",token);
	seg->window = (int) strtol(token,(char **)NULL,10);
	
	//trim whitespace tokens
	//printf("TOKEN REMAINS: %s\n",token);
	//token = strtok(NULL,"");
	token = token + 3;	
	seg->data = (char *) calloc(seg->payload_len + 1,sizeof(char));
	//seg->data = (char *) malloc(seg->payload_len + 1);
	if(seg->payload_len == 0)	
	   strcpy(seg->data,"");
	else 
	   strcpy(seg->data,token);
	
	return seg;
}

char * segment_to_buffer(segment my_segment){

//check to see if packet is valid
char * buffer = (char *) malloc(MAX_PACKET_SIZE + 1);
memset(buffer,0,MAX_PACKET_SIZE + 1);

sprintf(buffer,"%s %s %d %d %d %d\n\n%s",my_segment.magic,my_segment.type,my_segment.sequence_num,my_segment.ack_num,my_segment.payload_len,my_segment.window,my_segment.data);

return buffer;
}

int segment_handle(char * buffer, socket_info my_socket, int flag, FILE * fp){
	//convert buffer to segment: memory is allocated REMEMBER TO FREE!
	//printf("HELLO\n");
	segment * my_segment = buffer_to_segment(buffer);
	//printf("HELLO\n");
	
//fprintf(stdout,"%s %s %d %d %d %d %s\n",my_segment->magic,my_segment->type,my_segment->sequence_num,my_segment->ack_num,my_segment->payload_len,my_segment->window,my_segment->data);

	//determine what kind of packet
	if(my_segment == NULL){
		return 0;
	}else if(strcmp(my_segment->type,"DAT") == 0 && flag != SENDER){
	//DAT
		//printf("Sending ACK: Sequence..%d Request..%d\n",my_segment->sequence_num,request_number);
		
		if(my_segment->sequence_num + 1 == request_number){
			
			//printf("%s\n",my_segment->data);
			segment acknowledment_seg;
			strcpy(acknowledment_seg.magic,"CSC361");
			strcpy(acknowledment_seg.type,"ACK");
			acknowledment_seg.sequence_num = 0;

			request_number = request_number + my_segment->payload_len; 
			acknowledment_seg.ack_num =request_number;	

			acknowledment_seg.payload_len = 0;
			acknowledment_seg.window = 0; //bytes
			acknowledment_seg.data = (char *) calloc(1,sizeof(char));
			strcpy(acknowledment_seg.data,"");
			char * reply = segment_to_buffer(acknowledment_seg);
			//send acknowledgment				
			sendto((my_socket.sock_fdesc),(void *)reply,(strlen(reply) + 1),0,(struct sockaddr*)&(my_socket.socket),(sizeof my_socket.socket));
			fprintf(fp, "%s",my_segment->data);
			free(acknowledment_seg.data);
			free(reply);
		}else{
			//duplicate or out of order segment 
		}

	}else if(strcmp(my_segment->type,"ACK") == 0 && flag != RECIEVER){
	//ACK:

	}else if(strcmp(my_segment->type,"SYN") == 0 && flag != SENDER){
		//SYN: create acknowledment segment	
		//printf("HEREE\n");
		segment acknowledment_seg;
		strcpy(acknowledment_seg.magic,"CSC361");
		strcpy(acknowledment_seg.type,"ACK");
		acknowledment_seg.sequence_num = 0;
		acknowledment_seg.ack_num = my_segment->sequence_num + 1;
		request_number = my_segment->sequence_num + 1;
		acknowledment_seg.payload_len = 0;
		acknowledment_seg.window = 0; //bytes (MAX_PACKET_SIZE * WINDOW_SIZE)
		acknowledment_seg.data = (char *) calloc(1,sizeof(char));
		strcpy(acknowledment_seg.data,"");
		char * reply = segment_to_buffer(acknowledment_seg);
		//send acknowledgment	
		sendto((my_socket.sock_fdesc),(void *)reply,(strlen(reply) + 1),0,(struct sockaddr*)&(my_socket.socket),(sizeof my_socket.socket));
		free(acknowledment_seg.data);
		free(reply);

	}else if(strcmp(my_segment->type,"FIN") == 0 && flag != SENDER){
	//FIN:

	}else if(strcmp(my_segment->type,"RST") == 0){
	//RST

	}

	//Free segment memory
	free(my_segment->data);
	free(my_segment);
	return 1;
}

















