#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100

int handle_connection(int sock);

int main(int argc, char * argv[]) 
{
    int serverPort = -1;
    int rc          =  0;
    char* serverName;
    int serverSocket = -1;
    struct sockaddr_in serverAddr;
    int backlog = 10;

/*--parse command line args-----------------------------------------------------*/
    if (argc != 3) 
    {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
    }

    serverName = argv[0];
    serverPort = atoi(argv[2]);


    if (serverPort < 1500) 
    {
       fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", serverPort);
       exit(-1);
    }

/*--initialize and make socket---------------------------------------------------*/

    if (toupper(*(argv[1])) == 'K') minet_init(MINET_KERNEL);
    else if (toupper(*(argv[1])) == 'U') minet_init(MINET_USER);
    else 
    {
        fprintf(stderr, "First argument must be k or u\n");
        exit(-1);
    }

    serverSocket = minet_socket(SOCK_STREAM);//create the socket

    //socket error
    if(serverSocket < 0)
    { 
        perror("Failed to create socket");
        exit(1);
    }

/*--set server address----------------------------------------------------------*/

    serverAddr.sin_port = htons(serverPort);//Set the Port
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //Support any IP
    serverAddr.sin_family = AF_INET;//set to IP family
    printf("Server: %d\n", serverPort);

/*--bind listening socket-------------------------------------------------------*/
    
    if(minet_bind(serverSocket,&serverAddr) < 0)
    {
        perror("Failed to bind socket");
        exit(1);
    }

/*--start listening-------------------------------------------------------------*/

    if(minet_listen(serverSocket, backlog)){
        perror("Failed to listen");
        exit(1);        
    }

/*--connection handling loop: wait to accept connection-------------------------*/

    while (1) {
    int incomingSocket = -1;
    struct sockaddr_in incomingAddr;
    incomingSocket = minet_accept(serverSocket, &incomingAddr);
    /* handle connections */
    rc = handle_connection(incomingSocket);
    }
    
    minet_close(serverSocket);
    minet_deinit();

}

/*-----------------------------------------------------------------------------*/
/*HANDLE CONNECTION*/
/*-----------------------------------------------------------------------------*/

int handle_connection(int clientSocket) {
    bool ok = false;
    char * buf;
    memset(buf, 0, BUFSIZE);
    String headerStr;
    int readSize;

    char * ok_response_f = "HTTP/1.0 200 OK\r\n"    \
    "Content-type: text/plain\r\n"          \
    "Content-length: %d \r\n\r\n";
 
    char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"   \
    "Content-type: text/html\r\n\r\n"           \
    "<html><body bgColor=black text=white>\n"       \
    "<h2>404 FILE NOT FOUND</h2>\n"
    "</body></html>\n";
    
/*--first read loop -- get request and headers-----------------------------------*/
    while(true)
    {
        readSize = minet_read(clientSocket, buf, BUFSIZE-1);//read the first chunk of data
        

        if(readSize + headerStr.size() <= 4)//if there are 4 or fewer char's in the header
        {
            // if there are less than 4 char's, the header isn't properly formatted
            if(readSize<4)fprintf(stderr, "Header not formatted properly\n", ); 
            // if there are 4 char's, no file was specified
            if(readSize==4)fprintf(stderr, "No file specified\n", );
            return -1;
        }
       
        buf[readSize] = '\0';//add a NULL char so it can be read as a string
        headerStr.append(buf);
        if(buf.compare(strReq.size()-4,4,"\r\n\r\n"))
    }



/*--parse request to get file ---------------------------------------------------*/
/*Assumption: this is a GET request and filename contains no spaces*/


/*--try opening the file---------------------------------------------------------*/

/*--send response----------------------------------------------------------------*/
    if (ok) {
/*--send headers-----------------------------------------------------------------*/
    //minet_read();
/*--send file--------------------------------------------------------------------*/
    //minet_write();
    } else {
// send error response
    }
    
 /*--close socket and free space-------------------------------------------------*/
  
    if (ok) {
        minet_close(clientSocket);
    return 0;
    } else {
    return -1;
    }
}
