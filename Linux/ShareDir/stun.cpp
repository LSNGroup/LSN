#include <cassert>
#include <cstring>
#include <iostream>
#include <cstdlib>   

#ifdef WINCE
#include <winsock2.h>
#include <stdlib.h>
#include <time.h>
#else

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <net/if.h>

#endif


#define NOSSL

#include "udp.h"
#include "stun.h"


using namespace std;


static void
computeHmac(char* hmac, const char* input, int length, const char* key, int keySize);

static bool 
stunParseAtrAddress( char* body, unsigned int hdrLen,  StunAtrAddress4& result )
{
   if ( hdrLen != 8 )
   {
      clog << "hdrLen wrong for Address" <<endl;
      return false;
   }
   result.pad = *body++;
   result.family = *body++;
   if (result.family == IPv4Family)
   {
      UInt16 nport;
      memcpy(&nport, body, 2); body+=2;
      result.ipv4.port = ntohs(nport);
		
      UInt32bit naddr;
      memcpy(&naddr, body, 4); body+=4;
      result.ipv4.addr = ntohl(naddr);
      return true;
   }
   else if (result.family == IPv6Family)
   {
      clog << "ipv6 not supported" << endl;
   }
   else
   {
      clog << "bad address family: " << result.family << endl;
   }
	
   return false;
}

static bool 
stunParseAtrChangeRequest( char* body, unsigned int hdrLen,  StunAtrChangeRequest& result )
{
   if ( hdrLen != 4 )
   {
      clog << "hdr length = " << hdrLen << " expecting " << sizeof(result) << endl;
		
      clog << "Incorrect size for ChangeRequest" << endl;
      return false;
   }
   else
   {
      memcpy(&result.value, body, 4);
      result.value = ntohl(result.value);
      return true;
   }
}

static bool 
stunParseAtrError( char* body, unsigned int hdrLen,  StunAtrError& result )
{
   if ( hdrLen >= sizeof(result) )
   {
      clog << "head on Error too large" << endl;
      return false;
   }
   else
   {
      memcpy(&result.pad, body, 2); body+=2;
      result.pad = ntohs(result.pad);
      result.errorClass = *body++;
      result.number = *body++;
		
      result.sizeReason = hdrLen - 4;
      memcpy(&result.reason, body, result.sizeReason);
      result.reason[result.sizeReason] = 0;
      return true;
   }
}

static bool 
stunParseAtrUnknown( char* body, unsigned int hdrLen,  StunAtrUnknown& result )
{
   if ( hdrLen >= sizeof(result) )
   {
      return false;
   }
   else
   {
      if (hdrLen % 4 != 0) return false;
      result.numAttributes = hdrLen / 4;
      for (int i=0; i<result.numAttributes; i++)
      {
         memcpy(&result.attrType[i], body, 2); body+=2;
         result.attrType[i] = ntohs(result.attrType[i]);
      }
      return true;
   }
}


static bool 
stunParseAtrString( char* body, unsigned int hdrLen,  StunAtrString& result )
{
   if ( hdrLen >= STUN_MAX_STRING )
   {
      clog << "String is too large" << endl;
      return false;
   }
   else
   {
      if (hdrLen % 4 != 0)
      {
         clog << "Bad length string " << hdrLen << endl;
         return false;
      }
		
      result.sizeValue = hdrLen;
      memcpy(&result.value, body, hdrLen);
      result.value[hdrLen] = 0;
      return true;
   }
}


static bool 
stunParseAtrIntegrity( char* body, unsigned int hdrLen,  StunAtrIntegrity& result )
{
   if ( hdrLen != 20)
   {
      clog << "MessageIntegrity must be 20 bytes" << endl;
      return false;
   }
   else
   {
      memcpy(&result.hash, body, hdrLen);
      return true;
   }
}


