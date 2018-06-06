// TWSocket.h: interface for the TWSocket class.
//
//////////////////////////////////////////////////////////////////////
#ifndef _TWSOCKET_H
#define _TWSOCKET_H

#include "platform.h"
#include "udp.h"


#define    DefaultRcvSize      4096	//64536
#define    DefaultRcvSizeInc   4096 //64536
#define    DefaultRcvSizeMax   0             //{ Unlimited size }


enum TSocketState
{
	wsConnected,
	wsListening,
	wsClosed
};


class TWSocket  
{
protected:
	static    int       m_ObjectNums;         //对象的实例数,当ObjectNums==0时自动调用WSACleanup()
	                            //对象的实例数,当ObjectNums==1时自动调用WSAStartup()
	static    BOOL      m_InitSocketSuccess;  //是否已成功初始化WinSock.dll

public:
	SOCKET              m_Socket;
    int					FLastError;
public:
	static int       GetLastError();

	int              Create();
	                    //创建套接字的实例
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

	int              ShutDown();
	                    //当须关闭套接字时,先调用ShutDown然后调用Close
	                    //未防止数据丢失,应在调用ShutDown后,循环调用Recv
	                    //函数直到其返回0后调用Close()
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

    int              Close();
	                    //关闭套接字
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

    int              CloseSocket();
	                    //强制关闭套接字,可不调用ShutDown,但可能造成数据丢失
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

    int              InternalClose(bool bShut);

	int              Attach(SOCKET s);  
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

	int              Detach();  
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

	int              Accept(TWSocket* NewClass);
		                //从套接字上接收一个连接
						//NewClass新连接的套接字,NewClass在调用之前必须
						//已调用了构造函数，且不能调用Create()
						//成功返回0 失败返回<0  错误号由GetLastError()返回

	int              Bind(const char *IpAddr=NULL,int Port=0);
		                //将一本地地址与套接字连接
	                    //IpAddr相应的IP地址,如果IpAddr==NULL使用INADDR_ANY
	                    //Port相关连的端口
						//成功返回0 失败返回<0  错误号由GetLastError()返回

	virtual int      Connect(const char *IpAddr,int Port); 
	                    //与指定的Ip地址建立一个连接
	                    //IpAddr相应的IP地址
	                    //Port相关连的端口
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

	int              Listen(int  MaxConnectNum); 
	                    //建立长度为MaxConnectNum的连接请求队列来存放即将到来的连接请求
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

    int              Send(const char* Buffer,int len);
	                    //在已连接的套接字上发送数据
                        //如果成功,返回总共发送的字节数
	                    //Buffer为发送数据的缓冲区
	                    //len为需发送的字节数
 	                    //否则返回<0  错误号由GetLastError()返回

    int              Send_Stream(const char* Buffer,int len);
	                    //在已连接的套接字上发送指定长度数据，
                        //如果成功,返回len
	                    //对方关闭时,返回0
	                    //出错返回<0  错误号由GetLastError()返回

	int              WaitSend(const char* Buffer,int len,int DelayTime=0);
	                    //在已连接的套接字上发送数据
	                    //如果在等待DelayTime毫秒后,不能成功发送数据,则直接返回
	                    //返回值为-2
                        //如果成功,返回总共发送的字节数
	                    //Buffer为发送数据的缓冲区
	                    //len为需发送的字节数
	                    //否则返回<0  错误号由GetLastError()返回

    int              Recv(char* Buffer,int len);
	                    //在已连接的套接字上读取输入数据
                        //如果成功,返回收到的字节数
	                    //Buffer为接收数据的缓冲区
	                    //len为缓冲区的大小
	                    //否则返回<0  错误号由GetLastError()返回

    int              Recv_Stream(char* Buffer,int len);
	                    //在已连接的套接字上读取指定长度数据，
                        //如果成功,返回len
	                    //对方关闭时,返回0
	                    //出错返回<0  错误号由GetLastError()返回 

