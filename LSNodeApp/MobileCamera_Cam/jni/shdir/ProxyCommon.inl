//网络两端的收发缓冲区大小必须一致！！！
//所以Win32、Linux最好共用该文件
#define PROXY_TEMP_BUFFER_SIZE	(16*1024)


//Return Values:
// -1:error
//  0:
//  n:
static int RecvTcpTimeout(SOCKET sock, char *buff, int size, int miliSecs)
{
   int n = miliSecs/10;
   int count = 0;
   int len;
   fd_set target_fds;
   fd_set except_fds;
   struct timeval waitval;
   
   if (n < 1) {
   	   n = 1;
   }
	while (n > 0 && count < size)
	{
		FD_ZERO(&target_fds);
		FD_SET(sock, &target_fds);
		FD_ZERO(&except_fds);
		FD_SET(sock, &except_fds);
		waitval.tv_sec  = 0;
		waitval.tv_usec = 10000;
		if (select(sock+1, &target_fds, NULL, &except_fds, &waitval) < 0 || FD_ISSET(sock, &except_fds)) {
			//TRACE("Debug: sock select error[%d]!\n", WSAGetLastError());
			if (count > 0) return count;
			else           return -1;
		}
		if (!FD_ISSET(sock, &target_fds)) {
			n -= 1;
			continue;
		}
		len = recv(sock, buff + count, size - count, 0);
		if (len > 0)
		{
			count += len;
		}
		else 
		{
#ifdef WIN32
			if (WSAGetLastError() != WSAETIMEDOUT)
#else
			if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
#endif
			{
				//TRACE("Debug: sock Connection shutdown[%d]!\n", WSAGetLastError());
				if (count > 0) return count;
				else           return -1;
			}
			else {
				n -= 1;//TRACE("Debug: sock Recv timeout!\n");
			}
		}
	}
	return count;
}

// Loop1: sock --> fhandle
static int DataLoop1(SOCKET sock, SOCKET_TYPE ftype, SOCKET fhandle, BOOL *pbSingleQuit)
{
   char data[6 + PROXY_TEMP_BUFFER_SIZE];
   int len;

	while (!bQuit && !(*pbSingleQuit)) {

		len = RecvTcpTimeout(sock, data + 6, sizeof(data) - 6, 200);
		if (len > 0) {
			//encrypt_data((unsigned char *)data + 6, len);
			/*
			pf_set_word(data, htons(CMD_CODE_PROXY_DATA));
			pf_set_dword(data + 2, htonl(len));
			if (SendStream(ftype, fhandle, data, 6 + len) < 0)
			{
				//TRACE("Debug: udt_fhandle Send error[%s]!\n", UDT::getlasterror().getErrorMessage());
				*pbSingleQuit = TRUE;
				break;
			}
			*/
			if (CtrlCmd_PROXY_DATA(ftype, fhandle, (BYTE *)data + 6, len) < 0)
			{
				//TRACE("Debug: udt_fhandle Send error[%s]!\n", UDT::getlasterror().getErrorMessage());
				*pbSingleQuit = TRUE;
				break;
			}
		}
		else if (len < 0) {
			//TRACE("Debug: sock select error[%d]!\n", WSAGetLastError());
			*pbSingleQuit = TRUE;
			break;
		}
	}

	return 0;
}