bool
stunParseMessage( char* buf, unsigned int bufLen, StunMessage& msg, bool verbose)
{
   if (verbose) clog << "Received stun message: " << bufLen << " bytes" << endl;
   memset(&msg, 0, sizeof(msg));
	
   if (sizeof(StunMsgHdr) > bufLen)
   {
      clog << "Bad message" << endl;
      return false;
   }
	
   memcpy(&msg.msgHdr, buf, sizeof(StunMsgHdr));
   msg.msgHdr.msgType = ntohs(msg.msgHdr.msgType);
   msg.msgHdr.msgLength = ntohs(msg.msgHdr.msgLength);
	
   if (msg.msgHdr.msgLength + sizeof(StunMsgHdr) != bufLen)
   {
      clog << "Message header length doesn't match message size: "
           << msg.msgHdr.msgLength << " - " << bufLen << endl;
      return false;
   }
	
   char* body = buf + sizeof(StunMsgHdr);
   unsigned int size = msg.msgHdr.msgLength;
	
   //clog << "bytes after header = " << size << endl;
	
   while ( size > 0 )
   {
      // !jf! should check that there are enough bytes left in the buffer
		
      StunAtrHdr* attr = reinterpret_cast<StunAtrHdr*>(body);
		
      unsigned int attrLen = ntohs(attr->length);
      int atrType = ntohs(attr->type);
		
      //if (verbose) clog << "Found attribute type=" << AttrNames[atrType] << " length=" << attrLen << endl;
      if ( attrLen+4 > size ) 
      {
         clog << "claims attribute is larger than size of message " 
              <<"(attribute type="<<atrType<<")"<< endl;
         return false;
      }
		
      body += 4; // skip the length and type in attribute header
      size -= 4;
		
      switch ( atrType )
      {
         case MappedAddress:
            msg.hasMappedAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  msg.mappedAddress )== false )
            {
               clog << "problem parsing MappedAddress" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "MappedAddress = " << msg.mappedAddress.ipv4 << endl;
            }
					
            break;  

         case ResponseAddress:
            msg.hasResponseAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  msg.responseAddress )== false )
            {
               clog << "problem parsing ResponseAddress" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "ResponseAddress = " << msg.responseAddress.ipv4 << endl;
            }
            break;  
				
         case ChangeRequest:
            msg.hasChangeRequest = true;
            if (stunParseAtrChangeRequest( body, attrLen, msg.changeRequest) == false)
            {
               clog << "problem parsing ChangeRequest" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "ChangeRequest = " << msg.changeRequest.value << endl;
            }
            break;
				
         case SourceAddress:
            msg.hasSourceAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  msg.sourceAddress )== false )
            {
               clog << "problem parsing SourceAddress" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "SourceAddress = " << msg.sourceAddress.ipv4 << endl;
            }
            break;  
				
         case ChangedAddress:
            msg.hasChangedAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  msg.changedAddress )== false )
            {
               clog << "problem parsing ChangedAddress" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "ChangedAddress = " << msg.changedAddress.ipv4 << endl;
            }
            break;  
				
         case Username: 
            msg.hasUsername = true;
            if (stunParseAtrString( body, attrLen, msg.username) == false)
            {
               clog << "problem parsing Username" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "Username = " << msg.username.value << endl;
            }
					
            break;
				
         case Password: 
            msg.hasPassword = true;
            if (stunParseAtrString( body, attrLen, msg.password) == false)
            {
               clog << "problem parsing Password" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "Password = " << msg.password.value << endl;
            }
            break;
				
         case MessageIntegrity:
            msg.hasMessageIntegrity = true;
            if (stunParseAtrIntegrity( body, attrLen, msg.messageIntegrity) == false)
            {
               clog << "problem parsing MessageIntegrity" << endl;
               return false;
            }
            else
            {
               //if (verbose) clog << "MessageIntegrity = " << msg.messageIntegrity.hash << endl;
            }
					
            // read the current HMAC
            // look up the password given the user of given the transaction id 
            // compute the HMAC on the buffer
            // decide if they match or not
            break;
				
         case ErrorCode:
            msg.hasErrorCode = true;
            if (stunParseAtrError(body, attrLen, msg.errorCode) == false)
            {
               clog << "problem parsing ErrorCode" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "ErrorCode = " << int(msg.errorCode.errorClass) 
                                 << " " << int(msg.errorCode.number) 
                                 << " " << msg.errorCode.reason << endl;
            }
					
            break;
				
         case UnknownAttribute:
            msg.hasUnknownAttributes = true;
            if (stunParseAtrUnknown(body, attrLen, msg.unknownAttributes) == false)
            {
               clog << "problem parsing UnknownAttribute" << endl;
               return false;
            }
            break;
				
         case ReflectedFrom:
            msg.hasReflectedFrom = true;
            if ( stunParseAtrAddress(  body,  attrLen,  msg.reflectedFrom ) == false )
            {
               clog << "problem parsing ReflectedFrom" << endl;
               return false;
            }
            break;  
				
         case XorMappedAddress:
            msg.hasXorMappedAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  msg.xorMappedAddress ) == false )
            {
               clog << "problem parsing XorMappedAddress" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "XorMappedAddress = " << msg.mappedAddress.ipv4 << endl;
            }
            break;  

         case XorOnly:
            msg.xorOnly = true;
            if (verbose) 
            {
               clog << "xorOnly = true" << endl;
            }
            break;  
				
         case ServerName: 
            msg.hasServerName = true;
            if (stunParseAtrString( body, attrLen, msg.serverName) == false)
            {
               clog << "problem parsing ServerName" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "ServerName = " << msg.serverName.value << endl;
            }
            break;
				
         case SecondaryAddress:
            msg.hasSecondaryAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  msg.secondaryAddress ) == false )
            {
               clog << "problem parsing secondaryAddress" << endl;
               return false;
            }
            else
            {
               if (verbose) clog << "SecondaryAddress = " << msg.secondaryAddress.ipv4 << endl;
            }
            break;  
					
         default:
            if (verbose) clog << "Unknown attribute: " << atrType << endl;
            if ( atrType <= 0x7FFF ) 
            {
               return false;
            }
      }
		
      body += attrLen;
      size -= attrLen;
   }
    
   return true;
}


static char* 
encode16(char* buf, UInt16 data)
{
   UInt16 ndata = htons(data);
   memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(UInt16));
   return buf + sizeof(UInt16);
}

static char* 
encode32(char* buf, UInt32bit data)
{
   UInt32bit ndata = htonl(data);
   memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(UInt32bit));
   return buf + sizeof(UInt32bit);
}


static char* 
encode(char* buf, const char* data, unsigned int length)
{
   memcpy(buf, data, length);
   return buf + length;
}


static char* 
encodeAtrAddress4(char* ptr, UInt16 type, const StunAtrAddress4& atr)
{
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, 8);
   *ptr++ = atr.pad;
   *ptr++ = IPv4Family;
   ptr = encode16(ptr, atr.ipv4.port);
   ptr = encode32(ptr, atr.ipv4.addr);
	
   return ptr;
}

static char* 
encodeAtrChangeRequest(char* ptr, const StunAtrChangeRequest& atr)
{
   ptr = encode16(ptr, ChangeRequest);
   ptr = encode16(ptr, 4);
   ptr = encode32(ptr, atr.value);
   return ptr;
}

static char* 
encodeAtrError(char* ptr, const StunAtrError& atr)
{
   ptr = encode16(ptr, ErrorCode);
   ptr = encode16(ptr, 6 + atr.sizeReason);
   ptr = encode16(ptr, atr.pad);
   *ptr++ = atr.errorClass;
   *ptr++ = atr.number;
   ptr = encode(ptr, atr.reason, atr.sizeReason);
   return ptr;
}


static char* 
encodeAtrUnknown(char* ptr, const StunAtrUnknown& atr)
{
   ptr = encode16(ptr, UnknownAttribute);
   ptr = encode16(ptr, 2+2*atr.numAttributes);
   for (int i=0; i<atr.numAttributes; i++)
   {
      ptr = encode16(ptr, atr.attrType[i]);
   }
   return ptr;
}


static char* 
encodeXorOnly(char* ptr)
{
   ptr = encode16(ptr, XorOnly );
   return ptr;
}


