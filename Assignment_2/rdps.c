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
#include <time.h>
#include <sys/stat.h>
#include "global.h"


/*
 * Function:  size 
 * --------------------
 * computes current window size by traversing global queue_array
 * 
 * returns: the number of non NULL pointers
 */
int size();


/*
 * Function:  server_connection 
 * --------------------
 * Handles all connection management. Establishes a connection with client,
 * as well as connection teardown when the file has been transfered
 * 
 * socket_udp: socket file descriptor
 * socket: sockaddr_in that is used for various socket operations
 * length: lenght of socket
 * status: connecion status is either CONNECT or TEARDOWN
 */
void server_connection(int socket_udp,struct sockaddr_in socket,socklen_t length,int status);

/*
 * Function:  slide_window 
 * --------------------
 * Removes any packets currently in queue_array with a sequence number <= ack_num 
 *
 * ack_num: acknowledgment number 
 */
void slide_window(int ack_num);


/*
 * Function:  resend_expired_packets 
 * --------------------
 * Resends all packets that have timed out 
 *
 * current: current time
 * socket: socket file descriptor
 * sd: sockaddr_in that is used for various socket operations
 * r: destination ip address (for logging)
 * r_port: destination port number (for logging) 
 */
void resend_expired_packets(struct timeval * current,int socket, struct sockaddr_in sd,char * r,int r_port);

//packet buffer
queue_packet * queue_array[WINDOW_SIZE];
//contains all log info 
log_info sender_log;
//flag to indicate the entire file was read
int read_entire_file = 0;
char * source_ip;
char * destination_ip;
int source_port;
int destination_port;

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
	memset(&sender_log,0,sizeof(sender_log));
	
	source_port = sender_port_number;
	destination_port = reciever_port_number;
	
	source_ip = (char *) malloc(strlen(argv[1]) + 1);
	destination_ip = (char *) malloc(strlen(argv[3]) + 1);
	strcpy(source_ip,argv[1]);	
	strcpy(destination_ip,argv[3]);
	

	struct sockaddr_in sender;
	struct sockaddr_in reciever;

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
	
	//begin program timer
	struct timeval start;
	gettimeofday(&start,NULL);
	
	//connect to client
	server_connection(socket_udp,reciever,reciever_socket_length,CONNECT);

	//set socket to non blocking for main execution loop
	fcntl(socket_udp, F_SETFL, O_NONBLOCK);
	
	//open file
	FILE * fp;
	fp = fopen(argv[5],"rb");
	if(fp == NULL){
		fprintf(stderr, "ERROR: CANNOT OPEN FILE\n");
		close(socket_udp);	
		exit(EXIT_FAILURE);
	}
	
	char * src = destination_ip;
	int src_port = destination_port;
	
	//Begin file transfer	
	while(size() > 0 || !read_entire_file){

		memset(buffer,0,MAX_PACKET_SIZE + 1);
		recieved = recvfrom(socket_udp,(void*)buffer,MAX_PACKET_SIZE,0, (struct sockaddr*)&sender,&sender_socket_length);			
		//Check if ACK recieved 
		if(recieved > 0){
			//recieved ACK
			src =  inet_ntoa(sender.sin_addr);
	    		src_port = htons(sender.sin_port);			
			buffer[MAX_PACKET_SIZE] = '\0';	
			segment * s = buffer_to_segment(buffer);			
			log_segment('r',src,src_port,source_ip,source_port,s); 			
			slide_window(s->ack_num);
			sender_log.ACK += 1;			
			free(s->data);
			free(s);
		}
		
		int w_size = size();
		//Recieved at least WINDOW_SIZE/2 ACKs?
		if(w_size < WINDOW_SIZE/2 && !read_entire_file){ 			
			//fill window
			int i;
			for(i = 0; i < WINDOW_SIZE;i++){
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
				     file_contents[bytes_read] = '\0'; 
				     storage->packet->payload_len = bytes_read;
				     sender_sequence_number = sender_sequence_number + bytes_read;
				     strcpy(storage->packet->data,file_contents);
				     char * packet = segment_to_buffer(*(storage->packet));
				     gettimeofday(storage->timestamp,NULL);
				     log_segment('s',source_ip,source_port,src,src_port,storage->packet);
				     sendto(socket_udp,(void *)packet,(strlen(packet) + 1),0,(struct sockaddr*)&(reciever),sizeof reciever);
				     sender_log.total_bytes += bytes_read;
				     sender_log.unique_bytes += bytes_read;
				     sender_log.total_packets += 1;
				     sender_log.unique_packets += 1;						
				     strcpy(storage->buffer,packet);
				     //free packet
				     free(packet);
				     queue_array[i] = storage;
				}
			}			
		}
		
		//remove timed out packets
		struct timeval current_time;
		gettimeofday(&current_time,NULL);
		resend_expired_packets(&current_time,socket_udp,reciever,src,src_port); 
	
	}

	//stop program timer
	struct timeval end;
	gettimeofday(&end,NULL);	

	//compute time difference	
	double elapsedTime;
	elapsedTime = (end.tv_sec - start.tv_sec) * 1000.0;
	elapsedTime += (end.tv_usec - start.tv_usec)/ 1000.0;
	fclose(fp);
	
	//teardown connection	
	server_connection(socket_udp,reciever,reciever_socket_length,TEARDOWN);
	close(socket_udp);
	free(source_ip);
	free(destination_ip);
	fprintf(stdout,"total data bytes sent: %d\n",sender_log.total_bytes);
	fprintf(stdout,"unique data bytes sent: %d\n",sender_log.unique_bytes);
	fprintf(stdout,"total data packets sent: %d\n",sender_log.total_packets);
	fprintf(stdout,"unique data packets sent: %d\n",sender_log.unique_packets);
	fprintf(stdout,"SYN packets sent: %d\n",sender_log.SYN);
	fprintf(stdout,"FIN packets sent: %d\n",sender_log.FIN);
	fprintf(stdout,"RST packets sent: %d\n",sender_log.RST_sent);
	fprintf(stdout,"ACK packets received: %d\n",sender_log.ACK);
	fprintf(stdout,"RST packets received: %d\n",sender_log.RST_received);
	fprintf(stdout,"total time duration (second): %.3f\n",elapsedTime/1000);
	return 0;
}



