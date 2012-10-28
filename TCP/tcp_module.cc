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

struct TCPState {
    // need to write this
    std::ostream & Print(std::ostream &os) const { 
  os << "TCPState()" ; 
  return os;
    }
};


//---------------------------------------------------------------------------------
//-- function headers
//---------------------------------------------------------------------------------

void handlePacket(MinetHandle mux, Packet p);

bool retrieveTCPHeaderData(Packet p, unsigned short *src_port, unsigned short *dest_port, unsigned int *seq, unsigned int *ack, unsigned char *flags_recv);
TCPHeader setupTCPHeader(Packet p, unsigned short win, unsigned int ack, unsigned int seq, unsigned short src_port, unsigned short dest_port, unsigned char flags);

IPAddress retrieveIPHeaderData(Packet p, unsigned char *protocol);
IPHeader setupIPHeader(IPAddress dest_IP);


//---------------------------------------------------------------------------------
//-- client_connect
//-----(MinetHandle mux)
//---------------------------------------------------------------------------------

void client_connect(MinetHandle mux){ 
  
//  Packet p("Hello",strlen("Hello"));
  Packet p;
  unsigned char flags = 0;
  unsigned int seq = 47;
  unsigned int ack = 0;
  unsigned short win_size = 1000;
  unsigned short src_port = 6071;
  unsigned short dest_port = 6000;

  
  IPHeader ip_hdr = setupIPHeader("192.168.42.9");
  p.PushFrontHeader(ip_hdr);
  
  TCPHeader tcp_hdr = setupTCPHeader(p, win_size, ack, seq, src_port, dest_port, flags);
  SET_SYN(flags);  
  p.PushBackHeader(tcp_hdr);

  MinetSend(mux,p);
//  printf("Packet Sent\n");
  sleep(2);
  MinetSend(mux,p);

}

//---------------------------------------------------------------------------------
//-- main
//---------------------------------------------------------------------------------


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

// client_connect(mux);

    while (MinetGetNextEvent(event, timeout) == 0) {

      if ((event.eventtype == MinetEvent::Dataflow) && 
          (event.direction == MinetEvent::IN)) {
      
          if (event.handle == mux) {
            //handle packet
            Packet p;

            MinetReceive(mux, p);                      // Get the packet from Minet
            p.ExtractHeaderFromPayload<TCPHeader>(40);  // Parse the headers in the packet
              
            handlePacket(mux, p);
          }

          if (event.handle == sock) {
        // socket request or response has arrived
          }
      }

      if (event.eventtype == MinetEvent::Timeout) {
          // timeout ! probably need to resend some packets
      }

    }

    MinetDeinit();

    return 0;
}


//---------------------------------------------------------------------------------
//-- handlePacket
//-----(mux, flags, sequence number, data length, destination IP, destination port)
//---------------------------------------------------------------------------------

void handlePacket(MinetHandle mux, Packet p){

  Packet p_out;
  unsigned short dest_port, src_port;
  unsigned short win = 1000; 
  bool checksum;
  bool valid = false;
  unsigned int seq = 0;
  unsigned int ack = 0;
  unsigned char flags = 0;
  unsigned char flags_recv = 0;  
  unsigned char protocol;
  IPAddress dest_IP;
  IPAddress src_IP = MyIPAddr();
  IPHeader iph_out;
  TCPHeader tcp_out;

  printf("\n\n\n");

  checksum = retrieveTCPHeaderData(p, &dest_port, &src_port, &seq, &ack, &flags_recv);
  dest_IP = retrieveIPHeaderData(p, &protocol);

  tcp_out = setupTCPHeader(p_out, win, ack+1, seq, src_port, dest_port, flags);
  iph_out = setupIPHeader(dest_IP); 

          printf("in handle (2): \n");
          printf("win: %u \n",win);
          printf("ack: %u \n",ack);
          printf("seq: %u \n",seq);
          printf("src_port: %u \n",src_port);
          printf("dest_port: %u \n\n",dest_port);

  p_out.PushFrontHeader(iph_out); 

  /*Things that will be different depending on what sort of packet we receive*/
  if(IS_SYN(flags_recv) && IS_ACK(flags_recv)){
    //we got a syn ack packet send ack back
    printf("The packet we received is a SYN ACK PACKET\n");
    SET_ACK(flags);

    valid = true;
  }
  else if(IS_SYN(flags_recv)){
    //we got a syn packet send a syn ack back
    printf("We got a SYN packet!!\n");
    SET_SYN(flags);
    SET_ACK(flags);
    
            tcp_out.SetFlags(flags,p_out);
                      
    valid = true;
  }  
//NOTE: COMMENT ME OUT TO TEST CODE CHANGES
  /*else if(IS_PSH(flags_recv)){
    //we got a data packet we need to ack
    printf("We got a data packet \n");
    SET_ACK(flags);

    valid = true;
    } */
//FINISH COMMENT BLOCK HERE  
    else{
      printf("No flags could be met\n");  
      valid = false;
    }

    p_out.PushBackHeader(tcp_out);               
              
    if(valid == true) MinetSend(mux,p_out); 
}


