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


void client_connect(MinetHandle mux){ 
  
//  Packet p("Hello",strlen("Hello"));
  Packet p;
  unsigned char flags = 0;
  unsigned int seq = 47;
  unsigned int ack = 0;
  unsigned short win_size = 1000;
  
  
  IPHeader ip_hdr;
  ip_hdr.SetProtocol(IP_PROTO_TCP);
  ip_hdr.SetSourceIP(MyIPAddr());
  IPAddress dest = "192.168.42.9";
  
  ip_hdr.SetDestIP(dest);
  ip_hdr.SetTotalLength(IP_HEADER_BASE_LENGTH +  TCP_HEADER_BASE_LENGTH); 
  
  p.PushFrontHeader(ip_hdr);

  
  TCPHeader tcp_hdr;
  tcp_hdr.SetSourcePort(6071,p);
  tcp_hdr.SetDestPort(6000,p);
  tcp_hdr.SetHeaderLen(TCP_HEADER_BASE_LENGTH,p);
  SET_SYN(flags);
  tcp_hdr.SetFlags(flags,p);  
  tcp_hdr.SetSeqNum(seq,p);
  tcp_hdr.SetAckNum(ack,p);   
  tcp_hdr.SetWinSize(win_size,p);

  p.PushBackHeader(tcp_hdr);
  MinetSend(mux,p);
//  printf("Packet Sent\n");
  sleep(2);
  MinetSend(mux,p);

}


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
          Packet p;
          Packet p_out;
          unsigned short dest_port, src_port;
          unsigned short win = 1000; 
          bool checksum;
          bool valid = false;
          unsigned int seq = 0;
          unsigned int ack = 0;
          unsigned char flags = 0;
          unsigned char flags_recv = 0;  
          IPAddress dest_IP;
          IPHeader iph_out;
          TCPHeader tcp_out;


          MinetReceive(mux, p);                      // Get the packet from Minet
          p.ExtractHeaderFromPayload<TCPHeader>(40);  // Parse the headers in the packet
          //20 bytes is size of other TCP Headers
          TCPHeader tcph;
          tcph = p.FindHeader(Headers::TCPHeader);   // retrieve the udp header from the packet
          checksum = tcph.IsCorrectChecksum(p);    // verify the checksum is correct
          
          IPHeader iph;
          iph = p.FindHeader(Headers::IPHeader);     // retrieve the ip header from the packet
          
              
          tcph.GetAckNum(seq);
          tcph.GetSeqNum(ack);
          ack = ack + 1; //ack number + 1? Maybe should be ack + amt of data received
          
          printf("About to read information in from Connection c \n");  
        
          iph.GetSourceIP(dest_IP);
          tcph.GetDestPort(src_port);
                tcph.GetSourcePort(dest_port);
          tcph.GetFlags(flags_recv);  
          printf("We just pulled information from Connection c \n");  

          //Change to push the IP on first    
          iph_out.SetDestIP(dest_IP);
          iph_out.SetSourceIP(MyIPAddr()); //always going to be my ip
          iph_out.SetProtocol(IP_PROTO_TCP); //always TCP
          iph_out.SetTotalLength(TCP_HEADER_BASE_LENGTH+IP_HEADER_BASE_LENGTH);
          p_out.PushFrontHeader(iph_out);    

          tcp_out.SetWinSize(win,p_out);
          tcp_out.SetAckNum(ack,p_out);
          tcp_out.SetSeqNum(seq,p_out);
          tcp_out.SetSourcePort(src_port,p_out);
          tcp_out.SetDestPort(dest_port,p_out);

        
          
          /*Things that will be different depending on what sort of packet we receive*/
          if(IS_SYN(flags_recv) && IS_ACK(flags_recv)){
            //we got a syn ack packet send ack back
            printf("The packet we received is a SYN ACK PACKET\n"); 
            SET_ACK(flags);
            
            tcp_out.SetFlags(flags,p_out);
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
          else{

            printf("No flags could be met\n");  
          }
          tcp_out.SetHeaderLen(TCP_HEADER_BASE_LENGTH,p_out);
          p_out.PushBackHeader(tcp_out);               
          
          if(valid == true)MinetSend(mux,p_out); 

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