static char* 
encodeAtrString(char* ptr, UInt16 type, const StunAtrString& atr)
{
   assert(atr.sizeValue % 4 == 0);
	
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, atr.sizeValue);
   ptr = encode(ptr, atr.value, atr.sizeValue);
   return ptr;
}


static char* 
encodeAtrIntegrity(char* ptr, const StunAtrIntegrity& atr)
{
   ptr = encode16(ptr, MessageIntegrity);
   ptr = encode16(ptr, 20);
   ptr = encode(ptr, atr.hash, sizeof(atr.hash));
   return ptr;
}


unsigned int
stunEncodeMessage( const StunMessage& msg, 
                   char* buf, 
                   unsigned int bufLen, 
                   const StunAtrString& password, 
                   bool verbose)
{
   assert(bufLen >= sizeof(StunMsgHdr));
   char* ptr = buf;
	
   ptr = encode16(ptr, msg.msgHdr.msgType);
   char* lengthp = ptr;
   ptr = encode16(ptr, 0);
   ptr = encode(ptr, reinterpret_cast<const char*>(msg.msgHdr.id.octet), sizeof(msg.msgHdr.id));
	
   if (verbose) clog << "Encoding stun message: " << endl;
   if (msg.hasMappedAddress)
   {
      if (verbose) clog << "Encoding MappedAddress: " << msg.mappedAddress.ipv4 << endl;
      ptr = encodeAtrAddress4 (ptr, MappedAddress, msg.mappedAddress);
   }
   if (msg.hasResponseAddress)
   {
      if (verbose) clog << "Encoding ResponseAddress: " << msg.responseAddress.ipv4 << endl;
      ptr = encodeAtrAddress4(ptr, ResponseAddress, msg.responseAddress);
   }
   if (msg.hasChangeRequest)
   {
      if (verbose) clog << "Encoding ChangeRequest: " << msg.changeRequest.value << endl;
      ptr = encodeAtrChangeRequest(ptr, msg.changeRequest);
   }
   if (msg.hasSourceAddress)
   {
      if (verbose) clog << "Encoding SourceAddress: " << msg.sourceAddress.ipv4 << endl;
      ptr = encodeAtrAddress4(ptr, SourceAddress, msg.sourceAddress);
   }
   if (msg.hasChangedAddress)
   {
      if (verbose) clog << "Encoding ChangedAddress: " << msg.changedAddress.ipv4 << endl;
      ptr = encodeAtrAddress4(ptr, ChangedAddress, msg.changedAddress);
   }
   if (msg.hasUsername)
   {
      if (verbose) clog << "Encoding Username: " << msg.username.value << endl;
      ptr = encodeAtrString(ptr, Username, msg.username);
   }
   if (msg.hasPassword)
   {
      if (verbose) clog << "Encoding Password: " << msg.password.value << endl;
      ptr = encodeAtrString(ptr, Password, msg.password);
   }
   if (msg.hasErrorCode)
   {
      if (verbose) clog << "Encoding ErrorCode: class=" 
			<< int(msg.errorCode.errorClass)  
			<< " number=" << int(msg.errorCode.number) 
			<< " reason=" 
			<< msg.errorCode.reason 
			<< endl;
		
      ptr = encodeAtrError(ptr, msg.errorCode);
   }
   if (msg.hasUnknownAttributes)
   {
      if (verbose) clog << "Encoding UnknownAttribute: ???" << endl;
      ptr = encodeAtrUnknown(ptr, msg.unknownAttributes);
   }
   if (msg.hasReflectedFrom)
   {
      if (verbose) clog << "Encoding ReflectedFrom: " << msg.reflectedFrom.ipv4 << endl;
      ptr = encodeAtrAddress4(ptr, ReflectedFrom, msg.reflectedFrom);
   }
   if (msg.hasXorMappedAddress)
   {
      if (verbose) clog << "Encoding XorMappedAddress: " << msg.xorMappedAddress.ipv4 << endl;
      ptr = encodeAtrAddress4 (ptr, XorMappedAddress, msg.xorMappedAddress);
   }
   if (msg.xorOnly)
   {
      if (verbose) clog << "Encoding xorOnly: " << endl;
      ptr = encodeXorOnly( ptr );
   }
   if (msg.hasServerName)
   {
      if (verbose) clog << "Encoding ServerName: " << msg.serverName.value << endl;
      ptr = encodeAtrString(ptr, ServerName, msg.serverName);
   }
   if (msg.hasSecondaryAddress)
   {
      if (verbose) clog << "Encoding SecondaryAddress: " << msg.secondaryAddress.ipv4 << endl;
      ptr = encodeAtrAddress4 (ptr, SecondaryAddress, msg.secondaryAddress);
   }

   if (password.sizeValue > 0)
   {
      if (verbose) clog << "HMAC with password: " << password.value << endl;
		
      StunAtrIntegrity integrity;
      computeHmac(integrity.hash, buf, int(ptr-buf) , password.value, password.sizeValue);
      ptr = encodeAtrIntegrity(ptr, integrity);
   }
   if (verbose) clog << endl;
	
   encode16(lengthp, UInt16(ptr - buf - sizeof(StunMsgHdr)));
   return int(ptr - buf);
}

int 
stunRand()
{
   // return 32 bits of random stuff
   assert( sizeof(int) == 4 );
   static bool init=false;
   if ( !init )
   { 
      init = true;
		
      UInt64 tick;
		
#if 1//defined(ANDROID_NDK) && defined(PLATFORM_ARMV4I) 
      int fd=open("/dev/random",O_RDONLY);
      read(fd,&tick,sizeof(tick));
      closesocket(fd);
#else
#     error Need some way to seed the random number generator 
#endif 
      int seed = int(tick);
#ifdef WINCE
      srand(seed);
#else
      srandom(seed);
#endif
   }
	
#ifdef WINCE
   assert( RAND_MAX == 0x7fff );
   int r1 = rand();
   int r2 = rand();
	
   int ret = (r1<<16) + r2;
	
   return ret;
#else
   return random(); 
#endif
}


