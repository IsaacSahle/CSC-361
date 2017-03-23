#define MAX_PACKET_SIZE 1024
#define PAYLOAD_LENGTH 980 //bytes
#define FLAG_LENGTH 3
#define MAGIC_LENGTH 6
#define WINDOW_SIZE 20 //segments
#define SENDER 1
#define RECIEVER 0
#define CONNECTION_TIMEOUT 10000 //microseconds, for SYN and FIN 30000
#define PACKET_TIMEOUT 30.0
#define CONNECT 1
#define TEARDOWN 0
#define TIME_WAIT 50000 //needs to be greater than connection time out microseconds  

int sender_sequence_number;
int request_number;
int sequence_base;
int sequence_max;

typedef struct{
	char magic[MAGIC_LENGTH + 1]; //One possible value: CSC361
	char type[FLAG_LENGTH + 1]; //All flags same size
	int sequence_num; 
	int ack_num;
	int payload_len;
	int window;
	char * data; //size will be of payload_length
} segment;

typedef struct {
	int sock_fdesc;
	struct sockaddr_in socket;
	socklen_t socket_length;
} socket_info;

typedef struct {
	segment * packet;
	struct timeval * timestamp;
	char buffer[MAX_PACKET_SIZE + 1];
} queue_packet;

typedef struct{
	int total_bytes;
	int unique_bytes;
	int total_packets;
	int unique_packets;
	int SYN;	
	int FIN;
	int RST_sent;
	int RST_received;
	int ACK;
}log_info;

segment * buffer_to_segment(char * buffer);
char * segment_to_buffer(segment my_segment);
int segment_handle(char * buffer, socket_info my_socket,int flag, FILE * fp,log_info * rec);
void log_segment(char event, struct sockaddr_in * sender, struct sockaddr_in * reciever, segment * packet);
