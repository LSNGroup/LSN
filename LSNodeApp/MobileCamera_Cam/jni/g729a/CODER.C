/*
   ITU-T G.729A Speech Coder    ANSI-C Source Code
   Version 1.1    Last modified: September 1996

   Copyright (c) 1996,
   AT&T, France Telecom, NTT, Universite de Sherbrooke
   All rights reserved.
*/

/*-------------------------------------------------------------------*
 * Main program of the ITU-T G.729A  8 kbit/s encoder.               *
 *                                                                   *
 *    Usage : coder speech_file  bitstream_file                      *
 *-------------------------------------------------------------------*/

//#include <stdio.h>
//#include <stdlib.h>
#include <memory.h>

#include "typedef.h"
#include "basic_op.h"
#include "ld8a.h"


Word16 bad_lsf;        /* bad LSF indicator   */


#define  L_FRAME_COMPRESSED 10
#define  L_FRAME            80


void g729a_InitEncoder()
{
	Init_Pre_Process();
	Init_Coder_ld8a();
}
void g729a_Encoder(Word16 *speech, unsigned char *bitstream, int nSize)
{
	//FILE *f_speech;               /* File of speech data                   */
	//FILE *f_prm;               /* File of serial bits for transmission  */

	extern Word16 *new_speech;     /* Pointer to new speech data            */

	Word16 prm[PRM_SIZE];          /* Analysis parameters.                  */
	//Word16 serial[SERIAL_SIZE];    /* Output bitstream buffer               */

	//Word16 frame;                  /* frame counter */
	int i;

	memset(prm,0,sizeof(prm));

	//Set_zero(prm, PRM_SIZE);

	/* Loop for each "L_FRAME" speech data. */

	//frame =0;

	//if ( (f_prm = fopen("c:\\write.prm", "wb")) == NULL) {
	//	return;
	//}
	for( i = 0; i<nSize; i+= L_FRAME)
	{
		memcpy(new_speech,speech+i,L_FRAME * sizeof(Word16));

		Pre_Process(new_speech, L_FRAME);

		Coder_ld8a(prm);
		memset(bitstream+i/8,0,10);
		prm2bits_ld8k( prm, (Word16 *)(bitstream+i/8));
		//fwrite(prm, sizeof(short), PRM_SIZE, f_prm);
		//fwrite(serial, sizeof(Word16), SERIAL_SIZE, f_serial);
	}
	//fclose(f_prm);
}
void g729a_InitDecoder()
{
	bad_lsf = 0;          /* Initialize bad LSF indicator */
	Init_Decod_ld8a();
	Init_Post_Filter();
	Init_Post_Process();
}
void g729a_Decoder(unsigned char *bitstream, Word16 *synth_short, int bfi, int nSize)
{
	Word16  synth_buf[L_FRAME+M], *synth; /* Synthesis                   */
	Word16  parm[PRM_SIZE+1];             /* Synthesis parameters        */
	Word16  serial[SERIAL_SIZE];          /* Serial stream               */
	Word16  Az_dec[MP1*2];                /* Decoded Az for post-filter  */
	Word16  T2[2];                        /* Pitch lag for 2 subframes   */


	Word16  i, frame;
	//FILE   *f_prm;
	//if ( (f_prm = fopen("c:\\read.prm", "wb")) == NULL) {
	//	return;
	//}
	
	/*-----------------------------------------------------------------*
	*           Initialization of decoder                             *
	*-----------------------------------------------------------------*/



	for (i=0; i<M; i++) synth_buf[i] = 0;
	synth = synth_buf + M;

	


	/*-----------------------------------------------------------------*
	*            Loop for each "L_FRAME" speech data                  *
	*-----------------------------------------------------------------*/

	frame = L_FRAME/8;
	//while( fread(serial, sizeof(Word16), SERIAL_SIZE, f_serial) == SERIAL_SIZE)
	for( i = 0; i<nSize; i+= frame)
	{
		memcpy(serial,bitstream+i,frame);

		//printf("Frame =%d\r", frame++);
		//bits2prm_ld8k( &serial[2], &parm[1]);
		bits2prm_ld8k( serial, &parm[1]);

		//fwrite(&parm[1], sizeof(short), PRM_SIZE, f_prm);
		/* the hardware detects frame erasures by checking if all bits
		are set to zero
		*/
		parm[0] = bfi;           /* No frame erasure */
		//for (i=2; i < SERIAL_SIZE; i++)
		//	if (serial[i] == 0 ) parm[0] = 1; /* frame erased     */

		/* check pitch parity and put 1 in parm[4] if parity error */

		parm[4] = Check_Parity_Pitch(parm[3], parm[4]);

		Decod_ld8a(parm, synth, Az_dec, T2);

		Post_Filter(synth, Az_dec, T2);        /* Post-filter */

		Post_Process(synth, L_FRAME);
		
		memcpy(synth_short + i*8,synth,L_FRAME * sizeof(Word16));
		//fwrite(synth, sizeof(short), L_FRAME, f_syn);

	}
	//fclose(f_prm);
	
}


