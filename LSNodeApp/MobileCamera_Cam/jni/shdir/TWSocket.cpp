// TWSocket.cpp: implementation of the TWSocket class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "TWSocket.h"


int        TWSocket::m_ObjectNums=0;
BOOL       TWSocket::m_InitSocketSuccess=false;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TWSocket::TWSocket()
{
    if(m_ObjectNums==0)
	{ 
	   m_ObjectNums++;
       m_InitSocketSuccess=true;
	} 
	else
		m_ObjectNums++;

    m_Socket	=	INVALID_SOCKET; 
	FState		=	wsClosed;
}

TWSocket::~TWSocket()
{
   int  Res;
   if(m_Socket!=INVALID_SOCKET)
       CloseSocket();
   //Res=InterlockedDecrement(&m_ObjectNums);
   Res=m_ObjectNums--;
   if(Res<=0)
   {
       //m_InitSocketSuccess=false;
   }
}

int   TWSocket::GetLastError()
{
	return getErrno();
}

int   TWSocket::Create()
{
	SOCKET   s;
	if(!m_InitSocketSuccess)
		return -1;
	s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(s!=INVALID_SOCKET)
	{
		m_Socket=s;
		return 0;
	}
	else
		return -1;
}


int  TWSocket::ShutDown()
{
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
	    return  -1;
	int Res=shutdown(m_Socket,1);
	if(Res)
		return -1;
	else
		return 0;
}

int  TWSocket::Close()
{
	if(!m_InitSocketSuccess)
		return -1;
	return InternalClose(true);
}

int  TWSocket::InternalClose(bool bShut)
{
    int iStatus;
    if (m_Socket == INVALID_SOCKET )
	{
        if (GetState() !=wsClosed )
		{
            SetState(wsClosed);
        }
        return 0;
    }

    if (GetState() == wsClosed) return 0;

    if (bShut)
        ShutDown();

    if (m_Socket != INVALID_SOCKET )
	{
        do
		{
            iStatus = closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
            if ( iStatus != 0 )
			{
                FLastError = getErrno();
                if ( FLastError != EWOULDBLOCK )
				{
                    return -1;
                }
            }
		} while (iStatus) ;
    }
    SetState(wsClosed);
	return 0;
}

int  TWSocket::CloseSocket()
{
	int  Res;
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
	    return  0;
	struct linger  _linger;
	_linger.l_onoff=1;
	_linger.l_linger=0;
    Res=setsockopt(m_Socket,SOL_SOCKET,SO_LINGER,(const char *)&_linger,sizeof(struct linger));
	if(Res)
		return Res;
    Res=closesocket(m_Socket);
	SetState(wsClosed);
    m_Socket=INVALID_SOCKET;
	return Res;
}

int  TWSocket::Attach(SOCKET s)  
{
	if(!m_InitSocketSuccess)
		return -1;
	if(s==INVALID_SOCKET)
		return -1;
	if(m_Socket!=INVALID_SOCKET)
        CloseSocket();
    m_Socket=s;
	return 0;
}

int  TWSocket::Detach()
{
	m_Socket=INVALID_SOCKET;
	return 0;
}

int  TWSocket::Accept(TWSocket* NewClass)
{
	SOCKET     s;
	sockaddr   addr;
	socklen_t  len;

	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;

	len=sizeof(sockaddr);
	s=accept(m_Socket,&addr,&len);
	if(s==INVALID_SOCKET)
		return -1;
	else
	{
		NewClass->Attach(s);
		NewClass->SetState(wsConnected);
		return 0;
	}
}

int  TWSocket::Bind(const char *IpAddr,int Port)
{
	sockaddr_in    addr;
	DWORD          addr_long;
	int            Res;

	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;

	if(IpAddr==NULL)
		addr_long=htonl(INADDR_ANY);
	else
        addr_long=inet_addr(IpAddr); 
    if(addr_long==INADDR_NONE)
		return -1;

	memset(&addr,0,sizeof(sockaddr_in));
	addr.sin_family=AF_INET;
	addr.sin_port=htons((unsigned short)Port);
	addr.sin_addr.s_addr=addr_long;

	Res=bind(m_Socket,(sockaddr *)&addr,sizeof(addr));
	return Res;
}

int   TWSocket::Connect(const char *IpAddr,int Port)
{
	sockaddr_in    addr;
	DWORD          addr_long;
	int            Res;

	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;

    addr_long=inet_addr(IpAddr); 
    if(addr_long==INADDR_NONE)
		return -1;

	memset(&addr,0,sizeof(sockaddr_in));
	addr.sin_family=AF_INET;
	addr.sin_port=htons((unsigned short)Port);
	addr.sin_addr.s_addr=addr_long;

	Res=connect(m_Socket,(sockaddr *)&addr,sizeof(addr));
	if( Res==0 )
		SetState(wsConnected);
	return Res;
}

