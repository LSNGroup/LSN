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
#include "ProxyClient.h"
#include "phpMd5.h"
#include "g729a.h"
#include "avrtp_def.h"
#include "R_string_def.h"
#include "viewer_if.h"

#include <android/log.h>


#include "UPnP.h"
MyUPnP  myUPnP;
UPNPNAT_MAPPING mapping;


typedef struct _tag_conn_param
{
	char szNodeIdStr[MAX_LOADSTRING];
	char szPubIpStr[16];
	char szPubPortStr[8];
	BOOL bLanNode;
	BOOL no_nat;
	int nat_type;
} CONN_PARAM;


static char g_client_charset[16];
static char g_client_lang[16];

static BOOL g_bFirstCheckStun = TRUE;
static BOOL m_bNeedRegister = TRUE;
static pthread_mutex_t m_secBind;

static NOTIFICATION_ITEM g_notifications[MAX_NOTIFICATION_NUM];
static int g_notifi_count;
static int g_notifi_index = 0;

static pthread_t hRegisterThread = 0;
static pthread_t hConnectThread = 0;
static pthread_t hConnectThread_Proxy = 0;
static pthread_t hConnectThread2 = 0;
static pthread_t hConnectThreadTcp = 0;
static pthread_t hConnectThreadTcp_Proxy = 0;
static pthread_t hConnectThreadRev = 0;

void *RegisterThreadFn(void *pvThreadParam);
void *ConnectThreadFn(void *pvThreadParam);
void *ConnectThreadFn_Proxy(void *pvThreadParam);
void *ConnectThreadFn2(void *pvThreadParam);
void *ConnectThreadFnTcp(void *pvThreadParam);
void *ConnectThreadFnTcp_Proxy(void *pvThreadParam);
void *ConnectThreadFnRev(void *pvThreadParam);

