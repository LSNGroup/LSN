#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <jni.h>
#include "jni_func.h"
#include "platform.h"
#include "CommonLib.h"
#include "mobcam_if.h"

#include <android/log.h>


static JavaVM* g_vm = NULL;
static jobject g_objCam = NULL;



extern "C"
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "JNI_OnLoad()");
	
	if ((vm)->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) 
	{
		__android_log_print(ANDROID_LOG_INFO, "JNI_OnLoad", "(vm)->GetEnv() failed!");
		return -1;
	}
	g_vm = vm;
	
	
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
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(SetThisObject)
	(JNIEnv* env, jobject thiz)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "MobileCameraActivity_SetThisObject()");
	g_objCam = (env)->NewGlobalRef(thiz);
}

extern "C"
void JNI_OnUnload(JavaVM* vm, void* reserved)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "JNI_OnUnload()");
	
	JNIEnv* env = NULL;
	if ((vm)->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) 
	{
		return;
	}
	if (NULL != g_objCam)
	{
		(env)->DeleteGlobalRef(g_objCam);
		g_objCam = NULL;
	}
}


void if_on_push_result(int ret, int joined_channel_id)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_on_push_result", "(II)V");

	(env)->CallStaticVoidMethod(cls, mid, ret, joined_channel_id);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_on_register_network_error()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_on_register_network_error", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

int if_get_audio_channels()
{
	return 1;
}

int if_get_video_channels()
{
	return 2;
}

//
//  0: OK
// -1: NG
int if_get_device_uuid(char *buff, int size)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	__android_log_print(ANDROID_LOG_INFO, "if_get_device_uuid", "jclass = %ld", (unsigned long)cls);////Debug
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_get_device_uuid", "()Ljava/lang/String;");
	__android_log_print(ANDROID_LOG_INFO, "if_get_device_uuid", "jmethodID = %ld", (unsigned long)mid);////Debug
	
	jstring str = (jstring)(env)->CallStaticObjectMethod(cls, mid);
	if (NULL == str) {
		if (isAttached) {// From native thread
			g_vm->DetachCurrentThread();
		}
		return -1;
	}
	int len = (env)->GetStringLength(str);
	assert(len < size);
	buff[0] = '\0';
	(env)->GetStringUTFRegion(str, 0, len, buff);
	buff[len] = '\0';
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_get_nodeid(char *buff, int size)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_get_nodeid", "()Ljava/lang/String;");
	
	jstring str = (jstring)(env)->CallStaticObjectMethod(cls, mid);
	if (NULL == str) {
		if (isAttached) {// From native thread
			g_vm->DetachCurrentThread();
		}
		return -1;
	}
	int len = (env)->GetStringLength(str);
	assert(len < size);
	buff[0] = '\0';
	(env)->GetStringUTFRegion(str, 0, len, buff);
	buff[len] = '\0';
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_read_password(char *buff, int size)//orig pass
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_read_password", "()Ljava/lang/String;");
	
	jstring str = (jstring)(env)->CallStaticObjectMethod(cls, mid);
	if (NULL == str) {
		if (isAttached) {// From native thread
			g_vm->DetachCurrentThread();
		}
		return -1;
	}
	int len = (env)->GetStringUTFLength(str);
	assert(len < size);
	buff[0] = '\0';
	(env)->GetStringUTFRegion(str, 0, (env)->GetStringLength(str), buff);
	buff[len] = '\0';
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_read_nodename(char *buff, int size)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_read_nodename", "()Ljava/lang/String;");
	
	jstring str = (jstring)(env)->CallStaticObjectMethod(cls, mid);
	if (NULL == str) {
		if (isAttached) {// From native thread
			g_vm->DetachCurrentThread();
		}
		return -1;
	}
	int len = (env)->GetStringUTFLength(str);
	assert(len < size);
	buff[0] = '\0';
	(env)->GetStringUTFRegion(str, 0, (env)->GetStringLength(str), buff);
	buff[len] = '\0';
	
	char *p;
	p = strchr(buff, '.');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '.');
	}
	p = strchr(buff, '-');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '-');
	}
	p = strchr(buff, '_');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '_');
	}
	
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_get_osinfo(char *buff, int size)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_get_osinfo", "()Ljava/lang/String;");
	
	jstring str = (jstring)(env)->CallStaticObjectMethod(cls, mid);
	if (NULL == str) {
		if (isAttached) {// From native thread
			g_vm->DetachCurrentThread();
		}
		return -1;
	}
	int len = (env)->GetStringUTFLength(str);
	assert(len < size);
	buff[0] = '\0';
	(env)->GetStringUTFRegion(str, 0, (env)->GetStringLength(str), buff);
	buff[len] = '\0';
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return 0;
}