	int              WaitRecv(char* Buffer,int len,int DelayTime=0);
	                    //在已连接的套接字上读取输入数据
	                    //如果在等待DelayTime毫秒后,没有数据可接收,则直接返回
	                    //返回值为-2
                        //如果成功,返回收到的字节数
	                    //Buffer为接收数据的缓冲区
	                    //len为缓冲区的大小
	                    //否则返回<0  错误号由GetLastError()返回

    int              GetSocketState(int* Readready=NULL,int* Writeready=NULL,int* Error=NULL,int DelayTime=1000);
	                    //取得非阻塞套接字的状态(如:出错   是否准备好读 是否准备好写)
	                    //成功返回>=0
	                    //否则返回<0  错误号由GetLastError()返回
	                    //如果在等待指定的时间DelayTime毫秒没有满足的条件,返回值>0
	                    //如果有满足则立即返回不等待.
	                    //如果读准备好则*Readready=1
	                    //如果写准备好则*Readready=1
	                    //如果套接字发生错误则*Error=1
	                    //当指针为NULL时说明不关心指定的状态,但三者不能同时为NULL

    virtual int      GetPeerName(char* IpAddrBuffer,int Bufferlen,int* Port=NULL);
	                    //取得与套接字连接的对等方的地址
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回
	                    //如果成功,在IpAddrBuffer缓冲中返回对等方IP地址
	                    //Bufferlen为IpAddrBuffer缓冲区长度
	                    //若Port不为NULL, Port中返回对等方IP端口

    int              GetSockName(char* IpAddrBuffer,int Bufferlen,int* Port);
	                    //取得套接字本地地址,必须已调用Bind或Connect后
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回
	                    //如果成功,在IpAddrBuffer缓冲中返回套接字本地IP地址
	                    //Bufferlen为IpAddrBuffer缓冲区长度
	                    //Port中返回套接字本地IP端口

	int              SetSockSendBufferSize(int Size);
	                    //设置套接字发送缓冲区大小,当底层Socket实现不支持设置
	                    //时,函数并不能返回错误,可调用GetSockSendBufferSize()
	                    //来查看设置是否生效
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

	int              SetSockRecvBufferSize(int Size);
	                    //设置套接字接收缓冲区大小,当底层Socket实现不支持设置
	                    //时,函数并不能返回错误,可调用GetSockRecvBufferSize()
	                    //来查看设置是否生效
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

	int               GetSockSendBufferSize(int *Size);
	                    //得到套接字发送缓冲区大小
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

	int               GetSockRecvBufferSize(int *Size);
	                    //得到套接字接收缓冲区大小
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回
	
	static   void     IpHostToStr(DWORD nIpHost, char *Address, int nSize);

	static   int      GetHostName(char *Name,int Len);  
	                    //得到本地主机名称
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

	static   BOOL     StrIsIP(const char *Address, DWORD *lpIpHost=NULL); 
	                    //给定的字符串是否是数字IP
	                    //是返回true, 若lpIpHost不为NULL, 则返回主机字节顺序的IP地址
						//否则返回false

	static   int      GetHostByName(char *Name,char *FullName,int FullNameLen,
		                  char *AliasName,int AliasNameLen,char *IpAddr,int IpAddrLen);
	                    //给定一计算机名称得到其全称,别名和IP地址
	                    //Name可为计算机名或IP地址
	                    //FullName返回计算机全称,FullNameLen为FullName缓冲区长度
	                    //AliasName返回计算机别名,AliasNameLen为AliasName缓冲区长度
	                    //IpAddr返回计算机IP地址,IpAddrLen为IpAddr缓冲区长度
	                    //成功返回0 失败返回<0  错误号由GetLastError()返回

	static   int       GetIPAddr(char* HostName,char *IPAddr,int IPAddrLen);
	                    //由主机名得到IP地址
	                    //成功返回IP地址拷贝的字节数,
	                    //失败返回0

	BOOL               ValidateSocket();
	                    //是否已创建Socket,如果已创建则返回true,否则返回false

public:
	TSocketState	FState;

	void SetState(TSocketState state);
	TSocketState GetState();

public:
	TWSocket();
	virtual ~TWSocket();

};

#endif /* _TWSOCKET_H */
