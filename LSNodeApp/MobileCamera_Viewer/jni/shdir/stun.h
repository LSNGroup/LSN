#ifndef STUN_H
#define STUN_H

#include <iostream>
#include <time.h>

// if you change this version, change in makefile too 
#define STUN_VERSION "0.96"

#define STUN_MAX_STRING 256
#define STUN_MAX_UNKNOWN_ATTRIBUTES 8
#define STUN_MAX_MESSAGE_SIZE 2048

#define STUN_PORT 3478

// define some basic types
typedef unsigned char  UInt8;
typedef unsigned short UInt16;
typedef unsigned int   UInt32bit;
typedef unsigned long long  UInt64;
typedef struct { unsigned char octet[16]; }  UInt128;

/// define a structure to hold a stun address 
const UInt8  IPv4Family = 0x01;
const UInt8  IPv6Family = 0x02;

// define  flags  
const UInt32bit ChangeIpFlag   = 0x04;
const UInt32bit ChangePortFlag = 0x02;

// define  stun attribute
const UInt16 MappedAddress    = 0x0001;
const UInt16 ResponseAddress  = 0x0002;
const UInt16 ChangeRequest    = 0x0003;
const UInt16 SourceAddress    = 0x0004;
const UInt16 ChangedAddress   = 0x0005;
const UInt16 Username         = 0x0006;
const UInt16 Password         = 0x0007;
const UInt16 MessageIntegrity = 0x0008;
const UInt16 ErrorCode        = 0x0009;
const UInt16 UnknownAttribute = 0x000A;
const UInt16 ReflectedFrom    = 0x000B;
const UInt16 XorMappedAddress = 0x8020;
const UInt16 XorOnly          = 0x0021;
const UInt16 ServerName       = 0x8022;
const UInt16 SecondaryAddress = 0x8050; // Non standard extention

// define types for a stun message 
const UInt16 BindRequestMsg               = 0x0001;
const UInt16 BindResponseMsg              = 0x0101;
const UInt16 BindErrorResponseMsg         = 0x0111;
const UInt16 SharedSecretRequestMsg       = 0x0002;
const UInt16 SharedSecretResponseMsg      = 0x0102;
const UInt16 SharedSecretErrorResponseMsg = 0x0112;

typedef struct 
{
      UInt16 msgType;
      UInt16 msgLength;
      UInt128 id;
} StunMsgHdr;


typedef struct
{
      UInt16 type;
      UInt16 length;
} StunAtrHdr;

typedef struct
{
      UInt16 port;
      UInt32bit addr;
} StunAddress4;

typedef struct
{
      UInt8 pad;
      UInt8 family;
      StunAddress4 ipv4;
} StunAtrAddress4;

typedef struct
{
      UInt32bit value;
} StunAtrChangeRequest;

typedef struct
{
      UInt16 pad; // all 0
      UInt8 errorClass;
      UInt8 number;
      char reason[STUN_MAX_STRING];
      UInt16 sizeReason;
} StunAtrError;

typedef struct
{
      UInt16 attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
      UInt16 numAttributes;
} StunAtrUnknown;

typedef struct
{
      char value[STUN_MAX_STRING];      
      UInt16 sizeValue;
} StunAtrString;

typedef struct
{
      char hash[20];
} StunAtrIntegrity;

typedef enum 
{
   HmacUnkown=0,
   HmacOK,
   HmacBadUserName,
   HmacUnkownUserName,
   HmacFailed,
} StunHmacStatus;

typedef struct
{
      StunMsgHdr msgHdr;
	
      bool hasMappedAddress;
      StunAtrAddress4  mappedAddress;
	
      bool hasResponseAddress;
      StunAtrAddress4  responseAddress;
	
      bool hasChangeRequest;
      StunAtrChangeRequest changeRequest;
	
      bool hasSourceAddress;
      StunAtrAddress4 sourceAddress;
	
      bool hasChangedAddress;
      StunAtrAddress4 changedAddress;
	
      bool hasUsername;
      StunAtrString username;
	
      bool hasPassword;
      StunAtrString password;
	
      bool hasMessageIntegrity;
      StunAtrIntegrity messageIntegrity;
	
      bool hasErrorCode;
      StunAtrError errorCode;
	
      bool hasUnknownAttributes;
      StunAtrUnknown unknownAttributes;
	
      bool hasReflectedFrom;
      StunAtrAddress4 reflectedFrom;

      bool hasXorMappedAddress;
      StunAtrAddress4  xorMappedAddress;
	
      bool xorOnly;

      bool hasServerName;
      StunAtrString serverName;
      
      bool hasSecondaryAddress;
      StunAtrAddress4 secondaryAddress;
} StunMessage; 


