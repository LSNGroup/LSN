
static char regResultBuff[1024];


extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(MayiLoginUser)
	(JNIEnv* env, jobject thiz, jstring strUsername, jstring strPassword)
{
	int ret;
	const char *username;
	const char *password;
	
	username = (env)->GetStringUTFChars(strUsername, NULL);
	password = (env)->GetStringUTFChars(strPassword, NULL);
	
	ret = MayiLoginUser(g_client_charset, g_client_lang, username, password);
	
	(env)->ReleaseStringUTFChars(strUsername, username);
	(env)->ReleaseStringUTFChars(strPassword, password);
    return ret;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(MayiUseAck)
	(JNIEnv* env, jobject thiz, jint use_id, jint guaji_id, jstring strGuajiOwnerUsername)
{
	int ret;
	const char *guaji_owner_username;
	
	guaji_owner_username = (env)->GetStringUTFChars(strGuajiOwnerUsername, NULL);
	
	ret = MayiUseAck(g_client_charset, g_client_lang, (DWORD)use_id, (DWORD)guaji_id, guaji_owner_username);
	
	(env)->ReleaseStringUTFChars(strGuajiOwnerUsername, guaji_owner_username);
    return ret;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(MayiUsePay)
	(JNIEnv* env, jobject thiz, jint use_id, jint user_id, jstring strUserUsername, jint guaji_id, jstring strGuajiOwnerUsername)
{
	int ret;
	const char *user_username;
	const char *guaji_owner_username;
	
	user_username = (env)->GetStringUTFChars(strUserUsername, NULL);
	guaji_owner_username = (env)->GetStringUTFChars(strGuajiOwnerUsername, NULL);
	
	ret = MayiUsePay(g_client_charset, g_client_lang, (DWORD)use_id, (DWORD)user_id, user_username, (DWORD)guaji_id, guaji_owner_username);
	
	(env)->ReleaseStringUTFChars(strUserUsername, user_username);
	(env)->ReleaseStringUTFChars(strGuajiOwnerUsername, guaji_owner_username);
    return ret;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(MayiFetchScore)
	(JNIEnv* env, jobject thiz, jstring strUsername, jint guaji_id, jintArray arrResult)
{
	int ret;
	const char *username;
	int user_score,cash_quota,jifen_duihuan_jiage;
	jint arr[3];
	
	username = (env)->GetStringUTFChars(strUsername, NULL);
	
	ret = MayiFetchScore(g_client_charset, g_client_lang, username, (DWORD)guaji_id, &user_score, &cash_quota, &jifen_duihuan_jiage);
	if (ret > 0) {
		arr[0] = user_score;
		arr[1] = cash_quota;
		arr[2] = jifen_duihuan_jiage;
		(env)->SetIntArrayRegion(arrResult, 0, 3, arr);
	}
	
	(env)->ReleaseStringUTFChars(strUsername, username);
    return ret;
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetLocalGuajiInfo)
	(JNIEnv* env, jobject thiz)
{
	GetLocalGuajiInfo();
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(MayiRegister)
	(JNIEnv* env, jobject thiz, jboolean bIsOnline, jint owner_user_id, jstring strOwnerUsername)
{
	int ret;
	const char *owner_username;
	
	owner_username = (env)->GetStringUTFChars(strOwnerUsername, NULL);
	
	ret = MayiRegister(g_client_charset, g_client_lang, (BOOL)bIsOnline, (DWORD)owner_user_id, owner_username, regResultBuff, sizeof(regResultBuff));
	
	(env)->ReleaseStringUTFChars(strOwnerUsername, owner_username);
    return ret;
}

extern "C"
jstring
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(MayiRegisterResult)
	(JNIEnv* env, jobject thiz)
{
	return (env)->NewStringUTF(regResultBuff);
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(MayiUnregister)
	(JNIEnv* env, jobject thiz)
{
	int ret = MayiUnregister(g_client_charset, g_client_lang);
    return ret;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(MayiQueryUseList)
	(JNIEnv* env, jobject thiz, jint user_id, jint guaji_id)
{
	jint ret;
	int i;
	int numUse;
	int countUse = NODES_PER_PAGE;
	USE_NODE *useNodes;//[NODES_PER_PAGE];
	
	useNodes  = (USE_NODE *)malloc( sizeof(USE_NODE)*NODES_PER_PAGE );
	
	ret = MayiQueryUseList(g_client_charset, g_client_lang, user_id, guaji_id, useNodes, &countUse, &numUse);
	if (ret < 0) {
		countUse = 0;
	}
	
	if (countUse > 0)
	{
		__android_log_print(ANDROID_LOG_INFO, "MayiQueryUseList", "jobject = %ld", (unsigned long)thiz);////Debug
		
		jclass cls = (env)->GetObjectClass(thiz);
		__android_log_print(ANDROID_LOG_INFO, "MayiQueryUseList", "jclass1 = %ld", (unsigned long)cls);////Debug
		
		if (0 == cls) {
			cls = (env)->FindClass("com/wangling/remotephone/MobileCameraService");
			__android_log_print(ANDROID_LOG_INFO, "MayiQueryUseList", "jclass2 = %ld", (unsigned long)cls);////Debug
		}
		
		jmethodID mid = (env)->GetMethodID(cls, "FillUseNode", "(IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
		__android_log_print(ANDROID_LOG_INFO, "MayiQueryUseList", "jmethodID = %ld", (unsigned long)mid);////Debug
		
		for (i = 0; i < (int)countUse; i++)
		{
			__android_log_print(ANDROID_LOG_INFO, "MayiQueryUseList", "FillUseNode(index = %d)...", i);////Debug
			
			USE_NODE *pNode = &useNodes[i];
			(env)->CallVoidMethod(thiz, mid, 
									i, pNode->id, (env)->NewStringUTF(pNode->user_username), (env)->NewStringUTF(pNode->guaji_owner_username)
			, (env)->NewStringUTF(pNode->platform_type), (env)->NewStringUTF(pNode->sub_account), (env)->NewStringUTF(pNode->goods_category)
			, (env)->NewStringUTF(pNode->shop_name), (env)->NewStringUTF(pNode->order_num), (env)->NewStringUTF(pNode->order_money)
			, (env)->NewStringUTF(pNode->str_buy_time), (env)->NewStringUTF(pNode->str_review_time), (env)->NewStringUTF(pNode->review_content)
			, (env)->NewStringUTF(pNode->review_extra), (env)->NewStringUTF(pNode->str_ack_time), (env)->NewStringUTF(pNode->str_op_status) );
		
		}
	}
	
	free(useNodes);
    return countUse;
}

extern "C"
jstring
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetLoginResult)
	(JNIEnv* env, jobject thiz)
{
	return (env)->NewStringUTF(g1_login_result);
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetUserId)
	(JNIEnv* env, jobject thiz)
{
	return (jint)g1_user_id;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetGuajiRegisterPeriod)
	(JNIEnv* env, jobject thiz)
{
	return (jint)g1_guaji_register_period;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetUseRegisterPeriod)
	(JNIEnv* env, jobject thiz)
{
	return (jint)g1_use_register_period;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetGuajiId)
	(JNIEnv* env, jobject thiz)
{
	return (jint)g1_guaji_id;
}

extern "C"
jstring
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetSubAccounts)
	(JNIEnv* env, jobject thiz)
{
	return (env)->NewStringUTF(g1_sub_accounts);
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetTotalUsedTimes)
	(JNIEnv* env, jobject thiz)
{
	return (jint)g1_total_used_times;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetTotalComplainedTimes)
	(JNIEnv* env, jobject thiz)
{
	return (jint)g1_total_complained_times;
}

extern "C"
jstring
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetAuthInfo)
	(JNIEnv* env, jobject thiz)
{
	return (env)->NewStringUTF(g1_auth_info);
}

extern "C"
jstring
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetCommentsInfo)
	(JNIEnv* env, jobject thiz)
{
	return (env)->NewStringUTF(g1_comments_info);
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(GetTotalGuajiTime)
	(JNIEnv* env, jobject thiz)
{
	return (jint)g1_total_guaji_time;
}
