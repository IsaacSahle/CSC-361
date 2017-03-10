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


int server_connect(int socket_udp,struct sockaddr_in socket,socklen_t length);
int timeout_recvfrom (int sock, char *buf, struct sockaddr_in *connection, socklen_t * length ,int timeoutinseconds);
int size();
void enqueue(queue_packet * add_seg);
queue_packet * dequeue();
queue_packet * peek();
void display();

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
	server_connect(socket_udp,reciever,reciever_socket_length);

	//fcntl(socket_udp, F_SETFL, O_NONBLOCK);
	/*
	N  = window size
	Rn = request number
	Sn = sequence number
	Sb = sequence base
	Sm = sequence max
	       
	Sender:
	Sb = 0	=> sequenceBase
	Sm = N − 1
	Repeat the following steps forever:
	1. If you receive a request number where Rn > Sb 
	        Sm = (Sm - Sb) + Rn
	        Sb = Rn
	2.  If no packet is in transmission, 
	        Transmit a packet where Sb <= Sn <= Sm.  
	        Packets are transmitted in order.*/
	//open file
	fcntl(socket_udp, F_SETFL, O_NONBLOCK); 	
	FILE * fp;
	fp = fopen(argv[5],"r");
	if(fp == NULL){
		fprintf(stderr, "ERROR: CANNOT OPEN FILE\n");
		close(socket_udp);	
		exit(EXIT_FAILURE);
	}

	while(1){

		memset(buffer,0,MAX_PACKET_SIZE);
		recieved = recvfrom(socket_udp,(void*)buffer,MAX_PACKET_SIZE,0, (struct sockaddr*)&sender,&sender_socket_length);			
		//is there data to read? 
		if(recieved > 0){
			//recieved data 
			buffer[MAX_PACKET_SIZE - 1] = '\0';	
			//handle incoming request
			socket_info my_socket;
			my_socket.sock_fdesc = socket_udp;
			my_socket.socket = sender;

			segment_handle(buffer,my_socket,SENDER);

			//dequeue all relevant packets (sequence num < ack num) 	

		}
		//is the window size full? If not fire up packet, timestamp and toss in queue
		if(size() < WINDOW_SIZE){ //Queue is less than window size 
			//enqueue next packet for sending 
			segment data;
		}
		
		queue_packet * head = peek();
		struct timeval current_time;
		int status = gettimeofday(&current_time,NULL);
		//has packet at head expired? Dequeue, timestamp again, resend enqueue 
		if(head != NULL && status != -1){
			double elapsedTime;
			elapsedTime = (current_time.tv_sec - head->timestamp->tv_sec) * 1000.0;
	    		elapsedTime += (current_time.tv_usec - head->timestamp->tv_usec) / 1000.0;
			if(elapsedTime >= PACKET_TIMEOUT){
				//dequeue
				queue_packet * packet = dequeue();			
				gettimeofday(packet->timestamp,NULL);
				enqueue(packet);

			}				
		}
	
	}


	close(socket_udp);



	return 0;
}



int server_connect(int socket_udp,struct sockaddr_in socket,socklen_t length){
//Fire off SYN packet

//create segment
segment synchro;
char buff[MAX_PACKET_SIZE + 1];
strcpy(synchro.magic,"CSC361");
strcpy(synchro.type,"SYN");

srand(time(NULL));   // should only be called once
synchro.sequence_num = (rand() % 10000) + 10; //NOTE: select random sequence num > 10 (window size)
//synchro.sequence_num = 100; 
sender_sequence_number = synchro.sequence_num; //set global sequence
sequence_base = synchro.sequence_num;
synchro.ack_num = 0; //irrelevant
synchro.payload_len = 0;
synchro.window = 0;
synchro.data = (char *) calloc(1,sizeof(char));
strcpy(synchro.data,"");

char * packet = segment_to_buffer(synchro);
free(synchro.data);
sendto(socket_udp,(void *)packet,(strlen(packet) + 1),0,(struct sockaddr*)&(socket),(length));
free(packet);

//listen for reply and confirm it.
//fcntl(socket_udp, F_SETFL, O_NONBLOCK);
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
	    
	    //printf("%s\n",buff);
	    segment * init = buffer_to_segment(buff);
		//check for corrupt ACK 
		if(init == NULL){
			continue;
		}else if(strcmp(init->type,"ACK") == 0 && init->ack_num == sender_sequence_number + 1){
		//ACK:
			free(init->data);
			free(init);
	    	break;
		}

		free(init->data);
		free(init);

	}
}

return 0;

}


//http://www.sanfoundry.com/c-program-queue-using-array/
void enqueue(queue_packet * add_seg){

    if (rear == MAX - 1){
    printf("Queue Overflow \n");
    }else{
        if (front == - 1)
        /*If queue is initially empty */
        front = 0;
	rear = rear + 1;
        queue_array[rear] = add_seg;

   }

}
 
queue_packet * dequeue(){
    if (front == - 1 || front > rear){
        printf("Queue Underflow \n");
        return NULL;
    }else{
	queue_packet * ret = queue_array[front];
        front = front + 1;
	return ret; 
    }
}

int size(){
  return (front - rear) >= 0?(front - rear):0; 
}

queue_packet * peek(){
    return (front == - 1) ? NULL:queue_array[front];   
}

//debug purposes 
void display()

{
    int i;
    if (front == - 1){
        printf("Queue is empty \n");
    }else{
       //nothing 
    }
}