//---------------------------------------------------------------------------------
//-- retrieveTCPHeaderData
//-----(recieved packet, source port, destination port, sequence number, ack number, flags)
//---------------------------------------------------------------------------------

bool retrieveTCPHeaderData(Packet p, unsigned short *src_port, unsigned short *dest_port, unsigned int *seq, unsigned int *ack, unsigned char *flags_recv){
  //20 bytes is size of other TCP Headers
  TCPHeader tcph = p.FindHeader(Headers::TCPHeader);   // retrieve the udp header from the packet
  bool checksum = tcph.IsCorrectChecksum(p);    // verify the checksum is correct

  tcph.GetAckNum(*seq);
  tcph.GetSeqNum(*ack);

  printf("About to read information in from Connection c \n");  
            
  tcph.GetDestPort(*src_port);
  tcph.GetSourcePort(*dest_port);
  tcph.GetFlags(*flags_recv);  
  printf("We just pulled information from Connection c \n");  

  return checksum;
}
//---------------------------------------------------------------------------------
//-- setupTCPHeader
//-----(outgoing packet, )
//-----RETURN: partially constructed IP Header
//---------------------------------------------------------------------------------
TCPHeader setupTCPHeader(Packet p, unsigned short win, unsigned int ack, unsigned int seq, unsigned short src_port, unsigned short dest_port, unsigned char flags){
  TCPHeader tcph;

  tcph.SetWinSize(win,p);
  tcph.SetAckNum(ack,p);
  tcph.SetSeqNum(seq,p);
  tcph.SetSourcePort(src_port,p);
  tcph.SetDestPort(dest_port,p);
  tcph.SetHeaderLen(TCP_HEADER_BASE_LENGTH,p);
  tcph.SetFlags(flags,p);

  return tcph;
} 


//---------------------------------------------------------------------------------
//-- retrieveIPHeaderData
//-----(recieved packet, protocol used)
//-----RETURN: source IP of the packet (terrible workaround... fix if time)
//---------------------------------------------------------------------------------

IPAddress retrieveIPHeaderData(Packet p, unsigned char *protocol){
  IPHeader iph = p.FindHeader(Headers::IPHeader);     // retrieve the ip header from the packet
  IPAddress srcIP;

  iph.GetSourceIP(srcIP);
  iph.GetProtocol(*protocol);

  return srcIP;
}
//---------------------------------------------------------------------------------
//-- setupIPHeader
//-----(destination IP)
//-----RETURN: partially constructed IP header
//---------------------------------------------------------------------------------
IPHeader setupIPHeader(IPAddress dest_IP){
  IPHeader iph;
  //Change to push the IP on first    
  iph.SetDestIP(dest_IP);
  iph.SetSourceIP(MyIPAddr()); //always going to be my ip
  iph.SetProtocol(IP_PROTO_TCP); //always TCP
  iph.SetTotalLength(TCP_HEADER_BASE_LENGTH+IP_HEADER_BASE_LENGTH);

  return iph;
}