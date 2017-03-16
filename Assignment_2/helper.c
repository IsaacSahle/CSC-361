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
	char * p;
	segment * seg = (segment *) malloc(sizeof(segment));
	//printf("Token1: %s\n",token);
	strncpy(seg->magic,buffer,6);
	strncpy(seg->type,&buffer[8],3);
	p = &buffer[13];
	seg->sequence_num = (int) strtol(p,&p,10);
	p++;
	seg->ack_num = (int) strtol(p,&p,10);
	p++;
	seg->payload_len = (int) strtol(p,&p,10);
	p++;
	seg->window = (int) strtol(p,&p,10);
	
	p += 2;	
	printf("Data: %s\n",p);
	seg->data = (char *) calloc(seg->payload_len + 1,sizeof(char));
	//seg->data = (char *) malloc(seg->payload_len + 1);
	if(seg->payload_len == 0){
		strcpy(seg->data,"");
	}else{
	    //printf("DATA: %s",p);
		strcpy(seg->data,p);	
	}

	/*char * token;
	char copy[MAX_PACKET_SIZE + 1];
	memset(&copy, 0,MAX_PACKET_SIZE + 1);
	segment * seg = (segment *) malloc(sizeof(segment));
	strncpy(seg->magic,buffer,6);
	strncpy(seg->type,buffer[8],3);*/


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
			fprintf(stdout, "TO FILE: %s",my_segment->data);
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
	
		segment acknowledment_seg;
		strcpy(acknowledment_seg.magic,"CSC361");
		strcpy(acknowledment_seg.type,"ACK");
		acknowledment_seg.sequence_num = 0;
		acknowledment_seg.ack_num = my_segment->sequence_num + 1;
		acknowledment_seg.payload_len = 0;
		acknowledment_seg.window = 0; //bytes (MAX_PACKET_SIZE * WINDOW_SIZE)
		acknowledment_seg.data = (char *) calloc(1,sizeof(char));
		strcpy(acknowledment_seg.data,"");
		char * reply = segment_to_buffer(acknowledment_seg);
		//send acknowledgment	
		sendto((my_socket.sock_fdesc),(void *)reply,(strlen(reply) + 1),0,(struct sockaddr*)&(my_socket.socket),(sizeof my_socket.socket));
		//listen for no reply within the timeout time if fin is sent again, resend and if it is not fin sent again, send reset 
		fd_set socks;
		struct timeval t;
		t.tv_sec = CONNECTION_TIMEOUT;
		t.tv_usec = 0;
		char buff[MAX_PACKET_SIZE + 1];
		memset(buff,0,MAX_PACKET_SIZE);
		ssize_t recieved;
		while(1){
			FD_ZERO(&socks);
			FD_SET(my_socket.sock_fdesc, &socks);
			select(my_socket.sock_fdesc + 1, &socks, NULL, NULL, &t);

			if (FD_ISSET(my_socket.sock_fdesc, &socks)){
				recieved = recvfrom(my_socket.sock_fdesc,(void *)buff, MAX_PACKET_SIZE, 0, (struct sockaddr *)&(socket),&my_socket.socket_length);
				if (recieved < 0) {
						fprintf(stderr, "recvfrom failed\n");
						exit(EXIT_FAILURE);
				}
				//resend				
				segment * init = buffer_to_segment(buff);
				//check for FIN 
				if(init == NULL){
					continue;
				}else if(strcmp(init->type,"FIN") != 0){
				//NOT A FIN
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
					sendto(my_socket.sock_fdesc,(void *)p,(strlen(p) + 1),0,(struct sockaddr*)&(my_socket.socket),(sizeof my_socket.socket));
					free(p);
				 }
					
					free(init->data);
					free(init);
					free(my_segment->data);
					free(my_segment);
					fclose(fp);
					close(my_socket.sock_fdesc);
					fprintf(stderr,"ERROR: RESET FLAG SENT DURING CONNECTION TEARDOWN\n");
					exit(EXIT_FAILURE);
				}else{
				//recieved another fin, lets resend ack
				   	sendto((my_socket.sock_fdesc),(void *)reply,(strlen(reply) + 1),0,(struct sockaddr*)&(my_socket.socket),(sizeof my_socket.socket));
				}

				free(init->data);
				free(init);	
			}else{
				break;
			}
		}
					
		free(acknowledment_seg.data);
		free(reply);
		free(my_segment->data);
		free(my_segment);
		return 0;
	}else if(strcmp(my_segment->type,"RST") == 0){
	//RST: close everything and exit
	printf("RECIEVED RESET\n");
	}

	//Free segment memory
	free(my_segment->data);
	free(my_segment);
	return 1;
}

















