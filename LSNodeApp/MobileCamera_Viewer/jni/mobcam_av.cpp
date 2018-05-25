#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#ifdef ANDROID_NDK
#include <jni.h>
#include "jni_func.h"
#endif

#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#include "HttpOperate.h"
#include "colorspace.h"
#include "g729a.h"
#include "AudioCodec.h"
#include "H264Send.h"
#include "G729Send.h"
#include "TLVSend.h"
#include "mobcam_if.h"
#include "mobcam_av.h"

#if defined(ANDROID_NDK)
#include <android/log.h>
#else
#define __android_log_print(level, tag, format, ...) printf(format, ## __VA_ARGS__)
#endif


extern HttpOperate myHttpOperate;

static BOOL m_bAVStarted = FALSE;
static BOOL m_bAudioCome = FALSE;

static int m_req_width;
static int m_req_height;
static int m_req_fps;
static BOOL m_bVideoEnable;
static BOOL m_bAudioEnable;
static BOOL m_bVideoReliable;
static BOOL m_bAudioRedundance;
static BOOL m_bVideoH264;
static BOOL m_bVideoHWAcce;
static BOOL m_bAudioG729a;
static BOOL m_bAudioHWAcce;


extern "C"
{
	#include "x264.h"
}
static x264_t *pX264Handle = NULL;
static x264_param_t *pX264Param = NULL;



static unsigned long get_rtp_timestamp()//milliseconds
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	unsigned long ret = tv.tv_sec*1000 + tv.tv_usec/1000;
	ret &= 0x7fffffff;
	return ret;
}

static void init_x264_param(int cx, int cy, int framerate, BOOL videoReliable)
{
	if (NULL == pX264Param) {
		pX264Param = new x264_param_t;
	}
	assert(pX264Param);
	
	// "ultrafast", "superfast", "veryfast", "faster", "fast", "medium",
	// 将编码设置成superfast模式【相比其他模式，会有一些花屏】//将延时设置成最短
	if (cx*cy <= 320*240) {
		x264_param_default_preset(pX264Param, "fast", "zerolatency");
	}
	else if (cx*cy <= 480*320 && framerate <= 15) {
		x264_param_default_preset(pX264Param, "fast", "zerolatency");
	}
	else if (cx*cy == 480*320 && framerate > 15) {
		x264_param_default_preset(pX264Param, "faster", "zerolatency");
	}
	else if (cx*cy > 480*320 && framerate <= 15) {
		x264_param_default_preset(pX264Param, "veryfast", "zerolatency");
	}
	else {
		x264_param_default_preset(pX264Param, "superfast", "zerolatency");
	}
	
	pX264Param->i_bframe = 0;//关闭b帧
	pX264Param->b_cabac = 0;
	pX264Param->i_threads = X264_SYNC_LOOKAHEAD_AUTO;
	
	pX264Param->i_width = cx;
	pX264Param->i_height = cy;
	pX264Param->i_frame_total = 0;
	pX264Param->i_keyint_max = videoReliable ? (framerate * 1) : (framerate / 2);//1秒 0.5秒,关键帧IDR最大间隔 
	pX264Param->b_repeat_headers = 1;// 是否重复SPS/PPS 放到关键帧IDR前面
	
	pX264Param->i_fps_den = 1;//帧率分母
	pX264Param->i_fps_num = framerate;//帧率分子
	pX264Param->i_timebase_den = pX264Param->i_fps_num;
	pX264Param->i_timebase_num = pX264Param->i_fps_den;
	
	pX264Param->rc.f_rf_constant = 25;//实际质量，越大图像越花，越小越清晰
	pX264Param->rc.f_rf_constant_max = 45;//图像质量的最大值
	pX264Param->rc.i_rc_method = X264_RC_ABR;//参数i_rc_method表示码率控制，CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
	double df_bitRate = (cx*cy*3);//单位bps
	pX264Param->rc.i_vbv_max_bitrate=(int)((df_bitRate*1.8f)/1000);  // 平均码率模式下，最大瞬时码率，默认0(与-B设置相同)
	pX264Param->rc.i_bitrate = (int)(df_bitRate/1000);  // x264使用的bitrate需要/1000。x264里面以kbps为单位,ffmpeg以bps为单位
	
	x264_param_apply_profile(pX264Param, x264_profile_names[0]);//baseline profile
}


