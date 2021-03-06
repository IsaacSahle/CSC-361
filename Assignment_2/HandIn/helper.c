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
#include <sys/time.h>
#include <time.h>


#include "global.h"
int SYN_received = 0;

segment * buffer_to_segment(char * buffer){

	char * p;
	segment * seg = (segment *) malloc(sizeof(segment));
	memset(seg->magic,0,MAGIC_LENGTH + 1);
	memset(seg->type,0,FLAG_LENGTH + 1);
	strncpy(seg->magic,buffer,6);
	buffer[6] = '\0';
	strncpy(seg->type,&buffer[7],3);
	buffer[10] = '\0';
	p = &buffer[11];
	seg->sequence_num = (int) strtol(p,&p,10);
	p++;
	seg->ack_num = (int) strtol(p,&p,10);
	p++;
	seg->payload_len = (int) strtol(p,&p,10);
	p++;
	seg->window = (int) strtol(p,&p,10);

	p += 2;

	seg->data = (char *) calloc(seg->payload_len + 1,sizeof(char));
	if(seg->payload_len == 0){
		strcpy(seg->data,"");
	}else{
		strcpy(seg->data,p);		
	}
	return seg;
}

char * segment_to_buffer(segment my_segment){
	char * buffer = (char *) malloc(MAX_PACKET_SIZE + 1);
	memset(buffer,0,MAX_PACKET_SIZE + 1);
	sprintf(buffer,"%s %s %d %d %d %d\n\n%s",my_segment.magic,my_segment.type,my_segment.sequence_num,my_segment.ack_num,my_segment.payload_len,my_segment.window,my_segment.data);
	return buffer;
}

