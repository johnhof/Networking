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




//------------------------------------------------------------------------------------------------------------------
//--SOCKET STATUS REPLY
//------------------------------------------------------------------------------------------------------------------

void sockStatReply(int error, MinetHandle sock, SockRequestResponse req){
   SockRequestResponse repl; 
   repl.type = STATUS;         
    repl.connection = req.connection; 
    repl.error = error;              
    repl.bytes = 0;
    MinetSend(sock,repl);

}//END STATUS REPKLY





//------------------------------------------------------------------------------------------------------------------
//--PACKET HANDLER
//------------------------------------------------------------------------------------------------------------------

void packetHandler( ConnectionList<TCPState>::iterator cs, MinetHandle mux, unsigned char flags_recv, unsigned int ack, unsigned int seq, int data_length, IPAddress dest, unsigned short destport,Buffer x,MinetHandle sock, Connection conn, unsigned short sendwin){
  unsigned char flags = 0; 

  switch((*cs).state.GetState())
  {   

//---SYN SENT-------------------------------------------------------------------------------------------------------
    case SYN_SENT: 
    {
      if(IS_SYN(flags_recv) && IS_ACK(flags_recv))
      {  
        Packet outgoing_packet;
        SET_ACK(flags);

        IPHeader ip_hdr;
        ip_hdr.SetProtocol(IP_PROTO_TCP);       
        ip_hdr.SetSourceIP((*cs).connection.src); 
        ip_hdr.SetDestIP((*cs).connection.dest);
        ip_hdr.SetTotalLength(IP_HEADER_BASE_LENGTH +  TCP_HEADER_BASE_LENGTH); 
        outgoing_packet.PushFrontHeader(ip_hdr);

        TCPHeader tcp_hdr;
        tcp_hdr.SetSourcePort((*cs).connection.srcport,outgoing_packet);
        tcp_hdr.SetDestPort((*cs).connection.destport,outgoing_packet);
        tcp_hdr.SetFlags(flags,outgoing_packet);  
        tcp_hdr.SetSeqNum(ack,outgoing_packet);   
        tcp_hdr.SetAckNum(seq +1,outgoing_packet);    
        tcp_hdr.SetWinSize(1024,outgoing_packet);
        tcp_hdr.SetHeaderLen(TCP_HEADER_BASE_LENGTH,outgoing_packet);
        outgoing_packet.PushBackHeader(tcp_hdr);

        MinetSend(mux,outgoing_packet);

        (*cs).state.SetLastSent(ack);
        (*cs).state.SetState(ESTABLISHED);
        (*cs).state.SetSendRwnd(sendwin);
        (*cs).state.last_acked = seq +1;

        SockRequestResponse write(WRITE,(*cs).connection,x,data_length, EOK);
        MinetSend(sock,write);  
      } 
      break;
    }

//---FIN WAITS------------------------------------------------------------------------------------------------------
    case FIN_WAIT1:
    { 
      printf("\nin FIN_WAIT1\n");
      break;
    }
    case FIN_WAIT2:
    { 
      printf("\nin FIN_WAIT2\n");
      break;
    }

//---LISTEN---------------------------------------------------------------------------------------------------------
    case LISTEN:
    {
      if(IS_SYN(flags_recv))
      { 
        Packet outgoing_packet;
        SET_ACK(flags); 
        SET_SYN(flags);

        IPHeader ip_hdr;
        ip_hdr.SetProtocol(IP_PROTO_TCP);       
        ip_hdr.SetSourceIP((*cs).connection.src); 
        cout << (*cs).connection.src;
        ip_hdr.SetDestIP(dest);
        ip_hdr.SetTotalLength(IP_HEADER_BASE_LENGTH +  TCP_HEADER_BASE_LENGTH); 
        (*cs).state.SetLastSent(ack);
        outgoing_packet.PushFrontHeader(ip_hdr);

        TCPHeader tcp_hdr;
        tcp_hdr.SetSourcePort((*cs).connection.srcport,outgoing_packet);
        tcp_hdr.SetDestPort(destport,outgoing_packet);
        tcp_hdr.SetFlags(flags,outgoing_packet);  
        tcp_hdr.SetSeqNum(ack,outgoing_packet);   
        tcp_hdr.SetAckNum(seq +1,outgoing_packet);    
        tcp_hdr.SetWinSize(1024,outgoing_packet);
        tcp_hdr.SetHeaderLen(TCP_HEADER_BASE_LENGTH,outgoing_packet);
        outgoing_packet.PushBackHeader(tcp_hdr);

        MinetSend(mux,outgoing_packet);
        sleep(2);
        MinetSend(mux,outgoing_packet);

        (*cs).state.SetState(SYN_RCVD);
        (*cs).state.SetLastAcked(seq + 1);
        (*cs).state.SetLastSent(ack);
      } 
      break;
    }

//---SYN RECIEVED---------------------------------------------------------------------------------------------------
    case SYN_RCVD: 
    {
      if(IS_ACK(flags_recv))
      {
        (*cs).state.SetState(ESTABLISHED);
        (*cs).state.SetLastSent(ack);
        (*cs).state.SetLastAcked(seq + 1);
        (*cs).state.SetSendRwnd(sendwin);
      } 
      break;
    }

//---CLOSING--------------------------------------------------------------------------------------------------------
    case CLOSING:
    {
      printf("\nin CLOSING\n");
      if(IS_ACK(flags_recv))
      {
        (*cs).state.SetState(CLOSED);
      }
      break;
    }
    case CLOSE_WAIT:
    {
      printf("\nin CLOSE_WAIT\n");
      break;
    }
    case LAST_ACK: 
    {

      printf("\nin LAST_ACK\n");
      if(IS_ACK(flags_recv))
      {
        (*cs).state.SetState(CLOSED);
      }
      break;
      break;
    } 

//---ESTABLISHED----------------------------------------------------------------------------------------------------
    case ESTABLISHED:
    {
      //We recieved a FIN, begin closing sequence
      if(IS_FIN(flags_recv))
      {
        Packet outgoing_packet;
        SET_ACK(flags); 
        SET_FIN(flags);

        IPHeader ip_hdr;
        ip_hdr.SetProtocol(IP_PROTO_TCP);       
        ip_hdr.SetSourceIP((*cs).connection.src); 
        ip_hdr.SetDestIP(dest);
        ip_hdr.SetTotalLength(IP_HEADER_BASE_LENGTH +  TCP_HEADER_BASE_LENGTH); 
        outgoing_packet.PushFrontHeader(ip_hdr);

        TCPHeader tcp_hdr;
        tcp_hdr.SetSourcePort((*cs).connection.srcport,outgoing_packet);
        tcp_hdr.SetDestPort(destport,outgoing_packet);
        tcp_hdr.SetFlags(flags,outgoing_packet);  
        tcp_hdr.SetSeqNum(ack,outgoing_packet);   
        tcp_hdr.SetAckNum(seq+1,outgoing_packet);   
        tcp_hdr.SetWinSize(1024,outgoing_packet);
        tcp_hdr.SetHeaderLen(TCP_HEADER_BASE_LENGTH,outgoing_packet);
        outgoing_packet.PushBackHeader(tcp_hdr);

        MinetSend(mux,outgoing_packet);

        (*cs).state.SetState(LAST_ACK); 
        (*cs).state.SetLastSent(ack);
        (*cs).state.SetLastAcked(seq + 1);
        (*cs).state.SetLastRecvd(seq + data_length);
        break;
      }
      if(data_length > 0)
      {
        //REMOVE ME?
        SockRequestResponse write(WRITE,(*cs).connection,x,data_length, EOK);
        MinetSend(sock,write);  

    //if this seq is not what we are expecting, resend a request for the last acked packet (Go-Back-N)
    if(ack != (*cs).state.GetLastSent())
    {    
        printf("\nGO-BACK-N\nacked: %i\nseqed: %i\nexpected ack: %i\nexpected seq%i", ack, seq, (*cs).state.GetLastSent(), (*cs).state.GetLastAcked());
        //these are switched on header construction
        ack = (*cs).state.GetLastSent();
        seq = (*cs).state.GetLastAcked();
        data_length = 0;
      }

        Packet outgoing_packet;
        SET_ACK(flags);

        IPHeader ip_hdr;
        ip_hdr.SetProtocol(IP_PROTO_TCP);       
        ip_hdr.SetSourceIP((*cs).connection.src); 
        ip_hdr.SetDestIP(dest);
        ip_hdr.SetTotalLength(IP_HEADER_BASE_LENGTH +  TCP_HEADER_BASE_LENGTH); 
        outgoing_packet.PushFrontHeader(ip_hdr);


        TCPHeader tcp_hdr;
        tcp_hdr.SetSourcePort((*cs).connection.srcport,outgoing_packet);
        tcp_hdr.SetDestPort(destport,outgoing_packet);        
        tcp_hdr.SetFlags(flags,outgoing_packet);  
        tcp_hdr.SetSeqNum(ack,outgoing_packet);   
        tcp_hdr.SetAckNum(seq+data_length,outgoing_packet);   
        tcp_hdr.SetWinSize(1024,outgoing_packet);
        tcp_hdr.SetHeaderLen(TCP_HEADER_BASE_LENGTH,outgoing_packet);
        outgoing_packet.PushBackHeader(tcp_hdr);

        MinetSend(mux,outgoing_packet);

        (*cs).state.SetLastAcked(ack+data_length);        
        (*cs).state.SetSendRwnd(sendwin);       
        break;
      }
      break;
    }
    default:{break;}

  }//END SWITCH STATEMENT

}//END PACKET HANDLER