void server_connection(int socket_udp,struct sockaddr_in socket,socklen_t length,int status){
	segment synchro;
	char buff[MAX_PACKET_SIZE + 1];
	strcpy(synchro.magic,"CSC361");

	if(status == CONNECT){
		strcpy(synchro.type,"SYN");
		srand(time(NULL));
		synchro.sequence_num = (rand() % 10000) + 10; 
		//set global sequence number		
		sender_sequence_number = synchro.sequence_num;
		sequence_base = synchro.sequence_num;	
	}else{
		strcpy(synchro.type,"FIN");
		sender_sequence_number++;
	    	synchro.sequence_num = sender_sequence_number;
	}

	synchro.ack_num = 0;
	synchro.payload_len = 0;
	synchro.window = 0;
	synchro.data = (char *) calloc(1,sizeof(char));
	strcpy(synchro.data,"");

	char * packet = segment_to_buffer(synchro);
	
	log_segment('s',source_ip,source_port,destination_ip,destination_port,&synchro);

	//Send first SYN or FIN
	sendto(socket_udp,(void *)packet,(strlen(packet) + 1),0,(struct sockaddr*)&(socket),(length));
	
	//log
	if(status == CONNECT){
	sender_log.SYN += 1;
	}else{
	sender_log.FIN += 1;
	}

	//listen for reply and verify
	fd_set socks;
	struct timeval t;
	memset(buff,0,MAX_PACKET_SIZE);
	ssize_t recieved;
	char * src;
	int src_port;
	while(1){
		FD_ZERO(&socks);
		FD_SET(socket_udp, &socks);
		t.tv_sec = 0;
		t.tv_usec = CONNECTION_TIMEOUT;
		select(socket_udp + 1, &socks, NULL, NULL, &t);
		src =  inet_ntoa(socket.sin_addr);
		src_port = htons(socket.sin_port);

		if (FD_ISSET(socket_udp, &socks)){
			recieved = recvfrom(socket_udp,(void *)buff, MAX_PACKET_SIZE, 0, (struct sockaddr *)&(socket),&length);
			if (recieved < 0) {
		      	    fprintf(stderr, "recvfrom failed\n");
		      	    exit(EXIT_FAILURE);
		    	}
		    
			segment * init = buffer_to_segment(buff);
			//logging purposes			
			src =  inet_ntoa(socket.sin_addr);
			src_port = htons(socket.sin_port);

			//check for corrupt packet 
			if(init == NULL){
				continue;
			}else if(strcmp(init->type,"ACK") == 0 && init->ack_num == sender_sequence_number + 1){
			//correct ACK received
				log_segment('r',src,src_port,source_ip,source_port,init);
				sender_log.ACK += 1;
				free(init->data);
				free(init);
		    	break;
			}else{
				log_segment('r',src,src_port,source_ip,source_port,init);	
				if(strcmp(init->type,"RST") != 0){
					//not a ACK and not a RST, unexpected behavour...send reset flag
					segment reset;
					strcpy(reset.magic,"CSC361");
					strcpy(reset.type,"RST");
					reset.sequence_num = 0;
					reset.ack_num = 0;
					reset.payload_len = 0;
					reset.window = 0;
					reset.data = (char *) calloc(1,sizeof(char));
					strcpy(reset.data,"");	
					sender_log.RST_sent += 1;
					char * p = segment_to_buffer(reset);
					free(reset.data);
					sendto(socket_udp,(void *)p,(strlen(p) + 1),0,(struct sockaddr*)&(socket),(length));
					free(p);
				 }else if(strcmp(init->type,"RST") == 0){
					sender_log.RST_received += 1;
				 }
				//close
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

		if(status == CONNECT){
			sender_log.SYN += 1;
		}else{
		   sender_log.FIN += 1;
		}
		//resend SYN or FIN
		log_segment('S',source_ip,source_port,src,src_port,&synchro); 
		sendto(socket_udp,(void *)packet,(strlen(packet) + 1),0,(struct sockaddr*)&(socket),(length));
	}
	free(synchro.data);
	free(packet);
}
 
int size(){
	int i;
	int count = 0;
	for(i = 0; i < WINDOW_SIZE;i++){
		if(queue_array[i] != NULL){
			count++;
		}
	}
	return count;
}

void slide_window(int ack_num){
	int i;
	for(i = 0; i < WINDOW_SIZE;i++){
		if(queue_array[i] != NULL && queue_array[i]->packet->sequence_num < ack_num - 1){
			//remove from array!!
			free(queue_array[i]->packet->data);
			free(queue_array[i]->packet);
			free(queue_array[i]->timestamp);
			free(queue_array[i]);
			queue_array[i] = NULL;		
		}
	}	
}

//http://stackoverflow.com/questions/2150291/how-do-i-measure-a-time-interval-in-c
void resend_expired_packets(struct timeval * current,int socket, struct sockaddr_in sd,char * r,int r_port){
	int i;
	for(i = 0; i < WINDOW_SIZE;i++){
		if(queue_array[i] != NULL){
		//compute time diff
		double elapsedTime;
		elapsedTime = (current->tv_sec - queue_array[i]->timestamp->tv_sec) * 1000.0;
		elapsedTime += (current->tv_usec - queue_array[i]->timestamp->tv_usec) / 1000.0;
			if(elapsedTime >= PACKET_TIMEOUT){
				//resend buffer
				log_segment('S',source_ip,source_port,r,r_port,queue_array[i]->packet);			
				sendto(socket,(void *)queue_array[i]->buffer,(strlen(queue_array[i]->buffer) + 1),0,(struct sockaddr*)&sd,sizeof sd);				
				sender_log.total_bytes += queue_array[i]->packet->payload_len;
				sender_log.total_packets += 1;						
				gettimeofday(queue_array[i]->timestamp,NULL);
			}
		}
	}		
}
