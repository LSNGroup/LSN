#ifndef _G729A_H_
#define _G729A_H_


#define  L_G729A_FRAME_COMPRESSED 10
#define  L_G729A_FRAME            80



#ifdef __cplusplus
extern "C" {
#endif

void g729a_InitEncoder();
void g729a_Encoder(short *speech, unsigned char *bitstream, int nSize);

void g729a_InitDecoder();
void g729a_Decoder(unsigned char *bitstream, short *synth_short, int bfi, int nSize);/* bfi=0 */

#ifdef __cplusplus
}
#endif


#endif /* _G729A_H_ */