// Define enum with different types of NAT 
typedef enum 
{
   StunTypeUnknown=0,
   StunTypeFailure,
   StunTypeOpen,
   StunTypeBlocked,

   StunTypeIndependentFilter,
   StunTypeDependentFilter,
   StunTypePortDependedFilter,
   StunTypeDependentMapping,

   //StunTypeConeNat,
   //StunTypeRestrictedNat,
   //StunTypePortRestrictedNat,
   //StunTypeSymNat,
   
   StunTypeFirewall,
} NatType;

#ifdef WINCE
typedef SOCKET Socket;
#else
typedef int Socket;
#endif

#define MAX_MEDIA_RELAYS 500
#define MAX_RTP_MSG_SIZE 1500
#define MEDIA_RELAY_TIMEOUT 3*60

typedef struct 
{
      int relayPort;       // media relay port
      int fd;              // media relay file descriptor
      StunAddress4 destination; // NAT IP:port
      time_t expireTime;      // if no activity after time, close the socket 
} StunMediaRelay;

typedef struct
{
      StunAddress4 myAddr;
      StunAddress4 altAddr;
      Socket myFd;
      Socket altPortFd;
      Socket altIpFd;
      Socket altIpPortFd;
      bool relay; // true if media relaying is to be done
      StunMediaRelay relays[MAX_MEDIA_RELAYS];
} StunServerInfo;

bool
stunParseMessage( char* buf, 
                  unsigned int bufLen, 
                  StunMessage& message, 
                  bool verbose );

void
stunBuildReqSimple( StunMessage* msg,
                    const StunAtrString& username,
                    bool changePort, bool changeIp, unsigned int id=0 );

unsigned int
stunEncodeMessage( const StunMessage& message, 
                   char* buf, 
                   unsigned int bufLen, 
                   const StunAtrString& password,
                   bool verbose);

void
stunCreateUserName(const StunAddress4& addr, StunAtrString* username);

void 
stunGetUserNameAndPassword(  const StunAddress4& dest, 
                             StunAtrString* username,
                             StunAtrString* password);

void
stunCreatePassword(const StunAtrString& username, StunAtrString* password);

int 
stunRand();

UInt64
stunGetSystemTimeSecs();

/// find the IP address of a the specified stun server - return false is fails parse 
bool  
stunParseServerName( char* serverName, StunAddress4& stunServerAddr);

bool 
stunParseHostName( char* peerName,
                   UInt32bit& ip,
                   UInt16& portVal,
                   UInt16 defaultPort );


/// returns number of address found - take array or addres 
int 
stunFindLocalInterfaces(UInt32bit* addresses, int maxSize );

void 
stunTest( StunAddress4& dest, int testNum, bool verbose, StunAddress4* sAddr );

NatType
stunNatType( StunAddress4& dest, bool verbose, 
             bool* preservePort=0, // if set, is return for if NAT preservers ports or not
             bool* hairpin=0 ,  // if set, is the return for if NAT will hairpin packets
             int port=0, // port to use for the test, 0 to choose random port
             StunAddress4* sAddr=0 // NIC to use 
   );

/// prints a StunAddress
std::ostream& 
operator<<( std::ostream& strm, const StunAddress4& addr);

std::ostream& 
operator<< ( std::ostream& strm, const UInt128& );

int
stunOpenSocket( StunAddress4& dest, 
                StunAddress4* mappedAddr, 
                int port=0, 
                StunAddress4* srcAddr=0, 
                bool verbose=false );

bool
stunOpenSocketPair( StunAddress4& dest, StunAddress4* mappedAddr, 
                    int* fd1, int* fd2, 
                    int srcPort=0,  StunAddress4* srcAddr=0,
                    bool verbose=false);

int
stunRandomPort();

#endif