void DShowAV_Start(BYTE flags, BYTE video_size, BYTE video_framerate, DWORD audio_channel, DWORD video_channel)
{
	if (m_bAVStarted) {
		return;
	}
	
	m_bVideoEnable = (0 != (flags & AV_FLAGS_VIDEO_ENABLE));
	m_bAudioEnable = (0 != (flags & AV_FLAGS_AUDIO_ENABLE));
	m_bVideoReliable = (0 != (flags & AV_FLAGS_VIDEO_RELIABLE));
	m_bAudioRedundance = (0 != (flags & AV_FLAGS_AUDIO_REDUNDANCE));
	m_bVideoH264 = (0 != (flags & AV_FLAGS_VIDEO_H264));
	m_bVideoHWAcce = (0 != (flags & AV_FLAGS_VIDEO_HWACCE));
	m_bAudioG729a = (0 != (flags & AV_FLAGS_AUDIO_G729A));
	m_bAudioHWAcce = (0 != (flags & AV_FLAGS_AUDIO_HWACCE));
	
	if (!m_bVideoEnable && !m_bAudioEnable) {
		return;
	}
	
	m_bAVStarted = TRUE;
	m_bAudioCome = FALSE;
	
	
	int cx=0,cy=0;
	
	switch (video_size)
	{
		case AV_VIDEO_SIZE_QCIF:
			cx = 176;
			cy = 144;
		break;
		case AV_VIDEO_SIZE_QVGA:
			cx = 320;
			cy = 240;
		break;
		case AV_VIDEO_SIZE_CIF:
			cx = 352;
			cy = 288;
		break;
		case AV_VIDEO_SIZE_480x320:
			cx = 480;
			cy = 320;
		break;
		case AV_VIDEO_SIZE_VGA:
			cx = 640;
			cy = 480;
		break;
		default:
			cx = 0;
			cy = 0;
		break;
	}
	
	m_req_width = cx;
	m_req_height = cy;
	m_req_fps = (int)video_framerate;
	
	
	AudioEncoderInit();
	AudioDecoderInit();
	colorspace_init();
	
	
	if (m_bVideoEnable)
	{
		VideoSendStart(g_InConnection1 ? FIRST_CONNECT_PORT : SECOND_CONNECT_PORT, myHttpOperate.m1_use_peer_ip, myHttpOperate.m1_use_peer_port, myHttpOperate.m1_use_udp_sock, myHttpOperate.m1_use_udt_sock, myHttpOperate.m1_use_sock_type);
		VideoSendSetReliable(m_bVideoReliable);
		if (FALSE == m_bVideoHWAcce && TRUE == m_bVideoH264) {
			if_video_capture_start(cx, cy, (int)video_framerate, (int)video_channel);
		}
#ifdef ANDROID_NDK
		else if (TRUE == m_bVideoHWAcce && TRUE == m_bVideoH264) {
			//g_bQuitHWData = FALSE;
			//pthread_create(&g_hHW264DataThread, NULL, HW264DataThreadFn, NULL);
			//g_bSetHWParam = FALSE;
			//if_hw264_capture_start(cx, cy, (int)video_framerate, (int)video_channel);
		}
		else if (TRUE == m_bVideoHWAcce && FALSE == m_bVideoH264) {
			//g_bQuitHWData = FALSE;
			//pthread_create(&g_hHW263DataThread, NULL, HW263DataThreadFn, NULL);
			//g_bSetHWParam = FALSE;
			//if_hw263_capture_start(cx, cy, (int)video_framerate, (int)video_channel);
		}
#endif
	}
	if (m_bAudioEnable)
	{
		AudioSendStart(g_InConnection1 ? FIRST_CONNECT_PORT : SECOND_CONNECT_PORT, myHttpOperate.m1_use_peer_ip, myHttpOperate.m1_use_peer_port, myHttpOperate.m1_use_udp_sock, myHttpOperate.m1_use_udt_sock, myHttpOperate.m1_use_sock_type);
		AudioSendSetCodec(m_bAudioG729a ? AUDIO_CODEC_G729A : AUDIO_CODEC_G721);
		AudioSendSetRedundance(m_bAudioRedundance);
		if (FALSE == m_bAudioHWAcce) if_audio_record_start((int)audio_channel);
	}
	
	//if (m_bTLVEnable)
	{
		TLVSendStart(g_InConnection1 ? FIRST_CONNECT_PORT : SECOND_CONNECT_PORT, myHttpOperate.m1_use_peer_ip, myHttpOperate.m1_use_peer_port, myHttpOperate.m1_use_udp_sock, myHttpOperate.m1_use_udt_sock, myHttpOperate.m1_use_sock_type);
		TLVSendSetRedundance(FALSE);
		if_sensor_capture_start();
	}
}