static void InitVar();
static void HandleRegister();


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
	m_bNeedRegister = TRUE;
	
	pthread_mutex_init(&m_secBind, NULL);
	
	CtrlCmd_Init();
	
	// use this function to initialize the UDT library
	UdtStartup();
	
	InitVar();
	
	
	BOOL bMappingExists = FALSE;
	mapping.description = "";
	mapping.protocol = UNAT_UDP;
	mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * (WORD)rand() / (65536)) & 0xffffff00);
	while (UNAT_OK == myUPnP.GetNATSpecificEntry(&mapping, &bMappingExists) && bMappingExists)
	{
		__android_log_print(ANDROID_LOG_INFO, "StartNative", "Find NATPortMapping(%s), retry...\n", mapping.description.c_str());
		bMappingExists = FALSE;
		mapping.description = "";
		mapping.protocol = UNAT_UDP;
		mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * (WORD)rand() / (65536)) & 0xffffff00);
	}//找到一个未被占用的外部端口映射，或者路由器UPnP功能不可用
	
	
	pthread_create(&hRegisterThread, NULL, RegisterThreadFn, NULL);
	
	return 0;
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(StopNative)
	(JNIEnv* env, jobject thiz)
{
	// use this function to release the UDT library
	//UdtCleanup();
	
	CtrlCmd_Uninit();
	
	myUPnP.RemoveNATPortMapping(mapping);
	
	pthread_mutex_destroy(&m_secBind);
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(getAppVersion)
	(JNIEnv* env, jobject thiz)
{
	return MYSELF_VERSION;
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(getLowestLevelForAv)
	(JNIEnv* env, jobject thiz)
{
	return (jint)g1_lowest_level_for_av;
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(getLowestLevelForVnc)
	(JNIEnv* env, jobject thiz)
{
	return (jint)g1_lowest_level_for_vnc;
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
	WORD result_code;
	
	
	len = (env)->GetStringUTFLength(strPass);
	pass = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strPass, 0, (env)->GetStringLength(strPass), pass);
	pass[len] = '\0';
	
	ret = CtrlCmd_HELLO((SOCKET_TYPE)type, (SOCKET)fhandle, g0_node_id, g0_version, pass, g1_peer_node_id, &dwServerVersion, &bFuncFlags, &result_code);
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
jbyteArray
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdMAVTLV)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle, jintArray arrBreak)
{
	int ret;
	jint arr[1];
	unsigned char *buff;
	int buff_len;
	
	buff = (unsigned char *)malloc(TLV_TYPE_COUNT * 8);
	buff_len = TLV_TYPE_COUNT * 8;
	ret = CtrlCmd_MAV_TLV((SOCKET_TYPE)type, (SOCKET)fhandle, buff, &buff_len);
	if (ret != 0) {
		arr[0] = ret;
		(env)->SetIntArrayRegion(arrBreak, 0, 1, arr);
		free(buff);
		return 0;
	}
	else {
		arr[0] = ret;
		(env)->SetIntArrayRegion(arrBreak, 0, 1, arr);
		jbyteArray dataArray = (env)->NewByteArray(buff_len);
		(env)->SetByteArrayRegion(dataArray, 0, buff_len, (const jbyte *)(&buff[0]));
		free(buff);
		return dataArray;
	}
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


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdMAVSTART)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle)
{
	return CtrlCmd_MAV_START((SOCKET_TYPE)type, (SOCKET)fhandle);
}


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(CtrlCmdMAVSTOP)
	(JNIEnv* env, jobject thiz, jint type, jint fhandle)
{
    return CtrlCmd_MAV_STOP((SOCKET_TYPE)type, (SOCKET)fhandle);
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(ProxyClientStartSlave)
	(JNIEnv* env, jobject thiz, jint wLocalTcpPort)
{
    ProxyClientStartSlave((WORD)wLocalTcpPort);
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(ProxyClientStartProxy)
	(JNIEnv* env, jobject thiz, jint ftype, jint fhandle, jboolean bAutoClose, jint wLocalTcpPort)
{
    ProxyClientStartProxy((SOCKET_TYPE)ftype, (SOCKET)fhandle, (BOOL)bAutoClose, (WORD)wLocalTcpPort);
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(ProxyClientAllQuit)
	(JNIEnv* env, jobject thiz)
{
    ProxyClientAllQuit();
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(ProxyClientClearQuitFlag)
	(JNIEnv* env, jobject thiz)
{
    ProxyClientClearQuitFlag();
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
	
	__android_log_print(ANDROID_LOG_INFO, "SendVoice", "SendVoice(%d bytes g729a)...", len2);
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

static int IsNodeExist(ANYPC_NODE *nodesArray, int nCount, char *node_id_str)
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
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(DoSearchServers)
	(JNIEnv* env, jobject thiz, jstring strRequestNodes)
{
	jint ret;
	char *request_nodes;
	int len;
	DWORD broadcastDsts[MAX_BROADCAST_DST_NUM];
	int nDstsCnt = MAX_BROADCAST_DST_NUM;
	ANYPC_NODE *servers;//[NODES_PER_PAGE];
	DWORD cntServer = NODES_PER_PAGE;
	ANYPC_NODE *servers2;//[NODES_PER_PAGE];
	int cntServer2 = NODES_PER_PAGE;
	int nNum;
	int i;
	
	
	servers  = (ANYPC_NODE *)malloc( sizeof(ANYPC_NODE)*NODES_PER_PAGE );
	servers2 = (ANYPC_NODE *)malloc( sizeof(ANYPC_NODE)*NODES_PER_PAGE );
	
	len = (env)->GetStringLength(strRequestNodes);
	request_nodes = (char *)malloc(len+1);
	if (len > 0) {
		(env)->GetStringUTFRegion(strRequestNodes, 0, len, request_nodes);
		request_nodes[len] = '\0';
	}
	
	PrepareBroadcastDests(broadcastDsts, &nDstsCnt);
	__android_log_print(ANDROID_LOG_INFO, "DoSearchServers", "Broadcast address number = %ld", nDstsCnt);////Debug
	
	if (NO_ERROR != DMsg_SearchServers(g0_node_id, g0_node_name, g0_version, 
							broadcastDsts, nDstsCnt, servers, &cntServer))
	{
		if_messagetip(R_string::msg_communication_error);
		free(servers);
		free(servers2);
		free(request_nodes);
	    return -1;
	}
	
	if (len > 0)
	{
		ret = DoQueryList(g_client_charset, g_client_lang, request_nodes, servers2, &cntServer2, &nNum);
		if (ret < 0)
		{
			if_messagetip(R_string::msg_internet_error);
		}
		else if (ret == 0 && cntServer2 > 0)
		{
			for (i = 0; i < cntServer2; i++) {
				int ret_index = IsNodeExist(servers, cntServer, servers2[i].node_id_str);
				if (-1 == ret_index) {
					if (cntServer < NODES_PER_PAGE) {
						servers2[i].bLanNode = FALSE;
						servers[cntServer] = servers2[i];
						cntServer += 1;
					}
				}
				else {
					servers[ret_index].comments_id = servers2[i].comments_id;
				}
			}
		}
	}
	
	if (cntServer > 0)
	{
		//__android_log_print(ANDROID_LOG_INFO, "DoSearchServers", "jobject = %ld", (unsigned long)thiz);////Debug
		
		jclass cls = (env)->GetObjectClass(thiz);
		//__android_log_print(ANDROID_LOG_INFO, "DoSearchServers", "jclass1 = %ld", (unsigned long)cls);////Debug
		
		if (0 == cls) {
			cls = (env)->FindClass("com/wangling/mobilecamera/MainListActivity");
			//__android_log_print(ANDROID_LOG_INFO, "DoSearchServers", "jclass2 = %ld", (unsigned long)cls);////Debug
		}
		
		jmethodID mid = (env)->GetMethodID(cls, "FillAnyPCNode", "(IZLjava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ZIZZIILjava/lang/String;Ljava/lang/String;ILjava/lang/String;)V");
		//__android_log_print(ANDROID_LOG_INFO, "DoSearchServers", "jmethodID = %ld", (unsigned long)mid);////Debug
		
		for (i = 0; i < (int)cntServer; i++)
		{
			ANYPC_NODE *pNode = &servers[i];
			(env)->CallVoidMethod(thiz, mid, 
									i, (jboolean)pNode->bLanNode, (env)->NewStringUTF(pNode->node_id_str), (env)->NewStringUTF(pNode->node_name), 
									(jint)pNode->version, (env)->NewStringUTF(pNode->ip_str), (env)->NewStringUTF(pNode->port_str), 
									(env)->NewStringUTF(pNode->pub_ip_str), (env)->NewStringUTF(pNode->pub_port_str), (jboolean)pNode->no_nat, 
									pNode->nat_type, (jboolean)pNode->is_admin, (jboolean)pNode->is_busy, pNode->audio_channels, pNode->video_channels, 
									(env)->NewStringUTF(pNode->os_info), (env)->NewStringUTF(pNode->device_uuid), pNode->comments_id, (env)->NewStringUTF(pNode->location) );
		
			__android_log_print(ANDROID_LOG_INFO, "DoSearchServers", "FillAnyPCNode(index = %d)!", i);////Debug
		}
	}

	free(servers);
	free(servers2);
	free(request_nodes);
    return cntServer;
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(DoConnect)
	(JNIEnv* env, jobject thiz, jstring strNodeId, jstring strPubIp, jstring strPubPort, jboolean bLanNode, jboolean no_nat, jint nat_type)
{
	int len;
	char *node_id_str;
	char *pub_ip_str;
	char *pub_port_str;
	
	len = (env)->GetStringLength(strNodeId);
	node_id_str = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strNodeId, 0, len, node_id_str);
	node_id_str[len] = '\0';
	
	len = (env)->GetStringLength(strPubIp);
	pub_ip_str = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strPubIp, 0, len, pub_ip_str);
	pub_ip_str[len] = '\0';
	
	len = (env)->GetStringLength(strPubPort);
	pub_port_str = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strPubPort, 0, len, pub_port_str);
	pub_port_str[len] = '\0';
	
	
	CONN_PARAM *pThreadParam = (CONN_PARAM *)malloc(sizeof(CONN_PARAM));
	strncpy(pThreadParam->szNodeIdStr, node_id_str, sizeof(pThreadParam->szNodeIdStr));
	strncpy(pThreadParam->szPubIpStr, pub_ip_str, sizeof(pThreadParam->szPubIpStr));
	strncpy(pThreadParam->szPubPortStr, pub_port_str, sizeof(pThreadParam->szPubPortStr));
	pThreadParam->bLanNode = bLanNode;
	pThreadParam->no_nat = no_nat;
	pThreadParam->nat_type = nat_type;
	
	mac_addr(node_id_str, g1_peer_node_id, NULL);

	if ((FALSE == bLanNode) && (FALSE == no_nat)) {
		pthread_create(&hConnectThread, NULL, ConnectThreadFn, pThreadParam);
	}
	else {
		pthread_create(&hConnectThread2, NULL, ConnectThreadFn2, pThreadParam);
	}

	free(node_id_str);
	free(pub_ip_str);
	free(pub_port_str);
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(DoConnectTcp)
	(JNIEnv* env, jobject thiz, jstring strNodeId, jstring strPubIp, jstring strPubPort, jboolean bLanNode, jboolean no_nat, jint nat_type)
{
	int len;
	char *node_id_str;
	char *pub_ip_str;
	char *pub_port_str;
	
	len = (env)->GetStringLength(strNodeId);
	node_id_str = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strNodeId, 0, len, node_id_str);
	node_id_str[len] = '\0';
	
	len = (env)->GetStringLength(strPubIp);
	pub_ip_str = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strPubIp, 0, len, pub_ip_str);
	pub_ip_str[len] = '\0';
	
	len = (env)->GetStringLength(strPubPort);
	pub_port_str = (char *)malloc(len+1);
	(env)->GetStringUTFRegion(strPubPort, 0, len, pub_port_str);
	pub_port_str[len] = '\0';
	
	
	CONN_PARAM *pThreadParam = (CONN_PARAM *)malloc(sizeof(CONN_PARAM));
	strncpy(pThreadParam->szNodeIdStr, node_id_str, sizeof(pThreadParam->szNodeIdStr));
	strncpy(pThreadParam->szPubIpStr, pub_ip_str, sizeof(pThreadParam->szPubIpStr));
	strncpy(pThreadParam->szPubPortStr, pub_port_str, sizeof(pThreadParam->szPubPortStr));
	pThreadParam->bLanNode = bLanNode;
	pThreadParam->no_nat = no_nat;
	pThreadParam->nat_type = nat_type;
	
	mac_addr(node_id_str, g1_peer_node_id, NULL);
	
	pthread_create(&hConnectThreadTcp, NULL, ConnectThreadFnTcp, pThreadParam);
	
	
	free(node_id_str);
	free(pub_ip_str);
	free(pub_port_str);
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(DoDisconnect)
	(JNIEnv* env, jobject thiz)
{
	__android_log_print(ANDROID_LOG_INFO, "DoDisconnect", "DoDisconnect()...\n");
	
	usleep(3000000);
	
	if (g1_use_sock_type == SOCKET_TYPE_UDT)
	{
		UDT::unregister_learn_remote_addr(g1_use_udt_sock);
		
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
	}
	else if (g1_use_sock_type == SOCKET_TYPE_TCP)
	{
		closesocket(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////

static void learn_remote_addr(void *ctx, sockaddr* his_addr, int addr_len)
{
	sockaddr_in *sin = (sockaddr_in *)his_addr;
	g1_use_peer_port = ntohs(sin->sin_port);
	g1_use_peer_ip = sin->sin_addr.s_addr;
	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "learn_remote_addr() called, %s[%d]\n", inet_ntoa(sin->sin_addr), g1_use_peer_port);
}

static void InitVar()
{
	char szNodeId[20];
	if_get_viewer_nodeid(szNodeId, sizeof(szNodeId));
	__android_log_print(ANDROID_LOG_INFO, "InitVar", "if_get_viewer_nodeid() = %s", szNodeId);
	mac_addr(szNodeId, g0_node_id, NULL);
	
	strncpy(g0_device_uuid, "Viewer Device UUID", sizeof(g0_device_uuid));
	strncpy(g0_node_name, "Viewer Node Name", sizeof(g0_node_name));
	strncpy(g0_os_info, "Viewer OS Info", sizeof(g0_os_info));
	
	g0_version = MYSELF_VERSION;
	
	strncpy(g1_http_server, DEFAULT_HTTP_SERVER, sizeof(g1_http_server));
	strncpy(g1_stun_server, DEFAULT_STUN_SERVER, sizeof(g1_stun_server));
	g1_http_client_ip = 0;
	
	g0_nat_type = StunTypeUnknown;
	g0_pub_ip = 0;
	g0_pub_port = 0;
	
	g0_is_admin = 1;
	g0_is_busy = 0;
	g0_audio_channels = 0;
	g0_video_channels = 0;
	g1_register_period = DEFAULT_REGISTER_PERIOD;  /* Seconds */
	g1_expire_period = DEFAULT_EXPIRE_PERIOD;  /* Seconds */

	g1_lowest_level_for_av = 0;
	g1_lowest_level_for_vnc = 0;
	g1_lowest_level_for_ft = 0;
	g1_lowest_level_for_adb = 0;
	g1_lowest_level_for_webmoni = 0;
	g1_lowest_level_allow_hide = 0;

	g1_use_udp_sock = INVALID_SOCKET;
	g1_use_udt_sock = INVALID_SOCKET;
	g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
}

static void HandleRegister()
{
	int ret;
	StunAddress4 mapped;
	static BOOL bNoNAT;
	static int  nNatType;
	DWORD ipArrayTemp[MAX_ADAPTER_NUM];  /* Network byte order */
	int ipCountTemp;
	
	
	ret = DoRegister1(g_client_charset, g_client_lang);
	__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "DoRegister1...ret=%d\n", ret);
	
	if (ret == -1) {
		m_bNeedRegister = TRUE;
		return;
	}
	else if (ret == 0) {
		/* Exit this applicantion. */
		//::PostMessage(pDlgWnd, WM_REGISTER_EXIT, 0, 0);
		m_bNeedRegister = TRUE;
		return;
	}
	else if (ret == 1) {
		m_bNeedRegister = TRUE;
		return;
	}


	//::EnterCriticalSection(&(m_localbind_csec));
	pthread_mutex_lock(&m_secBind);

	/* STUN Information */
	if (strcmp(g1_stun_server, "NONE") == 0)
	{
		ret = CheckStunMyself(g1_http_server, FIRST_CONNECT_PORT, &mapped, &bNoNAT, &nNatType);
		if (ret == -1) {
			__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "CheckStunMyself() failed!\n");
			
		   mapped.addr = ntohl(g1_http_client_ip);
		   mapped.port = FIRST_CONNECT_PORT;
		   
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
		   
			g0_pub_ip = htonl(mapped.addr);
			g0_pub_port = mapped.port;
			g0_no_nat = bNoNAT;
			g0_nat_type = nNatType;
			__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
				(g0_pub_ip & 0x000000ff) >> 0,
				(g0_pub_ip & 0x0000ff00) >> 8,
				(g0_pub_ip & 0x00ff0000) >> 16,
				(g0_pub_ip & 0xff000000) >> 24,
				g0_pub_port,  g0_no_nat ? 1 : 0);
		}
		else {
			if_messagetip(R_string::msg_nat_detect_success);
			
			g0_pub_ip = htonl(mapped.addr);
			g0_pub_port = mapped.port;
			g0_no_nat = bNoNAT;
			g0_nat_type = nNatType;
			__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "CheckStunMyself: %d.%d.%d.%d[%d]\n", 
				(g0_pub_ip & 0x000000ff) >> 0,
				(g0_pub_ip & 0x0000ff00) >> 8,
				(g0_pub_ip & 0x00ff0000) >> 16,
				(g0_pub_ip & 0xff000000) >> 24,
				g0_pub_port);
		}
	}
	else if (g_bFirstCheckStun) {
		ret = CheckStun(g1_stun_server, FIRST_CONNECT_PORT, &mapped, &bNoNAT, &nNatType);
		if (ret == -1) {
			__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "CheckStun() failed!\n");
			
		   mapped.addr = ntohl(g1_http_client_ip);
		   mapped.port = FIRST_CONNECT_PORT;
		   
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
		   
			g0_pub_ip = htonl(mapped.addr);
			g0_pub_port = mapped.port;
			g0_no_nat = bNoNAT;
			g0_nat_type = nNatType;
			__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
				(g0_pub_ip & 0x000000ff) >> 0,
				(g0_pub_ip & 0x0000ff00) >> 8,
				(g0_pub_ip & 0x00ff0000) >> 16,
				(g0_pub_ip & 0xff000000) >> 24,
				g0_pub_port,  g0_no_nat ? 1 : 0);
		}
		else if (ret == 0) {
			//SetStatusInfo(pDlgWnd, _T("本端NAT防火墙无法穿透!!!您的网络环境可能封锁了UDP通信。"));
			if_messagetip(R_string::msg_nat_blocked);
			g0_pub_ip = 0;
			g0_pub_port = 0;
			g0_no_nat = FALSE;
			g0_nat_type = nNatType;
		}
		else {
			//SetStatusInfo(pDlgWnd, _T("本端NAT探测成功。"));
			if_messagetip(R_string::msg_nat_detect_success);
			g_bFirstCheckStun = FALSE;
			g0_pub_ip = htonl(mapped.addr);
			g0_pub_port = mapped.port;
			g0_no_nat = bNoNAT;
			g0_nat_type = nNatType;
#if 1 ////Debug
			__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "CheckStun: %s %d.%d.%d.%d[%d]", 
				g0_no_nat ? "NoNAT" : "NAT",
				(g0_pub_ip & 0x000000ff) >> 0,
				(g0_pub_ip & 0x0000ff00) >> 8,
				(g0_pub_ip & 0x00ff0000) >> 16,
				(g0_pub_ip & 0xff000000) >> 24,
				g0_pub_port);
#endif
		}
	}
	else {
		ret = CheckStunSimple(g1_stun_server, FIRST_CONNECT_PORT, &mapped);
		if (ret == -1) {
			__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "CheckStunSimple() failed!\n");
			mapped.addr = ntohl(g1_http_client_ip);
			mapped.port = FIRST_CONNECT_PORT;
			g0_pub_ip = htonl(mapped.addr);
			g0_pub_port = mapped.port;
		}
		else {
			g0_pub_ip = htonl(mapped.addr);
			g0_pub_port = mapped.port;
#if 1 ////Debug
			__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "CheckStunSimple: %s %d.%d.%d.%d[%d]", 
				g0_no_nat ? "NoNAT" : "NAT",
				(g0_pub_ip & 0x000000ff) >> 0,
				(g0_pub_ip & 0x0000ff00) >> 8,
				(g0_pub_ip & 0x00ff0000) >> 16,
				(g0_pub_ip & 0xff000000) >> 24,
				g0_pub_port);