int    TWSocket::Listen(int  MaxConnectNum)
{
	int   Res;
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;

	Res=listen(m_Socket,MaxConnectNum);
	if( Res==0 )
		SetState(wsListening);
	return Res;
}

int    TWSocket::Send(const char* Buffer,int len)
{
	int   Res;
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;

	Res=send(m_Socket,Buffer,len,0);
	return Res;
}

int    TWSocket::Send_Stream(const char* Buffer,int len)
{
	int   Res;
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;

	int remaining = len;
	int offset = 0;
	while( remaining>0 )
	{
		Res=send(m_Socket,Buffer+offset,remaining,0);
		if( Res<0 )
			return -1;
		if( Res==0 )
			return offset;
		offset += Res;
		remaining -= Res;
	}
	return len;
}

int     TWSocket::WaitSend(const char* Buffer,int len,int DelayTime)
{
	int    write;
	int    Res;
	Res=GetSocketState(NULL,&write,NULL,DelayTime);  
	if(Res<0)               //如果出错
		return -1;
	if(Res>0)               //在规定时间内未接收到服务器响应
		return -2;
	return Send(Buffer,len); 
}

int     TWSocket::Recv(char* Buffer,int len)
{
	int   Res;
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;

	Res=recv(m_Socket,Buffer,len,0);
	return Res;
}

int     TWSocket::Recv_Stream(char* Buffer,int len)
{
	int   Res;
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;

	int remaining = len;
	int offset = 0;
	while( remaining>0 )
	{
		Res=recv(m_Socket,Buffer+offset,remaining,0);
		if( Res<0 )
			return -1;
		if( Res==0 )
			return offset;
		offset += Res;
		remaining -= Res;
	}
	return len;
}

int    TWSocket::WaitRecv(char* Buffer,int len,int DelayTime)
{
	int    read,error;
	int    Res;
	Res=GetSocketState(&read,NULL,&error,DelayTime); 
	if(Res<0)               //如果出错
		return -1;
	if(Res>0)               //在规定时间内未接收到服务器响应
		return -2;
	if(error == 1)
	{
		return -1;
	}
	return Recv(Buffer,len); 
}

int  TWSocket::GetSocketState(int* Readready,int* Writeready,int* Error,int DelayTime)
{
     struct timeval  Selecttimeval;
	 int   second=DelayTime/1000;

	 if(!m_InitSocketSuccess)
		return -1;
	 if(m_Socket==INVALID_SOCKET)
		return -1;

     Selecttimeval.tv_sec=second;
     Selecttimeval.tv_usec=(DelayTime-second*1000)*1000;
	 fd_set  Readfds,Writefds,Exceptfds;
	 fd_set *rs;
     fd_set *ws;
	 fd_set *es;

	 if(Readready)
         rs=&Readfds;
	 else
         rs=NULL;
	 if(Writeready)
         ws=&Writefds;
	 else
         ws=NULL;
	 if(Error)
         es=&Exceptfds;
	 else
         es=NULL;
     
	 if(rs)
        FD_ZERO(rs);
	 if(ws)
	    FD_ZERO(ws);
	 if(es)
	    FD_ZERO(es);

	 if(rs)
	    FD_SET(m_Socket,rs);
	 if(ws)
	    FD_SET(m_Socket,ws);
	 if(es)
	    FD_SET(m_Socket,es);

	 if(Readready)
	    *Readready=0;
	 if(Writeready)
        *Writeready=0;
	 if(Error)
	    *Error=0;
	 int n=select(m_Socket+1,rs,ws,es,&Selecttimeval);
	 if(n==0)
		 return 1;
	 if(n==SOCKET_ERROR)
		 return -1;
	 if(rs)
       if(FD_ISSET(m_Socket,rs))
		   *Readready=1;
	 if(ws)
       if(FD_ISSET(m_Socket,ws))
          *Writeready=1;
	 if(es)
       if(FD_ISSET(m_Socket,es))
	      *Error=1;
	 return 0;
}

int    TWSocket::GetPeerName(char* IpAddrBuffer,int Bufferlen,int* Port)
{
	sockaddr_in    addr;
	int            Res;
	socklen_t      namelen;

	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;
    if(IpAddrBuffer==NULL)
		return -1;

	namelen=sizeof(addr);
    Res=getpeername(m_Socket,(sockaddr *)&addr,&namelen);
    if(Res)   //失败
		return Res;
	char *temp=inet_ntoa(addr.sin_addr);
    namelen=strlen(temp);
    if(Bufferlen<=namelen)
		return -1;
	strcpy(IpAddrBuffer,temp);
	if( Port!=NULL )
		*Port=ntohs(addr.sin_port);
	return Res;
}

