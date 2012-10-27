#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <streambuf>

using namespace std;

#define BUFFERSIZE 1024
#define FILENAMESIZE 100

int handle_connection(int serverSocket);
inline void sendMessage(int sock, char *buf, long n);

int main(int argc, char * argv[]) {
    int serverPort = -1;
    int rc          =  0;
    int serverSocket  = -1;
    int backlog = 5;
    int sockLimit = -1;
    struct sockaddr_in serverAddr;

    //two file descriptor sets for incoming sockets
    fd_set newFdList;
    FD_ZERO(&newFdList);
    fd_set fullFdList;
    FD_ZERO(&fullFdList);

    //set the timeout value
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 500000;


    /* parse command line args */
    if (argc != 3) {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
    }

    serverPort = atoi(argv[2]);

    if (serverPort < 1500) {
    fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", serverPort);
    exit(-1);
    }

/*initialize and make socket*/

    
if (toupper(*(argv[1])) == 'K') minet_init(MINET_KERNEL);
    else if (toupper(*(argv[1])) == 'U') minet_init(MINET_USER);
    else 
    {
        fprintf(stderr, "usage: http_server1 k|u port\n");
        exit(-1);
    }

    serverSocket = minet_socket(SOCK_STREAM);//create the socket

    //socket error
    if(serverSocket < 0)
    { 
        perror("Failed to create socket");
        exit(1);
    }

    /*set server address*/

    serverAddr.sin_port = htons(serverPort);//Set the Port
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //Support any IP
    serverAddr.sin_family = AF_INET;//set to IP family

    /*bind listening socket*/
    
    if(minet_bind(serverSocket,&serverAddr) < 0)
    {
        perror("Failed to bind socket");
        exit(1);
    }

    /*start listening*/

    if(minet_listen(serverSocket, backlog)){
        perror("Failed to listen");
        exit(1);        
    }


    /*connection handling loop: wait to accept connection*/

    FD_SET(serverSocket, &fullFdList);//add the server socket to the full fd list
    sockLimit = serverSocket;//for the purpose of iteration, we need to know the highest socket value

    while (1) 
    {    
        /*create read list*/
        newFdList = fullFdList;
    
        /*do a select*/
        if(minet_select(sockLimit+1, &newFdList, NULL, NULL, &timeout) < 0)
        {
          fprintf(stderr, "Failed to select\n");
          exit(-1);
        }
        /*process sockets that are ready*/
        for(int fdCount=0; fdCount<=sockLimit; fdCount++) //loop through all socket file desciptors less than the limit
        {
            if(FD_ISSET(fdCount, &newFdList)) //if a socket file descriptor exists in our list
            {
                if(fdCount == serverSocket) //if the socket from our list is the server socket
                {
                    /*for the accept socket, add accepted connection to connections*/
                    //wait for a new connection
                    int newSocket = minet_accept(serverSocket, &serverAddr);
                  
                    if(newSocket >=0) //if the connection was succesful, add it to the main fd set
                    {
                        FD_SET(newSocket, &fullFdList);

                        //if we have a new highest socket, remember it
                        if(newSocket > sockLimit) 
                        {
                            sockLimit = newSocket;
                        }
                    }
                    else //otherwise, start again
                    {
                        fprintf(stderr, "Failed to accept socket %d\n",serverSocket);
                        continue;
                    }
                }
                else //otherwise, handle the connection and remove the socket from our list
                {
                /*for a connection socket, handle the connection*/
                    rc = handle_connection(fdCount);
                    FD_CLR(fdCount,&fullFdList);
                }
            } 
        }
    }
    minet_close(serverSocket);
    minet_deinit();
}

int handle_connection(int sock) {
  bool ok = false;
  string request_string;
  string filename;
  char buf[BUFFERSIZE];//create buffer

  static char * ok_response_f = "HTTP/1.0 200 OK\r\n"   \
    "Content-type: text/plain\r\n"                      \
    "Content-length: %d \r\n\r\n";
  static const int ok_response_len = strlen(ok_response_f);
  static char * notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"      \
    "Content-type: text/html\r\n\r\n"                   \
    "<html><body bgColor=black text=white>\n"           \
    "<h2>404 FILE NOT FOUND</h2>\n"
    "</body></html>\n";
  static const int notok_response_len = strlen(notok_response);
   
  /* first read loop -- get request and headers*/
  int size = 0;
  do {    
    buf[size] = '\0';
    request_string += buf;
    if(request_string.size()>=4 && 0 == request_string.compare(request_string.size()-4,4,"\r\n\r\n")) {
      break;
    }
    size = minet_read(sock, buf,BUFFERSIZE);
    if(size == 0) {
      fprintf(stderr, "Connection Terminated\n");
      return -1;
    }
    else if(size == -1) {//assuming this will never happen
      fprintf(stderr, "Read Failure\n");
      return -1;
    }
  }while(size > 0);
   
  /* parse request to get file name */
  /* Assumption: this is a GET request and filename contains no spaces*/

  filename = request_string.substr(4,request_string.find(" ", 4)-4);
  if(filename[0] == '\\' || filename[0] == '/') filename = filename.substr(1);
 
  /* try opening the file */
  ifstream fin;// input file stream
  fin.open(filename,ios::in);//open file
  request_string.clear();
  if(fin.is_open()) {
    string tmp;
    while(fin.good()) {
      getline(fin, tmp);
      request_string += tmp;
    }
    ok = true;
  }else {//fail to open file
    ok =false;
  }
  fin.close();

  /* send response */
  if (ok) {
    /* send headers */
    char header[ok_response_len + 20];
    sprintf(header, ok_response_f, request_string.size());

    sendMessage(sock, header, strlen(header));
    /* send file */
    char *contentTmp = new char[request_string.size()+1];//create a tmp buffer
    strcpy(contentTmp, request_string.data());
    contentTmp[request_string.size()]= '\0';
    sendMessage(sock, contentTmp,request_string.size());
    delete contentTmp;
   
  } else {
    sendMessage(sock, notok_response, notok_response_len);
  }
   
  /* close socket and free space */
  minet_close(sock);
   
  if(ok) {
    return 0;
  } else {
    return -1;
  }
}

/*inline function for sending msg over socket*/
inline void sendMessage(int sock, char *buf, long n) {
  long total_bytes_sent = 0;
  while(total_bytes_sent < n) {
    int size = minet_write(sock, buf+total_bytes_sent, n-total_bytes_sent);
    if(size > 0) {
      total_bytes_sent += size;
    }
  }
}
