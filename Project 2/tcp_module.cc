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

//--------------------------------------------------------------------------------------------------
//----HELPER FUNCITON HEADERS
//--------------------------------------------------------------------------------------------------
void handShake(Packet p);
void handlePacket(Packet p);


//--------------------------------------------------------------------------------------------------
//----GLOBAL VARIABLES
//--------------------------------------------------------------------------------------------------
IPHeader ipHead;
int connState;
int targetPort;
char * targetIP;
int myPort;
unsigned int seqNum;
unsigned int ackNum;

//--------------------------------------------------------------------------------------------------
//----MAIN
//--------------------------------------------------------------------------------------------------
int main(int argc, char * argv[]) {

//---- initialize of global variables --------------------------------------------------------------

//!!!! NOTE: not sure what Ip to use here------
	targetIP = "192.168.102.100";
//!!!! NOTE: or here---------------------------
	targetPort = 5000;
	myPort = 500;
	connState = 0;//start with the state set to closed
	seqNum = 1;

//!!!! NOTE: unsure on this bit, its legit as far as I can tell
	ipHead.SetProtocol(IP_PROTO_TCP);
	ipHead.SetSourceIP(MyIPAddr());
	ipHead.SetDestIP(IPAddress(targetIP));
	ipHead.SetTotalLength(TCP_HEADER_BASE_LENGTH+IP_HEADER_BASE_LENGTH);


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

//-- execute initial handshake ------------------------------------------------------------------------
    Packet emptyPacket;
    handShake(emptyPacket);

    double timeout = 1;

    while (MinetGetNextEvent(event, timeout) == 0) 
    {

		if ((event.eventtype == MinetEvent::Dataflow) && 
		    (event.direction == MinetEvent::IN)) 
		{
//-- PRESUMABLY WE WILL FALL INTO THIS IF DUING AN NC CALL --------------------------------------------
		    if (event.handle == mux) 
		    {
				// ip packet has arrived!
				Packet p;
				MinetReceive(mux, p);
				switch(connState)
				{
					case LISTEN:
						//Should never reach this state.
						break;
					case SYN_RCVD:
					case SYN_SENT:
						handShake(p);
						break;
					case ESTABLISHED:
						//operate normally
						break;
					case FIN_WAIT1:
						//recieve ACK, go to FIN_WAIT2
						break;
					case FIN_WAIT2:
						//recieve FIN, send ACK, go to TIME_WAIT
						break;
					case TIME_WAIT:
						//Wait?
						break;
					case CLOSE_WAIT:
						//Send FIN and ACK after receiving FIN
						break;
					case LAST_ACK:
						//terminate connection
						break;
				}
//-- THIS SHOULD PRINT THE PACKET CONTENTS -----------------------------------------------------------
				cout << p << endl;
		    }

		    if (event.handle == sock) 
		    {
				// socket request or response has arrived
				
				SockRequestResponse req;
				MinetReceive(sock,req);
				
				switch (req.type) 
				{
					case CONNECT:
					{
						SockRequestResponse response;

						response.type=STATUS;
						response.connection=req.connection;
						response.bytes=0;
						response.error=EOK;
						

						MinetSend(sock,response);
					}
					break;
					case ACCEPT:
					{ 
						SockRequestResponse response;
						response.type=STATUS;
					    response.error=EOK;
					    MinetSend(sock,response);
					}
					  break;
					case STATUS:// ignored, no response needed
					  break;
					  // case SockRequestResponse::WRITE:
					case WRITE:
					{
						/*/*THIS IS FROM UPD, MAJOR ADDITIONS NEEDED
						//retrieve the data from the request
						int len = req.data.GetSize();
						char *buffer = (char *) malloc(len);
						req.data.GetData(buffer,len,0); 
						
						//retrieve the connection

						SockRequestResponse response;
						response.type=STATUS;
						response.connection=req.connection;
						response.error=EOK;
						response.bytes=bytes;

						MinetSend(sock,response);	
						*/
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
						/*THIS IS FROM UPD, MAJOR ADDITIONS NEEDED

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
		           		*/

					}
					  break;
					default:
					{
					    SockRequestResponse repl;
					    // repl.type=SockRequestResponse::STATUS;
					    repl.type=STATUS;
					    repl.error=EWHAT;
					    MinetSend(sock,repl);
					}
				}
		    }
		}

		if (event.eventtype == MinetEvent::Timeout) 
		{
		    // timeout ! probably need to resend some packets
		}

    }

    //MinetDeinit();

    return 0;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//----HELPER FUNCTIONS
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
//----handShake()
//		performs three part handshake to establish a connection
//		NOTE: must be called several times, it acts according to state
//--------------------------------------------------------------------------------------------------

void handShake(Packet inPacket)
{
	switch(connState)
	{
		//We have recieved a SYN to the initial shake,
		//deal with it and  send our response
		case SYN_RCVD:
		{
			Packet outPacket;
			unsigned short destPort=0;

			//grab the header and perform the checksum
			inPacket.ExtractHeaderFromPayload<TCPHeader>(
				TCPHeader::EstimateTCPHeaderLength(inPacket));			
			TCPHeader inHeader;
			inHeader = inPacket.FindHeader(Headers::TCPHeader);
			bool checksum = inHeader.IsCorrectChecksum(inPacket);
			if(!checksum){
				return;
			}

			//retrieve the seqnum and the port
			TCPHeader outHeader;
			inHeader.GetSeqNum(ackNum);
			outHeader.SetAckNum(ackNum++, outPacket);//dont forget to increment the ACK!
			outPacket.PushFrontHeader(ipHead);
			inHeader.GetSourcePort(destPort);
			outHeader.SetDestPort(destPort, outPacket);


			outHeader.SetSourcePort(myPort, outPacket);
			outHeader.SetHeaderLen(TCP_HEADER_BASE_LENGTH, outPacket);
//!!!! NOTE: I assume we need a better way to choose the starting seqnum 
			//But hardcoding to 50 should do for now
			outHeader.SetSeqNum(50, outPacket);

//!!!! NOTE (OC) this is set to zero
			unsigned char zero = 0;
			SET_ACK(zero);
			SET_SYN(zero);
			outHeader.SetFlags(zero, outPacket);
//!!!! NOTE: The window size is hardcoded to 100, should it be dynamic?
			outHeader.SetWinSize(100, outPacket);
			outPacket.PushBackHeader(outHeader);
			MinetSend(mux, outPacket);
			//sleep(2);
			//MinetSend(mux,outPacket);
			connState = ESTABLISHED;
		}
			break;
		//Our SYN has been sent, finish up and handle the packet
		case SYN_SENT:
		{
			Packet outPacket;
			//last_acked = GetAckNum(inPacket);

			//retrieve header and perform chescksum
			inPacket.ExtractHeaderFromPayload<TCPHeader>(TCPHeader::EstimateTCPHeaderLength(inPacket));
			TCPHeader inHeader;
			inHeader = inPacket.FindHeader(Headers::TCPHeader);
			bool checksum = inHeader.IsCorrectChecksum(inPacket);
			if(!checksum)return;

			//get the seq and acks fron the header
			inHeader.GetSeqNum(seqNum);
			inHeader.GetAckNum(ackNum);

			//set up the header
			outPacket.PushFrontHeader(ipHead);
			TCPHeader outHeader;
			outHeader.SetSourcePort(myPort, outPacket);
			outHeader.SetDestPort(targetPort, outPacket);
			outHeader.SetHeaderLen(TCP_HEADER_BASE_LENGTH,outPacket);

			unsigned char zero = 0;
			SET_ACK(zero);
			outHeader.SetFlags(zero, outPacket);
			outHeader.SetWinSize(TCP_MAXIMUM_SEGMENT_SIZE,outPacket);
			//dont forget to increment the seq and ack!
			outHeader.SetAckNum(ackNum++,outPacket);
			outHeader.SetSeqNum(seqNum++,outPacket);

			//tack the header to the packet abd send it
			outPacket.PushBackHeader(outHeader);
			MinetSend(mux, outPacket);
			connState = ESTABLISHED;

//!!!! NOTE: not sure on this
			char l[] = "abcgdf";
			packetMailer(false,false,false,l,sizeof(l));
		}
			break;
		//if we are not in the SYN_RCVD or SYN_SENT states, 
		//intitate the shake by sending our SYN
		default:
		{
			//tack on the IP header for the destination
			Packet p;
			p.PushFrontHeader(ipHead);

			//now set the TCP header ports and length
			TCPHeader header;
			header.SetSourcePort(myPort, p);
			header.SetDestPort(targetPort, p);
			header.SetHeaderLen(TCP_HEADER_BASE_LENGTH, p);//sets len and recomputes checksum
			
			//set up the ACK & SYN
			unsigned char ack = 0;
			seqNum = 1;
			header.SetSeqNum(seqNum, p);
//!!!! NOTE: cant find any documentation on this (OC: SET_SYN is always 0?)
			SET_SYN(ack);
			header.SetFlags(ack, p);//sets flags and recalculates checksum

			//tack the TCP header onto the packet and send it
			p.PushBackHeader(header);
			MinetSend(mux,p);
			//--OC
			//sleep(2);
			//MinetSend(mux,p);
			connState = SYN_SENT;
		}
	}
}
