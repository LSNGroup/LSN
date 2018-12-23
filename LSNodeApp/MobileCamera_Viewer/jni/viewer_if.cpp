#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <jni.h>
#include "jni_func.h"
#include "platform.h"
#include "CommonLib.h"
#include "viewer_if.h"

#include <android/log.h>


static JavaVM* g_JavaVM = NULL;
static jobject g_objMainList = NULL;



extern "C"
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	
	__android_log_print(ANDROID_LOG_INFO, "viewer_if", "JNI_OnLoad()");
	
	if ((vm)->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) 
	{
		__android_log_print(ANDROID_LOG_INFO, "JNI_OnLoad", "(vm)->GetEnv() failed!");
		return -1;
	}
	g_JavaVM = vm;
	
	
//	if (!registerNatives(env))
//	{
//		//зЂВс
//		return -1;
//	}
	
	/* success -- return valid version number */
	return JNI_VERSION_1_6;
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MainListActivity(SetThisObject)
	(JNIEnv* env, jobject thiz)
{
	__android_log_print(ANDROID_LOG_INFO, "viewer_if", "MainListActivity_SetThisObject()");
	g_objMainList = (env)->NewGlobalRef(thiz);
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_ShipList(SetThisObject)
	(JNIEnv* env, jobject thiz)
{
	MAKE_JNI_FUNC_NAME_FOR_MainListActivity(SetThisObject)
	(env, thiz);
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_Profile(SetThisObject)
	(JNIEnv* env, jobject thiz)
{
	MAKE_JNI_FUNC_NAME_FOR_MainListActivity(SetThisObject)
	(env, thiz);
}

extern "C"
void JNI_OnUnload(JavaVM* vm, void* reserved)
{
	__android_log_print(ANDROID_LOG_INFO, "viewer_if", "JNI_OnUnload()");
	
	JNIEnv* env = NULL;
	if ((vm)->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) 
	{
		return;
	}
	if (NULL != g_objMainList)
	{
		(env)->DeleteGlobalRef(g_objMainList);
		g_objMainList = NULL;
	}
}


//
//  0: OK
// -1: NG
int if_get_viewer_nodeid(char *buff, int size)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_JavaVM->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objMainList);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_get_viewer_nodeid", "()Ljava/lang/String;");
	
	jstring str = (jstring)(env)->CallStaticObjectMethod(cls, mid);
	if (NULL == str) {
		if (isAttached) {// From native thread
			g_JavaVM->DetachCurrentThread();
		}
		return -1;
	}
	int len = (env)->GetStringLength(str);
	assert(len < size);
	buff[0] = '\0';
	(env)->GetStringUTFRegion(str, 0, len, buff);
	buff[len] = '\0';
	if (isAttached) {// From native thread
		g_JavaVM->DetachCurrentThread();
	}
	return 0;
}


void if_show_text(char *utf8Text)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_JavaVM->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	__android_log_print(ANDROID_LOG_INFO, "if_show_text", "TEXT = %s", utf8Text);////Debug
	
	jclass cls = (env)->GetObjectClass(g_objMainList);	
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_show_text", "(Ljava/lang/String;)V");
	
	jstring  strText = (env)->NewStringUTF(utf8Text);
	(env)->CallStaticVoidMethod(cls, mid, strText);
	(env)->DeleteLocalRef(strText);
	
	if (isAttached) {// From native thread
		g_JavaVM->DetachCurrentThread();
	}
}


//
//  0: OK
// -1: NG
int if_messagebox(int msg_rid)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_JavaVM->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objMainList);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_messagebox", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, msg_rid);
	if (isAttached) {// From native thread
		g_JavaVM->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_messagetip(int msg_rid)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_JavaVM->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objMainList);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_messagetip", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, msg_rid);
	if (isAttached) {// From native thread
		g_JavaVM->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_progress_show(int msg_rid)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_JavaVM->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objMainList);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_progress_show", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, msg_rid);
	if (isAttached) {// From native thread
		g_JavaVM->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_progress_show_format1(int msg_rid, int arg1)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_JavaVM->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objMainList);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_progress_show_format1", "(II)V");

	(env)->CallStaticVoidMethod(cls, mid, msg_rid, arg1);
	if (isAttached) {// From native thread
		g_JavaVM->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_progress_show_format2(int msg_rid, int arg1, int arg2)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_JavaVM->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objMainList);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_progress_show_format2", "(III)V");

	(env)->CallStaticVoidMethod(cls, mid, msg_rid, arg1, arg2);
	if (isAttached) {// From native thread
		g_JavaVM->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_progress_cancel()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_JavaVM->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objMainList);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_progress_cancel", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_JavaVM->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_on_connected(int type, int fhandle)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_JavaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_JavaVM->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objMainList);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_on_connected", "(II)V");

	(env)->CallStaticVoidMethod(cls, mid, type, fhandle);
	if (isAttached) {// From native thread
		g_JavaVM->DetachCurrentThread();
	}
	return 0;
}
