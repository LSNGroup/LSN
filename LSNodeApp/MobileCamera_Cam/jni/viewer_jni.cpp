#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <jni.h>
#include "jni_func.h"
#include "platform.h"
#include "CommonLib.h"
#include "HttpOperate.h"
#include "Discovery.h"
#include "ControlCmd.h"
#include "phpMd5.h"
#include "UPnP.h"
#include "g729a.h"
#include "avrtp_def.h"
#include "R_string_def.h"
#include "viewer_if.h"
#include "viewer_node.h"

#include <android/log.h>


static MyUPnP  myUPnP;

static char g_client_charset[16];
static char g_client_lang[16];

static BOOL g_bFirstCheckStun = TRUE;

VIEWER_NODE viewerArray[MAX_VIEWER_NUM];
int currentSourceIndex = -1;
BOOL is_app_recv_av = FALSE;
DWORD joined_channel_id = -1;

static DWORD currentLastMediaTime = 0;
static DWORD timeoutMedia = 0;

static char ipc_socket_name[] = "multi_rudp.ipc_socket";
static int ipc_listen_sock = INVALID_SOCKET;
static int ipc_server_sock = INVALID_SOCKET;
static int ipc_client_sock = INVALID_SOCKET;
static pthread_t hIpcServerThread;
void *IpcServerThreadFn(void *pvThreadParam);

static pthread_t hCheckMediaThread;
void *CheckMediaThreadFn(void *pvThreadParam);

static pthread_t hRegisterThread;
void *RegisterThreadFn(void *pvThreadParam);

void *ConnectThreadFn(void *pvThreadParam);
void *ConnectThreadFn2(void *pvThreadParam);
void *ConnectThreadFnRev(void *pvThreadParam);

void *NewConnectThreadFn(void *pvThreadParam);

static void InitVar();
static void HandleRegister(VIEWER_NODE *pViewerNode);


static DWORD get_system_milliseconds()//milliseconds
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	unsigned long ret = tv.tv_sec*1000 + tv.tv_usec/1000;
	ret &= 0x7fffffff;
	return ret;
}

BYTE CheckNALUType(BYTE bNALUHdr)
{
	return ((bNALUHdr & 0x1f) >> 0);
}

BYTE CheckNALUNri(BYTE bNALUHdr)
{
	return ((bNALUHdr & 0x60) >> 5);
}

UDTSOCKET get_use_udt_sock()
{
	return ipc_client_sock;
}

SOCKET_TYPE get_use_sock_type()
{
	return SOCKET_TYPE_TCP;
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(StartNative)
	(JNIEnv* env, jobject thiz, jstring str_client_charset, jstring str_client_lang)
{
	int len;
	
	len = (env)->GetStringLength(str_client_charset);
	assert(len < sizeof(g_client_charset));
	(env)->GetStringUTFRegion(str_client_charset, 0, len, g_client_charset);
	g_client_charset[len] = '\0';
	
	len = (env)->GetStringLength(str_client_lang);
	assert(len < sizeof(g_client_lang));
	(env)->GetStringUTFRegion(str_client_lang, 0, len, g_client_lang);
	g_client_lang[len] = '\0';
	
	g_bFirstCheckStun = TRUE;
	
	CtrlCmd_Init();
	
	// use this function to initialize the UDT library
	UdtStartup();
	
	InitVar();
	
	bool hasUPnP = myUPnP.Search();
	
	for (int i = 0; i < MAX_VIEWER_NUM; i++ )
	{
		viewerArray[i].bUsing = FALSE;
		viewerArray[i].nID = -1;
		viewerArray[i].m_bNeedRegister = TRUE;
		pthread_mutex_init(&(viewerArray[i].m_secBind), NULL);
		
		viewerArray[i].bConnecting = FALSE;
		viewerArray[i].bConnected = FALSE;
		viewerArray[i].httpOP.m0_is_admin = TRUE;
		viewerArray[i].httpOP.m0_is_busy = FALSE;
		viewerArray[i].httpOP.m0_p2p_port = FIRST_CONNECT_PORT - (i+1)*4;
		
		//每次有不一样的node_id
		usleep(200*1000);
		generate_nodeid(viewerArray[i].httpOP.m0_node_id, sizeof(viewerArray[i].httpOP.m0_node_id));
		
		viewerArray[i].bQuitRecvSocketLoop = TRUE;
		
		
		BOOL bMappingExists = FALSE;
		viewerArray[i].mapping.description = "";
		viewerArray[i].mapping.protocol = UNAT_UDP;
		viewerArray[i].mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * (WORD)rand() / (65536)) & 0xffffff00);
		while (hasUPnP && UNAT_OK == myUPnP.GetNATSpecificEntry(&(viewerArray[i].mapping), &bMappingExists) && bMappingExists)
		{
			__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "Find NATPortMapping(%s), retry...\n", viewerArray[i].mapping.description.c_str());
			bMappingExists = FALSE;
			viewerArray[i].mapping.description = "";
			viewerArray[i].mapping.protocol = UNAT_UDP;
			viewerArray[i].mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * (WORD)rand() / (65536)) & 0xffffff00);
		}//找到一个未被占用的外部端口映射，或者路由器UPnP功能不可用
	
	}
	
	pthread_create(&hRegisterThread, NULL, RegisterThreadFn, NULL);
	
	pthread_create(&hCheckMediaThread, NULL, CheckMediaThreadFn, NULL);
	
	pthread_create(&hIpcServerThread, NULL, IpcServerThreadFn, NULL);
	
	return 0;
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(StopNative)
	(JNIEnv* env, jobject thiz)
{
	closesocket(ipc_client_sock);
	ipc_client_sock = INVALID_SOCKET;
	
	closesocket(ipc_server_sock);
	ipc_server_sock = INVALID_SOCKET;
	
	closesocket(ipc_listen_sock);
	ipc_listen_sock = INVALID_SOCKET;
	
	// use this function to release the UDT library
	//UdtCleanup();
	
	CtrlCmd_Uninit();
	
	for (int i = 0; i < MAX_VIEWER_NUM; i++ )
	{
		pthread_mutex_destroy(&(viewerArray[i].m_secBind));
		
		myUPnP.RemoveNATPortMapping(viewerArray[i].mapping);
	}
	
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(getAppVersion)
	(JNIEnv* env, jobject thiz)
{
	return MYSELF_VERSION;
}


extern "C"
jstring
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(phpMd5)
	(JNIEnv* env, jobject thiz, jstring strSrc)
{
	int len;
	char *src;
	char dst[256];
	
	len = (env)->GetStringUTFLength(strSrc);
	src = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strSrc, 0, (env)->GetStringLength(strSrc), src);
	src[len] = '\0';
	
	php_md5(src, dst, sizeof(dst));
	free(src);
    return (env)->NewStringUTF(dst);
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdHELLO)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle, jstring strPass, jintArray arrResults)
{
	jint ret;
	char *pass;
	int len;
	jint results[3];
	DWORD dwServerVersion;
	BYTE bFuncFlags;
	BYTE topo_level;
	WORD result_code;
	BYTE bZeroID[6] = {0, 0, 0, 0, 0, 0};
	
	len = (env)->GetStringUTFLength(strPass);
	pass = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strPass, 0, (env)->GetStringLength(strPass), pass);
	pass[len] = '\0';
	
	ret = CtrlCmd_HELLO((SOCKET_TYPE)type, (SOCKET)fhandle, bZeroID, g0_version, 0, pass, bZeroID, &dwServerVersion, &bFuncFlags, &topo_level, &result_code);
	results[0] = (jint)result_code;
	results[1] = (jint)dwServerVersion;
	results[2] = (jint)bFuncFlags;
	(env)->SetIntArrayRegion(arrResults, 0, 3, results);
	
	free(pass);	
    return ret;
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdRUN)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle, jstring strCmd)
{
	jint ret;
	char *cmd;
	int len;
	char result[8*1024];
	
	len = (env)->GetStringUTFLength(strCmd);
	cmd = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strCmd, 0, (env)->GetStringLength(strCmd), cmd);
	cmd[len] = '\0';
	
	ret = CtrlCmd_RUN((SOCKET_TYPE)type, (SOCKET)fhandle, cmd, result, sizeof(result));
	
	free(cmd);
    return ret;
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdPROXY)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle, jint wTcpPort)
{
	return CtrlCmd_PROXY((SOCKET_TYPE)type, (SOCKET)fhandle, (WORD)wTcpPort);
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdARM)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle)
{
    return CtrlCmd_ARM((SOCKET_TYPE)type, (SOCKET)fhandle);
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdDISARM)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle)
{
    return CtrlCmd_DISARM((SOCKET_TYPE)type, (SOCKET)fhandle);
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdAVSTART)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle, jbyte flags, jbyte video_size, jbyte video_framerate, jint audio_channel, jint video_channel)
{
	return CtrlCmd_AV_START((SOCKET_TYPE)type, (SOCKET)fhandle, flags, video_size, video_framerate, audio_channel, video_channel);
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdAVSTOP)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle)
{
    return CtrlCmd_AV_STOP((SOCKET_TYPE)type, (SOCKET)fhandle);
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdAVSWITCH)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle, jint video_channel)
{
	return CtrlCmd_AV_SWITCH((SOCKET_TYPE)type, (SOCKET)fhandle, (DWORD)video_channel);
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdAVCONTRL)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle, jint contrl, jint contrl_param)
{
	return CtrlCmd_AV_CONTRL((SOCKET_TYPE)type, (SOCKET)fhandle, (WORD)contrl, (DWORD)contrl_param);
}

