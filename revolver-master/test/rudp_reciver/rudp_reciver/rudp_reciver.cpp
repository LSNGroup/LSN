#include "core/core_packet.h"
#include "base_reactor_instance.h"
#include "base_bin_stream.h"
#include "base_packet.h"
#include "rudp_interface.h"
#include "udt/udt.h"

#include <iostream>


using namespace BASE;


DWORD WINAPI RecvThreadFn(LPVOID pvThreadParam)
{
	UDTSOCKET u = *(UDTSOCKET *)pvThreadParam;
	uint64_t last_ts = CBaseTimeValue::get_time_value().msec();
	uint64_t bytes = 0;
	char buf[16*1024];
	int ret;

	while (true)
	{
		ret = UDT::recv(u, buf, sizeof(buf), 0);
		if (ret < 0)
		{
			cout << "UDT::recv(" << u << ") failed!" << endl;
			break;
		}
		bytes += ret;

		uint64_t now_ts = CBaseTimeValue::get_time_value().msec();
		if (now_ts - last_ts >= 2000)
		{
			cout << "UDTSOCKET[" << u << "]: Recv... rate = " << bytes/1024/2 << "KB/S" << endl;
			last_ts = now_ts;
			bytes = 0;
		}
	}

	cout << "UDTSOCKET[" << u << "]: Recv thread exit..." << endl;
	return 0;
}


int main(int agc, char* argv[])
{
	cout << "start rudp recv... " << endl;

	UDT::startup();


	int ret;
	bool connected = false;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int addr_len = sizeof(their_addr);
	UDPSOCKET udp_sock = INVALID_SOCKET;
	UDTSOCKET serv = UDT::INVALID_SOCK;
	UDTSOCKET usock = UDT::INVALID_SOCK;
	UDTSOCKET serv_slave = UDT::INVALID_SOCK;
	UDTSOCKET usock_slave = UDT::INVALID_SOCK;
	DWORD dwThreadID;
	HANDLE hThread;
	DWORD dwThreadID2;
	HANDLE hThread2;

	while(true)
	{
		char c = getchar();

		if(c == 'e')
		{
			break;
		}
		else if(c == 'A')
		{
			udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (INVALID_SOCKET == udp_sock) {
				cout << "udp socket() failed!" << endl;
				break;
			}

			my_addr.sin_family = AF_INET;
			my_addr.sin_port = htons(20200);
			my_addr.sin_addr.s_addr = INADDR_ANY;
			memset(&(my_addr.sin_zero), '\0', 8);
			ret = bind(udp_sock, (sockaddr *)&my_addr, sizeof(my_addr));
			if (ret != 0) {
				cout << "udp bind() failed!" << endl;
				break;
			}


			serv = UDT::socket(AF_INET, SOCK_STREAM, 0);
			if (UDT::INVALID_SOCK == serv) {
				cout << "UDT::socket() failed!" << endl;
				break;
			}

			ret = UDT::bind(serv, udp_sock);
			if (ret != 0) {
				cout << "UDT::bind() failed!" << endl;
				break;
			}

			ret = UDT::listen(serv, 1);
			if (ret != 0) {
				cout << "UDT::listen() failed!" << endl;
				break;
			}

			cout << "UDT::accept()..." << endl;
			usock = UDT::accept(serv, (sockaddr *)&their_addr, &addr_len);
			if (usock == UDT::INVALID_SOCK) {
				cout << "UDT::accept() failed!" << endl;
				break;
			}

			UDT::close(serv);
			serv = UDT::INVALID_SOCK;

			connected = true;
			cout << "UDT client connected!!! My rudp_id = " << usock << endl;

			hThread = ::CreateThread(NULL,0,RecvThreadFn,&usock,0,&dwThreadID);
		}
		else if(c == 'a')
		{
			serv_slave = UDT::socket(AF_INET, SOCK_STREAM, 0);
			if (UDT::INVALID_SOCK == serv_slave) {
				cout << "UDT::socket() failed!" << endl;
				break;
			}

			ret = UDT::bind2(serv_slave, udp_sock);
			if (ret != 0) {
				cout << "UDT::bind2() failed!" << endl;
				break;
			}

			ret = UDT::listen(serv_slave, 1);
			if (ret != 0) {
				cout << "UDT::listen() failed!" << endl;
				break;
			}

			cout << "UDT::accept()..." << endl;
			usock_slave = UDT::accept(serv_slave, (sockaddr *)&their_addr, &addr_len);
			if (usock_slave == UDT::INVALID_SOCK) {
				cout << "UDT::accept() failed!" << endl;
				break;
			}

			UDT::close(serv_slave);
			serv_slave = UDT::INVALID_SOCK;

			cout << "UDT client connected!!! My rudp_id = " << usock_slave << endl;

			hThread2 = ::CreateThread(NULL,0,RecvThreadFn,&usock_slave,0,&dwThreadID2);
		}
		else if(c == 'd')
		{
			if (UDT::INVALID_SOCK != usock_slave) {
				cout << "UDT::close(" << usock_slave << ")..." << endl;
				UDT::close(usock_slave);
				usock_slave = UDT::INVALID_SOCK;
				::WaitForSingleObject(hThread2,  INFINITE);
				::CloseHandle(hThread2);
			}
		}
		else if(c == 'D')
		{
			if (connected && UDT::INVALID_SOCK == usock_slave) {
				cout << "UDT::close(" << usock << ")..." << endl;
				UDT::close(usock);
				usock = UDT::INVALID_SOCK;
				::WaitForSingleObject(hThread,  INFINITE);
				::CloseHandle(hThread);
				connected = false;
			}
			else {
				cout << "Please close usock_slave first!" << endl;
			}
		}
		else if (c == 's')
		{
			char send_buf[512+DIRECT_UDP_HEAD_LEN];
			char str[512];
			cin.getline(str,510);
			cin.getline(str,510);
			memset(send_buf, DIRECT_UDP_HEAD_VAL, DIRECT_UDP_HEAD_LEN);
			memcpy(send_buf + DIRECT_UDP_HEAD_LEN, str, strlen(str) + 1);
			if (sendto(udp_sock, send_buf, strlen(str) + 1 + DIRECT_UDP_HEAD_LEN, 0, (sockaddr *)&their_addr, addr_len) <= 0)
			{
				cout << "sendto() failed..." << endl;
			}
		}
	}

	if (UDT::INVALID_SOCK != serv_slave) {
		UDT::close(serv_slave);
	}
	if (UDT::INVALID_SOCK != usock_slave) {
		UDT::close(usock_slave);
	}
	if (UDT::INVALID_SOCK != serv) {
		UDT::close(serv);
	}
	if (UDT::INVALID_SOCK != usock) {
		UDT::close(usock);
	}
	if (INVALID_SOCKET != udp_sock) {
		closesocket(udp_sock);
	}
	UDT::cleanup();

	return 0;
}
