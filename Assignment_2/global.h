#define MAX_PACKET_SIZE 1024
#define PAYLOAD_LENGTH 980 //bytes
#define FLAG_LENGTH 3
#define MAGIC_LENGTH 6
#define WINDOW_SIZE 10 //segments
#define SENDER 1
#define RECIEVER 0
#define CONNECTION_TIMEOUT 2 //seconds
#define MAX 10	//SAME SIZE AS WINDOW!!
#define PACKET_TIMEOUT 2.0
 

int sender_sequence_number;
int request_number;
int sequence_base;
int sequence_max;

typedef struct {
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
} socket_info;

typedef struct {
	segment * packet;
	struct timeval * timestamp;
	char buffer[MAX_PACKET_SIZE + 1];
} queue_packet;

segment * buffer_to_segment(char * buffer);
char * segment_to_buffer(segment my_segment);
int segment_handle(char * buffer, socket_info my_socket,int flag, FILE * fp);
