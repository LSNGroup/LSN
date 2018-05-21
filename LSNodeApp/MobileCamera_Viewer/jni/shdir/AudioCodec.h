#ifndef _AUDIOCODEC_H
#define _AUDIOCODEC_H


typedef enum _tag_audio_codec_type {
	AUDIO_CODEC_G721 = 1,
	AUDIO_CODEC_G723_24,
	AUDIO_CODEC_G723_40,
	AUDIO_CODEC_G729A,
} AUDIO_CODEC_TYPE;


#define AUDIO_MIN_COMPRESSION	2
#define AUDIO_MAX_COMPRESSION	25



void AudioEncoderInit();
void AudioDecoderInit();

int AudioEncoder(BYTE type, unsigned char *inBuff, int inBytes, unsigned char *outBuff);
int AudioDecoder(BYTE type, unsigned char *inBuff, int inBytes, unsigned char *outBuff);


#endif /* _AUDIOCODEC_H */