void DShowAV_Stop()
{
	if (!m_bAVStarted) {
		return;
	}
	m_bAVStarted = FALSE;
	
	//if (m_bTLVEnable)
	{
		if_sensor_capture_stop();
		TLVSendStop();
	}
	
	if (m_bAudioEnable)
	{
		if (FALSE == m_bAudioHWAcce) if_audio_record_stop();
		AudioSendStop();
	}
	if (m_bVideoEnable)
	{
		if (FALSE == m_bVideoHWAcce && TRUE == m_bVideoH264) {
			if_video_capture_stop();
		}
#ifdef ANDROID_NDK
		else if (TRUE == m_bVideoHWAcce && TRUE == m_bVideoH264) {
			//if_hw264_capture_stop();
			//g_bQuitHWData = TRUE;
		}
		else if (TRUE == m_bVideoHWAcce && FALSE == m_bVideoH264) {
			//if_hw263_capture_stop();
			//g_bQuitHWData = TRUE;
		}
#endif
		VideoSendStop();
	}
    
	if (pX264Handle != NULL)
	{
		x264_encoder_close(pX264Handle);
		pX264Handle = NULL;
		
		delete pX264Param;
		pX264Param = NULL;
	}
}

void DShowAV_Switch(DWORD video_channel)
{
	if_video_switch(video_channel);
}

void DShowAV_Contrl(WORD contrl, DWORD contrl_param)
{
	switch (contrl)
	{
		case AV_CONTRL_ZOOM_IN:
			if_contrl_zoom_in();
		break;
		case AV_CONTRL_ZOOM_OUT:
			if_contrl_zoom_out();
		break;
		case AV_CONTRL_RECORDVOL_UP:
			if_contrl_recordvol_up();
		break;
		case AV_CONTRL_RECORDVOL_DOWN:
			if_contrl_recordvol_down();
		break;
		case AV_CONTRL_RECORDVOL_SET:
			if_contrl_recordvol_set(contrl_param);
		break;
		case AV_CONTRL_FLASH_ON:
			if_contrl_flash_on();
		break;
		case AV_CONTRL_FLASH_OFF:
			if_contrl_flash_off();
		break;
		case AV_CONTRL_SYSTEM_REBOOT:
			if_contrl_system_reboot();
		break;
		case AV_CONTRL_SYSTEM_SHUTDOWN:
			if_contrl_system_shutdown();
		break;
		
		case AV_CONTRL_LEFT_SERVO:
			if_contrl_left_servo(contrl_param);
		break;
		case AV_CONTRL_RIGHT_SERVO:
			if_contrl_right_servo(contrl_param);
		break;
		
		case AV_CONTRL_TURN_UP:
			if_contrl_turn_up();
		break;
		case AV_CONTRL_TURN_DOWN:
			if_contrl_turn_down();
		break;
		case AV_CONTRL_TURN_LEFT:
			if_contrl_turn_left();
		break;
		case AV_CONTRL_TURN_RIGHT:
			if_contrl_turn_right();
		break;
		
		case AV_CONTRL_JOYSTICK1:
			if_contrl_joystick1(contrl_param);
		break;
		case AV_CONTRL_JOYSTICK2:
			if_contrl_joystick2(contrl_param);
		break;
		case AV_CONTRL_BUTTON_A:
			if_contrl_button_a();
		break;
		case AV_CONTRL_BUTTON_B:
			if_contrl_button_b();
		break;
		case AV_CONTRL_BUTTON_X:
			if_contrl_button_x();
		break;
		case AV_CONTRL_BUTTON_Y:
			if_contrl_button_y();
		break;
		case AV_CONTRL_BUTTON_L1:
			if_contrl_button_l1(contrl_param);
		break;
		case AV_CONTRL_BUTTON_L2:
			if_contrl_button_l2(contrl_param);
		break;
		case AV_CONTRL_BUTTON_R1:
			if_contrl_button_r1(contrl_param);
		break;
		case AV_CONTRL_BUTTON_R2:
			if_contrl_button_r2(contrl_param);
		break;
		
		default:
		break;
	}
}