/// return a random number to use as a port 
int
stunRandomPort()
{
   int min=0x4000;
   int max=0x7FFF;
	
   int ret = stunRand();
   ret = ret|min;
   ret = ret&max;
	
   return ret;
}


#ifdef NOSSL
static void
computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
{
   strncpy(hmac,"hmac-not-implemented",20);
}
#else
#include <openssl/hmac.h>

static void
computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
{
   unsigned int resultSize=0;
   HMAC(EVP_sha1(), 
        key, sizeKey, 
        reinterpret_cast<const unsigned char*>(input), length, 
        reinterpret_cast<unsigned char*>(hmac), &resultSize);
   assert(resultSize == 20);
}
#endif


static void
toHex(const char* buffer, int bufferSize, char* output) 
{
   static char hexmap[] = "0123456789abcdef";
	
   const char* p = buffer;
   char* r = output;
   for (int i=0; i < bufferSize; i++)
   {
      unsigned char temp = *p++;
		
      int hi = (temp & 0xf0)>>4;
      int low = (temp & 0xf);
		
      *r++ = hexmap[hi];
      *r++ = hexmap[low];
   }
   *r = 0;
}

void
stunCreateUserName(const StunAddress4& source, StunAtrString* username)
{
   UInt64 time = stunGetSystemTimeSecs();
   time -= (time % 20*60);
   //UInt64 hitime = time >> 32;
   UInt64 lotime = time & 0xFFFFFFFF;
	
   char buffer[1024];
   sprintf(buffer,
           "%08x:%08x:%08x:", 
           (UInt32bit)(source.addr),
           (UInt32bit)(stunRand()),
           (UInt32bit)(lotime));
   assert( strlen(buffer) < 1024 );
	
   assert(strlen(buffer) + 41 < STUN_MAX_STRING);
	
   char hmac[20];
   char key[] = "Jason";
   computeHmac(hmac, buffer, strlen(buffer), key, strlen(key) );
   char hmacHex[41];
   toHex(hmac, 20, hmacHex );
   hmacHex[40] =0;
	
   strcat(buffer,hmacHex);
	
   int l = strlen(buffer);
   assert( l+1 < STUN_MAX_STRING );
   assert( l%4 == 0 );
   
   username->sizeValue = l;
   memcpy(username->value,buffer,l);
   username->value[l]=0;
	
   //if (verbose) clog << "computed username=" << username.value << endl;
}

void
stunCreatePassword(const StunAtrString& username, StunAtrString* password)
{
   char hmac[20];
   char key[] = "Fluffy";
   //char buffer[STUN_MAX_STRING];
   computeHmac(hmac, username.value, strlen(username.value), key, strlen(key));
   toHex(hmac, 20, password->value);
   password->sizeValue = 40;
   password->value[40]=0;
	
   //clog << "password=" << password->value << endl;
}


UInt64
stunGetSystemTimeSecs()
{
   UInt64 time=0;
#if defined(WINCE)  
   SYSTEMTIME t;
   // CJ TODO - this probably has bug on wrap around every 24 hours
   GetSystemTime( &t );
   time = (t.wHour*60+t.wMinute)*60+t.wSecond; 
#else
   struct timeval now;
   gettimeofday( &now , NULL );
   //assert( now );
   time = now.tv_sec;
#endif
   return time;
}


ostream& operator<< ( ostream& strm, const UInt128& r )
{
   strm << int(r.octet[0]);
   for ( int i=1; i<16; i++ )
   {
      strm << ':' << int(r.octet[i]);
   }
    
   return strm;
}

ostream& 
operator<<( ostream& strm, const StunAddress4& addr)
{
   UInt32bit ip = addr.addr;
   strm << ((int)(ip>>24)&0xFF) << ".";
   strm << ((int)(ip>>16)&0xFF) << ".";
   strm << ((int)(ip>> 8)&0xFF) << ".";
   strm << ((int)(ip>> 0)&0xFF) ;
	
   strm << ":" << addr.port;
	
   return strm;
}


// returns true if it scucceeded
bool 
stunParseHostName( char* peerName,
               UInt32bit& ip,
               UInt16& portVal,
               UInt16 defaultPort )
{
   in_addr sin_addr;
    
   char host[512];
   strncpy(host,peerName,512);
   host[512-1]='\0';
   char* port = NULL;
	
   int portNum = defaultPort;
	
   // pull out the port part if present.
   char* sep = strchr(host,':');
	
   if ( sep == NULL )
   {
      portNum = defaultPort;
   }
   else
   {
      *sep = '\0';
      port = sep + 1;
      // set port part
		
      char* endPtr=NULL;
		
      portNum = strtol(port,&endPtr,10);
		
      if ( endPtr != NULL )
      {
         if ( *endPtr != '\0' )
         {
            portNum = defaultPort;
         }
      }
   }
    
   if ( portNum < 1024 ) return false;
   if ( portNum >= 0xFFFF ) return false;
	
   // figure out the host part 
   struct hostent* h;
	
#ifdef WINCE
   assert( strlen(host) >= 1 );
   if ( isdigit( host[0] ) )
   {
      // assume it is a ip address 
      unsigned int a = inet_addr(host);
      //cerr << "a=0x" << hex << a << dec << endl;
		
      ip = ntohl( a );
   }
   else
   {
      // assume it is a host name 
      h = gethostbyname( host );
		
      if ( h == NULL )
      {
         int err = getErrno();
         std::cerr << "error was " << err << std::endl;
         assert( err != WSANOTINITIALISED );
			
         ip = ntohl( 0x7F000001L );
			
         return false;
      }
      else
      {
         sin_addr = *(struct in_addr*)h->h_addr;
         ip = ntohl( sin_addr.s_addr );
      }
   }
	
#else
   h = gethostbyname( host );
   if ( h == NULL )
   {
      int err = getErrno();
      std::cerr << "error was " << err << std::endl;
      ip = ntohl( 0x7F000001L );
      return false;
   }
   else
   {
      sin_addr = *(struct in_addr*)h->h_addr;
      ip = ntohl( sin_addr.s_addr );
   }
#endif
	
   portVal = portNum;
	
   return true;
}


