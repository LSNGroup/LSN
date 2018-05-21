#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include "jni_func.h"
#include "platform.h"
#include "CommonLib.h"
#include "HttpOperate.h"
#include "TLVRecv.h"
#include "G729Recv.h"
#include "FF264Recv.h"
#include "FF263Recv.h"

extern UDTSOCKET get_use_udt_sock();
extern SOCKET_TYPE get_use_sock_type();


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(TLVRecvStart)
	(JNIEnv* env, jobject thiz)
{
	TLVRecvStart(FIRST_CONNECT_PORT, 0/* Network byte order */, 0, INVALID_SOCKET, get_use_udt_sock(), get_use_sock_type());
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(TLVRecvStop)
	(JNIEnv* env, jobject thiz)
{
	TLVRecvStop();
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(TLVRecvSetPeriod)
	(JNIEnv* env, jobject thiz, jint miniSec)
{
	TLVRecvSetPeriod(miniSec);
}


extern "C"
jdouble
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(TLVRecvGetData)
	(JNIEnv* env, jobject thiz, jint tlv_type)
{
	return TLVRecvGetData(tlv_type);
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(AudioRecvStart)
	(JNIEnv* env, jobject thiz)
{
	AudioRecvStart(FIRST_CONNECT_PORT, 0/* Network byte order */, 0, INVALID_SOCKET, get_use_udt_sock(), get_use_sock_type());
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(AudioRecvStop)
	(JNIEnv* env, jobject thiz)
{
	AudioRecvStop();
}


extern "C"
jbyteArray
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(AudioRecvGetData)
	(JNIEnv* env, jobject thiz, jintArray arrBreak)
{
	int ret;
	jint arr[2];
	unsigned char *buff;
	BOOL full;
	DWORD rtptimestamp;
	
	buff = (unsigned char *)malloc(AUDIO_GET_BUFFER_SIZE);
	ret = AudioRecvGetData(buff, AUDIO_GET_BUFFER_SIZE, &full, &rtptimestamp);
	if (ret < 0) {
		arr[0] = 1;
		arr[1] = rtptimestamp;
		(env)->SetIntArrayRegion(arrBreak, 0, 2, arr);
		free(buff);
		return 0;
	}
	else if (ret == 0) {
		arr[0] = 0;
		arr[1] = rtptimestamp;
		(env)->SetIntArrayRegion(arrBreak, 0, 2, arr);
		free(buff);
		return 0;
	}
	else {
		arr[0] = 0;
		arr[1] = rtptimestamp;
		(env)->SetIntArrayRegion(arrBreak, 0, 2, arr);
		jbyteArray dataArray = (env)->NewByteArray(ret);
		(env)->SetByteArrayRegion(dataArray, 0, ret, (const jbyte *)(&buff[0]));
		free(buff);
		return dataArray;
	}
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(FF264RecvStart)
	(JNIEnv* env, jobject thiz)
{
	FF264RecvStart(FIRST_CONNECT_PORT, 0/* Network byte order */, 0, INVALID_SOCKET, get_use_udt_sock(), get_use_sock_type());
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(FF264RecvStop)
	(JNIEnv* env, jobject thiz)
{
	FF264RecvStop();
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(FF264RecvSetVflip)
	(JNIEnv* env, jobject thiz)
{
	FF264RecvSetVflip();
}


extern "C"
jintArray
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(FF264RecvGetData)
	(JNIEnv* env, jobject thiz, jintArray arrBreak)
{
	int ret;
	jint arr[6];
	jint width, height, timePerFrame;
	unsigned char *buff;
	BOOL full = false;
	DWORD rtptimestamp;
	
	buff = (unsigned char *)malloc(VIDEO_GET_BUFFER_SIZE);
	ret = FF264RecvGetData(VIDEO_MEDIASUBTYPE_RGB32, buff, VIDEO_GET_BUFFER_SIZE, &width, &height, &timePerFrame/* ms */, &full, &rtptimestamp);
	if (ret < 0) {
		arr[0] = 1;
		arr[1] = width;
		arr[2] = height;
		arr[3] = timePerFrame;
		arr[4] = full ? 1 : 0;
		arr[5] = (int)rtptimestamp;
		(env)->SetIntArrayRegion(arrBreak, 0, 6, arr);
		jintArray dataArray = (env)->NewIntArray(1);//fake
		(env)->SetIntArrayRegion(dataArray, 0, 1, (const jint *)(&buff[0]));
		free(buff);
		return dataArray;
	}
	else if (ret == 0) {
		arr[0] = 0;
		arr[1] = width;
		arr[2] = height;
		arr[3] = timePerFrame;
		arr[4] = full ? 1 : 0;
		arr[5] = (int)rtptimestamp;
		(env)->SetIntArrayRegion(arrBreak, 0, 6, arr);
		jintArray dataArray = (env)->NewIntArray(1);//fake
		(env)->SetIntArrayRegion(dataArray, 0, 1, (const jint *)(&buff[0]));
		free(buff);
		return dataArray;
	}
	else {
		arr[0] = 0;
		arr[1] = width;
		arr[2] = height;
		arr[3] = timePerFrame;
		arr[4] = full ? 1 : 0;
		arr[5] = (int)rtptimestamp;
		(env)->SetIntArrayRegion(arrBreak, 0, 6, arr);
		jintArray dataArray = (env)->NewIntArray(ret/4);
		(env)->SetIntArrayRegion(dataArray, 0, ret/4, (const jint *)(&buff[0]));
		free(buff);
		return dataArray;
	}
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(FF263RecvStart)
	(JNIEnv* env, jobject thiz)
{
	FF263RecvStart(FIRST_CONNECT_PORT, 0/* Network byte order */, 0, INVALID_SOCKET, get_use_udt_sock(), get_use_sock_type());
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(FF263RecvStop)
	(JNIEnv* env, jobject thiz)
{
	FF263RecvStop();
}


extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(FF263RecvSetVflip)
	(JNIEnv* env, jobject thiz)
{
	FF263RecvSetVflip();
}


extern "C"
jintArray
MAKE_JNI_FUNC_NAME_FOR_SharedFuncLib(FF263RecvGetData)
	(JNIEnv* env, jobject thiz, jintArray arrBreak)
{
	int ret;
	jint arr[6];
	jint width, height, timePerFrame;
	unsigned char *buff;
	BOOL full = false;
	DWORD rtptimestamp;
	
	buff = (unsigned char *)malloc(VIDEO_GET_BUFFER_SIZE);
	ret = FF263RecvGetData(VIDEO_MEDIASUBTYPE_RGB32, buff, VIDEO_GET_BUFFER_SIZE, &width, &height, &timePerFrame/* ms */, &full, &rtptimestamp);
	if (ret < 0) {
		arr[0] = 1;
		arr[1] = width;
		arr[2] = height;
		arr[3] = timePerFrame;
		arr[4] = full ? 1 : 0;
		arr[5] = (int)rtptimestamp;
		(env)->SetIntArrayRegion(arrBreak, 0, 6, arr);
		jintArray dataArray = (env)->NewIntArray(1);//fake
		(env)->SetIntArrayRegion(dataArray, 0, 1, (const jint *)(&buff[0]));
		free(buff);
		return dataArray;
	}
	else if (ret == 0) {
		arr[0] = 0;
		arr[1] = width;
		arr[2] = height;
		arr[3] = timePerFrame;
		arr[4] = full ? 1 : 0;
		arr[5] = (int)rtptimestamp;
		(env)->SetIntArrayRegion(arrBreak, 0, 6, arr);
		jintArray dataArray = (env)->NewIntArray(1);//fake
		(env)->SetIntArrayRegion(dataArray, 0, 1, (const jint *)(&buff[0]));
		free(buff);
		return dataArray;
	}
	else {
		arr[0] = 0;
		arr[1] = width;
		arr[2] = height;
		arr[3] = timePerFrame;
		arr[4] = full ? 1 : 0;
		arr[5] = (int)rtptimestamp;
		(env)->SetIntArrayRegion(arrBreak, 0, 6, arr);
		jintArray dataArray = (env)->NewIntArray(ret/4);
		(env)->SetIntArrayRegion(dataArray, 0, ret/4, (const jint *)(&buff[0]));
		free(buff);
		return dataArray;
	}
}