void Play_Voice(const BYTE *voice_data, int len)
{
	int nFrame = len / L_G729A_FRAME_COMPRESSED;
	int nFrame2 = 1024*1024 / (L_G729A_FRAME * sizeof(short));
	
	if (nFrame2 < nFrame) {
		nFrame = nFrame2;
	}
	
	if (NULL == voice_data || 0 == nFrame) {
		return;
	}
	
	g729a_InitDecoder();
	
	int len2 = L_G729A_FRAME * sizeof(short) * nFrame;
	BYTE *pOutput = (BYTE *)malloc(len2);////
	g729a_Decoder((unsigned char *)voice_data, (short *)pOutput, 0, nFrame * L_G729A_FRAME_COMPRESSED);
	__android_log_print(ANDROID_LOG_INFO, "mobcam_av", "if_play_pcm_data(%d bytes)...\n", len2);
	if_play_pcm_data(pOutput, len2);
	free(pOutput);////
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  0: OK
// -1: NG
int PutAudioData(unsigned long rtptimestamp, const BYTE *pData, int len) //Format:单声道,16bit,8000Hz
{
	int ret;
	
	if (!m_bAVStarted) {
		return -1;
	}
	
	if (pData == NULL || len <= 0) {
		return -1;
	}
	
	
	unsigned char *pOutput = (unsigned char *)malloc(len/AUDIO_MIN_COMPRESSION);
	int nOutBytes = AudioEncoder(m_bAudioG729a ? AUDIO_CODEC_G729A : AUDIO_CODEC_G721, (unsigned char *)pData, len, pOutput);
	
	
	ret = AudioSendData(rtptimestamp, pOutput, nOutBytes);
	
	free(pOutput);
	return ret;
}


#define PixelFormat_RGB_565			4	//4
#define PixelFormat_RGBA_8888		1	//1
#define PixelFormat_YCbCr_422_SP	16  //16,NV16
#define PixelFormat_YCbCr_420_SP	17	//17,NV21
#define PixelFormat_YCbCr_422_I		20	//20,YUY2

#define PixelFormat_YV12			842094169  //ImageFormat.YV12
#define PixelFormat_32BGRA			0x42475241


//
//  0: OK
// -1: NG
int PutVideoData(unsigned long rtptimestamp, const BYTE *pData, int len, int format, int width, int height, int fps)
{
	int ret = -1;
	int i;
	int iResult;
	int iNal = 0;
	x264_nal_t *pNals = NULL;
	
	if (!m_bAVStarted) {
		return -1;
	}
	
	if (pData == NULL) {
		return -1;
	}
	
	
	{/////////////////////////////////////////////////////////////////////////////////
		__android_log_print(ANDROID_LOG_INFO, "PutVideoData", "Video size: %dx%d %dfps --> %dx%d %dfps\n", m_req_width, m_req_height, m_req_fps, width, height, fps);
		m_req_width = width;
		m_req_height = height;
		m_req_fps = fps;
		
		//重新初始化x264，重新发送sps、pps
		if (FALSE == m_bVideoHWAcce && TRUE == m_bVideoH264)
		{
			int i;
			int iResult;
			int iNal = 0;
			x264_nal_t *pNals = NULL;
			
			if (pX264Handle == NULL)
			{
				init_x264_param(m_req_width, m_req_height, m_req_fps, m_bVideoReliable);
				pX264Handle = x264_encoder_open(pX264Param);
				assert(pX264Handle);
				
				VideoSendSetMediaType(m_req_width, m_req_height, m_req_fps);//H.264 SPS/PPS include width & height info
				
				iResult = x264_encoder_headers(pX264Handle, &pNals, &iNal);
				assert(iResult >= 0);
				for (i = 0; i < iNal; i++)
				{
					if (NAL_SPS == pNals[i].i_type) {
						VideoSendH264Data(0, pNals[i].p_payload, pNals[i].i_payload);
						break;
					}
				}
				for (i = 0; i < iNal; i++)
				{
					if (NAL_PPS == pNals[i].i_type) {
						VideoSendH264Data(0, pNals[i].p_payload, pNals[i].i_payload);
						break;
					}
				}
			}//if
		}
	}/////////////////////////////////////////////////////////////////////////////////
	
	
	int nWidth = width;
	int nHeight = height;
	int plane = nWidth * nHeight;
	
	x264_picture_t* pPicIn = new x264_picture_t;
	x264_picture_t* pPicOut = new x264_picture_t;
	
	x264_picture_init(pPicOut);
	x264_picture_alloc(pPicIn, X264_CSP_I420, pX264Param->i_width, pX264Param->i_height);
	pPicIn->img.i_csp = X264_CSP_I420;
	pPicIn->img.i_plane = 3;
	
	
	if (PixelFormat_YCbCr_420_SP == format)
	{
		memcpy(pPicIn->img.plane[0], pData, plane);/* Y */
		for(int i = 0; i < plane/4; i++)
		{
			/*V*/ *((BYTE *)(pPicIn->img.plane[2]) + i) = pData[plane + i * 2];
			/*U*/ *((BYTE *)(pPicIn->img.plane[1]) + i) = pData[plane + i * 2 + 1];
		}
	}
	else if (PixelFormat_YV12 == format)
	{
		memcpy(pPicIn->img.plane[0], pData,                   plane);  /* Y */
		memcpy(pPicIn->img.plane[2], pData + plane,           plane/4);/* V */
		memcpy(pPicIn->img.plane[1], pData + plane + plane/4, plane/4);/* U */
	}
	else if (PixelFormat_YCbCr_422_SP == format)
	{
		int row, volume;
		memcpy(pPicIn->img.plane[0], pData, plane);/* Y */
		for(int i = 0; i < plane/4; i++)
		{
			row = i/(nWidth/2);
			volume = i%(nWidth/2);
			/*V*/ *((BYTE *)(pPicIn->img.plane[2]) + i) = ( pData[plane + row * 2 * nWidth + 2 * volume]      +  pData[plane + (row * 2 + 1) * nWidth + 2 * volume]      )/2;
			/*U*/ *((BYTE *)(pPicIn->img.plane[1]) + i) = ( pData[plane + row * 2 * nWidth + 2 * volume + 1]  +  pData[plane + (row * 2 + 1) * nWidth + 2 * volume + 1]  )/2;
		}
	}
	else if (PixelFormat_YCbCr_422_I == format)//YUY2
	{
		yuyv_to_yv12_c((__u8 *)pData, nWidth * 2, 
						pPicIn->img.plane[0], /* Y */
						pPicIn->img.plane[2], /* V */
						pPicIn->img.plane[1], /* U */
						nWidth, (nWidth >> 1), 
						nWidth, nHeight, 0);
	}
	else if (PixelFormat_RGBA_8888 == format)
	{
		rgba_to_yv12_c((__u8 *)pData, nWidth * 4, 
						pPicIn->img.plane[0], /* Y */
						pPicIn->img.plane[2], /* V */
						pPicIn->img.plane[1], /* U */
					   nWidth, (nWidth >> 1), 
					   nWidth, nHeight, 0);
	}
	else if (PixelFormat_32BGRA == format)
	{
		bgra_to_yv12_c((__u8 *)pData, nWidth * 4, 
						pPicIn->img.plane[0], /* Y */
						pPicIn->img.plane[2], /* V */
						pPicIn->img.plane[1], /* U */
						nWidth, (nWidth >> 1), 
						nWidth, nHeight, 0);
	}
	else if (PixelFormat_RGB_565 == format)
	{
		rgb565_to_yv12_c((__u8 *)pData, nWidth * 2, 
						pPicIn->img.plane[0], /* Y */
						pPicIn->img.plane[2], /* V */
						pPicIn->img.plane[1], /* U */
						nWidth, (nWidth >> 1), 
						nWidth, nHeight, 0);
	}
	else {
		x264_picture_clean(pPicIn);
		//x264_picture_clean(pPicOut);
		delete pPicIn;
		delete pPicOut;
		return -1;
	}
	
	iResult = x264_encoder_encode(pX264Handle, &pNals, &iNal, pPicIn, pPicOut);
	if (iResult > 0)
	{
		for (i = 0; i < iNal; i++)
		{
			ret = VideoSendH264Data(rtptimestamp, pNals[i].p_payload, pNals[i].i_payload);
		}
		if (iNal > 1)
		{
			__android_log_print(ANDROID_LOG_INFO, "PutVideoData", "x264_encoder_encode() return %d NALs\n", iNal);
		}
	}
	
	x264_picture_clean(pPicIn);
	//x264_picture_clean(pPicOut);
	delete pPicIn;
	delete pPicOut;
	return ret;
}


#ifdef ANDROID_NDK

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(TLVSendUpdateValue)
	(JNIEnv* env, jobject thiz, jint tlv_type, jdouble val, jboolean send_now)
{
	TLVSendUpdateValue(tlv_type, (double)val);
	if (send_now) {
		TLVSendData();
	}
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(PutAudioData)
	(JNIEnv* env, jobject thiz, jbyteArray data, jint len)
{
	int ret;
	
	jbyte *pData = (env)->GetByteArrayElements(data, NULL);
	if (pData == NULL) {
		return -1;
	}
	
	m_bAudioCome = TRUE;
	
	ret = PutAudioData(get_rtp_timestamp() - len/16, (const BYTE *)pData, len);
	
	(env)->ReleaseByteArrayElements(data, pData, 0);
	return ret;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(PutVideoData)
	(JNIEnv* env, jobject thiz, jbyteArray data, jint len, jint format, jint width, jint height, jint fps)
{
	int ret;
	
	jbyte *pData = (env)->GetByteArrayElements(data, NULL);
	if (pData == NULL) {
		return -1;
	}
	
	if (TRUE == m_bAudioEnable && FALSE == m_bAudioCome)
	{
		BYTE fake_audio[640];//g729编码,每次只能编码160个字节,编码后为10个字节大小,16:1的压缩比
		memset(fake_audio, 0, sizeof(fake_audio));
		PutAudioData(get_rtp_timestamp() - sizeof(fake_audio)/16, fake_audio, sizeof(fake_audio));
	}
	
	ret = PutVideoData(get_rtp_timestamp(), (const BYTE *)pData, len, format, width, height, fps);
	
	(env)->ReleaseByteArrayElements(data, pData, 0);
	return ret;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(SetHWVideoParam)
	(JNIEnv* env, jobject thiz, jint width, jint height, jint fps)
{
	if (width != m_req_width || height != m_req_height || fps != m_req_fps)
	{
		__android_log_print(ANDROID_LOG_INFO, "SetHWVideoParam", "Video size changed: %dx%d %dfps --> %dx%d %dfps\n", m_req_width, m_req_height, m_req_fps, width, height, fps);
		m_req_width = width;
		m_req_height = height;
		m_req_fps = fps;
	}
	
	//g_bSetHWParam = TRUE;
	return 0;
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(AvSwitchToLocalStream)
	(JNIEnv* env, jobject thiz)
{
	if (FALSE == m_bVideoHWAcce && TRUE == m_bVideoH264)
	{
		if (pX264Handle != NULL)
		{
			x264_encoder_close(pX264Handle);
			pX264Handle = NULL;
			
			delete pX264Param;
			pX264Param = NULL;
		}
		
		
		int i;
		int iResult;
		int iNal = 0;
		x264_nal_t *pNals = NULL;
		
		if (pX264Handle == NULL)
		{
			init_x264_param(m_req_width, m_req_height, m_req_fps, m_bVideoReliable);
			pX264Handle = x264_encoder_open(pX264Param);
			assert(pX264Handle);
			
			VideoSendSetMediaType(m_req_width, m_req_height, m_req_fps);//H.264 SPS/PPS include width & height info
			
			iResult = x264_encoder_headers(pX264Handle, &pNals, &iNal);
			assert(iResult >= 0);
			for (i = 0; i < iNal; i++)
			{
				if (NAL_SPS == pNals[i].i_type) {
					VideoSendH264Data(0, pNals[i].p_payload, pNals[i].i_payload);
					break;
				}
			}
			for (i = 0; i < iNal; i++)
			{
				if (NAL_PPS == pNals[i].i_type) {
					VideoSendH264Data(0, pNals[i].p_payload, pNals[i].i_payload);
					break;
				}
			}
		}//if
	}
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(AvSwitchToRtpStream)
	(JNIEnv* env, jobject thiz)
{
	VideoSendSetMediaType(m_req_width, m_req_height, m_req_fps);//以此方式，重置接收端的H264解码器！
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(PutRtpAudioData)
	(JNIEnv* env, jobject thiz, jbyteArray data, jint offset, jint len, jint ptype)
{
	int ret;
	
	if (!m_bAudioEnable) {
		return -1;
	}
	
	jbyte *pData = (env)->GetByteArrayElements(data, NULL);
	if (pData == NULL) {
		return -1;
	}
	
	ret = AudioSendData(get_rtp_timestamp(), (const BYTE *)(pData + offset), len);
	
	(env)->ReleaseByteArrayElements(data, pData, 0);
	return ret;
}

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(PutRtpVideoData)
	(JNIEnv* env, jobject thiz, jbyteArray data, jint offset, jint len, jint ptype)
{
	int ret;
	
	if (!m_bVideoEnable) {
		return -1;
	}
	
	jbyte *pData = (env)->GetByteArrayElements(data, NULL);
	if (pData == NULL) {
		return -1;
	}
	
	ret = VideoSendH264Data(get_rtp_timestamp(), (const BYTE *)(pData + offset), len);
	
	(env)->ReleaseByteArrayElements(data, pData, 0);
	return ret;
}

#endif