/*
extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdVOICE)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle, jbyteArray data, jint len)
{
	int ret;
	
	jbyte *pData = (env)->GetByteArrayElements(data, NULL);
	if (pData == NULL) {
		return -1;
	}
	
	ret = CtrlCmd_VOICE((SOCKET_TYPE)type, (SOCKET)fhandle, (BYTE *)pData, len);
	
	(env)->ReleaseByteArrayElements(data, pData, 0);
	return ret;
}
*/


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdTEXT)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle, jstring strText)
{
	jint ret;
	char *text;
	int len;
	
	
	len = (env)->GetStringUTFLength(strText);
	text = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strText, 0, (env)->GetStringLength(strText), text);
	text[len] = '\0';
	
	ret = CtrlCmd_TEXT((SOCKET_TYPE)type, (SOCKET)fhandle, (BYTE *)text, len+1);
	
	free(text);
    return ret;
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdBYE)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle)
{
    return CtrlCmd_BYE((SOCKET_TYPE)type, (SOCKET)fhandle);
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdSendNULL)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle)
{
    return CtrlCmd_Send_NULL((SOCKET_TYPE)type, (SOCKET)fhandle);
}


///////////////////////////////////////////////////////////////////////////////////////////////
//
//  0: OK
// -1: NG
int SendVoice(SOCKET_TYPE type, SOCKET fhandle, const BYTE *pData, int len) //Format:单声道,16bit,8000Hz
{
	int ret;
	
	if (pData == NULL || len <= 0) {
		return -1;
	}
	
	int nFrame = len / (L_G729A_FRAME * sizeof(short));
	int len2 = nFrame * L_G729A_FRAME_COMPRESSED;
	if (nFrame == 0) {
		return 0;
	}
	
	g729a_InitEncoder();
	
	unsigned char *pOutput = (unsigned char *)malloc(len2);
	g729a_Encoder((short *)pData, pOutput, nFrame * L_G729A_FRAME);
	
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "SendVoice(%d bytes g729a)...", len2);
	ret = CtrlCmd_VOICE(type, fhandle, (BYTE *)pOutput, len2);
	
	free(pOutput);
	return ret;
}

extern "C"
void MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(SendVoice)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle, jbyteArray data, jint len)
{
	jbyte *pData = (env)->GetByteArrayElements(data, NULL);
	if (pData == NULL) {
		return;
	}
	
	SendVoice((SOCKET_TYPE)type, (SOCKET)fhandle, (const BYTE *)pData, len);
	
	(env)->ReleaseByteArrayElements(data, pData, 0);
}


///////////////////////////////////////////////////////////////////////////////////////////////

static int IsNodeExist(CHANNEL_NODE *nodesArray, int nCount, char *node_id_str)
{
	int i;

	for (i = 0; i < nCount; i++) {
		if (strcmp(nodesArray[i].node_id_str, node_id_str) == 0) {
			return i;
		}
	}

	return -1;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(DoSearchChannels)
	(JNIEnv* env, jobject thiz, jint page_offset, jint page_rows)
{
	jint ret;
	CHANNEL_NODE *servers;//[NODES_PER_PAGE];
	int cntServer = NODES_PER_PAGE;
	int nNum;
	int i;
	
	
	servers  = (CHANNEL_NODE *)malloc( sizeof(CHANNEL_NODE)*NODES_PER_PAGE );
	
	//if (len > 0)
	{
		ret = HttpOperate::DoQueryChannels(g_client_charset, g_client_lang, page_offset, page_rows, servers, &cntServer, &nNum);
		if (ret < 0)
		{
			if_messagetip(R_string::msg_internet_error);
		}
		else if (ret == 0 && cntServer > 0)
		{
			;
		}
	}
	
	if (cntServer > 0)
	{
		//__android_log_print(ANDROID_LOG_INFO, "DoSearchChannels", "jobject = %ld", (unsigned long)thiz);////Debug
		
		jclass cls = (env)->GetObjectClass(thiz);
		//__android_log_print(ANDROID_LOG_INFO, "DoSearchChannels", "jclass1 = %ld", (unsigned long)cls);////Debug
		
		if (0 == cls) {
			cls = (env)->FindClass("com/wangling/mobilecamera/MainListActivity");
			//__android_log_print(ANDROID_LOG_INFO, "DoSearchChannels", "jclass2 = %ld", (unsigned long)cls);////Debug
		}
		
		jmethodID mid = (env)->GetMethodID(cls, "FillChannelNode", "(IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
		//__android_log_print(ANDROID_LOG_INFO, "DoSearchChannels", "jmethodID = %ld", (unsigned long)mid);////Debug
		
		for (i = 0; i < (int)cntServer; i++)
		{
			CHANNEL_NODE *pNode = &servers[i];
			
			jstring channel_comments = (env)->NewStringUTF(pNode->channel_comments);
			jstring device_uuid = (env)->NewStringUTF(pNode->device_uuid);
			jstring node_id_str = (env)->NewStringUTF(pNode->node_id_str);
			jstring pub_ip_str = (env)->NewStringUTF(pNode->pub_ip_str);
			jstring location = (env)->NewStringUTF(pNode->location);
			
			(env)->CallVoidMethod(thiz, mid, 
									i, pNode->channel_id, channel_comments, 
									device_uuid, node_id_str, 
									pub_ip_str, location );
		
			__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "DoSearchChannels: FillChannelNode(index = %d)!", i);////Debug
			
			//这里如果不手动释放局部引用，很有可能造成局部引用表溢出(max:512)
			(env)->DeleteLocalRef(channel_comments);
			(env)->DeleteLocalRef(device_uuid);
			(env)->DeleteLocalRef(node_id_str);
			(env)->DeleteLocalRef(pub_ip_str);
			(env)->DeleteLocalRef(location);
		}
	}

	free(servers);
    return cntServer;
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(DoConnect)
	(JNIEnv* env, jobject thiz, jstring strPassword, jint channel_id)
{
	int ret;
	int len;
	char *password_str;
	
	currentSourceIndex = -1;
	is_app_recv_av = FALSE;
	currentLastMediaTime = 0;
	
	//////////////////////////////////////////////////////////////////////////////
	int data_sock;
	struct sockaddr_un addr;
	
	data_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = '\0';
    strncpy(addr.sun_path + 1, ipc_socket_name, sizeof(addr.sun_path) - 1);
    len = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(ipc_socket_name);
    
    if (connect(data_sock, (struct sockaddr *)&addr, len) < 0) {
    	__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "DoConnect: connect(%s) failed, wait 50 ms...\n", ipc_socket_name);
    	usleep(50000);
	    if (connect(data_sock, (struct sockaddr *)&addr, len) < 0) {
			__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "DoConnect: connect(%s) failed!\n", ipc_socket_name);
			close(data_sock);
			return;
		}
	}
	
	ipc_client_sock = data_sock;
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "DoConnect: Ipc connect(%s) ok! ipc_client_sock=%ld\n", ipc_socket_name, ipc_client_sock);
	//////////////////////////////////////////////////////////////////////////////
	
	
	len = (env)->GetStringLength(strPassword);
	password_str = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strPassword, 0, len, password_str);
	password_str[len] = '\0';
	//...
	free(password_str);
	
	
	joined_channel_id = channel_id;
	
	//建立快速连接。。。from_star
	{
		pthread_t hThread;
		pthread_create(&hThread, NULL, NewConnectThreadFn, (void *)1);
	}
	
	//尝试建立另一个连接...
	{
		pthread_t hThread;
		pthread_create(&hThread, NULL, NewConnectThreadFn, (void *)0);
	}
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(DoDisconnect)
	(JNIEnv* env, jobject thiz)
{
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "DoDisconnect()...\n");
	
	//if (sock_type == SOCKET_TYPE_TCP)
	{
		closesocket(ipc_client_sock);
		ipc_client_sock = INVALID_SOCKET;
	}
	
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (FALSE == viewerArray[i].bUsing) {
			continue;
		}
		viewerArray[i].bQuitRecvSocketLoop = TRUE;
		usleep(2000000);
		UdtClose(viewerArray[i].httpOP.m1_use_udt_sock);
		UdpClose(viewerArray[i].httpOP.m1_use_udp_sock);
	}
	
	currentSourceIndex = -1;
	is_app_recv_av = FALSE;
	currentLastMediaTime = 0;
	joined_channel_id = -1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////