bool
stunParseServerName( char* name, StunAddress4& addr)
{
   assert(name!=NULL);
	
   // TODO - put in DNS SRV stuff.
	
   bool ret = stunParseHostName( name, addr.addr, addr.port, 3478); 
   if ( ret != true ) 
   {
       addr.port=0xFFFF;
   }	
   return ret;
}


static void
stunCreateErrorResponse(StunMessage& response, int cl, int number, const char* msg)
{
   response.msgHdr.msgType = BindErrorResponseMsg;
   response.hasErrorCode = true;
   response.errorCode.errorClass = cl;
   response.errorCode.number = number;
   strcpy(response.errorCode.reason, msg);
}

#if 0
static void
stunCreateSharedSecretErrorResponse(StunMessage& response, int cl, int number, const char* msg)
{
   response.msgHdr.msgType = SharedSecretErrorResponseMsg;
   response.hasErrorCode = true;
   response.errorCode.errorClass = cl;
   response.errorCode.number = number;
   strcpy(response.errorCode.reason, msg);
}
#endif

static void
stunCreateSharedSecretResponse(const StunMessage& request, const StunAddress4& source, StunMessage& response)
{
   response.msgHdr.msgType = SharedSecretResponseMsg;
   response.msgHdr.id = request.msgHdr.id;
	
   response.hasUsername = true;
   stunCreateUserName( source, &response.username);
	
   response.hasPassword = true;
   stunCreatePassword( response.username, &response.password);
}


int 
stunFindLocalInterfaces(UInt32bit* addresses,int maxRet)
{
#if defined(WINCE) || defined(__sparc__)
   return 0;
#else
   struct ifconf ifc;
	
   int s = socket( AF_INET, SOCK_DGRAM, 0 );
   int len = 100 * sizeof(struct ifreq);
	
   char buf[ len ];
	
   ifc.ifc_len = len;
   ifc.ifc_buf = buf;
	
   int e = ioctl(s,SIOCGIFCONF,&ifc);
   char *ptr = buf;
   int tl = ifc.ifc_len;
   int count=0;
	
   while ( (tl > 0) && ( count < maxRet) )
   {
      struct ifreq* ifr = (struct ifreq *)ptr;
		
      int si = sizeof(ifr->ifr_name) + sizeof(struct sockaddr);
      tl -= si;
      ptr += si;
      //char* name = ifr->ifr_ifrn.ifrn_name;
      //cerr << "name = " << name << endl;
		
      struct ifreq ifr2;
      ifr2 = *ifr;
		
      e = ioctl(s,SIOCGIFADDR,&ifr2);
      if ( e == -1 )
      {
         break;
      }
		
      //cerr << "ioctl addr e = " << e << endl;
		
      struct sockaddr a = ifr2.ifr_addr;
      struct sockaddr_in* addr = (struct sockaddr_in*) &a;
		
      UInt32bit ai = ntohl( addr->sin_addr.s_addr );
      if (int((ai>>24)&0xFF) != 127)
      {
         addresses[count++] = ai;
      }
		
#if 0
      cerr << "Detected interface "
           << int((ai>>24)&0xFF) << "." 
           << int((ai>>16)&0xFF) << "." 
           << int((ai>> 8)&0xFF) << "." 
           << int((ai    )&0xFF) << endl;
#endif
   }
	
   closesocket(s);
	
   return count;
#endif
}


void
stunBuildReqSimple( StunMessage* msg,
                    const StunAtrString& username,
                    bool changePort, bool changeIp, unsigned int id )
{
   assert( msg != NULL );
   memset( msg , 0 , sizeof(*msg) );
	
   msg->msgHdr.msgType = BindRequestMsg;
	
   for ( int i=0; i<16; i=i+4 )
   {
      assert(i+3<16);
      int r = stunRand();
      msg->msgHdr.id.octet[i+0]= r>>0;
      msg->msgHdr.id.octet[i+1]= r>>8;
      msg->msgHdr.id.octet[i+2]= r>>16;
      msg->msgHdr.id.octet[i+3]= r>>24;
   }
	
   if ( id != 0 )
   {
      msg->msgHdr.id.octet[0] = id; 
   }
	
   msg->hasChangeRequest = true;
   msg->changeRequest.value =(changeIp?ChangeIpFlag:0) | 
      (changePort?ChangePortFlag:0);
	
   if ( username.sizeValue > 0 )
   {
      msg->hasUsername = true;
      msg->username = username;
   }
}


static void 
stunSendTest( Socket myFd, StunAddress4& dest, 
              const StunAtrString& username, const StunAtrString& password, 
              int testNum, bool verbose )
{ 
   assert( dest.addr != 0 );
   assert( dest.port != 0 );
	
   bool changePort=false;
   bool changeIP=false;
   bool discard=false;
	
   switch (testNum)
   {
      case 1:
      case 10:
      case 11:
         break;
      case 2:
         //changePort=true;
         changeIP=true;
         break;
      case 3:
         changePort=true;
         break;
      case 4:
         changeIP=true;
         break;
      case 5:
         discard=true;
         break;
      default:
         cerr << "Test " << testNum <<" is unkown\n";
         assert(0);
   }
	
   StunMessage req;
   memset(&req, 0, sizeof(StunMessage));
	
   stunBuildReqSimple( &req, username, 
                       changePort , changeIP , 
                       testNum );
	
   char buf[STUN_MAX_MESSAGE_SIZE];
   int len = STUN_MAX_MESSAGE_SIZE;
	
   len = stunEncodeMessage( req, buf, len, password,verbose );
	
   if ( verbose )
   {
      clog << "About to send msg of len " << len << " to " << dest << endl;
   }
	
   sendMessage( myFd, buf, len, dest.addr, dest.port, verbose );
	
   // add some delay so the packets don't get sent too quickly 
	usleep(10*1000);
}


