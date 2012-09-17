#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h> //for sock_addr
#define BUFSIZE 1024

int main(int argc, char * argv[]) {

	fd_set read_sock;
	char buffer[BUFSIZE];
    char * server_name = NULL;
    int server_port    = -1;
    char * server_path = NULL;
    char * req         = NULL;
    bool ok            = false;
	int req_length;
	int num_sent;
	int socket_fd;
	int client_socket;
	char return_code[3];

    /*parse args */
    if (argc != 5) {
	fprintf(stderr, "usage: http_client k|u server port path\n");
	exit(-1);
    }

    server_name = argv[2];
    server_port = atoi(argv[3]);
    server_path = argv[4];

    req = (char *)malloc(strlen("GET  HTTP/1.0\r\n\r\n") 
			 + strlen(server_path) + 1);  

    /* initialize */
    if (toupper(*(argv[1])) == 'K') { 
	minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') { 
	minet_init(MINET_USER);
    } else {
	fprintf(stderr, "First argument must be k or u\n");
	exit(-1);
    }

    /* make socket */

	socket_fd = minet_socket(SOCK_STREAM);
	
	if(socket_fd < 0){
		minet_perror("Socket Not Created");
		exit(1);
	}
	
    /* get host IP address  */
    /* Hint: use gethostbyname() */
	// hostent is the struct that is returned by gethostbyname(). host_addr
	// is the struct name that is going to hold all the information that is returned
	hostent * host_addr = gethostbyname(server_name);

	if(host_address == null){
		minet_perror("host not found");
		exit(2);
	}
	
    /* set address */
	memcpy(&server_socket.sin_addr, host_addr->h_addr_list[0], host_addr->h_length);
	server_socket.sin_family = AF_INET;
	server_socket.sin_port = htons(server_port);

    /* connect to the server socket */
	
	client_socket = minet_connect(socket_fd, &server_socket);
	if(client_socket < 0) {
		minet_perror("could not connect");
		exit(3);
	}

    /* send request message */
    sprintf(req, "GET %s HTTP/1.0\r\n\r\n", server_path);
	req_length = strlen(req);
	
	// while there is still stuff to send
	while(req_length > 0){
		// write and save length written
		num_sent = minet_write(client_socket, req, len);
		
		// if it didn't sent anything, error
		if(num_sent < 0){
			minet_perror("could not send request");
			exit(4);
		}
		
		// request length minus the total amount sent
		req_length -= num_sent;
	}

    /* wait till socket can be read. */
    /* Hint: use select(), and ignore timeout for now. */
	
	// have the fd be 0's
	FD_ZERO(&read_sock);
	
	// set the fd
	FD_SET(client_socket, $read_sock);
	
	// no set timeout so it will wait until it is ready
	int select_current = minet_select(sizeof(read_sock)*8, &read_sock, NULL, NULL, NULL);
	
	if(select_current < 0){
		minet_perror("select failed");
		exit(5);
	}
	
    /* first read loop -- read headers */
	buf[BUFSIZE-1] = '\0';
	int bytes_read = minet_read(client_socket, buf, BUFSIZE-1);
	printf("bytesread= %d\n", bytes_read);
	if(bytes_read < 0 || byte_read >= 12){
		minut_perror("Couldn't read");
		exit(6);
	}

    /* examine return code */

    //Skip "HTTP/1.0"
    //remove the '\0'

    // Normal reply has return code 200	
	return_code[0]= buffer[9];
	return_code[1]= buffer[10];
	return_code[2]= buffer[11];
	int return_code_i = atoi(return_code);
	if(return_code_i == 200){
		ok = true;
	}

    /* print first part of response: header, error code, etc. */
	printf("%s", buffer);	
	
    /* second read loop -- print out the rest of the response: real web content */
	do{
		bytes_read = minet_read(client_socket, buffer, BUFSIZE);
		if(bytes_read < 0){
			minut_perror("Couldn't read");
			exit(1);
		}
		printf("%s", buffer);
		printf("read is %d\n", bytes_read);
	}while(read > 0);
/*close socket and deinitialize */
	minet_close(client_socket);
	free(req);
    /*close socket and deinitialize */
	minet_close(client_socket);
	free(req);
    if (ok) {
		return 0;
    } else {
		return -1;
    }
}