int if_video_capture_start(int width, int height, int fps, int channel)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_video_capture_start", "(IIII)I");

	int ret = (env)->CallStaticIntMethod(cls, mid, width, height, fps, channel);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return ret;
}

void if_video_capture_stop()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_video_capture_stop", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

int if_hw264_capture_start(int width, int height, int fps, int channel)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_hw264_capture_start", "(IIII)I");

	int ret = (env)->CallStaticIntMethod(cls, mid, width, height, fps, channel);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return ret;
}

void if_hw264_capture_stop()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_hw264_capture_stop", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

int if_hw263_capture_start(int width, int height, int fps, int channel)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_hw263_capture_start", "(IIII)I");

	int ret = (env)->CallStaticIntMethod(cls, mid, width, height, fps, channel);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return ret;
}

void if_hw263_capture_stop()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_hw263_capture_stop", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

int if_audio_record_start(int channel)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_audio_record_start", "(I)I");

	int ret = (env)->CallStaticIntMethod(cls, mid, channel);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return ret;
}

void if_audio_record_stop()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_audio_record_stop", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

int if_sensor_capture_start()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_sensor_capture_start", "()I");

	int ret = (env)->CallStaticIntMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return ret;
}

void if_sensor_capture_stop()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_sensor_capture_stop", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_play_pcm_data(const BYTE *pcm_data, int len)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	__android_log_print(ANDROID_LOG_INFO, "if_play_pcm_data", "jclass = %ld", (unsigned long)cls);////Debug
	
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_play_pcm_data", "([B)V");
	__android_log_print(ANDROID_LOG_INFO, "if_play_pcm_data", "jmethodID = %ld", (unsigned long)mid);////Debug
	
	jbyteArray jcarr = (env)->NewByteArray(len);
	(env)->SetByteArrayRegion(jcarr, 0, len, (const jbyte *)pcm_data);
	
	(env)->CallStaticVoidMethod(cls, mid, jcarr);
	
	(env)->DeleteLocalRef(jcarr);
	
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_video_switch(int param)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_video_switch(%d)", param);
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_video_switch", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, param);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_zoom_in()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_zoom_in()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_zoom_in", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_zoom_out()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_zoom_out()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_zoom_out", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_recordvol_up()
{
	
}

void if_contrl_recordvol_down()
{
	
}

void if_contrl_recordvol_set(int param)
{
	
}

void if_contrl_flash_on()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_flash_on()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_flash_on", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_flash_off()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_flash_off()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_flash_off", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_left_servo(int param)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_left_servo(%d)", param);
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_left_servo", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, param);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_right_servo(int param)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_right_servo(%d)", param);
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_right_servo", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, param);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_take_picture()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_take_picture()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_take_picture", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_turn_up()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_turn_up()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_turn_up", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_turn_down()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_turn_down()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_turn_down", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_turn_left()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_turn_left()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_turn_left", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_turn_right()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_turn_right()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_turn_right", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_joystick1(int param)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_joystick1(0x%lx)", param);
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_joystick1", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, param);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_joystick2(int param)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_joystick2(0x%lx)", param);
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_joystick2", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, param);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_button_a()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_button_a()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_button_a", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_button_b()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_button_b()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_button_b", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_button_x()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_button_x()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_button_x", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_button_y()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_button_y()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_button_y", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_button_l1(int param)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_button_l1(0x%lx)", param);
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_button_l1", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, param);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_button_l2(int param)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_button_l2(0x%lx)", param);
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_button_l2", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, param);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_button_r1(int param)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_button_r1(0x%lx)", param);
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_button_r1", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, param);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_button_r2(int param)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_button_r2(0x%lx)", param);
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_button_r2", "(I)V");

	(env)->CallStaticVoidMethod(cls, mid, param);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_mc_arm()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_mc_arm()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_mc_arm", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_mc_disarm()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_mc_disarm()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_mc_disarm", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_system_reboot()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_system_reboot()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_system_reboot", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_contrl_system_shutdown()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_contrl_system_shutdown()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_contrl_system_shutdown", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_on_client_connected()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_on_client_connected", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_on_client_disconnected()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_on_client_disconnected", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