int segment_handle(char * buffer, socket_info my_socket, int flag, FILE * fp,log_info * rec, char * receiver_ip, int * receiver_port){
	segment * my_segment = buffer_to_segment(buffer);
	//logging purposes
	char * s_address = inet_ntoa(my_socket.socket.sin_addr);
	int s_port = htons(my_socket.socket.sin_port); 
	   
	//determine what kind of segment
	if(my_segment == NULL){
		return 0;
	}else if(strcmp(my_segment->type,"DAT") == 0 && flag != SENDER){
		int resend_ack = 0;		
		segment acknowledment_seg;
		strcpy(acknowledment_seg.magic,"CSC361");
		strcpy(acknowledment_seg.type,"ACK");
		acknowledment_seg.sequence_num = 0;		
		if(my_segment->sequence_num + 1 == request_number){			
			//unique data segment			
			log_segment('r',s_address,s_port,receiver_ip,*receiver_port,my_segment);					
			request_number = request_number + my_segment->payload_len; 
			acknowledment_seg.ack_num =request_number;	
			fwrite(my_segment->data,sizeof(char),my_segment->payload_len,fp);
			rec->total_bytes += my_segment->payload_len;
			rec->unique_bytes += my_segment->payload_len;
			rec->total_packets += 1;
			rec->unique_packets += 1;
		}else{
			//duplicate or out of order segment
			log_segment('R',s_address,s_port,receiver_ip,*receiver_port,my_segment);
			acknowledment_seg.ack_num =request_number;	
			resend_ack = 1;			
			rec->total_bytes += my_segment->payload_len;
			rec->total_packets += 1; 
		}

		acknowledment_seg.payload_len = 0;
		acknowledment_seg.window = 0; //bytes
		acknowledment_seg.data = (char *) calloc(1,sizeof(char));
		strcpy(acknowledment_seg.data,"");
		char * reply = segment_to_buffer(acknowledment_seg);
		//send acknowledgment
		(resend_ack == 0)?log_segment('s',receiver_ip,*receiver_port,s_address,s_port,&acknowledment_seg):log_segment('S',receiver_ip,*receiver_port,s_address,s_port,&acknowledment_seg);				
		sendto((my_socket.sock_fdesc),(void *)reply,(strlen(reply) + 1),0,(struct sockaddr*)&(my_socket.socket),(sizeof my_socket.socket));
		rec->ACK += 1;					
		free(acknowledment_seg.data);
		free(reply);

	}else if(strcmp(my_segment->type,"ACK") == 0 && flag != RECIEVER){
	//handled on sender side

	}else if(strcmp(my_segment->type,"SYN") == 0 && flag != SENDER){	
		segment acknowledment_seg;
		strcpy(acknowledment_seg.magic,"CSC361");
		strcpy(acknowledment_seg.type,"ACK");
		acknowledment_seg.sequence_num = 0;
		acknowledment_seg.ack_num = my_segment->sequence_num + 1;
		request_number = my_segment->sequence_num + 1;
		acknowledment_seg.payload_len = 0;
		acknowledment_seg.window = 0;
		acknowledment_seg.data = (char *) calloc(1,sizeof(char));
		strcpy(acknowledment_seg.data,"");
		char * reply = segment_to_buffer(acknowledment_seg);
		(SYN_received == 0)?log_segment('r',s_address,s_port,receiver_ip,*receiver_port,my_segment):log_segment('R',s_address,s_port,receiver_ip,*receiver_port,my_segment);	
		//send acknowledgment
		sendto((my_socket.sock_fdesc),(void *)reply,(strlen(reply) + 1),0,(struct sockaddr*)&(my_socket.socket),(sizeof my_socket.socket));
		rec->ACK += 1;
		rec->SYN += 1;			
		free(acknowledment_seg.data);
		free(reply);
		SYN_received = 1;
	}else if(strcmp(my_segment->type,"FIN") == 0 && flag != SENDER){
		rec->FIN += 1;	
		segment acknowledment_seg;
		strcpy(acknowledment_seg.magic,"CSC361");
		strcpy(acknowledment_seg.type,"ACK");
		acknowledment_seg.sequence_num = 0;
		acknowledment_seg.ack_num = my_segment->sequence_num + 1;
		acknowledment_seg.payload_len = 0;
		acknowledment_seg.window = 0;
		acknowledment_seg.data = (char *) calloc(1,sizeof(char));
		strcpy(acknowledment_seg.data,"");
		char * reply = segment_to_buffer(acknowledment_seg);
		//send acknowledgment	
		log_segment('r',s_address,s_port,receiver_ip,*receiver_port,my_segment);
		log_segment('s',receiver_ip,*receiver_port,s_address,s_port,&acknowledment_seg);	
		sendto((my_socket.sock_fdesc),(void *)reply,(strlen(reply) + 1),0,(struct sockaddr*)&(my_socket.socket),(sizeof my_socket.socket));
		//listen for no reply within TIME_WAIT time, if fin is sent again resend ACK 
		fd_set socks;
		struct timeval t;
		char buff[MAX_PACKET_SIZE + 1];
		memset(buff,0,MAX_PACKET_SIZE);
		ssize_t recieved;
		while(1){
			FD_ZERO(&socks);
			FD_SET(my_socket.sock_fdesc, &socks);
			t.tv_sec = 0;
			t.tv_usec = TIME_WAIT;
			select(my_socket.sock_fdesc + 1, &socks, NULL, NULL, &t);

			if (FD_ISSET(my_socket.sock_fdesc, &socks)){
				recieved = recvfrom(my_socket.sock_fdesc,(void *)buff, MAX_PACKET_SIZE, 0, (struct sockaddr *)&(my_socket.socket),&my_socket.socket_length);
				if (recieved < 0) {
						fprintf(stderr, "recvfrom failed while in the helper\n");
						exit(EXIT_FAILURE);
				}
						
				segment * init = buffer_to_segment(buff);
				s_address = inet_ntoa(my_socket.socket.sin_addr);
				s_port = htons(my_socket.socket.sin_port);
				
				if(init == NULL){
					continue;
				}else if(strcmp(init->type,"FIN") != 0){
					
					log_segment('r',s_address,s_port,receiver_ip,*receiver_port,my_segment);
					 if(strcmp(init->type,"RST") != 0){
						//not a ACK and not a RST...send reset flag
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
						rec->RST_sent += 1;					
						free(p);
					 }else if(strcmp(init->type,"RST") == 0){
						rec->RST_received +=1;				
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
						rec->FIN += 1;
						log_segment('R',s_address,s_port,receiver_ip,*receiver_port,init);
						log_segment('S',receiver_ip,*receiver_port,s_address,s_port,&acknowledment_seg);
						sendto((my_socket.sock_fdesc),(void *)reply,(strlen(reply) + 1),0,(struct sockaddr*)&(my_socket.socket),(sizeof my_socket.socket));
				}

				free(init->data);
				free(init);	
			}else{
				//nothing received, close
				break;
			}
		}
					
		free(acknowledment_seg.data);
		free(reply);
		free(my_segment->data);
		free(my_segment);
		return 0;
	}else if(strcmp(my_segment->type,"RST") == 0){
		//close everything and exit
		log_segment('r',s_address,s_port,receiver_ip,*receiver_port,my_segment);		
		free(my_segment->data);
		free(my_segment);
		fclose(fp);
		close(my_socket.sock_fdesc);
		fprintf(stderr,"ERROR: RESET FLAG SENT DURING CONNECTION TEARDOWN\n");
		exit(EXIT_FAILURE);
	}

	//Free segment memory
	free(my_segment->data);
	free(my_segment);
	return 1;
}

//http://stackoverflow.com/questions/2408976/struct-timeval-to-printable-format
void log_segment(char event,char * source_ip, int source_port, char * destination_ip, int destination_port, segment * packet){
	struct timeval tv;
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[100];
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	nowtm = localtime(&nowtime);
	strftime(tmbuf, 100,"%H:%M:%S", nowtm);
	fprintf(stdout, "%s.%06li %c %s:%d %s:%d %s",tmbuf,(long int)tv.tv_usec,event,source_ip,source_port,destination_ip,destination_port,packet->type);
	strcmp(packet->type,"ACK") == 0?fprintf(stdout, " %d %d\n", packet->ack_num,packet->window):fprintf(stdout, " %d %d\n",packet->sequence_num,packet->payload_len);
}