static void learn_remote_addr(void *ctx, sockaddr* his_addr, int addr_len)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)ctx;
	sockaddr_in *sin = (sockaddr_in *)his_addr;
	pViewerNode->httpOP.m1_use_peer_port = ntohs(sin->sin_port);
	pViewerNode->httpOP.m1_use_peer_ip = sin->sin_addr.s_addr;
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "learn_remote_addr() called, %s[%d]\n", inet_ntoa(sin->sin_addr), pViewerNode->httpOP.m1_use_peer_port);
}

static void InitVar()
{
	strncpy(g0_device_uuid, "Viewer Device UUID", sizeof(g0_device_uuid));
	strncpy(g0_node_name, "Viewer Node Name", sizeof(g0_node_name));
	strncpy(g0_os_info, "Viewer OS Info", sizeof(g0_os_info));
	
	g0_version = MYSELF_VERSION;
	
	strncpy(g1_http_server, DEFAULT_HTTP_SERVER, sizeof(g1_http_server));
	strncpy(g1_stun_server, DEFAULT_STUN_SERVER, sizeof(g1_stun_server));
	
	g1_register_period = DEFAULT_REGISTER_PERIOD;  /* Seconds */
	g1_expire_period = DEFAULT_EXPIRE_PERIOD;  /* Seconds */
}

static void HandleRegister(VIEWER_NODE *pViewerNode)
{
	int ret;
	StunAddress4 mapped;
	static BOOL bNoNAT;
	static int  nNatType;
	DWORD ipArrayTemp[MAX_ADAPTER_NUM];  /* Network byte order */
	int ipCountTemp;
	
	
	ret = pViewerNode->httpOP.DoRegister1(g_client_charset, g_client_lang);
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "HandleRegister: DoRegister1()=%d\n", ret);
	
	timeoutMedia = g1_stream_timeout_l3 + g1_stream_timeout_step + g1_stream_timeout_step;
	
	if (ret == -1) {
		pViewerNode->m_bNeedRegister = TRUE;
		return;
	}
	else if (ret == 0) {
		/* Exit this applicantion. */
		//::PostMessage(pDlgWnd, WM_REGISTER_EXIT, 0, 0);
		pViewerNode->m_bNeedRegister = TRUE;
		return;
	}
	else if (ret == 1) {
		pViewerNode->m_bNeedRegister = TRUE;
		return;
	}


	//::EnterCriticalSection(&(m_localbind_csec));
	pthread_mutex_lock(&(pViewerNode->m_secBind));

	/* STUN Information */
	if (strcmp(g1_stun_server, "NONE") == 0)
	{
		ret = CheckStunMyself(g1_http_server, pViewerNode->httpOP.m0_p2p_port, &mapped, &bNoNAT, &nNatType);
		if (ret == -1) {
			__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "HandleRegister: CheckStunMyself() failed!\n");
			
		   mapped.addr = ntohl(pViewerNode->httpOP.m1_http_client_ip);
		   mapped.port = pViewerNode->httpOP.m0_p2p_port;
		   
		   Socket s = openPort( 0/*use ephemeral*/, mapped.addr, false);
		   if (s != INVALID_SOCKET)
		   {
		      closesocket(s);
		      bNoNAT = TRUE;
		      nNatType = StunTypeOpen;
		   }
		   else
		   {
		      bNoNAT = FALSE;
		      nNatType = StunTypeIndependentFilter;
		   }
		   
		   if_messagetip(R_string::msg_nat_detect_success);
		   
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "HandleRegister: CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port,  pViewerNode->httpOP.m0_no_nat ? 1 : 0);
		}
		else {
			if_messagetip(R_string::msg_nat_detect_success);
			
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "HandleRegister: CheckStunMyself: %d.%d.%d.%d[%d]\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port);
		}
	}
	else if (g_bFirstCheckStun) {
		ret = CheckStun(g1_stun_server, pViewerNode->httpOP.m0_p2p_port, &mapped, &bNoNAT, &nNatType);
		if (ret == -1) {
			__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "HandleRegister: CheckStun() failed!\n");
			
		   mapped.addr = ntohl(pViewerNode->httpOP.m1_http_client_ip);
		   mapped.port = pViewerNode->httpOP.m0_p2p_port;
		   
		   Socket s = openPort( 0/*use ephemeral*/, mapped.addr, false);
		   if (s != INVALID_SOCKET)
		   {
		      closesocket(s);
		      bNoNAT = TRUE;
		      nNatType = StunTypeOpen;
		   }
		   else
		   {
		      bNoNAT = FALSE;
		      nNatType = StunTypeIndependentFilter;
		   }
		   
		   if_messagetip(R_string::msg_nat_detect_success);
		   
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "HandleRegister: CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port,  pViewerNode->httpOP.m0_no_nat ? 1 : 0);
		}
		else if (ret == 0) {
			//SetStatusInfo(pDlgWnd, _T("本端NAT防火墙无法穿透!!!您的网络环境可能封锁了UDP通信。"));
			if_messagetip(R_string::msg_nat_blocked);
			pViewerNode->httpOP.m0_pub_ip = 0;
			pViewerNode->httpOP.m0_pub_port = 0;
			pViewerNode->httpOP.m0_no_nat = FALSE;
			pViewerNode->httpOP.m0_nat_type = nNatType;
		}
		else {
			//SetStatusInfo(pDlgWnd, _T("本端NAT探测成功。"));
			if_messagetip(R_string::msg_nat_detect_success);
			g_bFirstCheckStun = FALSE;
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
#if 1 ////Debug
			__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "HandleRegister: CheckStun: %s %d.%d.%d.%d[%d]", 
				pViewerNode->httpOP.m0_no_nat ? "NoNAT" : "NAT",
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port);
#endif
		}
	}
	else {
		ret = CheckStunSimple(g1_stun_server, pViewerNode->httpOP.m0_p2p_port, &mapped);
		if (ret == -1) {
			__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "HandleRegister: CheckStunSimple() failed!\n");
			mapped.addr = ntohl(pViewerNode->httpOP.m1_http_client_ip);
			mapped.port = pViewerNode->httpOP.m0_p2p_port;;
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
		}
		else {
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
#if 1 ////Debug
			__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "HandleRegister: CheckStunSimple: %s %d.%d.%d.%d[%d]", 
				pViewerNode->httpOP.m0_no_nat ? "NoNAT" : "NAT",
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port);
#endif
		}
	}


	/* 本地路由器UPnP处理*/
	if (FALSE == bNoNAT)
	{
		char extIp[16];
		strcpy(extIp, "");
		myUPnP.GetNATExternalIp(extIp);
		if (strlen(extIp) > 0 && 0 != pViewerNode->httpOP.m0_pub_ip && inet_addr(extIp) == pViewerNode->httpOP.m0_pub_ip)
		{
			pViewerNode->mapping.description = "UP2P";
			//pViewerNode->mapping.protocol = ;
			//pViewerNode->mapping.externalPort = ;
			pViewerNode->mapping.internalPort = pViewerNode->httpOP.m0_p2p_port - 2;//SECOND_CONNECT_PORT
			if (UNAT_OK == myUPnP.AddNATPortMapping(&(pViewerNode->mapping), false))
			{
				g_bFirstCheckStun = FALSE;

				pViewerNode->httpOP.m0_pub_ip = inet_addr(extIp);
				pViewerNode->httpOP.m0_pub_port = pViewerNode->mapping.externalPort;
				pViewerNode->httpOP.m0_no_nat = TRUE;
				pViewerNode->httpOP.m0_nat_type = StunTypeOpen;

				__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "HandleRegister: UPnP AddPortMapping OK: %s[%d] --> %s[%d]", extIp, pViewerNode->mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), pViewerNode->mapping.internalPort);
			}
			else
			{
				BOOL bTempExists = FALSE;
				myUPnP.GetNATSpecificEntry(&(pViewerNode->mapping), &bTempExists);
				if (bTempExists)
				{
					g_bFirstCheckStun = FALSE;

					pViewerNode->httpOP.m0_pub_ip = inet_addr(extIp);
					pViewerNode->httpOP.m0_pub_port = pViewerNode->mapping.externalPort;
					pViewerNode->httpOP.m0_no_nat = TRUE;
					pViewerNode->httpOP.m0_nat_type = StunTypeOpen;

					__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "HandleRegister: UPnP PortMapping Exists: %s[%d] --> %s[%d]", extIp, pViewerNode->mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), pViewerNode->mapping.internalPort);
				}
				else {
					pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
					pViewerNode->httpOP.m0_pub_port = mapped.port;
					pViewerNode->httpOP.m0_no_nat = bNoNAT;
					pViewerNode->httpOP.m0_nat_type = nNatType;
				}
			}
		}
		else {
			__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "HandleRegister: UPnP PortMapping Skipped: %s --> %s", extIp, myUPnP.GetLocalIPStr().c_str());
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
		}
	}
	
	//修正公网IP受控端的连接端口问题,2014-6-15
	if (bNoNAT) {
		pViewerNode->httpOP.m0_pub_port = pViewerNode->httpOP.m0_p2p_port - 2;//SECOND_CONNECT_PORT
	}
	
	
	/* Local IP Address */
	ipCountTemp = MAX_ADAPTER_NUM;
	if (GetLocalAddress(ipArrayTemp, &ipCountTemp) != NO_ERROR) {
		//TRACE("GetLocalAddress: fialed!\n");
	}
	else {
		pViewerNode->httpOP.m0_ipCount = ipCountTemp;
		memcpy(pViewerNode->httpOP.m0_ipArray, ipArrayTemp, sizeof(DWORD)*ipCountTemp);
		pViewerNode->httpOP.m0_port = pViewerNode->httpOP.m0_p2p_port;//FIRST_CONNECT_PORT;
#if 1 ////Debug
		__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "HandleRegister: GetLocalAddress: (count=%d) %d.%d.%d.%d[%d]", 
			pViewerNode->httpOP.m0_ipCount,
			(pViewerNode->httpOP.m0_ipArray[0] & 0x000000ff) >> 0,
			(pViewerNode->httpOP.m0_ipArray[0] & 0x0000ff00) >> 8,
			(pViewerNode->httpOP.m0_ipArray[0] & 0x00ff0000) >> 16,
			(pViewerNode->httpOP.m0_ipArray[0] & 0xff000000) >> 24,
			pViewerNode->httpOP.m0_port);
