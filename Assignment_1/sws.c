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
#include <fcntl.h>
#include <time.h>
#include <termios.h>

#define BUFFER_SIZE 1024 
#define METHOD "get"
#define HTTP_VERSION "http/1.0"
#define MAX_BUFFER 2000
#define UDP_CHUNCK 500
#define NUM_TOKENS 3
#define UNDETERMINED -1


void print_log_message(char * search_dir,char * buff,char * response,char * serving_dir,struct sockaddr_in source_sock,int port_num){


	//build log without date info	
	//https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm
	char *ip = inet_ntoa(source_sock.sin_addr);
	int port = htons(source_sock.sin_port);	//
	time_t rawtime;
	struct tm *info;
	time(&rawtime);
	info = localtime(&rawtime);

  	char * mytime = asctime(info);
	mytime[20] = '\0';
	if(mytime[8] == ' ') mytime[8] = '0';
	mytime = mytime + 4;
	
	//strip whitespace off request and response 
	int response_length = strlen(response);	
	int buff_length = strlen(buff);
	char * p = response + (response_length - 1);
	char * x = buff + (buff_length - 1);
	int count_response = 0;	
	int count_buff = 0;	
	while(isspace((unsigned char)*p)){
	 p--;
	 count_response++;
	}

	while(isspace((unsigned char)*x)){
	 x--;
	 count_buff++;
	}

	int index_response = (response_length - count_response);
	int index_buff = (buff_length - count_buff);	
	char my_copy_response[index_response + 1];
	char my_copy_buff[index_buff + 1];
	memset(my_copy_response,'\0',index_response + 1);
	memset(my_copy_buff,'\0',index_buff + 1);

	strncpy(my_copy_response,response,index_response);
	strncpy(my_copy_buff,buff,index_buff);
	
	if(search_dir[0] == '/'){
	(strcmp(search_dir,"/") == 0)?fprintf(stdout,"%s %s:%d %s; %s; %s%sindex.html\n\n",mytime,ip,port,my_copy_buff,my_copy_response,serving_dir,search_dir):
	fprintf(stdout,"%s %s:%d %s; %s; %s%s\n\n",mytime,ip,port,my_copy_buff,my_copy_response,serving_dir,search_dir);

	}else{
		fprintf(stdout,"%s %s:%d %s; %s; %s/%s\n\n",mytime,ip,port,my_copy_buff,my_copy_response,serving_dir,search_dir);
	}


}



void build_send_response(int code,struct sockaddr_in source_sock, FILE * file,int fd, int socket,char * buff,char * serving_dir,int port_num,char * search_dir){
	
	char * response;	
	switch(code){

	case 400:
	response = "HTTP/1.0 400 Bad Request\r\n\r\n";
	sendto(socket,response,strlen(response)+1,0,(struct sockaddr*)&source_sock,(sizeof source_sock));
	print_log_message(search_dir,buff,response,serving_dir,source_sock,port_num);
	return;
	break;
	
	case 404:
	response = "HTTP/1.0 404 Not Found\r\n\r\n";
	sendto(socket,response,strlen(response)+1,0,(struct sockaddr*)&source_sock,(sizeof source_sock));
	print_log_message(search_dir,buff,response,serving_dir,source_sock,port_num);
	return;
	break;

	case 200:
	response = "HTTP/1.0 200 OK\r\n\r\n";
	break;

	}

	struct stat f_state; 
	fstat(fd,&f_state);
	char * ending = "\r\n\r\n";
	char * complete_buffer = (char *) malloc(strlen(response) + 2 + f_state.st_size + strlen(ending));

	char file_contents[f_state.st_size];
	char temp[f_state.st_size];
	memset(file_contents,'\0',f_state.st_size);
	strcpy(complete_buffer,response);
	

	while(fgets(temp,f_state.st_size,file) != NULL){
		strcat(file_contents,temp);
	}

	strcat(complete_buffer,file_contents);
	strcat(complete_buffer,ending);
	int mess_len = strlen(complete_buffer) + 1;
	if(mess_len <= MAX_BUFFER){
		sendto(socket,(void *)complete_buffer,mess_len,0,(struct sockaddr*)&source_sock,(sizeof source_sock));
		print_log_message(search_dir,buff,response,serving_dir,source_sock,port_num);
		free(complete_buffer);
		return; 
	}
	
	//message needs to be broken into chuncks
	char * p = complete_buffer;	
	while(mess_len >= 0){
		char temp[UDP_CHUNCK + 1]; 
		strncpy(temp,p,UDP_CHUNCK);
		sendto(socket,(void *)temp,strlen(temp) + 1,0,(struct sockaddr*)&source_sock,(sizeof source_sock));
		 mess_len = mess_len - UDP_CHUNCK;
		 p = p + UDP_CHUNCK;
	}
	//ending
	print_log_message(search_dir,buff,response,serving_dir,source_sock,port_num);

	free(complete_buffer);
}

//stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int check_tokens(char * tokens [],char * buff){


	int i;
	for(i = 0; tokens[0][i] != '\0';i++){
		tokens[0][i] = tolower(tokens[0][i]);
	}

	for(i = 0; tokens[2][i] != '\0';i++){
		tokens[2][i] = tolower(tokens[2][i]);
	}

	//change to lower case 0 and 2
	if(strcmp(tokens[0],METHOD) != 0){
		return 400;
	}else if(strcmp(tokens[2],HTTP_VERSION) != 0){
		return 400;
	}else if(tokens[1][0] != '/'){
		return 400;
	}else if(buff[strlen(buff) - 1] != '\n'){
		return 400;
	}else if(strstr(tokens[1],"/../") != NULL){
		return 404;
	}else{
		return UNDETERMINED;
	}
	

}