#endif
		}
	}


	/* 本地路由器UPnP处理*/
	if (FALSE == bNoNAT)
	{
		char extIp[16];
		strcpy(extIp, "");
		myUPnP.GetNATExternalIp(extIp);
		if (strlen(extIp) > 0 && 0 != g0_pub_ip && inet_addr(extIp) == g0_pub_ip)
		{
			mapping.description = "UP2P";
			//mapping.protocol = ;
			//mapping.externalPort = ;
			mapping.internalPort = SECOND_CONNECT_PORT;
			if (UNAT_OK == myUPnP.AddNATPortMapping(&mapping, false))
			{
				g_bFirstCheckStun = FALSE;

				g0_pub_ip = inet_addr(extIp);
				g0_pub_port = mapping.externalPort;
				g0_no_nat = TRUE;
				g0_nat_type = StunTypeOpen;

				__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "UPnP AddPortMapping OK: %s[%d] --> %s[%d]", extIp, mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), mapping.internalPort);
			}
			else
			{
				BOOL bTempExists = FALSE;
				myUPnP.GetNATSpecificEntry(&mapping, &bTempExists);
				if (bTempExists)
				{
					g_bFirstCheckStun = FALSE;

					g0_pub_ip = inet_addr(extIp);
					g0_pub_port = mapping.externalPort;
					g0_no_nat = TRUE;
					g0_nat_type = StunTypeOpen;

					__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "UPnP PortMapping Exists: %s[%d] --> %s[%d]", extIp, mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), mapping.internalPort);
				}
				else {
					g0_pub_ip = htonl(mapped.addr);
					g0_pub_port = mapped.port;
					g0_no_nat = bNoNAT;
					g0_nat_type = nNatType;
				}
			}
		}
		else {
			__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "UPnP PortMapping Skipped: %s --> %s", extIp, myUPnP.GetLocalIPStr().c_str());
			g0_pub_ip = htonl(mapped.addr);
			g0_pub_port = mapped.port;
			g0_no_nat = bNoNAT;
			g0_nat_type = nNatType;
		}
	}
	
	//修正公网IP受控端的连接端口问题,2014-6-15
	if (bNoNAT) {
		g0_pub_port = SECOND_CONNECT_PORT;
	}
	
	
	/* Local IP Address */
	ipCountTemp = MAX_ADAPTER_NUM;
	if (GetLocalAddress(ipArrayTemp, &ipCountTemp) != NO_ERROR) {
		//TRACE("GetLocalAddress: fialed!\n");
	}
	else {
		g0_ipCount = ipCountTemp;
		memcpy(g0_ipArray, ipArrayTemp, sizeof(DWORD)*ipCountTemp);
		g0_port = FIRST_CONNECT_PORT;
#if 1 ////Debug
		__android_log_print(ANDROID_LOG_INFO, "HandleRegister", "GetLocalAddress: (count=%d) %d.%d.%d.%d[%d]", 
			g0_ipCount,
			(g0_ipArray[0] & 0x000000ff) >> 0,
			(g0_ipArray[0] & 0x0000ff00) >> 8,
			(g0_ipArray[0] & 0x00ff0000) >> 16,
			(g0_ipArray[0] & 0xff000000) >> 24,
			g0_port);
#endif
	}
	
	if (g0_no_nat) {
		g0_port = SECOND_CONNECT_PORT;
	}

	m_bNeedRegister = FALSE;

