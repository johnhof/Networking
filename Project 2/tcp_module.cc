// You will build this in project part B - this is merely a
// stub that does nothing but integrate into the stack

// For project parts A and B, an appropriate binary will be 
// copied over as part of the build process



#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#include <iostream>

#include "Minet.h"

using namespace std;

enum eState { CLOSED      = 0,
	      LISTEN      = 1,
	      SYN_RCVD    = 2,
	      SYN_SENT    = 3,
	      SYN_SENT1   = 4,
	      ESTABLISHED = 5,
	      SEND_DATA   = 6,
	      CLOSE_WAIT  = 7,
	      FIN_WAIT1   = 8,
	      CLOSING     = 9,
	      LAST_ACK    = 10,
	      FIN_WAIT2   = 11,
	      TIME_WAIT   = 12 };

struct TCPState {
    // need to write this
    eState stateNUM;

    std::ostream & Print(std::ostream &os) const { 
	os << "TCPState()" ; 
	return os;
    }
};


int main(int argc, char * argv[]) {
    MinetHandle mux;
    MinetHandle sock;
    
    ConnectionList<TCPState> clist;

    MinetInit(MINET_TCP_MODULE);

    mux = MinetIsModuleInConfig(MINET_IP_MUX) ?  
	MinetConnect(MINET_IP_MUX) : 
	MINET_NOHANDLE;
    
    sock = MinetIsModuleInConfig(MINET_SOCK_MODULE) ? 
	MinetAccept(MINET_SOCK_MODULE) : 
	MINET_NOHANDLE;

    if ( (mux == MINET_NOHANDLE) && 
	 (MinetIsModuleInConfig(MINET_IP_MUX)) ) {

	MinetSendToMonitor(MinetMonitoringEvent("Can't connect to ip_mux"));

	return -1;
    }

    if ( (sock == MINET_NOHANDLE) && 
	 (MinetIsModuleInConfig(MINET_SOCK_MODULE)) ) {

	MinetSendToMonitor(MinetMonitoringEvent("Can't accept from sock_module"));

	return -1;
    }
    
    cerr << "tcp_module STUB VERSION handling tcp traffic.......\n";

    MinetSendToMonitor(MinetMonitoringEvent("tcp_module STUB VERSION handling tcp traffic........"));

    MinetEvent event;
    double timeout = 1;

    while (MinetGetNextEvent(event, timeout) == 0) {

		if ((event.eventtype == MinetEvent::Dataflow) && 
		    (event.direction == MinetEvent::IN)) {
		
		    if (event.handle == mux) {
			// ip packet has arrived!
		    }

		    if (event.handle == sock) {
			// socket request or response has arrived
			
				{SockRequestResponse req;
			MinetReceive(sock,req);
			
			switch (req.type) {
				case CONNECT
				{
					SockRequestResponse response;

					response.type=STATUS;
					response.connection=req.connection;
					response.bytes=0;
					response.error=EOK;

//------------------is this where we do the handshake?--------------------------------------
					
					MinetSend(sock,response);
				}
				case ACCEPT:
				{ 
					SockRequestResponse response;
				    response.bytes=0;
				    response.error=EOK;
				    MinetSend(sock,repl);
				}
				  break;
				case STATUS:
				  // ignored, no response needed
				  break;
				  // case SockRequestResponse::WRITE:
				case WRITE:
				{
					//retrieve the data from the request
					int len = req.data.GetSize();
					char *buffer = (char *) malloc(len);
					req.data.GetData(buffer,len,0); 
					
					//retrieve the connection
					ConnectionList<TCPDriver>::iterator cs = clist.FindMatching(req.connection);
					unsigned bytes = (*cs).state.packetMailer(false,false,false,buffer,len);
					free(buffer);

					SockRequestResponse response;
					response.type=STATUS;
					response.connection=req.connection;
					response.error=EOK;
					response.bytes=bytes;

					MinetSend(sock,response);	
				}
				  break;
				  // case SockRequestResponse::FORWARD:
				case FORWARD:
				{
					// We dont need to deal with this
					SockRequestResponse response;
					response.type=STATUS;
					response.error=EOK;
					MinetSend(sock,response);	
				}
				  break;
				  // case SockRequestResponse::CLOSE:
				case CLOSE:
				{
					//retrieve the connection
					ConnectionList<TCPDriver>::iterator cs = clist.FindMatching(req.connection);
					
					SockRequestResponse response;
	            	response.connection=req.connection;
	            	response.type=STATUS;
	            	if (cs==clist.end()) {
	              		response.error=ENOMATCH;
	            	} else {
	             		response.error=EOK;
	              		clist.erase(cs);
	            	}
	           		MinetSend(sock,response);
				}
				  break;
				default:
				    SockRequestResponse repl;
				    // repl.type=SockRequestResponse::STATUS;
				    repl.type=STATUS;
				    repl.error=EWHAT;
				    MinetSend(sock,repl);
				}
			}
		    }
		}

		if (event.eventtype == MinetEvent::Timeout) {
		    // timeout ! probably need to resend some packets
		}

    }

    MinetDeinit();

    return 0;
}