int    TWSocket::GetSockName(char* IpAddrBuffer,int Bufferlen,int* Port)
{
	sockaddr_in    addr;
	int            Res;
	socklen_t      namelen;

	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;
    if(IpAddrBuffer==NULL)
		return -1;
    if(Port==NULL)
		return -1;
	namelen=sizeof(addr);
    Res=getsockname(m_Socket,(sockaddr *)&addr,&namelen);
    if(Res)   //失败
		return Res;
	char *temp=inet_ntoa(addr.sin_addr);
    namelen=strlen(temp);
    if(Bufferlen<=namelen)
		return -1;
	strcpy(IpAddrBuffer,temp);
	*Port=ntohs(addr.sin_port);
	return Res;
}

int  TWSocket::SetSockSendBufferSize(int Size)
{
    int             Res;
	
    if(Size<=0)
		return -1;
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;
    Res=setsockopt(m_Socket,SOL_SOCKET,SO_SNDBUF,(const char*)&Size,sizeof(int)); 
	return Res;
}

int  TWSocket::SetSockRecvBufferSize(int Size)
{
    int             Res;
    if(Size<=0)
		return -1;
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;
    Res=setsockopt(m_Socket,SOL_SOCKET,SO_RCVBUF,(const char*)&Size,sizeof(int)); 
	return Res;
}

int   TWSocket::GetSockSendBufferSize(int *Size)
{
    int             Res;
	socklen_t       len;
	
    if(Size<=0)
		return -1;
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;
	if(Size==NULL)
		return -1;
	len=sizeof(int);
    Res=getsockopt(m_Socket,SOL_SOCKET,SO_SNDBUF,(char*)Size,&len); 
	return Res;
}

int   TWSocket::GetSockRecvBufferSize(int *Size)
{
    int             Res;
	socklen_t       len;
	
    if(Size<=0)
		return -1;
	if(!m_InitSocketSuccess)
		return -1;
	if(m_Socket==INVALID_SOCKET)
		return -1;
	if(Size==NULL)
		return -1;
	len=sizeof(int);
    Res=getsockopt(m_Socket,SOL_SOCKET,SO_RCVBUF,(char*)Size,&len); 
	return Res;
}

int   TWSocket::GetHostName(char *Name,int Len)  
{
	return gethostname(Name,Len);
}

void TWSocket::IpHostToStr(DWORD nIpHost, char *Address, int nSize)
{
	in_addr in;
	char *temp;
	
	in.s_addr = htonl(nIpHost);
	temp = inet_ntoa(in);
	strncpy(Address, temp, nSize);
}

BOOL TWSocket::StrIsIP(const char *Address, DWORD *lpIpHost)  
{
	DWORD  temp;
	if(Address==NULL)
		return false;
    temp=inet_addr(Address); 
	if(temp==INADDR_NONE)
		return false;
	else
	{
		if( lpIpHost )
		    *lpIpHost = ntohl(temp);
		return true;
	}
}

int  TWSocket::GetHostByName(char *Name,char *FullName,int FullNameLen,
		                  char *AliasName,int AliasNameLen,char *IpAddr,int IpAddrLen)
{
	hostent   *hs;
	DWORD  temp;

    if(Name==NULL)
		return -1;
	if(FullName==NULL)
		return -1;
    if(AliasName==NULL)
		return -1;
    if(IpAddr==NULL)
		return -1;
    if(FullNameLen<=0)
		return -1;
    if(AliasNameLen<=0)
		return -1;
    if(IpAddrLen<=0)
		return -1;

	temp=inet_addr(Name); 
	if(temp==INADDR_NONE)//Name是计算机名称
        hs=gethostbyname(Name);
	else//Name 是IP地址
		hs=gethostbyaddr((const char*)&temp,4,PF_INET);
	if(hs==NULL)
		return -1;
	int  len;
	len=strlen(hs->h_name);
	if(len>(FullNameLen-1))
		len=FullNameLen-1;
	strncpy(FullName,hs->h_name,len);
    FullName[len]=0;

	if(hs->h_aliases[0]==NULL)
         AliasName[0]=0;
	else
	{
	   len=strlen(hs->h_aliases[0]);
	   if(len>(AliasNameLen-1))
	     	len=AliasNameLen-1;
	   strncpy(AliasName,hs->h_aliases[0],len);
       AliasName[len]=0;
	}

	char *tempchar;
    tempchar=inet_ntoa(*(struct in_addr *)*hs->h_addr_list);
	len=strlen(tempchar);
	if(len>(IpAddrLen-1))
		len=IpAddrLen-1;
	strncpy(IpAddr,tempchar,len);
    IpAddr[len]=0;
	return 0;
}

int    TWSocket::GetIPAddr(char* HostName,char *IPAddr,int IPAddrLen)
{
	char   FullName[256];
	char   AliasName[256];
	int    Res;
	Res=GetHostByName(HostName,FullName,256,AliasName,256,IPAddr,IPAddrLen);
	if(Res)
		return 0;
	return strlen(IPAddr);
}

BOOL   TWSocket::ValidateSocket()
{
    return !(m_Socket==INVALID_SOCKET);
}

void  TWSocket::SetState(TSocketState state)
{
     FState = state;
}

TSocketState  TWSocket::GetState()
{
     return FState;
}