#endif
	}
	
	if (pViewerNode->httpOP.m0_no_nat) {
		pViewerNode->httpOP.m0_port = SECOND_CONNECT_PORT;
	}

	pViewerNode->m_bNeedRegister = FALSE;

_OUT:
	//::LeaveCriticalSection(&(m_localbind_csec));
	pthread_mutex_unlock(&(pViewerNode->m_secBind));
	return;
}

void *RegisterThreadFn(void *pvThreadParam)
{
	for (int i = 0; i < MAX_VIEWER_NUM; i++ )
	{
		HandleRegister(&(viewerArray[i]));
	}
	return 0;
}

static void SwitchMediaSource(int oldIndex, int newIndex)
{
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "SwitchMediaSource(%d => %d)...\n", oldIndex, newIndex);

	if (oldIndex < 0 || oldIndex >= MAX_VIEWER_NUM)	{
		return;
	}
	if (newIndex < 0 || newIndex >= MAX_VIEWER_NUM)	{
		return;
	}
	if (oldIndex == newIndex)	{
		return;
	}

	//切换进行中。。。避免下次Check又重复切换操作
	currentLastMediaTime = 0;

	//viewerArray[oldIndex].bTopoPrimary = FALSE;
	//viewerArray[newIndex].bTopoPrimary = TRUE;
	currentSourceIndex = newIndex;

	//if (viewerArray[newIndex].bTopoPrimary)
	{
		//必须视频可靠传输，音频非冗余传输！！！
		BYTE bFlags = AV_FLAGS_VIDEO_ENABLE | AV_FLAGS_AUDIO_ENABLE | AV_FLAGS_VIDEO_RELIABLE | AV_FLAGS_VIDEO_H264 | AV_FLAGS_AUDIO_G729A;
		BYTE bVideoSize = AV_VIDEO_SIZE_VGA;
		BYTE bVideoFrameRate = 25;
		DWORD audioChannel = 0;
		DWORD videoChannel = 0;
		CtrlCmd_AV_START(viewerArray[newIndex].httpOP.m1_use_sock_type, viewerArray[newIndex].httpOP.m1_use_udt_sock, bFlags, bVideoSize, bVideoFrameRate, audioChannel, videoChannel);
	}

	//if (viewerArray[oldIndex].bTopoPrimary == FALSE)
	{
		CtrlCmd_AV_STOP(viewerArray[oldIndex].httpOP.m1_use_sock_type, viewerArray[oldIndex].httpOP.m1_use_udt_sock);
	}
}

void *CheckMediaThreadFn(void *pvThreadParam)
{
	while (1)
	{
		usleep(50000);
		
		if (timeoutMedia > 0 && currentLastMediaTime > 0 && currentSourceIndex != -1)
		{
			DWORD nowTime = get_system_milliseconds();
			if (nowTime - currentLastMediaTime > timeoutMedia) {
				__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "CheckMediaThreadFn: Media stream timeout...");
				
				//尝试找到另一个已连接的ViewNode
				int i;
				for (i = 0; i < MAX_VIEWER_NUM; i++)
				{
					if (viewerArray[i].bUsing == FALSE || viewerArray[i].bConnected == FALSE) {
						continue;
					}
					if (currentSourceIndex != i) {
						break;
					}
				}
				if (i < MAX_VIEWER_NUM)//找到一个！切换。。。
				{
					int oldIndex = currentSourceIndex;
					SwitchMediaSource(currentSourceIndex, i);
					DisconnectNode(&(viewerArray[oldIndex]));
				}

			}
		}
	}
	return 0;
}