void 
stunGetUserNameAndPassword(  const StunAddress4& dest, 
                             StunAtrString* username,
                             StunAtrString* password)
{ 
   // !cj! This is totally bogus - need to make TLS connection to dest and get a
   // username and password to use 
   stunCreateUserName(dest, username);
   stunCreatePassword(*username, password);
}


void 
stunTest( StunAddress4& dest, int testNum, bool verbose, StunAddress4* sAddr )
{ 
   assert( dest.addr != 0 );
   assert( dest.port != 0 );
	
   int port = stunRandomPort();
   UInt32bit interfaceIp=0;
   if (sAddr)
   {
      interfaceIp = sAddr->addr;
      if ( sAddr->port != 0 )
      {
        port = sAddr->port;
      }
   }
   Socket myFd = openPort(port,interfaceIp,verbose);
   if (myFd == INVALID_SOCKET)
   {
        cerr << "Some problem opening port/interface to send on" << endl;
       return;
   }

   StunAtrString username;
   StunAtrString password;
   char msg[STUN_MAX_MESSAGE_SIZE];
   int msgLen = STUN_MAX_MESSAGE_SIZE;
   StunAddress4 from;

   username.sizeValue = 0;
   password.sizeValue = 0;



   int count=0;
   while ( count < 3 )
   {
      struct timeval tv;
      fd_set fdSet; 
#ifdef WINCE
      unsigned int fdSetSize;
#else
      int fdSetSize;
#endif
      FD_ZERO(&fdSet); fdSetSize=0;
      FD_SET(myFd,&fdSet); fdSetSize = (myFd+1>fdSetSize) ? myFd+1 : fdSetSize;
      tv.tv_sec=1;
      tv.tv_usec=500*1000; // 150 ms --> 1500 ms
      if ( count == 0 ) {tv.tv_sec=0; tv.tv_usec=0;}
		
      int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
      int e = getErrno();
      if ( err == SOCKET_ERROR )
      {
         // error occured
         //cerr << "Error " << e << " " << strerror(e) << " in _select" << endl;
	   closesocket(myFd);
	   return;
     }
      else if ( err == 0 )
      {
         // timeout occured 
         count++;
			
		#ifdef USE_TLS
		   stunGetUserNameAndPassword( dest, username, password );
		#endif
			
		   stunSendTest( myFd, dest, username, password, testNum, verbose );
			

      }
      else
      {
         //if (verbose) clog << "-----------------------------------------" << endl;
         assert( err>0 );
         // data is avialbe on some fd 

		   if ( FD_ISSET(myFd,&fdSet) )
		   {
			   getMessage( myFd,
						   msg,
						   &msgLen,
						   &from.addr,
						   &from.port,verbose );

			   StunMessage resp;
			   memset(&resp, 0, sizeof(StunMessage));
				
			   if ( verbose ) clog << "Got a response" << endl;
			   bool ok = stunParseMessage( msg,msgLen, resp,verbose );
				
			   if ( verbose )
			   {
				  clog << "\t ok=" << ok << endl;
				  clog << "\t id=" << resp.msgHdr.id << endl;
				  clog << "\t mappedAddr=" << resp.mappedAddress.ipv4 << endl;
				  clog << "\t changedAddr=" << resp.changedAddress.ipv4 << endl;
				  clog << endl;
			   }
				
			   if (sAddr)
			   {
				  sAddr->port = resp.mappedAddress.ipv4.port;
				  sAddr->addr = resp.mappedAddress.ipv4.addr;
			   }

			   break;
			}
	  }
   }


   closesocket(myFd);
}


