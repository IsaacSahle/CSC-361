#define MAX_PACKET_SIZE 1024
#define PAYLOAD_LENGTH 980 //bytes
#define FLAG_LENGTH 3
#define MAGIC_LENGTH 6
#define WINDOW_SIZE 10 //segments
#define SENDER 1
#define RECIEVER 0
#define CONNECTION_TIMEOUT 1 //seconds, for SYN and FIN
#define PACKET_TIMEOUT 500.0
#define CONNECT 1
#define TEARDOWN 0
 

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
	int sender_bytes;
	int reciever_bytes;
	int SYN_sent;	
	int SYN_recieved;
	int FIN_sent;
	int FIN_recieved;
	int RST_sent;
	int RST_recieved;
	int ACK_sent;
	int ACK_recieved;
}log_info;

segment * buffer_to_segment(char * buffer);
char * segment_to_buffer(segment my_segment);
int segment_handle(char * buffer, socket_info my_socket,int flag, FILE * fp);
