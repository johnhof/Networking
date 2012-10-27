#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

#define BUFFERSIZE 1024
#define FILENAMESIZE 100

int handle_connection(int sock);
inline void sendMessage(int sock, char *buf, long n);

int main(int argc, char * argv[]) {
  int server_port = -1;
  int rc          =  0;
  int sock        = -1;

  struct sockaddr_in addr;
  const static int BACKLOG = 5;//number of connection can be waiting

  /* parse command line args */
  if (argc != 3) {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
  }

  server_port = atoi(argv[2]);

  if (server_port < 1500) {
    fprintf(stderr, "INVALID PORT NUMBER: %d; can't be < 1500\n", server_port);
    exit(-1);
  }
   
  /*init*/
  if(toupper(argv[1][0]) == 'K') {
    minet_init(MINET_KERNEL);
  }
  else if(toupper(argv[1][0]) == 'U') {
    minet_init(MINET_USER);
  }
  else {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
  }

  /* initialize and make socket */
  sock = minet_socket(SOCK_STREAM);
  if(sock < 0)
  {
    minet_perror("Failed to create socket\n");
    exit(1);
  }

  /* set server address*/
  bzero(&addr, sizeof(addr));  
  addr.sin_port = htons(server_port);
  addr.sin_family = AF_INET;

   
  /* bind listening socket */
  rc = minet_bind(sock, &addr);
  if(rc < 0)
  {
    minet_perror("Failed to bind socket\n");
    exit(2);
  }

  /* start listening */
  rc = minet_listen(sock, BACKLOG);
  if(rc < 0)
  {
    minet_perror("Failed to listen\n");
    exit(3);
  }

  /* connection handling loop: wait to accept connection */
  while (1) {
    int accept_sock = minet_accept(sock,&addr);

    if(accept_sock <= 0) {
      fprintf(stderr, "Failed to accept");
      continue;
    }
    /* handle connections */
    rc = handle_connection(accept_sock);
  }
 
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