NatType
stunNatType( StunAddress4& dest, 
             bool verbose,
             bool* preservePort, // if set, is return for if NAT preservers ports or not
             bool* hairpin,  // if set, is the return for if NAT will hairpin packets
             int port, // port to use for the test, 0 to choose random port
             StunAddress4* sAddr // NIC to use 
   )
{ 
   assert( dest.addr != 0 );
   assert( dest.port != 0 );
	
   if ( hairpin ) 
   {
      *hairpin = false;
   }
	
   if ( port == 0 )
   {
      port = stunRandomPort();
   }
   UInt32bit interfaceIp=0;
   if (sAddr)
   {
      interfaceIp = sAddr->addr;
   }
   Socket myFd1 = openPort(port,interfaceIp,verbose);
   Socket myFd2 = openPort(port+1,interfaceIp,verbose);

   if ( ( myFd1 == INVALID_SOCKET) || ( myFd2 == INVALID_SOCKET) )
   {
   	   closesocket(myFd1);
	   closesocket(myFd2);
        cerr << "Some problem opening port/interface to send on" << endl;
       return StunTypeFailure; 
   }

   assert( myFd1 != INVALID_SOCKET );
   assert( myFd2 != INVALID_SOCKET );
    
   bool respTestI=false;
   bool isNat=true;
   StunAddress4 testIchangedAddr;
   StunAddress4 testImappedAddr;
   bool respTestI2=false; 
   bool mappedIpSame = true;
   StunAddress4 testI2mappedAddr;
   StunAddress4 testI2dest=dest;
   bool respTestII=false;
   bool respTestIII=false;

   bool respTestHairpin=false;
   bool respTestPreservePort=false;
	
   memset(&testImappedAddr,0,sizeof(testImappedAddr));
	
   StunAtrString username;
   StunAtrString password;
	
   username.sizeValue = 0;
   password.sizeValue = 0;
	
#ifdef USE_TLS 
   stunGetUserNameAndPassword( dest, username, password );
#endif
	
   int count=0;
   while ( count < 7 )
   {
      struct timeval tv;
      fd_set fdSet; 
#ifdef WINCE
      unsigned int fdSetSize;
#else
      int fdSetSize;
#endif
      FD_ZERO(&fdSet); fdSetSize=0;
      FD_SET(myFd1,&fdSet); fdSetSize = (myFd1+1>fdSetSize) ? myFd1+1 : fdSetSize;
      FD_SET(myFd2,&fdSet); fdSetSize = (myFd2+1>fdSetSize) ? myFd2+1 : fdSetSize;
      tv.tv_sec=1;
      tv.tv_usec=500*1000; // 150 ms --> 1500 ms
      if ( count == 0 ) {tv.tv_sec=0; tv.tv_usec=0;}
		
      int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
      int e = getErrno();
      if ( err == SOCKET_ERROR )
      {
         // error occured
	   cerr << "Error " << e << " " << strerror(e) << " in _select" << endl;
	   closesocket(myFd1);
	   closesocket(myFd2);
        return StunTypeFailure; 
     }
      else if ( err == 0 )
      {
         // timeout occured 
         count++;
			
         if ( !respTestI ) 
         {
            stunSendTest( myFd1, dest, username, password, 1 ,verbose );
         }         
			
         if ( (!respTestI2) && respTestI ) 
         {
            // check the address to send to if valid 
            if (  ( testI2dest.addr != 0 ) &&
                  ( testI2dest.port != 0 ) )
            {
               stunSendTest( myFd1, testI2dest, username, password, 10  ,verbose);
            }
         }
			
         if ( !respTestII )
         {
            stunSendTest( myFd2, dest, username, password, 2 ,verbose );
         }
			
         if ( !respTestIII )
         {
            stunSendTest( myFd2, dest, username, password, 3 ,verbose );
         }
			
         if ( respTestI && (!respTestHairpin) )
         {
            if (  ( testImappedAddr.addr != 0 ) &&
                  ( testImappedAddr.port != 0 ) )
            {
               stunSendTest( myFd1, testImappedAddr, username, password, 11 ,verbose );
            }
         }
      }
      else
      {
         //if (verbose) clog << "-----------------------------------------" << endl;
         assert( err>0 );
         // data is avialbe on some fd 
			
         for ( int i=0; i<2; i++)
         {
            Socket myFd;
            if ( i==0 ) 
            {
               myFd=myFd1;
            }
            else
            {
               myFd=myFd2;
            }
				
            if ( myFd!=INVALID_SOCKET ) 
            {					
               if ( FD_ISSET(myFd,&fdSet) )
               {
                  char msg[STUN_MAX_MESSAGE_SIZE];
                  int msgLen = sizeof(msg);
                  						
                  StunAddress4 from;
						
                  getMessage( myFd,
                              msg,
                              &msgLen,
                              &from.addr,
                              &from.port,verbose );
						
                  StunMessage resp;
                  memset(&resp, 0, sizeof(StunMessage));
						
                  stunParseMessage( msg,msgLen, resp,verbose );
						
                  if ( verbose )
                  {
                     clog << "Received message of type " << resp.msgHdr.msgType 
                          << "  id=" << (int)(resp.msgHdr.id.octet[0]) << endl;
                  }
						
                  switch( resp.msgHdr.id.octet[0] )
                  {
                     case 1:
                     {
                        if ( !respTestI )
                        {
									
                           testIchangedAddr.addr = resp.changedAddress.ipv4.addr;
                           testIchangedAddr.port = resp.changedAddress.ipv4.port;
                           testImappedAddr.addr = resp.mappedAddress.ipv4.addr;
                           testImappedAddr.port = resp.mappedAddress.ipv4.port;
			
                           respTestPreservePort = ( testImappedAddr.port == port ); 
                           if ( preservePort )
                           {
                              *preservePort = respTestPreservePort;
                           }								
									
                           testI2dest.addr = resp.changedAddress.ipv4.addr;
									
                           if (sAddr)
                           {
                              sAddr->port = testImappedAddr.port;
                              sAddr->addr = testImappedAddr.addr;
                           }
									
                           count = 0;
                        }		
                        respTestI=true;
                     }
                     break;
                     case 2:
                     {  
                        respTestII=true;
                     }
                     break;
                     case 3:
                     {
                        respTestIII=true;
                     }
                     break;
                     case 10:
                     {
                        if ( !respTestI2 )
                        {
                           testI2mappedAddr.addr = resp.mappedAddress.ipv4.addr;
                           testI2mappedAddr.port = resp.mappedAddress.ipv4.port;
								
                           mappedIpSame = false;
                           if ( (testI2mappedAddr.addr  == testImappedAddr.addr ) &&
                                (testI2mappedAddr.port == testImappedAddr.port ))
                           { 
                              mappedIpSame = true;
                           }
								
							
                        }
                        respTestI2=true;
                     }
                     break;
                     case 11:
                     {
							
                        if ( hairpin ) 
                        {
                           *hairpin = true;
                        }
                        respTestHairpin = true;
                     }
                     break;
                  }
               }
            }
         }
      }
   }
	
   // see if we can bind to this address 
   //cerr << "try binding to " << testImappedAddr << endl;
   Socket s = openPort( 0/*use ephemeral*/, testImappedAddr.addr, false );
   if ( s != INVALID_SOCKET )
   {
      closesocket(s);
      isNat = false;
      //cerr << "binding worked" << endl;
   }
   else
   {
      isNat = true;
      //cerr << "binding failed" << endl;
   }
	
   if (verbose)
   {
      clog << "test I = " << respTestI << endl;
      clog << "test II = " << respTestII << endl;
      clog << "test III = " << respTestIII << endl;
      clog << "test I(2) = " << respTestI2 << endl;
      clog << "is nat  = " << isNat <<endl;
      clog << "mapped IP same = " << mappedIpSame << endl;
      clog << "hairpin = " << respTestHairpin << endl;
      clog << "preserver port = " << respTestPreservePort << endl;
   }

   	
   closesocket(myFd1);
   closesocket(myFd2);

#if 0
   // implement logic flow chart from draft RFC
   if ( respTestI )
   {
      if ( isNat )
      {
         if (respTestII)
         {
            return StunTypeConeNat;
         }
         else
         {
            if ( mappedIpSame )
            {
               if ( respTestIII )
               {
                  return StunTypeRestrictedNat;
               }
               else
               {
                  return StunTypePortRestrictedNat;
               }
            }
            else
            {
               return StunTypeSymNat;
            }
         }
      }
      else
      {
         if (respTestII)
         {
            return StunTypeOpen;
         }
         else
         {
            return StunTypeSymFirewall;
         }
      }
   }
   else
   {
      return StunTypeBlocked;
   }
#else
   if ( respTestI ) // not blocked 
   {
      if ( isNat )
      {
         if ( mappedIpSame )
         {
            if (respTestII)
            {
               return StunTypeIndependentFilter;
            }
            else
            {
               if ( respTestIII )
               {
                  return StunTypeDependentFilter;
               }
               else
               {
                  return StunTypePortDependedFilter;
               }
            }
         }
         else // mappedIp is not same 
         {
            return StunTypeDependentMapping;
         }
      }
      else  // isNat is false
      {
         if (respTestII)
         {
            return StunTypeOpen;
         }
         else
         {
            return StunTypeFirewall;
         }
      }
   }
   else
   {
      return StunTypeBlocked;
   }
#endif

   return StunTypeUnknown;
}