_OUT:
	//::LeaveCriticalSection(&(m_localbind_csec));
	pthread_mutex_unlock(&m_secBind);
	return;
}

void *RegisterThreadFn(void *pvThreadParam)
{
	HandleRegister();
	
	g_notifi_index = 0;
	g_notifi_count = MAX_NOTIFICATION_NUM;
	if (DoQueryNotificationList(g_client_charset, g_client_lang, g_notifications, &g_notifi_count) == 0 && g_notifi_count > 0) {
		while (g_notifi_index < g_notifi_count)
		{
			srand((unsigned int)time(NULL));
			int r = rand();
			usleep((30 + (r % 90)) * 1000000);
			
			if_show_text(g_notifications[g_notifi_index].text);
			
			g_notifi_index += 1;
		}
	}
	
	return 0;
}

void *ConnectThreadFn(void *pvThreadParam)
{
	CONN_PARAM *pParam = (CONN_PARAM *)pvThreadParam;
	CONN_PARAM conn_param = *pParam;
	DWORD use_ip;
	WORD use_port;
	int ret;
	int i, nRetry;


	free(pvThreadParam);
	__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFn", "ConnectThreadFn()...\n");


	if_progress_show(R_string::msg_please_wait);

	if (	StunTypeUnknown == conn_param.nat_type ||
			StunTypeFailure == conn_param.nat_type ||
			StunTypeBlocked == conn_param.nat_type         )
	{
		ret = DoProxy(g_client_charset, g_client_lang, conn_param.szNodeIdStr, FALSE, 0);
		if (ret == 1) {
			//if_progress_cancel();
			pthread_create(&hConnectThread_Proxy, NULL, ConnectThreadFn_Proxy, NULL);
			return 0;
		}
		else {
			if_progress_cancel();
			//MessageBox(NULL, _T("无法建立连接，网络状况异常，或者您的ID级别不够。"), (LPCTSTR)strProductName, MB_OK|MB_ICONINFORMATION);
			if_messagebox(R_string::msg_connect_proxy_failed);
			return 0;
		}
	}


	//SetStatusInfo(pDlg->m_hWnd, _T("正在提交本机连接信息，请稍候。。。"), 20);
	if_progress_show(R_string::msg_connect_register_local);
	nRetry = 5;
	do {
		HandleRegister();
	} while (nRetry-- > 0 && m_bNeedRegister);
	if (m_bNeedRegister) {
		if_progress_cancel();
		//MessageBox(NULL, _T("提交连接信息失败，请重试。"), (LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_register_local_failed);
		return 0;
	}


	pthread_mutex_lock(&m_secBind);

	//SetStatusInfo(pDlg->m_hWnd, _T("正在获取对方连接信息。。。"));
	if_progress_show(R_string::msg_connect_get_peer);
	nRetry = 5;
	ret = -1;
	while (nRetry-- > 0 && ret == -1) {
		ret = DoConnect(g_client_charset, g_client_lang, conn_param.szNodeIdStr);
	}
	if (ret < 1) {
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		//MessageBox(NULL, _T("获取对方连接信息失败，请刷新列表后重试。"), (LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_get_peer_failed);
		return 0;
	}

	if (ret == 2) {
		g1_wait_time -= 5;
		if (g1_wait_time < 0) {
			g1_wait_time = 0;
		}
	}

	//if (g1_wait_time > 30) {
	//	pDlg->ShowNotificationBox(_T("提示：升级ID后可以快速获得服务响应，避免长时间排队等待。"));
	//}

	for (i = g1_wait_time; i > 0; i--) {
		//wsprintf(szStatusStr, _T("正在排队等待服务响应，%d秒。。。"), i);
		//SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		if_progress_show_format1(R_string::msg_connect_wait_queue_format, i);
		usleep(1000000);
	}

	if (ret == 2) {
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		pthread_create(&hConnectThreadRev, NULL, ConnectThreadFnRev, NULL);
		return 0;
	}


	//SetStatusInfo(pDlg->m_hWnd, _T("正在打通连接信道。。。"), 20);
	if_progress_show(R_string::msg_connect_nat_hole);

	g1_use_udp_sock = UdpOpen(INADDR_ANY, FIRST_CONNECT_PORT);
	if (g1_use_udp_sock == INVALID_SOCKET) {
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udpopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed1);
		return 0;
	}

	if (FindOutConnection(g1_use_udp_sock, g0_node_id, g1_peer_node_id,
							g1_peer_pri_ipArray, g1_peer_pri_ipCount, g1_peer_pri_port,
							g1_peer_ip, g1_peer_port,
							&use_ip, &use_port) != 0) {
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_show(R_string::msg_please_wait);
		ret = DoProxy(g_client_charset, g_client_lang, conn_param.szNodeIdStr, FALSE, 0);
		if (ret == 1) {
			//if_progress_cancel();
			pthread_create(&hConnectThread_Proxy, NULL, ConnectThreadFn_Proxy, NULL);
			return 0;
		}
		else {
			if_progress_cancel();
			//MessageBox(NULL, _T("无法建立连接，网络状况异常，或者您的ID级别不够。"), (LPCTSTR)strProductName, MB_OK|MB_ICONINFORMATION);
			if_messagebox(R_string::msg_connect_proxy_failed);
			return 0;
		}
	}
	g1_use_peer_ip = use_ip;
	g1_use_peer_port = use_port;