BOOL if_should_do_upnp()
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return FALSE;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_should_do_upnp", "()Z");

	jboolean ret = (env)->CallStaticBooleanMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return (BOOL)ret;
}

void if_on_mavlink_start()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_on_mavlink_start()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_on_mavlink_start", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_on_mavlink_stop()
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_on_mavlink_stop()");
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_on_mavlink_stop", "()V");

	(env)->CallStaticVoidMethod(cls, mid);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

void if_on_mavlink_guid(float lati, float longi, float alti)
{
	__android_log_print(ANDROID_LOG_INFO, "mobcam_if", "if_on_mavlink_guid(%f,%f,%f)", lati, longi, alti);
	
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;

	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_on_mavlink_guid", "(FFF)V");

	(env)->CallStaticVoidMethod(cls, mid, lati, longi, alti);
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
}

//
//  0: OK
// -1: NG
int if_get_wp_data(WP_ITEM **lpItems, int *lpNum)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;
	
	if (NULL == lpItems || NULL == lpNum)
	{
		return -1;
	}
	*lpItems = NULL;
	*lpNum = 0;
	
	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_get_wp_data", "()[B");
	
	jbyteArray arr = (jbyteArray)(env)->CallStaticObjectMethod(cls, mid);
	if (NULL == arr) {
		if (isAttached) {// From native thread
			g_vm->DetachCurrentThread();
		}
		return -1;
	}
	
	int len = (env)->GetArrayLength(arr);
	if (len > 0)
	{
		int nNum = len/20;
		jbyte* ba = (env)->GetByteArrayElements(arr, 0);
		WP_ITEM *p = (WP_ITEM *)malloc(nNum * sizeof(WP_ITEM));
		//memcpy(p, ba, len);
		for (int index = 0; index < nNum; index ++)
		{
			p[index].wpFlags  = ntohl(pf_get_dword(ba + 20*index));
			p[index].wpType   = ntohl(pf_get_dword(ba + 20*index + 4));
			p[index].wpLati   = ntohl(pf_get_dword(ba + 20*index + 8));
			p[index].wpLongi  = ntohl(pf_get_dword(ba + 20*index + 12));
			p[index].wpAlti   = ntohl(pf_get_dword(ba + 20*index + 16));
		}
		(env)->ReleaseByteArrayElements(arr, ba, 0);
		*lpItems = p;
		*lpNum = nNum;
	}
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return 0;
}

//
//  0: OK
// -1: NG
int if_get_tlv_data(BYTE **lpPtr, int *lpLen)//caller free(*lpPtr)
{
	int status;
	bool isAttached = false;
	JNIEnv *env = NULL;
	
	if (NULL == lpPtr || NULL == lpLen)
	{
		return -1;
	}
	*lpPtr = NULL;
	*lpLen = 0;
	
	status = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if(status != JNI_OK)
	{
		status = g_vm->AttachCurrentThread(&env, NULL);
		if(status != JNI_OK)
		{
			return -1;
		}
		isAttached = true;
	}
	
	
	jclass cls = (env)->GetObjectClass(g_objCam);
	jmethodID mid = (env)->GetStaticMethodID(cls, "j_get_tlv_data", "()[B");
	
	jbyteArray arr = (jbyteArray)(env)->CallStaticObjectMethod(cls, mid);
	if (NULL == arr) {
		if (isAttached) {// From native thread
			g_vm->DetachCurrentThread();
		}
		return -1;
	}
	
	int len = (env)->GetArrayLength(arr);
	if (len > 0)
	{
		jbyte* ba = (env)->GetByteArrayElements(arr, 0);
		unsigned char *p = (unsigned char *)malloc(len);
		memcpy(p, ba, len);
		(env)->ReleaseByteArrayElements(arr, ba, 0);
		*lpPtr = p;
		*lpLen = len;
	}
	if (isAttached) {// From native thread
		g_vm->DetachCurrentThread();
	}
	return 0;
}