int
stunOpenSocket( StunAddress4& dest, StunAddress4* mapAddr, 
                int port, StunAddress4* srcAddr, 
                bool verbose )
{
   assert( dest.addr != 0 );
   assert( dest.port != 0 );
   assert( mapAddr != NULL );
   
   if ( port == 0 )
   {
      port = stunRandomPort();
   }
   unsigned int interfaceIp = 0;
   if ( srcAddr )
   {
      interfaceIp = srcAddr->addr;
   }
   
   Socket myFd = openPort(port,interfaceIp,verbose);
   if (myFd == INVALID_SOCKET)
   {
      return myFd;
   }
   
   char msg[STUN_MAX_MESSAGE_SIZE];
   int msgLen = sizeof(msg);
	
   StunAtrString username;
   StunAtrString password;
	
   username.sizeValue = 0;
   password.sizeValue = 0;
	
#ifdef USE_TLS
   stunGetUserNameAndPassword( dest, username, password );
#endif
	
   stunSendTest(myFd, dest, username, password, 1, 0/*false*/ );
	
   StunAddress4 from;
	
   getMessage( myFd, msg, &msgLen, &from.addr, &from.port,verbose );
	
   StunMessage resp;
   memset(&resp, 0, sizeof(StunMessage));
	
   bool ok = stunParseMessage( msg, msgLen, resp,verbose );
   if (!ok)
   {
      return -1;
   }
	
   StunAddress4 mappedAddr = resp.mappedAddress.ipv4;
   StunAddress4 changedAddr = resp.changedAddress.ipv4;
	
   //clog << "--- stunOpenSocket --- " << endl;
   //clog << "\treq  id=" << req.id << endl;
   //clog << "\tresp id=" << id << endl;
   //clog << "\tmappedAddr=" << mappedAddr << endl;
	
   *mapAddr = mappedAddr;
	
   return myFd;
}


bool
stunOpenSocketPair( StunAddress4& dest, StunAddress4* mapAddr, 
                    int* fd1, int* fd2, 
                    int port, StunAddress4* srcAddr, 
                    bool verbose )
{
   assert( dest.addr!= 0 );
   assert( dest.port != 0 );
   assert( mapAddr != NULL );
   
   const int NUM=3;
	
   if ( port == 0 )
   {
      port = stunRandomPort();
   }
	
   *fd1=-1;
   *fd2=-1;
	
   char msg[STUN_MAX_MESSAGE_SIZE];
   int msgLen =sizeof(msg);
	
   StunAddress4 from;
   int fd[NUM];
   int i;
	
   unsigned int interfaceIp = 0;
   if ( srcAddr )
   {
      interfaceIp = srcAddr->addr;
   }

   for( i=0; i<NUM; i++)
   {
      fd[i] = openPort( (port == 0) ? 0 : (port + i), 
                        interfaceIp, verbose);
      if (fd[i] < 0) 
      {
         while (i > 0)
         {
            closesocket(fd[--i]);
         }
         return false;
      }
   }
	
   StunAtrString username;
   StunAtrString password;
	
   username.sizeValue = 0;
   password.sizeValue = 0;
	
#ifdef USE_TLS
   stunGetUserNameAndPassword( dest, username, password );
#endif
	
   for( i=0; i<NUM; i++)
   {
      stunSendTest(fd[i], dest, username, password, 1/*testNum*/, verbose );
   }
	
   StunAddress4 mappedAddr[NUM];
   for( i=0; i<NUM; i++)
   {
      msgLen = sizeof(msg)/sizeof(*msg);
      getMessage( fd[i],
                  msg,
                  &msgLen,
                  &from.addr,
                  &from.port ,verbose);
		
      StunMessage resp;
      memset(&resp, 0, sizeof(StunMessage));
		
      bool ok = stunParseMessage( msg, msgLen, resp, verbose );
      if (!ok) 
      {
         return false;
      }
		
      mappedAddr[i] = resp.mappedAddress.ipv4;
      StunAddress4 changedAddr = resp.changedAddress.ipv4;
   }
	
   if (verbose)
   {               
      clog << "--- stunOpenSocketPair --- " << endl;
      for( i=0; i<NUM; i++)
      {
         clog << "\t mappedAddr=" << mappedAddr[i] << endl;
      }
   }
	
   if ( mappedAddr[0].port %2 == 0 )
   {
      if (  mappedAddr[0].port+1 ==  mappedAddr[1].port )
      {
         *mapAddr = mappedAddr[0];
         *fd1 = fd[0];
         *fd2 = fd[1];
         closesocket( fd[2] );
         return true;
      }
   }
   else
   {
      if (( mappedAddr[1].port %2 == 0 )
          && (  mappedAddr[1].port+1 ==  mappedAddr[2].port ))
      {
         *mapAddr = mappedAddr[1];
         *fd1 = fd[1];
         *fd2 = fd[2];
         closesocket( fd[0] );
         return true;
      }
   }

   // something failed, close all and return error
   for( i=0; i<NUM; i++)
   {
      closesocket( fd[i] );
   }
	
   return false;
}