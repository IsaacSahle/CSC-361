#define MAX_PACKET_SIZE 1024
#define PAYLOAD_LENGTH 980 //bytes
#define FLAG_LENGTH 3
#define MAGIC_LENGTH 6
#define WINDOW_SIZE 20 //segments
#define SENDER 1
#define RECIEVER 0
#define CONNECTION_TIMEOUT 100000 //microseconds
#define PACKET_TIMEOUT 30.0
#define CONNECT 1
#define TEARDOWN 0
#define TIME_WAIT 500000 //microseconds Needs to be at least 2*CONNECTION_TIMEOUT  

int sender_sequence_number;
int request_number;
int sequence_base;

typedef struct{
	char magic[MAGIC_LENGTH + 1];
	char type[FLAG_LENGTH + 1];
	int sequence_num; 
	int ack_num;
	int payload_len;
	int window;
	char * data;
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

/*
 * Function:  buffer_to_segment 
 * --------------------
 * converts a buffer to a segment struct for 
 * ease of processing. Note memory is allocated for segment.
 * 
 * buffer: the buffer to be converted  
 * 
 * returns: pointer to segment 
 */
segment * buffer_to_segment(char * buffer);

/*
 * Function:  segment_to_buffer 
 * --------------------
 * converts a segment to a buffer. Note memory is allocated 
 * for buffer.
 * 
 * my_segment: the segment to be converted  
 * 
 * returns: pointer to buffer 
 */
char * segment_to_buffer(segment my_segment);

/*
 * Function:  segment_handle 
 * --------------------
 * Handles all incoming packets on the recievers end
 * and replies accordingly
 * buffer: the buffer to be converted  
 * my_socket: all socket information needed
 * flag: indicates whether reciever or sender called the function
 * fp: file pointer
 * rec: log information
 * receiver_ip: logging purposes 
 * receiver_port: logging purposes
 * 
 * returns: whether receiver connection has been closed or not 
 */
int segment_handle(char * buffer, socket_info my_socket,int flag, FILE * fp,log_info * rec,char * receiver_ip, int * receiver_port);

/*
 * Function:  log_segment 
 * --------------------
 * Logs segment 
 */
void log_segment(char event,char * source_ip, int source_port, char * destination_ip, int destination_port, segment * packet);