void *NewConnectThreadFn(void *pvThreadParam)
{
	BOOL from_star = (pvThreadParam != 0);
	
	int i;
	//{{{{--------------------------------------->
	for (i = 0; i < MAX_VIEWER_NUM; i++ ) {
		if (viewerArray[i].bUsing == FALSE) {
			break;
		}
	}
	
	if (i == MAX_VIEWER_NUM) { // no free node
		//MessageBox("对不起，当前并发连接数已达到上限!", (LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		;
	}
	else
	{
		viewerArray[i].bUsing = TRUE;
		viewerArray[i].nID = i;
		//<---------------------------------------}}}}
		
		
		//{{{{--------------------------------------->
		viewerArray[i].hConnectThread = NULL;
		viewerArray[i].hConnectThread2 = NULL;
		viewerArray[i].hConnectThreadRev = NULL;
		viewerArray[i].bQuitRecvSocketLoop = TRUE;
		viewerArray[i].bConnecting = TRUE;
		viewerArray[i].bConnected = FALSE;
		viewerArray[i].from_star = from_star;

		strncpy(viewerArray[i].password, "123456", sizeof(viewerArray[i].password));
		//<---------------------------------------}}}}
		
		//mac_addr(node_id_str, g1_peer_node_id, NULL);
		
		HandleRegister(&(viewerArray[i]));
		
		if (ipc_client_sock != INVALID_SOCKET)
		{
			int ret = viewerArray[i].httpOP.DoPull(g_client_charset, g_client_lang, joined_channel_id, viewerArray[i].from_star);
			if (ret == 1 && ipc_client_sock != INVALID_SOCKET)
			{
				if (TRUE == viewerArray[i].httpOP.m1_peer_nonat) {
					pthread_create(&(viewerArray[i].hConnectThread2), NULL, ConnectThreadFn2, &(viewerArray[i]));
				}
				else if (TRUE == viewerArray[i].httpOP.m0_no_nat) {
					pthread_create(&(viewerArray[i].hConnectThreadRev), NULL, ConnectThreadFnRev, &(viewerArray[i]));
				}
				else {
					pthread_create(&(viewerArray[i].hConnectThread), NULL, ConnectThreadFn, &(viewerArray[i]));
				}
			}//if (ret == 1)
		}
	}//i < MAX_VIEWER_NUM
	
	return 0;
}

//
// Return values:
// -1: Network error
//  0: NG
//  1: OK
//
static int CheckPassword(VIEWER_NODE *pViewerNode, SOCKET_TYPE sock_type, SOCKET fhandle)
{
	BYTE bTopoPrimary;
	DWORD dwServerVersion;
	BYTE bFuncFlags;
	BYTE bTopoLevel;
	WORD wResult;
	char szPassEnc[MAX_PATH];


	//检查看是否第一个rudp连接，第一个rudp连接应该是TopoPrimary
	BOOL hasConnection = FALSE;
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (viewerArray[i].bUsing == FALSE) {
			continue;
		}
		if (viewerArray[i].bConnected) {
			hasConnection = TRUE;
			break;
		}
	}
	if (hasConnection) {
		bTopoPrimary = 0;
	}
	else {
		bTopoPrimary = 1;
		currentSourceIndex = pViewerNode->nID;
	}

	php_md5(pViewerNode->password, szPassEnc, sizeof(szPassEnc));

	if (CtrlCmd_HELLO(sock_type, fhandle, pViewerNode->httpOP.m0_node_id, g0_version, bTopoPrimary, szPassEnc, pViewerNode->httpOP.m1_peer_node_id, &dwServerVersion, &bFuncFlags, &bTopoLevel, &wResult) == 0) {
		if (wResult == CTRLCMD_RESULT_OK) {
			//set_encdec_key((unsigned char *)szPassEnc, strlen(szPassEnc));
			//dwServerVersion
			//bFuncFlags;
			//bTopoLevel;
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return -1;
	}
}

static void OnFakeRtpRecv(void *ctx, int payload_type, unsigned long rtptimestamp, unsigned char *data, int len)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)ctx;
	if (NULL == pViewerNode) {
		return;
	}
	
	//__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "OnFakeRtpRecv: FakeRtpPacket[%d], len=%ld\n", payload_type, len);
	
	if (is_app_recv_av && currentSourceIndex == pViewerNode->nID)
	{
		int ret = CtrlCmd_Send_FAKERTP_RESP(SOCKET_TYPE_TCP, ipc_server_sock, data, len);
		if (ret < 0) {
			__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "OnFakeRtpRecv: CtrlCmd_Send_FAKERTP_RESP() failed!\n");
		}
	}
}

static void RecvSocketDataLoop(VIEWER_NODE *pViewerNode, SOCKET_TYPE ftype, SOCKET fhandle)
{
	BOOL bRecvEnd = FALSE;

	if (fhandle == INVALID_SOCKET) {
		return;
	}

	while (FALSE == pViewerNode->bQuitRecvSocketLoop)
	{
		char buf[32];
		WORD wCmd;
		DWORD copy_len;
		BYTE topo_level;
		BYTE dest_node_id[6];
		int index;
		unsigned char *pRecvData;
		int ret;


		if (RecvStream(ftype, fhandle, buf, 6) < 0)
		{
			break;
		}

		wCmd = ntohs(pf_get_word(buf + 0));
		copy_len = ntohl(pf_get_dword(buf + 2));
		//assert(wCmd == CMD_CODE_FAKERTP_RESP && copy_len > 0);

		switch (wCmd)
		{
		case CMD_CODE_FAKERTP_RESP:

			pRecvData = (unsigned char *)malloc(copy_len);
			if (NULL == pRecvData) {
#if LOG_MSG
				log_msg("RecvSocketDataLoop: No memory!\n", LOG_LEVEL_DEBUG);
#endif
				return;
			}


			ret = RecvStream(ftype, fhandle, (char *)pRecvData, copy_len);
			if (ret == 0) {
				BYTE bType = CheckNALUType(pRecvData[8]);
				if (bType != 30) {//VideoSendSetMediaType
					currentLastMediaTime = get_system_milliseconds();
				}
				OnFakeRtpRecv(pViewerNode, pRecvData[0], ntohl(pf_get_dword(pRecvData + 4)), pRecvData, copy_len);
				free(pRecvData);
			}
			else {
				free(pRecvData);
				return;
			}

			break;

		case CMD_CODE_END:
			bRecvEnd = TRUE;
#if LOG_MSG
			log_msg("RecvSocketDataLoop: CMD_CODE_END?????????\n", LOG_LEVEL_DEBUG);
#endif
			break;

		default:
#if LOG_MSG
			log_msg("RecvSocketDataLoop: CMD_CODE_???????...\n", LOG_LEVEL_DEBUG);
#endif
			for (int i = 0; i < copy_len; i++) {
				ret = RecvStream(ftype, fhandle, buf, 1);
				if (ret != 0) {
					return;
				}
			}
			break;
		}

	}//while


	if (bRecvEnd == FALSE) {
#if LOG_MSG
		log_msg("RecvSocketDataLoop: CtrlCmd_Recv_AV_END()...\n", LOG_LEVEL_DEBUG);
#endif
		CtrlCmd_Recv_AV_END(ftype, fhandle);
	}
}