#if 1 ////Debug
	__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFn", "@@@ Use ip/port: %d.%d.%d.%d[%d]\n", 
		(g1_use_peer_ip & 0x000000ff) >> 0,
		(g1_use_peer_ip & 0x0000ff00) >> 8,
		(g1_use_peer_ip & 0x00ff0000) >> 16,
		(g1_use_peer_ip & 0xff000000) >> 24,
		g1_use_peer_port);
#endif

	/* 为了节省连接时间，在 eloop 中执行 DoUnregister  */
	//eloop_register_timeout(3, 0, HandleDoUnregister, NULL, NULL);


	//SetStatusInfo(pDlg->m_hWnd, _T("正在同对方建立连接，请稍候。。。"), 30);
	if_progress_show(R_string::msg_connect_connecting);

	g1_use_udt_sock = UdtOpenEx(g1_use_udp_sock);
	if (g1_use_udt_sock == INVALID_SOCKET) {
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udtopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed2);
		return 0;
	}


	UDTSOCKET fhandle2 = UdtWaitConnect(g1_use_udt_sock, &g1_use_peer_ip, &g1_use_peer_port);
	if (INVALID_SOCKET == fhandle2) {
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed8);
		return 0;
	}
	else {
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = fhandle2;
	}
	
	
	g1_use_sock_type = SOCKET_TYPE_UDT;
	if_on_connected(g1_use_sock_type, g1_use_udt_sock);
	if (SOCKET_TYPE_UDT == g1_use_sock_type) {
		UDT::register_learn_remote_addr(g1_use_udt_sock, learn_remote_addr, NULL);
	}


	/*
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
	*/
	
	pthread_mutex_unlock(&m_secBind);
	return 0;
}

