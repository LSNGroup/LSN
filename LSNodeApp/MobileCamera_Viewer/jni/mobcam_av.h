#ifndef _MOBCAM_AV_H_
#define _MOBCAM_AV_H_

extern BOOL g_bDoConnection1;
extern BOOL g_InConnection1;
extern BOOL g_InConnection2;


void DShowAV_Start(BYTE flags, BYTE video_size, BYTE video_framerate, DWORD audio_channel, DWORD video_channel);
void DShowAV_Stop();
void DShowAV_Switch(DWORD video_channel);
void DShowAV_Contrl(WORD contrl, DWORD contrl_param);

void Play_Voice(const BYTE *voice_data, int len);



//
//  0: OK
// -1: NG
int PutAudioData(unsigned long rtptimestamp, const BYTE *pData, int len); //Format:µ¥ÉùµÀ,16bit,8000Hz

//
//  0: OK
// -1: NG
int PutVideoData(unsigned long rtptimestamp, const BYTE *pData, int len, int format, int width, int height, int fps); //Format:YUV420p (YYYYUV)



#endif /* _MOBCAM_AV_H_ */