//------------------------------------------------------------------------------------------------------------------
//--CLIENT CONNECT
//------------------------------------------------------------------------------------------------------------------

void client_connect_packet(Packet outgoing_packet, SockRequestResponse c, MinetHandle mux, unsigned char flags ,ConnectionList<TCPState>::iterator cs){ 
  
  
  unsigned int seq = 47;    //should be random
  unsigned int ack = 0;   //should also be random
  unsigned short win_size = 500;  
  
  IPHeader ip_hdr;
  ip_hdr.SetProtocol(IP_PROTO_TCP);
  ip_hdr.SetSourceIP(c.connection.src); 
  ip_hdr.SetDestIP(c.connection.dest);
  ip_hdr.SetTotalLength(IP_HEADER_BASE_LENGTH +  TCP_HEADER_BASE_LENGTH); 
  (*cs).state.SetLastSent(seq);
  outgoing_packet.PushFrontHeader(ip_hdr);

  TCPHeader tcp_hdr;
  tcp_hdr.SetSourcePort(c.connection.srcport,outgoing_packet);
  tcp_hdr.SetDestPort(c.connection.destport,outgoing_packet);
  tcp_hdr.SetFlags(flags,outgoing_packet);  
  tcp_hdr.SetSeqNum(seq,outgoing_packet);
  tcp_hdr.SetAckNum(ack,outgoing_packet);   
  tcp_hdr.SetWinSize(win_size,outgoing_packet);
  tcp_hdr.SetHeaderLen(TCP_HEADER_BASE_LENGTH,outgoing_packet);
  outgoing_packet.PushBackHeader(tcp_hdr);
  MinetSend(mux,outgoing_packet);

}//END CLIENT CONNECT



