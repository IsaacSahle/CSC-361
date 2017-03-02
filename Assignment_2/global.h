#define MAX_PACKET_SIZE 1024
#define FLAG_LENGTH 3
#define MAGIC_LENGTH 6
#define WINDOW_SIZE 10 //segments
#define SENDER 1
#define RECIEVER 0 

typedef struct {
	char magic[MAGIC_LENGTH]; //One possible value: CSC361
	char type[FLAG_LENGTH]; //All flags same size
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

segment * buffer_to_segment(char * buffer);
char * segment_to_buffer(segment my_segment);
void segment_handle(char * buffer, socket_info my_socket,int flag);
