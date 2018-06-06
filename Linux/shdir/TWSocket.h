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
	static    int       m_ObjectNums;         //�����ʵ����,��ObjectNums==0ʱ�Զ�����WSACleanup()
	                            //�����ʵ����,��ObjectNums==1ʱ�Զ�����WSAStartup()
	static    BOOL      m_InitSocketSuccess;  //�Ƿ��ѳɹ���ʼ��WinSock.dll

public:
	SOCKET              m_Socket;
    int					FLastError;
public:
	static int       GetLastError();

	int              Create();
	                    //�����׽��ֵ�ʵ��
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	int              ShutDown();
	                    //����ر��׽���ʱ,�ȵ���ShutDownȻ�����Close
	                    //δ��ֹ���ݶ�ʧ,Ӧ�ڵ���ShutDown��,ѭ������Recv
	                    //����ֱ���䷵��0�����Close()
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

    int              Close();
	                    //�ر��׽���
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

    int              CloseSocket();
	                    //ǿ�ƹر��׽���,�ɲ�����ShutDown,������������ݶ�ʧ
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

    int              InternalClose(bool bShut);

	int              Attach(SOCKET s);  
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	int              Detach();  
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	int              Accept(TWSocket* NewClass);
		                //���׽����Ͻ���һ������
						//NewClass�����ӵ��׽���,NewClass�ڵ���֮ǰ����
						//�ѵ����˹��캯�����Ҳ��ܵ���Create()
						//�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	int              Bind(const char *IpAddr=NULL,int Port=0);
		                //��һ���ص�ַ���׽�������
	                    //IpAddr��Ӧ��IP��ַ,���IpAddr==NULLʹ��INADDR_ANY
	                    //Port������Ķ˿�
						//�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	virtual int      Connect(const char *IpAddr,int Port); 
	                    //��ָ����Ip��ַ����һ������
	                    //IpAddr��Ӧ��IP��ַ
	                    //Port������Ķ˿�
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	int              Listen(int  MaxConnectNum); 
	                    //��������ΪMaxConnectNum�����������������ż�����������������
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

    int              Send(const char* Buffer,int len);
	                    //�������ӵ��׽����Ϸ�������
                        //����ɹ�,�����ܹ����͵��ֽ���
	                    //BufferΪ�������ݵĻ�����
	                    //lenΪ�跢�͵��ֽ���
 	                    //���򷵻�<0  �������GetLastError()����

    int              Send_Stream(const char* Buffer,int len);
	                    //�������ӵ��׽����Ϸ���ָ���������ݣ�
                        //����ɹ�,����len
	                    //�Է��ر�ʱ,����0
	                    //������<0  �������GetLastError()����

	int              WaitSend(const char* Buffer,int len,int DelayTime=0);
	                    //�������ӵ��׽����Ϸ�������
	                    //����ڵȴ�DelayTime�����,���ܳɹ���������,��ֱ�ӷ���
	                    //����ֵΪ-2
                        //����ɹ�,�����ܹ����͵��ֽ���
	                    //BufferΪ�������ݵĻ�����
	                    //lenΪ�跢�͵��ֽ���
	                    //���򷵻�<0  �������GetLastError()����

    int              Recv(char* Buffer,int len);
	                    //�������ӵ��׽����϶�ȡ��������
                        //����ɹ�,�����յ����ֽ���
	                    //BufferΪ�������ݵĻ�����
	                    //lenΪ�������Ĵ�С
	                    //���򷵻�<0  �������GetLastError()����

    int              Recv_Stream(char* Buffer,int len);
	                    //�������ӵ��׽����϶�ȡָ���������ݣ�
                        //����ɹ�,����len
	                    //�Է��ر�ʱ,����0
	                    //������<0  �������GetLastError()���� 

	int              WaitRecv(char* Buffer,int len,int DelayTime=0);
	                    //�������ӵ��׽����϶�ȡ��������
	                    //����ڵȴ�DelayTime�����,û�����ݿɽ���,��ֱ�ӷ���
	                    //����ֵΪ-2
                        //����ɹ�,�����յ����ֽ���
	                    //BufferΪ�������ݵĻ�����
	                    //lenΪ�������Ĵ�С
	                    //���򷵻�<0  �������GetLastError()����

    int              GetSocketState(int* Readready=NULL,int* Writeready=NULL,int* Error=NULL,int DelayTime=1000);
	                    //ȡ�÷������׽��ֵ�״̬(��:����   �Ƿ�׼���ö� �Ƿ�׼����д)
	                    //�ɹ�����>=0
	                    //���򷵻�<0  �������GetLastError()����
	                    //����ڵȴ�ָ����ʱ��DelayTime����û�����������,����ֵ>0
	                    //������������������ز��ȴ�.
	                    //�����׼������*Readready=1
	                    //���д׼������*Readready=1
	                    //����׽��ַ���������*Error=1
	                    //��ָ��ΪNULLʱ˵��������ָ����״̬,�����߲���ͬʱΪNULL

    virtual int      GetPeerName(char* IpAddrBuffer,int Bufferlen,int* Port=NULL);
	                    //ȡ�����׽������ӵĶԵȷ��ĵ�ַ
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����
	                    //����ɹ�,��IpAddrBuffer�����з��ضԵȷ�IP��ַ
	                    //BufferlenΪIpAddrBuffer����������
	                    //��Port��ΪNULL, Port�з��ضԵȷ�IP�˿�

    int              GetSockName(char* IpAddrBuffer,int Bufferlen,int* Port);
	                    //ȡ���׽��ֱ��ص�ַ,�����ѵ���Bind��Connect��
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����
	                    //����ɹ�,��IpAddrBuffer�����з����׽��ֱ���IP��ַ
	                    //BufferlenΪIpAddrBuffer����������
	                    //Port�з����׽��ֱ���IP�˿�

	int              SetSockSendBufferSize(int Size);
	                    //�����׽��ַ��ͻ�������С,���ײ�Socketʵ�ֲ�֧������
	                    //ʱ,���������ܷ��ش���,�ɵ���GetSockSendBufferSize()
	                    //���鿴�����Ƿ���Ч
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	int              SetSockRecvBufferSize(int Size);
	                    //�����׽��ֽ��ջ�������С,���ײ�Socketʵ�ֲ�֧������
	                    //ʱ,���������ܷ��ش���,�ɵ���GetSockRecvBufferSize()
	                    //���鿴�����Ƿ���Ч
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	int               GetSockSendBufferSize(int *Size);
	                    //�õ��׽��ַ��ͻ�������С
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	int               GetSockRecvBufferSize(int *Size);
	                    //�õ��׽��ֽ��ջ�������С
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����
	
	static   void     IpHostToStr(DWORD nIpHost, char *Address, int nSize);

	static   int      GetHostName(char *Name,int Len);  
	                    //�õ�������������
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	static   BOOL     StrIsIP(const char *Address, DWORD *lpIpHost=NULL); 
	                    //�������ַ����Ƿ�������IP
	                    //�Ƿ���true, ��lpIpHost��ΪNULL, �򷵻������ֽ�˳���IP��ַ
						//���򷵻�false

	static   int      GetHostByName(char *Name,char *FullName,int FullNameLen,
		                  char *AliasName,int AliasNameLen,char *IpAddr,int IpAddrLen);
	                    //����һ��������Ƶõ���ȫ��,������IP��ַ
	                    //Name��Ϊ���������IP��ַ
	                    //FullName���ؼ����ȫ��,FullNameLenΪFullName����������
	                    //AliasName���ؼ��������,AliasNameLenΪAliasName����������
	                    //IpAddr���ؼ����IP��ַ,IpAddrLenΪIpAddr����������
	                    //�ɹ�����0 ʧ�ܷ���<0  �������GetLastError()����

	static   int       GetIPAddr(char* HostName,char *IPAddr,int IPAddrLen);
	                    //���������õ�IP��ַ
	                    //�ɹ�����IP��ַ�������ֽ���,
	                    //ʧ�ܷ���0

	BOOL               ValidateSocket();
	                    //�Ƿ��Ѵ���Socket,����Ѵ����򷵻�true,���򷵻�false

public:
	TSocketState	FState;

	void SetState(TSocketState state);
	TSocketState GetState();

public:
	TWSocket();
	virtual ~TWSocket();

};

#endif /* _TWSOCKET_H */
