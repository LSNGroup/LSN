#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"
#include "g729a.h"
#include "g72x.h"
#include "AudioCodec.h"


static 		struct g72x_state	enc_state;
static 		struct g72x_state	dec_state;


void AudioEncoderInit()
{
	memset(&enc_state, 0, sizeof(enc_state));
	g72x_init_state(&enc_state);
	
	g729a_InitEncoder();
}

void AudioDecoderInit()
{
	memset(&dec_state, 0, sizeof(dec_state));
	g72x_init_state(&dec_state);
	
	g729a_InitDecoder();
}

int AudioEncoder(BYTE type, unsigned char *inBuff, int inBytes, unsigned char *outBuff)
{
	if (AUDIO_CODEC_G729A == type) {
		int nFrame = inBytes / (L_G729A_FRAME * sizeof(short));
		g729a_Encoder((short *)inBuff, outBuff, nFrame * L_G729A_FRAME);
		return nFrame * L_G729A_FRAME_COMPRESSED;
	}
	else {
		short			sample0, sample1;
		unsigned char		code0, code1;
		int i;
		
		for (i = 0; i < inBytes/4; i++) {
			sample0 = (inBuff[i*4] & 0x00ff) | ((inBuff[i*4 + 1] << 8) & 0xff00);
			sample1 = (inBuff[i*4 + 2] & 0x00ff) | ((inBuff[i*4 + 3] << 8) & 0xff00);
			code0 = g721_encoder(sample0, AUDIO_ENCODING_LINEAR, &enc_state);
			code1 = g721_encoder(sample1, AUDIO_ENCODING_LINEAR, &enc_state);
			outBuff[i] = (code0 & 0x0f) | ((code1 << 4) & 0xf0);
		}
		return inBytes/4;
	}
}

int AudioDecoder(BYTE type, unsigned char *inBuff, int inBytes, unsigned char *outBuff)
{
	if (AUDIO_CODEC_G729A == type) {
		int nFrame = inBytes / L_G729A_FRAME_COMPRESSED;
		g729a_Decoder(inBuff, (short *)outBuff, 0, nFrame * L_G729A_FRAME_COMPRESSED);
		return L_G729A_FRAME * sizeof(short) * nFrame;
	}
	else {
		short			sample0, sample1;
		unsigned char		code0, code1;
		int i;
		
		for (i = 0; i < inBytes; i++) {
			code0 = inBuff[i] & 0x0f;
			code1 = (inBuff[i] >> 4) & 0x0f;
			sample0 = g721_decoder(code0, AUDIO_ENCODING_LINEAR, &dec_state);
			sample1 = g721_decoder(code1, AUDIO_ENCODING_LINEAR, &dec_state);
			outBuff[i*4] = sample0 & 0xff;
			outBuff[i*4 + 1] = (sample0 >> 8) & 0xff;
			outBuff[i*4 + 2] = sample1 & 0xff;
			outBuff[i*4 + 3] = (sample1 >> 8) & 0xff;
		}
		
		return inBytes * 4;
	}
}
