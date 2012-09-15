#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100

int handle_connection(int sock);

int main(int argc, char * argv[]) {
    int serverPort = -1;
    int rc          =  0;
    int sock        = -1;
    char* serverName;
    sockaddr_in socket;

/*--parse command line args-----------------------------------------------------*/
    if (argc != 3) {
	fprintf(stderr, "usage: http_server1 k|u port\n");
	exit(-1);
    }

    serverName = argv[0];

    serverPort = atoi(argv[2]);

    if (serverPort < 1500) {
	fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", serverPort);
	exit(-1);
    }

/*--initialize and make socket---------------------------------------------------*/

    if (toupper(*(argv[1])) == 'K') minet_init(MINET_KERNEL);
    else if (toupper(*(argv[1])) == 'U') minet_init(MINET_USER);
    else {
        fprintf(stderr, "First argument must be k or u\n");
        exit(-1);
    }

    socket = minet_socket(SOCK_STREAM);

    if(socket < 0)
    { //Socket didn't work.
        perror("Socket not created");
        exit(1);
    }
/*--set server address----------------------------------------------------------*/


/*--bind listening socket-------------------------------------------------------*/
    
    minet_bind(serverPort,serverAddr);
/*--start listening-------------------------------------------------------------*/

/*--connection handling loop: wait to accept connection-------------------------*/

    while (1) {
	/* handle connections */
	rc = handle_connection(sock);
    }
}

int handle_connection(int sock) {
    bool ok = false;

    char * ok_response_f = "HTTP/1.0 200 OK\r\n"	\
	"Content-type: text/plain\r\n"			\
	"Content-length: %d \r\n\r\n";
 
    char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"	\
	"Content-type: text/html\r\n\r\n"			\
	"<html><body bgColor=black text=white>\n"		\
	"<h2>404 FILE NOT FOUND</h2>\n"
	"</body></html>\n";
    
/*--first read loop -- get request and headers-----------------------------------*/
   
/*--parse request to get file ---------------------------------------------------*/
/*--Assumption: this is a GET request and filename contains no spaces------------*/

/*--try opening the file---------------------------------------------------------*/

/*--send response----------------------------------------------------------------*/
    if (ok) {
/*--send headers-----------------------------------------------------------------*/
	
/*--send file--------------------------------------------------------------------*/
	
    } else {
// send error response
    }
    
 /*--close socket and free space-------------------------------------------------*/
  
    if (ok) {
        minet_deinit();
	return 0;
    } else {
	return -1;
    }
}
