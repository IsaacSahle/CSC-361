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

return NULL;
}

char * segment_to_buffer(segment my_segment){

return NULL;
}

void segment_handle(char * buffer, socket_info my_socket, int flag){
	//convert buffer to segment: memory is allocated REMEMBER TO FREE!
	segment * my_segment = buffer_to_segment(buffer);
	//determine what kind of packet
	if(strcmp(my_segment->type,"DAT") == 0 && flag != SENDER){
	//DAT


	}else if(strcmp(my_segment->type,"ACK") == 0 && flag != RECIEVER){
	//ACK: shouldn't be recieving ack segment


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
}