/*
This variable should be always set to zero unless transmission errors
in LSP indices are detected.
This variable is useful if the channel coding designer decides to
perform error checking on these important parameters. If an error is
detected on the  LSP indices, the corresponding flag is
set to 1 signalling to the decoder to perform parameter substitution.
(The flags should be set back to 0 for correct transmission).
*/

/*-----------------------------------------------------------------*
*            Main decoder routine                                 *
*-----------------------------------------------------------------*/

//int Decode(int argc, char *argv[] )
//{
//	Word16  synth_buf[L_FRAME+M], *synth; /* Synthesis                   */
//	Word16  parm[PRM_SIZE+1];             /* Synthesis parameters        */
//	Word16  serial[SERIAL_SIZE];          /* Serial stream               */
//	Word16  Az_dec[MP1*2];                /* Decoded Az for post-filter  */
//	Word16  T2[2];                        /* Pitch lag for 2 subframes   */
//
//
//	Word16  i, frame;
//	FILE   *f_syn, *f_serial;
//
//	printf("\n");
//	printf("************   G.729a 8.0 KBIT/S SPEECH DECODER  ************\n");
//	printf("\n");
//	printf("------------------- Fixed point C simulation ----------------\n");
//	printf("\n");
//	printf("-----------------          Version 1.1        ---------------\n");
//	printf("\n");
//
//	/* Passed arguments */
//
//	if ( argc != 3)
//	{
//		printf("Usage :%s bitstream_file  outputspeech_file\n",argv[0]);
//		printf("\n");
//		printf("Format for bitstream_file:\n");
//		printf("  One (2-byte) synchronization word \n");
//		printf("  One (2-byte) size word,\n");
//		printf("  80 words (2-byte) containing 80 bits.\n");
//		printf("\n");
//		printf("Format for outputspeech_file:\n");
//		printf("  Synthesis is written to a binary file of 16 bits data.\n");
//		exit( 1 );
//	}
//
//	/* Open file for synthesis and packed serial stream */
//
//	if( (f_serial = fopen(argv[1],"rb") ) == NULL )
//	{
//		printf("%s - Error opening file  %s !!\n", argv[0], argv[1]);
//		exit(0);
//	}
//
//	if( (f_syn = fopen(argv[2], "wb") ) == NULL )
//	{
//		printf("%s - Error opening file  %s !!\n", argv[0], argv[2]);
//		exit(0);
//	}
//
//	printf("Input bitstream file  :   %s\n",argv[1]);
//	printf("Synthesis speech file :   %s\n",argv[2]);
//
//	/*-----------------------------------------------------------------*
//	*           Initialization of decoder                             *
//	*-----------------------------------------------------------------*/
//
//	for (i=0; i<M; i++) synth_buf[i] = 0;
//	synth = synth_buf + M;
//
//	bad_lsf = 0;          /* Initialize bad LSF indicator */
//	Init_Decod_ld8a();
//	Init_Post_Filter();
//	Init_Post_Process();
//
//
//	/*-----------------------------------------------------------------*
//	*            Loop for each "L_FRAME" speech data                  *
//	*-----------------------------------------------------------------*/
//
//	frame = 0;
//	while( fread(serial, sizeof(Word16), SERIAL_SIZE, f_serial) == SERIAL_SIZE)
//	{
//		printf("Frame =%d\r", frame++);
//
//		bits2prm_ld8k( &serial[2], &parm[1]);
//
//		/* the hardware detects frame erasures by checking if all bits
//		are set to zero
//		*/
//		parm[0] = 0;           /* No frame erasure */
//		for (i=2; i < SERIAL_SIZE; i++)
//			if (serial[i] == 0 ) parm[0] = 1; /* frame erased     */
//
//		/* check pitch parity and put 1 in parm[4] if parity error */
//
//		parm[4] = Check_Parity_Pitch(parm[3], parm[4]);
//
//		Decod_ld8a(parm, synth, Az_dec, T2);
//
//		Post_Filter(synth, Az_dec, T2);        /* Post-filter */
//
//		Post_Process(synth, L_FRAME);
//
//		fwrite(synth, sizeof(short), L_FRAME, f_syn);
//
//	}
//	return(0);
//}
//
//int Code(int argc, char *argv[] )
//{
//  FILE *f_speech;               /* File of speech data                   */
//  FILE *f_serial;               /* File of serial bits for transmission  */
//
//  extern Word16 *new_speech;     /* Pointer to new speech data            */
//
//  Word16 prm[PRM_SIZE];          /* Analysis parameters.                  */
//  Word16 serial[SERIAL_SIZE];    /* Output bitstream buffer               */
//
//  Word16 frame;                  /* frame counter */
//
//
//  printf("\n");
//  printf("***********    ITU G.729A 8 KBIT/S SPEECH CODER    ***********\n");
//  printf("\n");
//  printf("------------------- Fixed point C simulation -----------------\n");
//  printf("\n");
//  printf("-------------------       Version 1.1        -----------------\n");
//  printf("\n");
//
//
///*--------------------------------------------------------------------------*
// * Open speech file and result file (output serial bit stream)              *
// *--------------------------------------------------------------------------*/
//
//  if ( argc != 3 )
//    {
//       printf("Usage : coder speech_file  bitstream_file\n");
//       printf("\n");
//       printf("Format for speech_file:\n");
//       printf("  Speech is read from a binary file of 16 bits PCM data.\n");
//       printf("\n");
//       printf("Format for bitstream_file:\n");
//       printf("  One (2-byte) synchronization word \n");
//       printf("  One (2-byte) size word,\n");
//       printf("  80 words (2-byte) containing 80 bits.\n");
//       printf("\n");
//       exit(1);
//    }
//
//  if ( (f_speech = fopen(argv[1], "rb")) == NULL) {
//     printf("%s - Error opening file  %s !!\n", argv[0], argv[1]);
//     exit(0);
//  }
//  printf(" Input speech file    :  %s\n", argv[1]);
//
//  if ( (f_serial = fopen(argv[2], "wb")) == NULL) {
//     printf("%s - Error opening file  %s !!\n", argv[0], argv[2]);
//     exit(0);
//  }
//  printf(" Output bitstream file:  %s\n", argv[2]);
//
///*--------------------------------------------------------------------------*
// * Initialization of the coder.                                             *
// *--------------------------------------------------------------------------*/
//
//  Init_Pre_Process();
//  Init_Coder_ld8a();
//  Set_zero(prm, PRM_SIZE);
//
//  /* Loop for each "L_FRAME" speech data. */
//
//  frame =0;
//  while( fread(new_speech, sizeof(Word16), L_FRAME, f_speech) == L_FRAME)
//  {
//    printf("Frame =%d\r", frame++);
//
//    Pre_Process(new_speech, L_FRAME);
//
//    Coder_ld8a(prm);
//
//    prm2bits_ld8k( prm, serial);
//
//    fwrite(serial, sizeof(Word16), SERIAL_SIZE, f_serial);
//  }
//  return (0);
//}