//------------------------------------------------------------------------------------------------------------------
//--MAIN
//------------------------------------------------------------------------------------------------------------------

int main(int argc, char * argv[]) {
  MinetHandle mux;
  MinetHandle sock;
    
  ConnectionList<TCPState> clist;

  MinetInit(MINET_TCP_MODULE);

  mux = MinetIsModuleInConfig(MINET_IP_MUX) ? MinetConnect(MINET_IP_MUX) : MINET_NOHANDLE;
  sock = MinetIsModuleInConfig(MINET_SOCK_MODULE) ? MinetAccept(MINET_SOCK_MODULE) : MINET_NOHANDLE;

  if ( (mux == MINET_NOHANDLE) && (MinetIsModuleInConfig(MINET_IP_MUX)) ) 
  {
    MinetSendToMonitor(MinetMonitoringEvent("Can't connect to ip_mux"));
    return -1;
  }

  if ( (sock == MINET_NOHANDLE) && (MinetIsModuleInConfig(MINET_SOCK_MODULE)) ) 
  {
    MinetSendToMonitor(MinetMonitoringEvent("Can't accept from sock_module"));
    return -1;
  }
    
  cerr << "tcp_module STUB VERSION handling tcp traffic.......\n";

  MinetSendToMonitor(MinetMonitoringEvent("tcp_module STUB VERSION handling tcp traffic........"));

  MinetEvent event;
  double timeout = 1;

//------------------------------------------------------------------------------------------------------------------
//--EVENT LOOP
//------------------------------------------------------------------------------------------------------------------

  while (MinetGetNextEvent(event, timeout) == 0) 
  {
    if ((event.eventtype == MinetEvent::Dataflow) && (event.direction == MinetEvent::IN)) 
    {
//------------------------------------------------------------------------------------------------------------------
//--MUX HANDLING
//------------------------------------------------------------------------------------------------------------------
      if (event.handle == mux) 
      {
        Packet p;
        Packet packet_to_send;
        unsigned char head_len=0;
        unsigned short tot_len = 0;
        bool checksumok;
        unsigned int seq = 0;
        unsigned int ack = 0;
        IPAddress destination_ip;
        unsigned char flags_recv = 0;   
        IPHeader iph_send_back;
        TCPHeader tcp_send_back;
        int data_length = 0;
        unsigned short send_win = 0;
        
       
        MinetReceive(mux, p);                      
        head_len = TCPHeader::EstimateTCPHeaderLength(p);   
        p.ExtractHeaderFromPayload<TCPHeader>(head_len);  
        TCPHeader tcph;
        tcph = p.FindHeader(Headers::TCPHeader);   
        checksumok = tcph.IsCorrectChecksum(p);    
           
        IPHeader iph;
        iph = p.FindHeader(Headers::IPHeader); 
        iph.GetTotalLength(tot_len);
      
        data_length = (int)tot_len - 20 - (int)head_len;    
        Buffer data;

        if(data_length>0)
        {
          data = p.GetPayload().ExtractFront(data_length);
        }
        
        Connection c;                              
        iph.GetDestIP(c.src);
        iph.GetSourceIP(c.dest);
        iph.GetProtocol(c.protocol);
        tcph.GetDestPort(c.srcport);
        tcph.GetSourcePort(c.destport);   
        tcph.GetFlags(flags_recv);
        tcph.GetAckNum(ack);      
        tcph.GetSeqNum(seq);
        /*Grabbing info for flow control*/
        tcph.GetWinSize(send_win);  

        ConnectionList<TCPState>::iterator cs = clist.FindMatching(c);

        if (cs != clist.end()) 
        {   
          if((*cs).state.GetState() == LISTEN)
          {
            ConnectionToStateMapping<TCPState> m;
            m.connection = c;
            m.state.SetState(LISTEN);
            clist.push_back(m);
            (*cs) = m;
          }
          else if((*cs).state.GetState() == CLOSED)
          {
            (*cs).state.SetState(LISTEN);
          }   
          packetHandler(cs,mux,flags_recv,ack,seq,data_length, c.dest,c.destport,data,sock,c,send_win); //Handles a with this given state 
        }
        else 
        {  
          if(IS_SYN(flags_recv) && !IS_ACK(flags_recv))
          {
            ConnectionToStateMapping<TCPState> m;
            m.connection = c;
            m.state.SetState(LISTEN);
            clist.push_back(m);
            (*cs) = m;
            packetHandler(cs,mux,flags_recv,ack,seq,data_length,c.dest,c.destport,data,sock,c,send_win);
          } 
        }

      }//end mux handling

//------------------------------------------------------------------------------------------------------------------
//--SOCKET HANDLING
//------------------------------------------------------------------------------------------------------------------
      if (event.handle == sock) 
      {
        SockRequestResponse req;      
        MinetReceive(sock, req);         

        switch (req.type) 
        {

//--CONNECT----------------------------------------------------------------------------------------------------
          case CONNECT: 
          {        
                  ConnectionList<TCPState>::iterator cs = clist.FindMatching(req.connection);
            if(cs != clist.end())
            {         
              if((*cs).state.GetState() == CLOSED)
              {  
                Packet return_packet;     
                unsigned char flags=0; 
                SET_SYN(flags);

                (*cs).state.SetState(SYN_SENT); 
                client_connect_packet(return_packet, req,mux,flags,cs);
                sockStatReply(ERESOURCE_UNAVAIL,sock,req);
              }
              else{
                sockStatReply(ERESOURCE_UNAVAIL,sock,req);
              }
            }
            else
            {
              ConnectionToStateMapping<TCPState> m;  
              m.connection = req.connection;        
              Packet return_packet;     
              unsigned char flags=0; 
              SET_SYN(flags);

              client_connect_packet(return_packet, req,mux,flags,cs);
              sleep(2);         
              client_connect_packet(return_packet, req,mux,flags,cs);     
              m.state.SetState(SYN_SENT);
              clist.push_back(m);
              sockStatReply(EOK,sock,req);
            } 
            break;
          }

//--LISTEN-----------------------------------------------------------------------------------------------------
          case LISTEN:
          {        
            printf("\nsocket listen\n");
            ConnectionList<TCPState>::iterator cs = clist.FindMatching(req.connection);
            if(cs != clist.end())
            {         
              if((*cs).state.GetState() == CLOSED)
              {  
                (*cs).state.SetState(LISTEN);
                sockStatReply(EOK,sock,req);
              }
              else 
              {        
                sockStatReply(ERESOURCE_UNAVAIL,sock,req);
              }
            }
            else
            { 
              ConnectionToStateMapping<TCPState> m;  
               m.connection = req.connection;            
              m.state.SetState(LISTEN);       
              clist.push_back(m);
              sockStatReply(EOK,sock,req);
            }
            break;
          }

//--WRITE------------------------------------------------------------------------------------------------------
          case WRITE:
          {            
              ConnectionList<TCPState>::iterator cs = clist.FindMatching(req.connection);
              if(cs != clist.end())
              {     
                if((*cs).state.GetState() == ESTABLISHED)
                { 
                  unsigned int bytes = MIN_MACRO(536, req.data.GetSize());
                  unsigned char flags = 0;  
                  if((*cs).state.GetRwnd() > bytes)
                  {
            printf("\nestablished, GetRwnd\n");
                    Packet outgoing_packet(req.data.ExtractFront(bytes));
                    SET_ACK(flags);
                    SET_PSH(flags);

                    IPHeader ip_hdr;
                    ip_hdr.SetProtocol(IP_PROTO_TCP);     
                    ip_hdr.SetSourceIP(req.connection.src);
                    ip_hdr.SetDestIP(req.connection.dest);
                    ip_hdr.SetTotalLength(IP_HEADER_BASE_LENGTH +  TCP_HEADER_BASE_LENGTH + bytes); 
                    outgoing_packet.PushFrontHeader(ip_hdr);

                    TCPHeader tcp_hdr;
                    tcp_hdr.SetSourcePort(req.connection.srcport,outgoing_packet);
                    tcp_hdr.SetDestPort(req.connection.destport,outgoing_packet);
                    tcp_hdr.SetFlags(flags,outgoing_packet);
                    tcp_hdr.SetSeqNum((*cs).state.GetLastSent(),outgoing_packet);           
                    tcp_hdr.SetAckNum((*cs).state.GetLastAcked(),outgoing_packet);
                    tcp_hdr.SetWinSize(1054,outgoing_packet);
                    tcp_hdr.SetHeaderLen(TCP_HEADER_BASE_LENGTH,outgoing_packet);
                    outgoing_packet.PushBackHeader(tcp_hdr);

                    MinetSend(mux,outgoing_packet);

                    (*cs).state.SetLastSent((*cs).state.GetLastSent() + bytes);
                    SockRequestResponse repl;
                    repl.type = STATUS; 
                    repl.connection = req.connection; 
                    repl.bytes = bytes; 
                    repl.error = EOK; 
                    MinetSend(sock, repl);
                  }
                  else
                  {
            printf("\nestablished other\n");
                    bytes = (*cs).state.GetRwnd();  
                    Packet outgoing_packet(req.data.ExtractFront(bytes));
                    SET_ACK(flags);
                    SET_PSH(flags);

                    IPHeader ip_hdr;
                    ip_hdr.SetProtocol(IP_PROTO_TCP);     
                    ip_hdr.SetSourceIP(req.connection.src);
                    ip_hdr.SetDestIP(req.connection.dest);
                    ip_hdr.SetTotalLength(IP_HEADER_BASE_LENGTH +  TCP_HEADER_BASE_LENGTH + bytes); 
                    outgoing_packet.PushFrontHeader(ip_hdr);

                    TCPHeader tcp_hdr;
                    tcp_hdr.SetSourcePort(req.connection.srcport,outgoing_packet);
                    tcp_hdr.SetDestPort(req.connection.destport,outgoing_packet);
                    tcp_hdr.SetFlags(flags,outgoing_packet);
                    tcp_hdr.SetSeqNum((*cs).state.GetLastSent(),outgoing_packet);           
                    tcp_hdr.SetAckNum((*cs).state.GetLastAcked(),outgoing_packet);
                    tcp_hdr.SetWinSize(1054,outgoing_packet);
                    tcp_hdr.SetHeaderLen(TCP_HEADER_BASE_LENGTH,outgoing_packet);
                    outgoing_packet.PushBackHeader(tcp_hdr);

                    MinetSend(mux,outgoing_packet);

                    (*cs).state.SetLastSent((*cs).state.GetLastSent() + bytes);
                    SockRequestResponse repl;
                    repl.type = STATUS; 
                    repl.connection = req.connection; 
                    repl.bytes = bytes; 
                    repl.error = EOK; 
                    MinetSend(sock, repl);
                  }
                }
                else
                {  
                  sockStatReply(ERESOURCE_UNAVAIL,sock,req);
                }
              }
              else
              {    
                sockStatReply(ENOMATCH,sock,req);
              }
            break;
          }

//--UNUSED-----------------------------------------------------------------------------------------------------
          case FORWARD: {break;}
          case STATUS: {break;}

//--CLOSE------------------------------------------------------------------------------------------------------
          case CLOSE: 
          {
              printf("IN CLOSE CASE \n");
              ConnectionList<TCPState>::iterator cs = clist.FindMatching(req.connection);
              if(cs != clist.end())
              {         
                if((*cs).state.GetState() == ESTABLISHED)
                {  
                  
                  printf("Preparing FIN ACK to be sent \n");  
                  unsigned char flags = 0;  
                  Packet outgoing_packet;
                  SET_FIN(flags); 
                  SET_ACK(flags);

                  IPHeader ip_hdr;
                  ip_hdr.SetProtocol(IP_PROTO_TCP);     
                  ip_hdr.SetSourceIP(req.connection.src);
                  ip_hdr.SetDestIP(req.connection.dest);
                  ip_hdr.SetTotalLength(IP_HEADER_BASE_LENGTH +  TCP_HEADER_BASE_LENGTH); 
                  outgoing_packet.PushFrontHeader(ip_hdr);

                  TCPHeader tcp_hdr;
                  tcp_hdr.SetSourcePort(req.connection.srcport,outgoing_packet);
                  tcp_hdr.SetDestPort(req.connection.destport,outgoing_packet);
                  tcp_hdr.SetFlags(flags,outgoing_packet);
                  tcp_hdr.SetSeqNum((*cs).state.GetLastSent(),outgoing_packet);           
                  tcp_hdr.SetAckNum((*cs).state.GetLastAcked(),outgoing_packet);
                  tcp_hdr.SetWinSize(1054,outgoing_packet);
                  tcp_hdr.SetHeaderLen(TCP_HEADER_BASE_LENGTH,outgoing_packet);
                  outgoing_packet.PushBackHeader(tcp_hdr);

                  MinetSend(mux,outgoing_packet);

                  (*cs).state.SetLastSent((*cs).state.GetLastSent() + 1);
                  SockRequestResponse repl;
                  repl.type = STATUS; 
                  repl.connection = req.connection; 
                  repl.bytes = 0; 
                  repl.error = EOK; 
                  MinetSend(sock, repl);
                  (*cs).state.SetState(FIN_WAIT1);
                }
                else 
                {  
                  sockStatReply(ERESOURCE_UNAVAIL,sock,req);
                }
              }
              else
              { 
                sockStatReply(ENOMATCH,sock,req);
              }
            break;
          }

//--DEFAULT-----------------------------------------------------------------------------------------------------
          default: {break;}

        }//END SWITCH STATEMENT

      }//END SOCKET HANDLING
  
    }//END DATAFLOW/IN EVENT

    if (event.eventtype == MinetEvent::Timeout) {break;}//END TIMEOUT EVENT
  }//END EVENT LOOP

  MinetDeinit();
  return 0;
}//END MAIN