void DoInConnection(VIEWER_NODE *pViewerNode, BOOL bProxy = FALSE);
void DoInConnection(VIEWER_NODE *pViewerNode, BOOL bProxy)
{
	/* 为了节省连接时间，在 eloop 中执行 DoUnregister  */
	//eloop_register_timeout(1, 0, HandleDoUnregister, pViewerNode, NULL);
	
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "DoInConnection: ### connected...");
	pViewerNode->bConnected = TRUE;
	
	//连接是为了视频监控。。。
	if (currentSourceIndex == pViewerNode->nID)
	{
		//必须视频可靠传输，音频非冗余传输！！！
		BYTE bFlags = AV_FLAGS_VIDEO_ENABLE | AV_FLAGS_AUDIO_ENABLE | AV_FLAGS_VIDEO_RELIABLE | AV_FLAGS_VIDEO_H264 | AV_FLAGS_AUDIO_G729A;
		BYTE bVideoSize = AV_VIDEO_SIZE_VGA;
		BYTE bVideoFrameRate = 25;
		DWORD audioChannel = 0;
		DWORD videoChannel = 0;
		CtrlCmd_AV_START(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, bFlags, bVideoSize, bVideoFrameRate, audioChannel, videoChannel);
	}
	
	//Loop...,需要CShiyong::DisconnectNode()才能退出Loop！！！
	pViewerNode->bQuitRecvSocketLoop = FALSE;
	RecvSocketDataLoop(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);

	{
		//CtrlCmd_AV_STOP()将在CShiyong::DisconnectNode()中调用
		//CtrlCmd_AV_STOP(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
	}
	
	pViewerNode->bConnected = FALSE;
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "DoInConnection: ### disconnected!!!");
	
	
	//异常断开rudp连接时，处理Primary Viewer问题。。。
	if (currentSourceIndex == pViewerNode->nID)
	{
		__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "DoInConnection: Need to handle [Primary Viewer]...");

		//尝试找到另一个已连接的ViewNode
		int i;
		for (i = 0; i < MAX_VIEWER_NUM; i++)
		{
			if (viewerArray[i].bUsing == FALSE || viewerArray[i].bConnected == FALSE) {
				continue;
			}
			if (currentSourceIndex != i) {
				break;
			}
		}
		if (i < MAX_VIEWER_NUM)//找到一个！切换。。。
		{
			SwitchMediaSource(currentSourceIndex, i);
		}
		else {
			currentSourceIndex = -1;
		}
	}
	
	
	//检查剩余连接数，看是否需要新建连接。。。
	int nConnections = 0;
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (viewerArray[i].bUsing == FALSE) {
			continue;
		}
		if (viewerArray[i].bConnected) {
			nConnections += 1;
		}
	}
	if (nConnections <= 1 && ipc_client_sock != INVALID_SOCKET)
	{
		__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "DoInConnection: Need new connection... nConnections=%d", nConnections);
		
		pthread_t hThread;
		pthread_create(&hThread, NULL, NewConnectThreadFn, (void *)0);
	}
}

void *ConnectThreadFn(void *pvThreadParam)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)pvThreadParam;
	DWORD use_ip;
	WORD use_port;
	int ret;
	int i, nRetry;


	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "ConnectThreadFn()...\n");

	if_progress_show(R_string::msg_please_wait);

	if (	StunTypeUnknown == pViewerNode->httpOP.m1_peer_nattype ||
			StunTypeFailure == pViewerNode->httpOP.m1_peer_nattype ||
			StunTypeBlocked == pViewerNode->httpOP.m1_peer_nattype         )
	{
			if_progress_cancel();
			//MessageBox(NULL, _T("无法建立连接，网络状况异常，或者您的ID级别不够。"), (LPCTSTR)strProductName, MB_OK|MB_ICONINFORMATION);
			if_messagebox(R_string::msg_connect_proxy_failed);
			ReturnViewerNode(pViewerNode);
			return 0;
	}


	for (i = pViewerNode->httpOP.m1_wait_time; i > 0; i--) {
		//wsprintf(szStatusStr, _T("正在排队等待服务响应，%d秒。。。"), i);
		//SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		if_progress_show_format1(R_string::msg_connect_wait_queue_format, i);
		usleep(1000*1000);
	}

	//SetStatusInfo(pDlg->m_hWnd, _T("正在打通连接信道。。。"), 20);
	if_progress_show(R_string::msg_connect_nat_hole);

	pthread_mutex_lock(&(pViewerNode->m_secBind));

	pViewerNode->httpOP.m1_use_udp_sock = UdpOpen(INADDR_ANY, pViewerNode->httpOP.m0_p2p_port);
	if (pViewerNode->httpOP.m1_use_udp_sock == INVALID_SOCKET) {
		pthread_mutex_unlock(&(pViewerNode->m_secBind));
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udpopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed1);
		ReturnViewerNode(pViewerNode);
		return 0;
	}

	if (FindOutConnection(pViewerNode->httpOP.m1_use_udp_sock, pViewerNode->httpOP.m0_node_id, pViewerNode->httpOP.m1_peer_node_id,
							pViewerNode->httpOP.m1_peer_pri_ipArray, pViewerNode->httpOP.m1_peer_pri_ipCount, pViewerNode->httpOP.m1_peer_pri_port,
							pViewerNode->httpOP.m1_peer_ip, pViewerNode->httpOP.m1_peer_port,
							&use_ip, &use_port) != 0) {
		UdpClose(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&(pViewerNode->m_secBind));
		if_progress_cancel();
		//MessageBox(NULL, _T("无法建立连接，网络状况异常，或者您的ID级别不够。"), (LPCTSTR)strProductName, MB_OK|MB_ICONINFORMATION);
		if_messagebox(R_string::msg_connect_proxy_failed);
		ReturnViewerNode(pViewerNode);
		return 0;
	}
	pViewerNode->httpOP.m1_use_peer_ip = use_ip;
	pViewerNode->httpOP.m1_use_peer_port = use_port;
#if 1 ////Debug
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "ConnectThreadFn: @@@ Use ip/port: %d.%d.%d.%d[%d]\n", 
		(pViewerNode->httpOP.m1_use_peer_ip & 0x000000ff) >> 0,
		(pViewerNode->httpOP.m1_use_peer_ip & 0x0000ff00) >> 8,
		(pViewerNode->httpOP.m1_use_peer_ip & 0x00ff0000) >> 16,
		(pViewerNode->httpOP.m1_use_peer_ip & 0xff000000) >> 24,
		pViewerNode->httpOP.m1_use_peer_port);
#endif


	//SetStatusInfo(pDlg->m_hWnd, _T("正在同对方建立连接，请稍候。。。"), 30);
	if_progress_show(R_string::msg_connect_connecting);

	pViewerNode->httpOP.m1_use_udt_sock = UdtOpenEx(pViewerNode->httpOP.m1_use_udp_sock);
	if (pViewerNode->httpOP.m1_use_udt_sock == INVALID_SOCKET) {
		UdpClose(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&(pViewerNode->m_secBind));
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udtopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed2);
		ReturnViewerNode(pViewerNode);
		return 0;
	}


	UDTSOCKET fhandle2 = UdtWaitConnect(pViewerNode->httpOP.m1_use_udt_sock, &(pViewerNode->httpOP.m1_use_peer_ip), &(pViewerNode->httpOP.m1_use_peer_port));
	if (INVALID_SOCKET == fhandle2) {
		UdtClose(pViewerNode->httpOP.m1_use_udt_sock);
		pViewerNode->httpOP.m1_use_udt_sock = INVALID_SOCKET;
		UdpClose(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&(pViewerNode->m_secBind));
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed8);
		ReturnViewerNode(pViewerNode);
		return 0;
	}
	else {
		UdtClose(pViewerNode->httpOP.m1_use_udt_sock);
		pViewerNode->httpOP.m1_use_udt_sock = fhandle2;
	}
	
	
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UDT;
	if_on_connected(get_use_sock_type(), get_use_udt_sock());
	if (SOCKET_TYPE_UDT == pViewerNode->httpOP.m1_use_sock_type) {
		UDT::register_learn_remote_addr(pViewerNode->httpOP.m1_use_udt_sock, learn_remote_addr, pViewerNode);
	}

	//SetStatusInfo(pDlg->m_hWnd, _T("成功建立连接，正在验证访问密码。。。"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
	if (-1 == ret) {
		//SetStatusInfo(pDlg->m_hWnd, "验证密码时通信出错，无法连接！");
	}
	else if (0 == ret)
	{
		//SetStatusInfo(pDlg->m_hWnd, "认证密码错误，无法连接！");
	}
	else
	{

		DoInConnection(pViewerNode);

	}

	CtrlCmd_BYE(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);

	usleep(3000*1000);


	UdtClose(pViewerNode->httpOP.m1_use_udt_sock);
	pViewerNode->httpOP.m1_use_udt_sock = INVALID_SOCKET;
	UdpClose(pViewerNode->httpOP.m1_use_udp_sock);
	pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
	if_progress_cancel();
	
	pthread_mutex_unlock(&(pViewerNode->m_secBind));
	ReturnViewerNode(pViewerNode);
	return 0;
}

