#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <streambuf>

using namespace std;

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
    int backlog = 5;

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
        struct sockaddr_in incomingAddr;
        int incomingSocket = minet_accept(serverSocket, &incomingAddr);

        if(incomingSocket <= 0) {
            fprintf(stderr, "No socket accepted");
            perror("Could not accept socket connection");
            continue;
        }
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
    char buffer [BUFSIZE];
    string headerStr;
    string filePath;
    string dataFromFile;
    int readSize;
    int sizeToSend;


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
        readSize = minet_read(clientSocket, buffer, BUFSIZE-1);//read the first chunk of data
        if(readSize + headerStr.size() < 4 || readSize == 0)//check header integrity
        {
            if(readSize + headerStr.size()<4)fprintf(stderr, "Header not formatted properly\n"); 
            if(readSize==0)fprintf(stderr, "Header not formatted properly\n"); 
            return -1;
        }
        buffer[readSize] = '\0';//add a NULL char so it can be read as a string
        headerStr.append(buffer);//append the buffer to the header string

        //if we have read the header terminating string, break
        if(headerStr.substr(headerStr.size()-4).compare("\r\n\r\n") == 0)break;
    }

/*--parse request to get file ---------------------------------------------------*/
/*Assumption: this is a GET request and filename contains no spaces*/
 
    filePath.assign(headerStr.substr(4,headerStr.find(" ", 4)-4));//cut off "GET " and "HTTP...\n"


/*--try opening the file---------------------------------------------------------*/

    //open the file
    ifstream fileStream(filePath);
    if(fileStream.good())
    {
        ok = true;
        //allocate the array memory before copying
        fileStream.seekg(0, std::ios::end);   
        dataFromFile.reserve(fileStream.tellg());
        fileStream.seekg(0, std::ios::beg);


        //dump the file to a string
        dataFromFile = string((std::istreambuf_iterator<char>(fileStream)),
                                std::istreambuf_iterator<char>());
    }

/*--send response----------------------------------------------------------------*/
    if (ok) {
/*--send headers-----------------------------------------------------------------*/
        //char headerStr[sizeof(ok_response_f)+sizeof(dataFromFile.size())];
        char headerStr[sizeof(ok_response_f)+dataFromFile.size()];
        
        //insert the length of the file into our headers file size indicator
        sprintf(headerStr,ok_response_f,dataFromFile.size());
        unsigned int sizeSent = 0;


        //while we have data to send
        unsigned int sizeToSend =  strlen(headerStr);
        while(sizeSent<sizeToSend) {
            //send it
            sizeSent = minet_write(clientSocket, headerStr+sizeSent, strlen(headerStr));
        
            // if it didn't sent anything, error
            if(sizeSent < 0){
                minet_perror("could not send header");
                return -1;
            }
            sizeSent++;
        }



/*--send file--------------------------------------------------------------------*/

        sizeSent = 0;
        //printf("about to send file: %i\n",dataFromFile.size());
        //while we have data to send
        while(sizeSent<dataFromFile.size()) {
            string dataToSend = dataFromFile.substr(sizeSent);
            //send it
            sizeSent = minet_write(clientSocket, ((char *)(dataToSend.c_str()))+sizeSent, dataToSend.size());
        
            // if it didn't sent anything, error
            if(sizeSent < 0){
                minet_perror("No data sent");
                return -1;
            }
            sizeSent++;
        }
    } else {//send error message
        int sizeSent = 0;
        sizeToSend =  strlen(notok_response);
        while(sizeSent<sizeToSend) {
            //send it
            sizeSent = minet_write(clientSocket, notok_response+sizeSent, strlen(notok_response));
        
            // if it didn't sent anything, error
            if(sizeSent < 0){
                minet_perror("Could not send error");
                return -1;
            }
            sizeSent++;
        }
    }
    
 /*--close socket and free space-------------------------------------------------*/
  
    minet_close(clientSocket);
    minet_deinit();
    
    if (ok) {
        return 0;
    } else {
        return -1;
    }
}