void *ConnectThreadFn_Proxy(void *pvThreadParam)
{
	int ret;
	int nRetry;


	__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFn_Proxy", "ConnectThreadFn_Proxy()...\n");

	pthread_mutex_lock(&m_secBind);


	g1_use_peer_ip = g1_peer_ip;
	g1_use_peer_port = g1_peer_port;


	//SetStatusInfo(pDlg->m_hWnd, _T("正在同对方建立连接，请稍候。。。"), 30);
	if_progress_show(R_string::msg_connect_connecting);

	g1_use_udp_sock = UdpOpen(INADDR_ANY, FIRST_CONNECT_PORT);
	if (g1_use_udp_sock == INVALID_SOCKET) {
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udpopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed1);
		return 0;
	}


	if (WaitForProxyReady(g1_use_udp_sock, g0_node_id, g1_peer_ip, g1_peer_port) < 0) {
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败(WaitForProxyReady failed)，请重试。"), (LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed4);
		return 0;
	}


	g1_use_udt_sock = UdtOpenEx(g1_use_udp_sock);
	if (g1_use_udt_sock == INVALID_SOCKET) {
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udtopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed2);
		return 0;
	}


	UDTSOCKET fhandle2 = UdtWaitConnect(g1_use_udt_sock, &g1_use_peer_ip, &g1_use_peer_port);
	if (INVALID_SOCKET == fhandle2) {
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed8);
		return 0;
	}
	else {
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = fhandle2;
	}


	g1_use_sock_type = SOCKET_TYPE_UDT;
	if_on_connected(g1_use_sock_type, g1_use_udt_sock);
	if (SOCKET_TYPE_UDT == g1_use_sock_type) {
		UDT::register_learn_remote_addr(g1_use_udt_sock, learn_remote_addr, NULL);
	}


	//EndProxy(g1_use_udp_sock, g1_peer_ip, g1_peer_port);
	/*
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
	*/

	pthread_mutex_unlock(&m_secBind);
	return 0;
}

void *ConnectThreadFn2(void *pvThreadParam)
{
	CONN_PARAM *pParam = (CONN_PARAM *)pvThreadParam;
	CONN_PARAM conn_param = *pParam;
	int ret;
	int nRetry;


	free(pvThreadParam);
	__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFn2", "ConnectThreadFn2()...\n");


	g1_use_peer_ip = inet_addr(conn_param.szPubIpStr);
	g1_use_peer_port = atol(conn_param.szPubPortStr);
	//修正公网IP受控端的连接端口问题,2014-6-15
	//if (FALSE == conn_param.bLanNode) {
	//	g1_use_peer_port = SECOND_CONNECT_PORT;
	//}
#if 1 ////Debug
	__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFn2", "@@@ Use ip/port: %d.%d.%d.%d[%d]\n", 
		(g1_use_peer_ip & 0x000000ff) >> 0,
		(g1_use_peer_ip & 0x0000ff00) >> 8,
		(g1_use_peer_ip & 0x00ff0000) >> 16,
		(g1_use_peer_ip & 0xff000000) >> 24,
		g1_use_peer_port);
