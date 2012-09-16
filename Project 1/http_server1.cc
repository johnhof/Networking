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
    char* serverName;
    int serverSocket = -1;
    struct sockaddr_in severAddr;
    int backLog = 5;

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

    serverSocket = minet_socket(SOCK_STREAM);//create the socket

    //socket error
    if(serverSocket < 0)
    { 
        perror("Socket not created");
        exit(1);
    }

/*--set server address----------------------------------------------------------*/

    memset(&serverAddr.sin_addr, 0, sizeof(serverAddr));
    serverAddr.sin_port = htons(serverPort);//Set the Port
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //Support any IP
    serverAddr.sin_family = AF_INET;//set to IP family

/*--bind listening socket-------------------------------------------------------*/
    
    minet_bind(serverSocket,serverAddr,sizeof(serverAddr));

/*--start listening-------------------------------------------------------------*/

    minet_listen(serverPort, backLog);

/*--connection handling loop: wait to accept connection-------------------------*/

    while (1) {
    int incomingSocket -1;
    struct sockaddr_in incomingAddr;

    int incomingSocket = accept(serverSocket, (struct sockaddr *)&incomingAddr, sizeof(incomingAddr));

	/* handle connections */
	rc = handle_connection(incomingSocket);
    }
}

/*-----------------------------------------------------------------------------*/
/*HANDLE CONNECTION*/
/*-----------------------------------------------------------------------------*/

int handle_connection(int serverSocket) {
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
/*Assumption: this is a GET request and filename contains no spaces*/
    buf[BUFSIZE-1] = '\0'; //Add this to the end so it can print as a string.
    int bytesRead = minet_read(client_socket, buf, BUFSIZE-1);
    printf("bytesread= %d\n", bytesRead);
    if(bytesRead < 0 || bytesRead >= 12){
        perror("Couldn't read");
        exit(1);
    }

/*--try opening the file---------------------------------------------------------*/

/*--send response----------------------------------------------------------------*/
    if (ok) {
/*--send headers-----------------------------------------------------------------*/
	minet_read();
/*--send file--------------------------------------------------------------------*/
	minet_write();
    } else {
// send error response
    }
    
 /*--close socket and free space-------------------------------------------------*/
  
    if (ok) {
        minet_close(serverSocket);
        minet_deinit();
	return 0;
    } else {
	return -1;
    }
}