int request_handle(char * buff,char * serving_dir,struct sockaddr_in  source_sock, int socket, int port_num){

	char * token;
	char * saved_pointer;
	char buff_copy[BUFFER_SIZE];
	char * my_tokens[NUM_TOKENS];
	//null terminate buffer
	buff[BUFFER_SIZE - 1] = '\0';

	strncpy(buff_copy,buff,BUFFER_SIZE);

	token = strtok_r(buff_copy," \t\n\v\f\r",&saved_pointer);
	int token_num = 0;

	while(token != NULL && token_num < 4){
		my_tokens[token_num] = strdup(token);  
		token = strtok_r(NULL," \t\n\v\f\r",&saved_pointer);
		token_num++;
	}
	//FREE my_tokens!!
	int response_code = check_tokens(my_tokens,buff);
	
	if(response_code != UNDETERMINED){
		build_send_response(response_code,source_sock,NULL,0,socket,buff,serving_dir,port_num,my_tokens[1]);
		free(my_tokens[0]);
		free(my_tokens[1]);
		free(my_tokens[2]);
		return 1;
	}

	//Lets begin locating file on server 
	char * final_directory = (char *) malloc(strlen(my_tokens[1]) + strlen(serving_dir) + 2);
	//drop the initial forward slash
	//serving_dir++;
	strcpy(final_directory,serving_dir);
	strcat(final_directory,my_tokens[1]);

	//check if looking for default index.html
	if(strcmp(my_tokens[1],"/") == 0){
		final_directory = (char *) realloc(final_directory,strlen("index.html") + 1);
		strcat(final_directory,"index.html");
	}

	//open file
	FILE * fp = NULL;

	if(is_regular_file(final_directory))
	fp = fopen(final_directory,"r");

	//check if file is here
	if(fp == NULL){
		response_code = 404;
		build_send_response(response_code,source_sock,NULL,0,socket,buff,serving_dir,port_num,my_tokens[1]);
	}else{
		//file existtttt!!! Sweet dude
		response_code = 200;
		int fd = open(final_directory,O_RDONLY);
		build_send_response(response_code,source_sock,fp,fd,socket,buff,serving_dir,port_num,my_tokens[1]);
		close(fd);
		fclose(fp);
	}

	free(final_directory);
	free(my_tokens[0]);
	free(my_tokens[1]);
	free(my_tokens[2]);

	return 1;

}


int main(int argc, char * argv[]){
	//validate number of parameters 
	if(argc != 3){
		fprintf(stderr,"Usage: ./sws <port> <directory>\n");
		exit(EXIT_FAILURE);
	}

	char * ptr;
	int port_number = strtol(argv[1],&ptr,10);
	//valid port number
	if(ptr == argv[1]){
	    fprintf(stderr,"Invalid Port Number");
    	    exit(EXIT_FAILURE);
	}

	//create endpoint for communication
	int socket_udp = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	
	//create socket structure
	struct sockaddr_in sockd;
	//set all fields
	memset(&sockd, 0, sizeof sockd);
	sockd.sin_family = AF_INET;
	sockd.sin_port = htons(port_number);
	sockd.sin_addr.s_addr = htonl(INADDR_ANY);
	socklen_t socket_length = sizeof sockd; 
	
	int option = 1;
	//set socket option 
	if (setsockopt(socket_udp,SOL_SOCKET,SO_REUSEADDR,&option,sizeof option) == -1){
		fprintf(stderr,"Set Socket Option Failed\n");	
	    close(socket_udp);
    	exit(EXIT_FAILURE);
	}

	//bind socket
	if(bind(socket_udp,(struct sockaddr *)&sockd, sizeof sockd) == -1){
       	//bind failed
	    fprintf(stderr,"Binding failed\n");	
	    close(socket_udp);
    	exit(EXIT_FAILURE);
	}

	//successful binded socket to port
	fprintf(stdout,"sws is running on UDP port %d and serving %s\npress \'q\' to quit...\n",port_number,argv[2]);

	ssize_t recieved;
	char buffer[BUFFER_SIZE];
	size_t buff_size = sizeof buffer;

	fd_set fd;	
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	int max_descriptor = (socket_udp > STDIN_FILENO)?socket_udp:STDIN_FILENO;
	
	//stackoverflow.com/questions/2984307/how-to-handle-key-pressed-in-a-linux-console-in-c/2984565#2984565
    struct termios orig_term_attr;
    struct termios new_term_attr;

    /* set the terminal to raw mode */
    tcgetattr(fileno(stdin), &orig_term_attr);
    memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN] = 0;
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

	while(1){

		FD_ZERO(&fd);
		FD_SET(STDIN_FILENO,&fd);
		FD_SET(socket_udp,&fd);
		
		select((max_descriptor + 1),&fd,NULL,NULL,&timeout);
		
		
		if(FD_ISSET(STDIN_FILENO, &fd)){
		    char c = getchar();
		    if(c == 'q') 
			break;
		  		
		}else if(FD_ISSET(socket_udp, &fd)){
			//listening
			memset(buffer,0,buff_size);
			recieved = recvfrom(socket_udp,(void*)buffer,buff_size,0, (struct sockaddr*)&sockd,&socket_length);			
	
			if (recieved < 0) {
		      	    fprintf(stderr, "recvfrom failed\n");
		      	    exit(EXIT_FAILURE);
		    	}
		    	
			//handle incoming request
			request_handle(buffer,argv[2],sockd,socket_udp,port_number);

		}else{
		//nothing
		}

	}
	
	/* restore the original terminal attributes */
    	tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);
	//close socket
	close(socket_udp);
	
	return 0;
}