#endif

	//SetStatusInfo(pDlg->m_hWnd, _T("正在同对方建立连接，请稍候。。。"), 30);
	if_progress_show(R_string::msg_connect_connecting);

	g1_use_udp_sock = UdpOpen(INADDR_ANY, 0);
	if (g1_use_udp_sock == INVALID_SOCKET) {
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udpopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed1);
		return 0;
	}

	g1_use_udt_sock = UdtOpenEx(g1_use_udp_sock);
	if (g1_use_udt_sock == INVALID_SOCKET) {
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udtopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed2);
		return 0;
	}

	nRetry = UDT_CONNECT_TIMES;
	ret = UDT::ERROR;
	while (nRetry-- > 0 && ret == UDT::ERROR) {
		//TRACE("@@@ UDT::connect()...\n");
		ret = UdtConnect(g1_use_udt_sock, g1_use_peer_ip, g1_use_peer_port);
	}
	if (UDT::ERROR == ret)
	{
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败(connect failed)，请重试。如果重试多次失败，建议采用中转方式连接。"), (LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed3);
		return 0;
	}

	
	g1_use_sock_type = SOCKET_TYPE_UDT;
	if_on_connected(g1_use_sock_type, g1_use_udt_sock);
	if (SOCKET_TYPE_UDT == g1_use_sock_type) {
		UDT::register_learn_remote_addr(g1_use_udt_sock, learn_remote_addr, NULL);
	}
	
	
	return 0;
}

void *ConnectThreadFnTcp(void *pvThreadParam)
{
	CONN_PARAM *pParam = (CONN_PARAM *)pvThreadParam;
	CONN_PARAM conn_param = *pParam;
	DWORD use_ip;
	WORD use_port;
	int ret;
	int i, nRetry;


	free(pvThreadParam);
	__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFnTcp", "ConnectThreadFnTcp()...\n");


	if_progress_show(R_string::msg_please_wait);

	if (	StunTypeUnknown == conn_param.nat_type ||
			StunTypeFailure == conn_param.nat_type ||
			StunTypeBlocked == conn_param.nat_type         )
	{
		{
			if_progress_cancel();
			CONN_PARAM *pThreadParamTemp = (CONN_PARAM *)malloc(sizeof(CONN_PARAM));
			strncpy(pThreadParamTemp->szNodeIdStr, conn_param.szNodeIdStr, sizeof(pThreadParamTemp->szNodeIdStr));
			pthread_create(&hConnectThreadTcp_Proxy, NULL, ConnectThreadFnTcp_Proxy, pThreadParamTemp);
			return 0;
		}
	}


	//SetStatusInfo(pDlg->m_hWnd, _T("正在提交本机连接信息，请稍候。。。"), 20);
	if_progress_show(R_string::msg_connect_register_local);
	nRetry = 5;
	do {
		HandleRegister();
	} while (nRetry-- > 0 && m_bNeedRegister);
	if (m_bNeedRegister) {
		if_progress_cancel();
		//MessageBox(NULL, _T("提交连接信息失败，请重试。"), (LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_register_local_failed);
		return 0;
	}


	pthread_mutex_lock(&m_secBind);

	//SetStatusInfo(pDlg->m_hWnd, _T("正在获取对方连接信息。。。"));
	if_progress_show(R_string::msg_connect_get_peer);
	nRetry = 5;
	ret = -1;
	while (nRetry-- > 0 && ret == -1) {
		ret = DoConnect(g_client_charset, g_client_lang, conn_param.szNodeIdStr);
	}
	if (ret < 1) {
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		//MessageBox(NULL, _T("获取对方连接信息失败，请刷新列表后重试。"), (LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_get_peer_failed);
		return 0;
	}

	if (ret == 2) {
		g1_wait_time -= 5;
		if (g1_wait_time < 0) {
			g1_wait_time = 0;
		}
	}

	//if (g1_wait_time > 30) {
	//	pDlg->ShowNotificationBox(_T("提示：升级ID后可以快速获得服务响应，避免长时间排队等待。"));
	//}

	for (i = g1_wait_time; i > 0; i--) {
		//wsprintf(szStatusStr, _T("正在排队等待服务响应，%d秒。。。"), i);
		//SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		if_progress_show_format1(R_string::msg_connect_wait_queue_format, i);
		usleep(1000000);
	}

	if (ret == 2) {
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		pthread_create(&hConnectThreadRev, NULL, ConnectThreadFnRev, NULL);
		return 0;
	}


	//SetStatusInfo(pDlg->m_hWnd, _T("正在打通连接信道。。。"), 20);
	if_progress_show(R_string::msg_connect_nat_hole);

	g1_use_udp_sock = UdpOpen(INADDR_ANY, FIRST_CONNECT_PORT);
	if (g1_use_udp_sock == INVALID_SOCKET) {
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udpopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed1);
		return 0;
	}

	if (FindOutConnection(g1_use_udp_sock, g0_node_id, g1_peer_node_id,
							g1_peer_pri_ipArray, g1_peer_pri_ipCount, g1_peer_pri_port,
							g1_peer_ip, g1_peer_port,
							&use_ip, &use_port) != 0) {
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_show(R_string::msg_please_wait);
		{
			if_progress_cancel();
			CONN_PARAM *pThreadParamTemp = (CONN_PARAM *)malloc(sizeof(CONN_PARAM));
			strncpy(pThreadParamTemp->szNodeIdStr, conn_param.szNodeIdStr, sizeof(pThreadParamTemp->szNodeIdStr));
			pthread_create(&hConnectThreadTcp_Proxy, NULL, ConnectThreadFnTcp_Proxy, pThreadParamTemp);
			return 0;
		}
	}
	g1_use_peer_ip = use_ip;
	g1_use_peer_port = use_port;
#if 1 ////Debug
	__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFn", "@@@ Use ip/port: %d.%d.%d.%d[%d]\n", 
		(g1_use_peer_ip & 0x000000ff) >> 0,
		(g1_use_peer_ip & 0x0000ff00) >> 8,
		(g1_use_peer_ip & 0x00ff0000) >> 16,
		(g1_use_peer_ip & 0xff000000) >> 24,
		g1_use_peer_port);