// Loop2: tcp fhandle --> sock
static int DataLoop2Tcp(SOCKET fhandle, SOCKET sock, BOOL *pbSingleQuit)
{
   char data[PROXY_TEMP_BUFFER_SIZE];
   int ret;
   WORD wCommand;
   DWORD dwDataLength;
   fd_set target_fds;
   fd_set except_fds;
   struct timeval waitval;

	while (!bQuit && !(*pbSingleQuit)) {

		FD_ZERO(&target_fds);
		FD_SET(fhandle, &target_fds);
		FD_ZERO(&except_fds);
		FD_SET(fhandle, &except_fds);
		waitval.tv_sec  = 1;
		waitval.tv_usec = 0;
		if (select(fhandle+1, &target_fds, NULL, &except_fds, &waitval) < 0 || FD_ISSET(fhandle, &except_fds)) {
			//TRACE("Debug: fhandle select error[%d]!\n", WSAGetLastError());
			*pbSingleQuit = TRUE;
			break;
		}
		else if (!FD_ISSET(fhandle, &target_fds)) {
			continue;
		}

		ret = RecvStreamTcp(fhandle, data, 6);
		if (ret != 0) {
			*pbSingleQuit = TRUE;
			break;
		}
		wCommand = ntohs(pf_get_word(data));
		dwDataLength = ntohl(pf_get_dword(data+2));
		
		//中途可能收到保活包
		if (CMD_CODE_NULL == wCommand && dwDataLength == 0) {
			continue;
		}
		
		if (CMD_CODE_PROXY_DATA != wCommand || dwDataLength == 0) {
			*pbSingleQuit = TRUE;
			break;
		}
		
		if (dwDataLength > sizeof(data)) {
			//Data too long...
			*pbSingleQuit = TRUE;
			break;
		}
		
		ret = RecvStreamTcp(fhandle, data, dwDataLength);
		if (ret == 0) {
			//decrypt_data((unsigned char *)data, dwDataLength);
			if (SendStreamTcp(sock, data, dwDataLength) < 0)
			{
				//TRACE("Debug: sock Send error[%d]!\n", WSAGetLastError());
				*pbSingleQuit = TRUE;
				break;
			}
		}
		else 
		{
			*pbSingleQuit = TRUE;
			break;
		}
	}

	return 0;
}

// Loop2: udt fhandle --> sock
static int DataLoop2Udt(SOCKET fhandle, SOCKET sock, BOOL *pbSingleQuit)
{
   char data[PROXY_TEMP_BUFFER_SIZE];
   int ret;
   WORD wCommand;
   DWORD dwDataLength;
   struct timeval waitval;

	while (!bQuit && !(*pbSingleQuit)) {
		
		waitval.tv_sec  = 1;
		waitval.tv_usec = 0;
		ret = UDT::select_read(fhandle+1, fhandle, NULL, NULL, &waitval);
		if (ret < 0) {
			*pbSingleQuit = TRUE;
			break;
		}
		else if (ret == 0) {
			continue;
		}

		ret = RecvStreamUdt(fhandle, data, 6);
		if (ret != 0) {
			*pbSingleQuit = TRUE;
			break;
		}
		wCommand = ntohs(pf_get_word(data));
		dwDataLength = ntohl(pf_get_dword(data+2));
		
		//中途可能收到保活包
		if (CMD_CODE_NULL == wCommand && dwDataLength == 0) {
			continue;
		}
		
		if (CMD_CODE_PROXY_DATA != wCommand || dwDataLength == 0) {
			*pbSingleQuit = TRUE;
			break;
		}
		
		if (dwDataLength > sizeof(data)) {
			//Data too long...
			*pbSingleQuit = TRUE;
			break;
		}
		
		ret = RecvStreamUdt(fhandle, data, dwDataLength);
		if (ret == 0) {
			//decrypt_data((unsigned char *)data, dwDataLength);
			if (SendStreamTcp(sock, data, dwDataLength) < 0)
			{
				//TRACE("Debug: sock Send error[%d]!\n", WSAGetLastError());
				*pbSingleQuit = TRUE;
				break;
			}
		}
		else {
			//TRACE("Debug: udt_fhandle Connection shutdown[%s]!\n", UDT::getlasterror().getErrorMessage());
			*pbSingleQuit = TRUE;
			break;
		}
	}

	return 0;
}

static int DataLoop2(SOCKET_TYPE ftype, SOCKET fhandle, SOCKET sock, BOOL *pbSingleQuit)
{
	if (SOCKET_TYPE_TCP == ftype)
	{
		return DataLoop2Tcp(fhandle, sock, pbSingleQuit);
	}
	else if (SOCKET_TYPE_UDT == ftype)
	{
		return DataLoop2Udt(fhandle, sock, pbSingleQuit);
	}
	return 0;
}
