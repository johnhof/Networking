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
#include "tcpstate.h"


#include <iostream>

#include "Minet.h"

using namespace std;
/*
//NOT SURE WHAT EXACTLY SHOULD GO ON HERE. USE ENUMERATORS FOR NOW
struct TCPState {
    // need to write this
    std::ostream & Print(std::ostream &os) const { 
  os << "TCPState()" ; 
  return os;
    }
};*/
#define CLOSED = 0
#define SYN_SENT = 1
#define LISTEN = 2
#define SYN_RCVD = 3
#define ESTABLSHED = 4


//---------------------------------------------------------------------------------
//-- function headers & globals
//---------------------------------------------------------------------------------

void handlePacket(MinetHandle mux, Packet p);

bool retrieveTCPHeaderData(Packet p, unsigned short *src_port, unsigned short *dest_port, unsigned int *seq, unsigned int *ack, unsigned char *flags_recv, unsigned char *header_size);
TCPHeader setupTCPHeader(Packet p, unsigned short win, unsigned int ack, unsigned int seq, unsigned short src_port, unsigned short dest_port, unsigned char flags);

IPAddress retrieveIPHeaderData(Packet p, unsigned char *protocol, int *data_size);
IPHeader setupIPHeader(IPAddress dest_IP);

//for now, default as a server
int state = LISTEN;


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
  state = SYN_SENT;

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
        unsigned char headerSize = 0;
      
          if (event.handle == mux) {
            //handle packet
            Packet p;

            MinetReceive(mux, p);                      // Get the packet from Minet
            headerSize = TCPHeader::EstimateTCPHeaderLength(p);  
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
  int data_size = 0;
  unsigned short dest_port, src_port;
  unsigned short win = 1000; 
  bool checksum;
  bool valid = false;
  unsigned int seq = 0;
  unsigned int ack = 0;
  unsigned char flags = 0;
  unsigned char flags_recv = 0;  
  unsigned char protocol;
  unsigned char header_size = 0;
  IPAddress dest_IP;
  IPAddress src_IP = MyIPAddr();
  IPHeader iph_out;
  TCPHeader tcp_out;

  //data_size is temporarily storing the tcp header size, then passed into retrieveIPheader to get data size
  checksum = retrieveTCPHeaderData(p, &dest_port, &src_port, &seq, &ack, &flags_recv, &data_size);
  printf("header size handler: %i \n",data_size);
  dest_IP = retrieveIPHeaderData(p, &protocol, &data_size);
  printf("total size handler: %i \n",data_size);


  tcp_out = setupTCPHeader(p_out, win, ack+1, seq, src_port, dest_port, flags);
  iph_out = setupIPHeader(dest_IP); 

  p_out.PushFrontHeader(iph_out); 

  switch(state)
  {

    //--CLIENT STATE---------------------------------------------------------------

    case SYN_SENT://looking for a SYN/ACK (client side only)
    {
      if(IS_SYN(flags_recv) && IS_ACK(flags_recv)){ //respond with ACK

        printf("The packet we received is a SYN ACK PACKET\n");
        SET_ACK(flags);

        p_out.PushBackHeader(tcp_out);               
        MinetSend(mux,p_out); 

        state = ESTABLISHED;
      }
    }break;


    //--SERVER STATES--------------------------------------------------------------

    case LISTEN://looking for a SYN (server side only)
    {
      else if(IS_SYN(flags_recv)){ //respond with SYN/ACK

        printf("We got a SYN packet!!\n");
        SET_SYN(flags);
        SET_ACK(flags);

        p_out.PushBackHeader(tcp_out);               
        MinetSend(mux,p_out); 

        state = SYN_RCVD;
      }  
    }break;

    case SYN_RCVD://looking for ack
    {
      if(IS_ACK(flags_recv)){

        printf("we got an ACK packet!!");
        state = ESTABLISHED;

        //if we recieved data, respond
        if(data_length>0){
          printf("data recieved, responding");

          p_out.PushBackHeader(tcp_out);  

        }

      }
    }break;

    //--GENERAL STATES-------------------------------------------------------------
    
    case ESTABLISHED://looking for packets with data
    {
        if(data_length>0){
          printf("data recieved, responding");

          p_out.PushBackHeader(tcp_out);  
          
        }
    }break;
  }
}


//---------------------------------------------------------------------------------
//-- retrieveTCPHeaderData
//-----(recieved packet, source port, destination port, sequence number, ack number, flags)
//---------------------------------------------------------------------------------

bool retrieveTCPHeaderData(Packet p, unsigned short *src_port, unsigned short *dest_port, unsigned int *seq, unsigned int *ack, unsigned char *flags_recv, int *header_size){
  *header_size = (int)TCPHeader::EstimateTCPHeaderLength(p);

  printf("header size: %i", header_size);
  
  TCPHeader tcph = p.FindHeader(Headers::TCPHeader);   // retrieve the udp header from the packet
  bool checksum = tcph.IsCorrectChecksum(p);    // verify the checksum is correct

  tcph.GetAckNum(*seq);
  tcph.GetSeqNum(*ack);
  tcph.GetDestPort(*src_port);
  tcph.GetSourcePort(*dest_port);
  tcph.GetFlags(*flags_recv);  

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
//-----(recieved packet, protocol used, size of the tcp header: returned as data size)
//-----RETURN: source IP of the packet (terrible workaround... fix if time)
//---------------------------------------------------------------------------------

IPAddress retrieveIPHeaderData(Packet p, unsigned char *protocol, int *headersize_to_datasize){
  IPHeader iph = p.FindHeader(Headers::IPHeader);     // retrieve the ip header from the packet
  IPAddress srcIP;
  unsigned char total_size = 0;

  iph.GetSourceIP(srcIP);
  iph.GetProtocol(*protocol);
  iph.GetTotalLength(total_size);  
  printf("total size: %i \n",(int)tot_len);
  *headersize_to_datasize = (int)total_size - 20 - *headersize_to_datasize;
  printf("total size: %i \n",*headersize_to_datasize);

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
   