void *ConnectThreadFn2(void *pvThreadParam)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)pvThreadParam;
	int ret;
	int nRetry;


	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "ConnectThreadFn2()...\n");

	pViewerNode->httpOP.m1_use_peer_ip = pViewerNode->httpOP.m1_peer_ip;
	pViewerNode->httpOP.m1_use_peer_port = pViewerNode->httpOP.m1_peer_port;
#if 1 ////Debug
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "ConnectThreadFn2: @@@ Use ip/port: %d.%d.%d.%d[%d]\n", 
		(pViewerNode->httpOP.m1_use_peer_ip & 0x000000ff) >> 0,
		(pViewerNode->httpOP.m1_use_peer_ip & 0x0000ff00) >> 8,
		(pViewerNode->httpOP.m1_use_peer_ip & 0x00ff0000) >> 16,
		(pViewerNode->httpOP.m1_use_peer_ip & 0xff000000) >> 24,
		pViewerNode->httpOP.m1_use_peer_port);
#endif

	//SetStatusInfo(pDlg->m_hWnd, _T("正在同对方建立连接，请稍候。。。"), 30);
	if_progress_show(R_string::msg_connect_connecting);

	pViewerNode->httpOP.m1_use_udp_sock = UdpOpen(INADDR_ANY, 0);
	if (pViewerNode->httpOP.m1_use_udp_sock == INVALID_SOCKET) {
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udpopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed1);
		ReturnViewerNode(pViewerNode);
		return 0;
	}

	pViewerNode->httpOP.m1_use_udt_sock = UdtOpenEx(pViewerNode->httpOP.m1_use_udp_sock);
	if (pViewerNode->httpOP.m1_use_udt_sock == INVALID_SOCKET) {
		UdpClose(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udtopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed2);
		ReturnViewerNode(pViewerNode);
		return 0;
	}

	nRetry = UDT_CONNECT_TIMES;
	ret = UDT::ERROR;
	while (nRetry-- > 0 && ret == UDT::ERROR) {
		//TRACE("@@@ UDT::connect()...\n");
		ret = UdtConnect(pViewerNode->httpOP.m1_use_udt_sock, pViewerNode->httpOP.m1_use_peer_ip, pViewerNode->httpOP.m1_use_peer_port);
	}
	if (UDT::ERROR == ret)
	{
		UdtClose(pViewerNode->httpOP.m1_use_udt_sock);
		pViewerNode->httpOP.m1_use_udt_sock = INVALID_SOCKET;
		UdpClose(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败(connect failed)，请重试。如果重试多次失败，建议采用中转方式连接。"), (LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed3);
		ReturnViewerNode(pViewerNode);
		return 0;
	}


	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UDT;
	if_on_connected(get_use_sock_type(), get_use_udt_sock());
	if (SOCKET_TYPE_UDT == pViewerNode->httpOP.m1_use_sock_type) {
		UDT::register_learn_remote_addr(pViewerNode->httpOP.m1_use_udt_sock, learn_remote_addr, pViewerNode);
	}
	
	
	//SetStatusInfo(pDlg->m_hWnd, _T("成功建立连接，正在验证访问密码。。。"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
	if (-1 == ret) {
		//SetStatusInfo(pDlg->m_hWnd, "验证密码时通信出错，无法连接！");
	}
	else if (0 == ret)
	{
		//SetStatusInfo(pDlg->m_hWnd, "认证密码错误，无法连接！");
	}
	else
	{

		DoInConnection(pViewerNode);

	}

	CtrlCmd_BYE(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);

	usleep(3000*1000);


	UdtClose(pViewerNode->httpOP.m1_use_udt_sock);
	pViewerNode->httpOP.m1_use_udt_sock = INVALID_SOCKET;
	UdpClose(pViewerNode->httpOP.m1_use_udp_sock);
	pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
	if_progress_cancel();
	
	ReturnViewerNode(pViewerNode);
	return 0;
}

void *ConnectThreadFnRev(void *pvThreadParam)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)pvThreadParam;
	SOCKET udp_sock;
	UDTSOCKET serv;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	UDTSOCKET fhandle;
	int ret;
	int i;


	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "ConnectThreadFnRev()...\n");

	pViewerNode->httpOP.m1_wait_time -= 5;
	if (pViewerNode->httpOP.m1_wait_time < 0) {
		pViewerNode->httpOP.m1_wait_time = 0;
	}

	for (i = pViewerNode->httpOP.m1_wait_time; i > 0; i--) {
		//wsprintf(szStatusStr, _T("正在排队等待服务响应，%d秒。。。"), i);
		//SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		if_progress_show_format1(R_string::msg_connect_wait_queue_format, i);
		usleep(1000000);
	}

	if_progress_show(R_string::msg_connect_reverse_connecting);

	udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_sock == INVALID_SOCKET) {
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed1);
		__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "ConnectThreadFnRev: UDP socket() failed!\n");
		ReturnViewerNode(pViewerNode);
		return 0;
	}

	int flag = 1;
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(pViewerNode->httpOP.m0_p2p_port - 2);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed1);
		__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "ConnectThreadFnRev: UDP bind() failed!\n");
		closesocket(udp_sock);
		ReturnViewerNode(pViewerNode);
		return 0;
	}


	serv = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(serv);

	if (UDT::ERROR == UDT::bind(serv, udp_sock))
	{
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed2);
		__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "ConnectThreadFnRev: UDT::bind() failed!\n");
		UDT::close(serv);
		closesocket(udp_sock);
		ReturnViewerNode(pViewerNode);
		return 0;
	}

	if (UDT::ERROR == UDT::listen(serv, 1))
	{
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed2);
		__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "ConnectThreadFnRev: UDT::listen() failed!\n");
		UDT::close(serv);
		closesocket(udp_sock);
		ReturnViewerNode(pViewerNode);
		return 0;
	}

#if 1
///////////////
	struct timeval waitval;
	waitval.tv_sec  = UDT_ACCEPT_TIME + 10;
	waitval.tv_usec = 0;
	ret = UDT::select(serv + 1, serv, NULL, NULL, &waitval);
	if (ret == UDT::ERROR || ret == 0) {
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed8);
		__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "ConnectThreadFnRev: UDT::select() failed/timeout!\n");
		UDT::close(serv);
		closesocket(udp_sock);
		ReturnViewerNode(pViewerNode);
		return 0;
	}
//////////////////
#endif

	if ((fhandle = UDT::accept(serv, (sockaddr*)&their_addr, &namelen)) == UDT::INVALID_SOCK) {
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed8);
		__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "ConnectThreadFnRev: UDT::accept() failed!\n");
		UDT::close(serv);
		closesocket(udp_sock);
		ReturnViewerNode(pViewerNode);
		return 0;
	}

	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "ConnectThreadFnRev: UDT::accept() OK!\n");

	UDT::close(serv);

	pViewerNode->httpOP.m1_use_udp_sock = udp_sock;
	pViewerNode->httpOP.m1_use_peer_ip = their_addr.sin_addr.s_addr;
	pViewerNode->httpOP.m1_use_peer_port = ntohs(their_addr.sin_port);
	pViewerNode->httpOP.m1_use_udt_sock = fhandle;
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UDT;
	
	if_on_connected(get_use_sock_type(), get_use_udt_sock());
	if (SOCKET_TYPE_UDT == pViewerNode->httpOP.m1_use_sock_type) {
		UDT::register_learn_remote_addr(pViewerNode->httpOP.m1_use_udt_sock, learn_remote_addr, pViewerNode);
	}


	//SetStatusInfo(pDlg->m_hWnd, _T("成功建立连接，正在验证访问密码。。。"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
	if (-1 == ret) {
		//SetStatusInfo(pDlg->m_hWnd, "验证密码时通信出错，无法连接！");
	}
	else if (0 == ret)
	{
		//SetStatusInfo(pDlg->m_hWnd, "认证密码错误，无法连接！");
	}
	else
	{

		DoInConnection(pViewerNode);

	}

	CtrlCmd_BYE(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);

	usleep(3000*1000);


	UDT::close(pViewerNode->httpOP.m1_use_udt_sock);
	pViewerNode->httpOP.m1_use_udt_sock = INVALID_SOCKET;
	closesocket(pViewerNode->httpOP.m1_use_udp_sock);
	pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
	if_progress_cancel();

	ReturnViewerNode(pViewerNode);
	return 0;
}

