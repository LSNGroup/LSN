#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <time.h>

#include <stdlib.h>
#include <string.h>

#include "udp.h"

using namespace std;


Socket
openPort( unsigned short port, unsigned int interfaceIp, bool verbose )
{
   Socket fd;
    
   fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if ( fd == INVALID_SOCKET )
   {
      int err = getErrno();
      cerr << "Could not create a UDP socket:" << err << endl;
      return INVALID_SOCKET;
   }
    
    int flag = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    
   struct sockaddr_in addr;
   memset((char*) &(addr),0, sizeof((addr)));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(port);
    
   if ( (interfaceIp != 0) && 
        ( interfaceIp != 0x100007f ) )
   {
      addr.sin_addr.s_addr = htonl(interfaceIp);
      if (verbose )
      {
         clog << "Binding to interface " 
              << hex << "0x" << htonl(interfaceIp) << dec << endl;
      }
   }
	
   if ( bind( fd,(struct sockaddr*)&addr, sizeof(addr)) != 0 )
   {
   	   closesocket(fd);
      int e = getErrno();
        
      switch (e)
      {
         case 0:
         {
            cerr << "Could not bind socket" << endl;
            return INVALID_SOCKET;
         }
         case EADDRINUSE:
         {
            cerr << "Port " << port << " for receiving UDP is in use" << endl;
            return INVALID_SOCKET;
         }
         break;
         case EADDRNOTAVAIL:
         {
            if ( verbose ) 
            {
               cerr << "Cannot assign requested address" << endl;
            }
            return INVALID_SOCKET;
         }
         break;
         default:
         {
            //cerr << "Could not bind UDP receive port"
            //     << "Error=" << e << " " << strerror(e) << endl;
            return INVALID_SOCKET;
         }
         break;
      }
   }
   if ( verbose )
   {
      clog << "Opened port " << port << " with fd " << fd << endl;
   }
   
   assert( fd != INVALID_SOCKET  );
	
   return fd;
}


bool 
getMessage( Socket fd, char* buf, int* len,
            unsigned int* srcIp, unsigned short* srcPort,
            bool verbose)
{
   assert( fd != INVALID_SOCKET );
	
   int originalSize = *len;
   assert( originalSize > 0 );
   
   struct sockaddr_in from;
   int fromLen = sizeof(from);
	
   *len = recvfrom(fd,
                   buf,
                   originalSize,
                   0,
                   (struct sockaddr *)&from,
                   (socklen_t*)&fromLen);
	
   if ( *len == SOCKET_ERROR )
   {
      int err = getErrno();
		
      switch (err)
      {
         case ENOTSOCK:
            cerr << "Error fd not a socket" <<   endl;
            break;
         case ECONNRESET:
            cerr << "Error connection reset - host not reachable" <<   endl;
            break;
				
         default:
            cerr << "Socket Error=" << err << endl;
      }
		
      return false;
   }
	
   if ( *len < 0 )
   {
      clog << "socket closed? negative len" << endl;
      return false;
   }
    
   if ( *len == 0 )
   {
      clog << "socket closed? zero len" << endl;
      return false;
   }
    
   *srcPort = ntohs(from.sin_port);
   *srcIp = ntohl(from.sin_addr.s_addr);
	
   if ( (*len)+1 >= originalSize )
   {
      if (verbose)
      {
         clog << "Received a message that was too large" << endl;
      }
      return false;
   }
   buf[*len]=0;
    
   return true;
}


bool 
sendMessage( Socket fd, char* buf, int l, 
             unsigned int dstIp, unsigned short dstPort,
             bool verbose)
{
   assert( fd != INVALID_SOCKET );
    
   int s;
   if ( dstPort == 0 )
   {
      // sending on a connected port 
      assert( dstIp == 0 );
		
      s = send(fd,buf,l,0);
   }
   else
   {
      assert( dstIp != 0 );
      assert( dstPort != 0 );
        
      struct sockaddr_in to;
      int toLen = sizeof(to);
      memset(&to,0,toLen);
        
      to.sin_family = AF_INET;
      to.sin_port = htons(dstPort);
      to.sin_addr.s_addr = htonl(dstIp);
        
      s = sendto(fd, buf, l, 0,(sockaddr*)&to, toLen);
   }
    
   if ( s == SOCKET_ERROR )
   {
      int e = getErrno();
      switch (e)
      {
         case ECONNREFUSED:
         case EHOSTDOWN:
         case EHOSTUNREACH:
         {
            // quietly ignore this 
         }
         break;
         case EAFNOSUPPORT:
         {
            cerr << "err EAFNOSUPPORT in send" << endl;
         }
         break;
         //default:
         //{
         //   cerr << "err " << e << " "  << strerror(e) << " in send" << endl;
         //}
      }
      return false;
   }
    
   if ( s == 0 )
   {
      cerr << "no data sent in send" << endl;
      return false;
   }
    
   if ( s != l )
   {
      if (verbose)
      {
         cerr << "only " << s << " out of " << l << " bytes sent" << endl;
      }
      return false;
   }
    
   return true;
}
