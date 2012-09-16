#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h> //for sock_addr
#define BUFSIZE 1024

int main(int argc, char * argv[]) {

    char * server_name = NULL;
    int server_port    = -1;
    char * server_path = NULL;
    char * req         = NULL;
    bool ok            = false;
	int socket_fd;

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
	// hostent is the struct that is returned by gethostbyname(). Host_address
	// is the struct name that is going to hold all the information that is returned
	hostent * host_address = gethostbyname(server_name);

	if(host_address == null){
		minet_perror("host not found");
		exit(2);
	}
	
    /* set address */

    /* connect to the server socket */

    /* send request message */
    sprintf(req, "GET %s HTTP/1.0\r\n\r\n", server_path);

    /* wait till socket can be read. */
    /* Hint: use select(), and ignore timeout for now. */

    /* first read loop -- read headers */

    /* examine return code */   

    //Skip "HTTP/1.0"
    //remove the '\0'

    // Normal reply has return code 200

    /* print first part of response: header, error code, etc. */

    /* second read loop -- print out the rest of the response: real web content */

    /*close socket and deinitialize */

    if (ok) {
	return 0;
    } else {
	return -1;
    }
}