void DisconnectNode(VIEWER_NODE *pViewerNode)
{
	__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "DisconnectNode(%d)...\n", pViewerNode->nID);

	if (pViewerNode->bUsing == FALSE) {
		return;
	}
	//if (pViewerNode->bTopoPrimary) {
	//	CtrlCmd_AV_STOP(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
	//}
	if (currentSourceIndex == pViewerNode->nID) {

		//尝试找到另一个已连接的ViewNode
		int i;
		for (i = 0; i < MAX_VIEWER_NUM; i++)
		{
			if (viewerArray[i].bUsing == FALSE || viewerArray[i].bConnected == FALSE) {
				continue;
			}
			if (currentSourceIndex != i) {
				break;
			}
		}
		if (i < MAX_VIEWER_NUM)//找到一个！切换。。。
		{
			SwitchMediaSource(currentSourceIndex, i);
		}
		else {
			CtrlCmd_AV_STOP(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
			currentSourceIndex = -1;
		}
	}

	pViewerNode->bQuitRecvSocketLoop = TRUE;

	usleep(2000*1000);

	if (pViewerNode->httpOP.m1_use_sock_type == SOCKET_TYPE_UDT)
	{
		UDT::close(pViewerNode->httpOP.m1_use_udt_sock);
		closesocket(pViewerNode->httpOP.m1_use_udp_sock);
	}
	else if (pViewerNode->httpOP.m1_use_sock_type == SOCKET_TYPE_TCP)
	{
		closesocket(pViewerNode->httpOP.m1_use_udt_sock);
	}
}

void ReturnViewerNode(VIEWER_NODE *pViewerNode)
{
	if (NULL != pViewerNode)
	{
		pViewerNode->bConnected = FALSE;
		pViewerNode->bConnecting = FALSE;
		pViewerNode->bUsing = FALSE;
		pViewerNode->nID = -1;
	}
}

static BYTE getFakeServerFuncFlags()
{
	BYTE ret = 0;
	ret |= FUNC_FLAGS_AV;
	ret |= FUNC_FLAGS_SHELL;
	ret |= FUNC_FLAGS_HASROOT;
	ret |= FUNC_FLAGS_ACTIVATED;
	return ret;
}

void *IpcServerThreadFn(void *pvThreadParam)
{
	struct sockaddr_un addr;
	int len;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	
	ipc_listen_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = '\0';
    strncpy(addr.sun_path + 1, ipc_socket_name, sizeof(addr.sun_path) - 1);
    len = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(ipc_socket_name);
    
	if (bind(ipc_listen_sock, (sockaddr *)&addr, len) < 0) {
		__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "IpcServerThreadFn: TCP bind() failed!\n");
		closesocket(ipc_listen_sock);
		ipc_listen_sock = INVALID_SOCKET;
		return 0;
	}
	
	listen(ipc_listen_sock, 1);
	
	while (1)
	{
		//Blocking...
		SOCKET fhandle = accept(ipc_listen_sock, (sockaddr*)&their_addr, &namelen);
		if (INVALID_SOCKET == fhandle) {
			__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "IpcServerThreadFn: TCP accept() failed!\n");
			break;
		}
		ipc_server_sock = fhandle;
		
		while (1)
		{
			BOOL bQuitLoop = FALSE;
			SOCKET_TYPE type = SOCKET_TYPE_TCP;
			int ret;
			char buff[16*1024];
			WORD wCommand;
			DWORD dwDataLength;
			char *temp_ptr;
			BYTE bZeroID[6] = {0, 0, 0, 0, 0, 0};
			
			ret = RecvStream(type, fhandle, buff, 6);
			if (ret != 0) {
				__android_log_print(ANDROID_LOG_ERROR, "viewer_jni", "IpcServerThreadFn: RecvStream(6) failed!\n");
				break;
			}
			
			wCommand = ntohs(pf_get_word(buff+0));
			dwDataLength = ntohl(pf_get_dword(buff+2));
			
			__android_log_print(ANDROID_LOG_INFO, "viewer_jni", "IpcServerThreadFn: RecvStream(6) wCommand=0x%04x, dwDataLength=%ld\n", wCommand, dwDataLength);
			
			switch (wCommand)
			{
				case CMD_CODE_HELLO_REQ:
					ret = RecvStream(type, fhandle, buff, 6);
					if (ret != 0) {
						bQuitLoop = TRUE;
						break;
					}
					//if (memcmp(g1_peer_node_id, buff, 6) != 0) {
					//	break;
					//}

					ret = RecvStream(type, fhandle, buff, 4);
					if (ret != 0) {
						bQuitLoop = TRUE;
						break;
					}

					ret = RecvStream(type, fhandle, buff, 1);
					if (ret != 0) {
						bQuitLoop = TRUE;
						break;
					}
					//(*(BYTE *)buff == 1

					ret = RecvStream(type, fhandle, buff, 256);
					if (ret != 0) {
						bQuitLoop = TRUE;
						break;
					}

					CtrlCmd_Send_HELLO_RESP(type, fhandle, bZeroID, g0_version, getFakeServerFuncFlags(), 0, CTRLCMD_RESULT_OK);

					break;

				case CMD_CODE_AV_START_REQ:
					ret = RecvStream(type, fhandle, buff, 1+1+1+4+4);
					if (ret != 0) {
						bQuitLoop = TRUE;
						break;
					}
					
					is_app_recv_av = TRUE;
					
					break;


				case CMD_CODE_AV_STOP_REQ:
					
					is_app_recv_av = FALSE;
					
					break;
					
				case CMD_CODE_BYE_REQ:
					
					break;
					
				case CMD_CODE_AV_SWITCH_REQ:
				case CMD_CODE_AV_CONTRL_REQ:
				case CMD_CODE_VOICE_REQ:
				case CMD_CODE_TEXT_REQ:
				case CMD_CODE_ARM_REQ:
				case CMD_CODE_DISARM_REQ:
				case CMD_CODE_NULL:
				default:
					temp_ptr = (char *)malloc(6+dwDataLength);
					pf_set_word(temp_ptr + 0, htons(wCommand));
					pf_set_dword(temp_ptr + 2, htonl(dwDataLength));
					ret = RecvStream(type, fhandle, temp_ptr+6, dwDataLength);
					if (ret != 0) {
						free(temp_ptr);
						bQuitLoop = TRUE;
						break;
					}
					if (currentSourceIndex != -1)
					{
						CtrlCmd_Send_Raw(viewerArray[currentSourceIndex].httpOP.m1_use_sock_type, viewerArray[currentSourceIndex].httpOP.m1_use_udt_sock, (BYTE *)temp_ptr, 6+dwDataLength);
					}
					free(temp_ptr);
					break;
			}
			
			if (bQuitLoop) {
				break;
			}
		}//while(1)
		
		closesocket(ipc_server_sock);
		ipc_server_sock = INVALID_SOCKET;
	}//while(1)
	
	closesocket(ipc_listen_sock);
	ipc_listen_sock = INVALID_SOCKET;
}
