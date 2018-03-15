#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>

#ifdef ANDROID_NDK
#include <android/log.h>
#endif


extern "C"
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	
	__android_log_print(ANDROID_LOG_INFO, "up2p_jni", "JNI_OnLoad()");
	
	if ((vm)->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) 
	{
		__android_log_print(ANDROID_LOG_INFO, "JNI_OnLoad", "(vm)->GetEnv() failed!");
		return -1;
	}
	
	
	/* success -- return valid version number */
	return JNI_VERSION_1_6;
}