#endif

	/* 为了节省连接时间，在 eloop 中执行 DoUnregister  */
	//eloop_register_timeout(3, 0, HandleDoUnregister, NULL, NULL);


	//SetStatusInfo(pDlg->m_hWnd, _T("正在同对方建立连接，请稍候。。。"), 30);
	if_progress_show(R_string::msg_connect_connecting);

	g1_use_udt_sock = UdtOpenEx(g1_use_udp_sock);
	if (g1_use_udt_sock == INVALID_SOCKET) {
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		//MessageBox(NULL, _T("连接失败！(udtopen failed)"),(LPCTSTR)strProductName, MB_OK|MB_ICONWARNING);
		if_messagebox(R_string::msg_connect_connecting_failed2);
		return 0;
	}


	UDTSOCKET fhandle2 = UdtWaitConnect(g1_use_udt_sock, &g1_use_peer_ip, &g1_use_peer_port);
	if (INVALID_SOCKET == fhandle2) {
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed8);
		return 0;
	}
	else {
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = fhandle2;
	}
	
	
	g1_use_sock_type = SOCKET_TYPE_UDT;
	if_on_connected(g1_use_sock_type, g1_use_udt_sock);
	if (SOCKET_TYPE_UDT == g1_use_sock_type) {
		UDT::register_learn_remote_addr(g1_use_udt_sock, learn_remote_addr, NULL);
	}


	/*
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		pthread_mutex_unlock(&m_secBind);
		if_progress_cancel();
	*/
	
	pthread_mutex_unlock(&m_secBind);
	return 0;
}

void *ConnectThreadFnTcp_Proxy(void *pvThreadParam)
{
	CONN_PARAM *pParam = (CONN_PARAM *)pvThreadParam;
	CONN_PARAM conn_param = *pParam;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	int ret;
	int nRetry;


	free(pvThreadParam);
	__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFnTcp_Proxy", "ConnectThreadFnTcp_Proxy()...\n");


	if_progress_show(R_string::msg_please_wait);

	ret = DoProxy(g_client_charset, g_client_lang, conn_param.szNodeIdStr, TRUE, FIRST_CONNECT_PORT);
	if (ret != 1) {
		if_progress_cancel();
		//MessageBox(NULL, _T("无法建立连接，网络状况异常，或者您的ID级别不够。"), (LPCTSTR)strProductName, MB_OK|MB_ICONINFORMATION);
		if_messagebox(R_string::msg_connect_proxy_failed);
		return 0;
	}


	g1_use_peer_ip = g1_peer_ip;
	g1_use_peer_port = g1_peer_port;


	//SetStatusInfo(pDlg->m_hWnd, _T("正在同对方建立连接，请稍候。。。"), 30);
	if_progress_show(R_string::msg_connect_connecting);

	g1_use_udt_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g1_use_udt_sock == INVALID_SOCKET) {
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed5);
		return 0;
	}

	struct timeval timeo = {5, 0};
	setsockopt(g1_use_udt_sock, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(0);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(g1_use_udt_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed6);
		closesocket(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		return 0;
	}


	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(g1_use_peer_port);
	their_addr.sin_addr.s_addr = g1_use_peer_ip;
	memset(&(their_addr.sin_zero), '\0', 8);
	nRetry = 2;
	ret = -1;
	while (nRetry-- > 0 && ret < 0) {
		//TRACE("@@@ TCP::connect()...\n");
		ret = connect(g1_use_udt_sock, (sockaddr*)&their_addr, sizeof(their_addr));
	}
	if (ret < 0)
	{
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed7);
		closesocket(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		return 0;
	}


	g1_use_sock_type = SOCKET_TYPE_TCP;
	if_on_connected(g1_use_sock_type, g1_use_udt_sock);
	if (SOCKET_TYPE_UDT == g1_use_sock_type) {
		UDT::register_learn_remote_addr(g1_use_udt_sock, learn_remote_addr, NULL);
	}


	/*
		closesocket(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		if_progress_cancel();
	*/

	return 0;
}

void *ConnectThreadFnRev(void *pvThreadParam)
{
	SOCKET udp_sock;
	UDTSOCKET serv;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	UDTSOCKET fhandle;
	int ret;


	if_progress_show(R_string::msg_connect_reverse_connecting);

	udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_sock == INVALID_SOCKET) {
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed1);
		__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFnRev", "UDP socket() failed!\n");
		return 0;
	}

	int flag = 1;
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(SECOND_CONNECT_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed1);
		__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFnRev", "UDP bind() failed!\n");
		closesocket(udp_sock);
		return 0;
	}


	serv = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(serv);

	if (UDT::ERROR == UDT::bind(serv, udp_sock))
	{
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed2);
		__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFnRev", "UDT::bind() failed!\n");
		UDT::close(serv);
		closesocket(udp_sock);
		return 0;
	}

	if (UDT::ERROR == UDT::listen(serv, 1))
	{
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed2);
		__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFnRev", "UDT::listen() failed!\n");
		UDT::close(serv);
		closesocket(udp_sock);
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
		__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFnRev", "UDT::select() failed/timeout!\n");
		UDT::close(serv);
		closesocket(udp_sock);
		return 0;
	}
//////////////////
#endif

	if ((fhandle = UDT::accept(serv, (sockaddr*)&their_addr, &namelen)) == UDT::INVALID_SOCK) {
		if_progress_cancel();
		if_messagebox(R_string::msg_connect_connecting_failed8);
		__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFnRev", "UDT::accept() failed!\n");
		UDT::close(serv);
		closesocket(udp_sock);
		return 0;
	}

	__android_log_print(ANDROID_LOG_INFO, "ConnectThreadFnRev", "UDT::accept() OK!\n");

	UDT::close(serv);

	g1_use_udp_sock = udp_sock;
	g1_use_peer_ip = their_addr.sin_addr.s_addr;
	g1_use_peer_port = ntohs(their_addr.sin_port);
	g1_use_udt_sock = fhandle;
	g1_use_sock_type = SOCKET_TYPE_UDT;
	
	if_on_connected(g1_use_sock_type, g1_use_udt_sock);
	if (SOCKET_TYPE_UDT == g1_use_sock_type) {
		UDT::register_learn_remote_addr(g1_use_udt_sock, learn_remote_addr, NULL);
	}

	return 0;
}
