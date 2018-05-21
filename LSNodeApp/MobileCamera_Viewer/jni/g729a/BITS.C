/*
   ITU-T G.729A Speech Coder    ANSI-C Source Code
   Version 1.1    Last modified: September 1996

   Copyright (c) 1996,
   AT&T, France Telecom, NTT, Universite de Sherbrooke
   All rights reserved.
*/

/*****************************************************************************/
/* bit stream manipulation routines                                          */
/*****************************************************************************/
#include "typedef.h"
#include "ld8a.h"
#include "tab_ld8a.h"
#define min(a,b) (((a) < (b)) ? (a) : (b)) 
/* prototypes for local functions */
static void  int2bin(Word16 value, Word16 no_of_bits, Word16 *bitstream);
static Word16  bin2int(Word16 no_of_bits, Word16 *bitstream);
static void int2bit(
					unsigned short value,             /* input : decimal value         */
					Word16 no_of_bits,        /* input : number of bits to use */
					Word16 *bitstream,        /* output: bitstream             */
					int nBits				  /* input : bits in stream        */	
					);
static Word16 bit2int(       /* output: decimal value of bit pattern */
					  Word16 no_of_bits,          /* input : number of bits to read       */
					  Word16 *bitstream,          /* input : array containing bits        */
					  int nBits			    	  /* input : begin bits in stream         */	
					  );
/*----------------------------------------------------------------------------
 * prm2bits_ld8k -converts encoder parameter vector into vector of serial bits
 * bits2prm_ld8k - converts serial received bits to  encoder parameter vector
 *
 * The transmitted parameters are:
 *
 *     LPC:     1st codebook           7+1 bit
 *              2nd codebook           5+5 bit
 *
 *     1st subframe:
 *          pitch period                 8 bit
 *          parity check on 1st period   1 bit
 *          codebook index1 (positions) 13 bit
 *          codebook index2 (signs)      4 bit
 *          pitch and codebook gains   4+3 bit
 *
 *     2nd subframe:
 *          pitch period (relative)      5 bit
 *          codebook index1 (positions) 13 bit
 *          codebook index2 (signs)      4 bit
 *          pitch and codebook gains   4+3 bit
 *----------------------------------------------------------------------------
 */
void prm2bits_ld8k(
 Word16   prm[],           /* input : encoded parameters  (PRM_SIZE parameters)  */
  Word16 bits[]            /* output: serial bits (SERIAL_SIZE ) bits[0] = bfi
                                    bits[1] = 80 */
)
{
   //Word16 i;
   //*bits++ = SYNC_WORD;     /* bit[0], at receiver this bits indicates BFI */
   //*bits++ = SIZE_WORD;     /* bit[1], to be compatible with hardware      */

   //for (i = 0; i < PRM_SIZE; i++)
   //  {
   //     int2bin(prm[i], bitsno[i], bits);
   //     bits += bitsno[i];
   //  }

	Word16 i;
	int nBits = 0;
	for (i = 0; i < PRM_SIZE; i++)
	  {
	     int2bit(prm[i],bitsno[i],bits,nBits);	
		 nBits +=bitsno[i];
	  }

   return;
}


/*----------------------------------------------------------------------------
* int2bit convert integer to binary and write the bits bitstream array
*----------------------------------------------------------------------------
*/
static void int2bit(
					unsigned short value,             /* input : decimal value         */
					Word16 no_of_bits,        /* input : number of bits to use */
					Word16 *bitstream,        /* output: bitstream             */
					int nBits				  /* input : bits in stream        */	
					)
{
	do 
	{
		Word16 nBitsPack;
		Word16 packvalue;
		int nWordPos = nBits/16;
		Word16 nMask = -1;
		int nRest = nBits%16;

		nRest = 16 - nRest;//剩余比特数

		nBitsPack = min(nRest,no_of_bits);//填充比特数

		packvalue = value >>(no_of_bits - nBitsPack);
		packvalue <<=(nRest - nBitsPack);
		bitstream[nWordPos]|=packvalue;

		nBits += nBitsPack;
		no_of_bits -= nBitsPack;

		if(no_of_bits == 0)
		{
			break;
		}
	} while(1);
	
	
}

/*----------------------------------------------------------------------------
 * int2bin convert integer to binary and write the bits bitstream array
 *----------------------------------------------------------------------------
 */
static void int2bin(
 Word16 value,             /* input : decimal value         */
 Word16 no_of_bits,        /* input : number of bits to use */
 Word16 *bitstream         /* output: bitstream             */
)
{
   Word16 *pt_bitstream;
   Word16   i, bit;

   pt_bitstream = bitstream + no_of_bits;

   for (i = 0; i < no_of_bits; i++)
   {
     bit = value & (Word16)0x0001;      /* get lsb */
     if (bit == 0)
         *--pt_bitstream = BIT_0;
     else
         *--pt_bitstream = BIT_1;
     value >>= 1;
   }
}

/*----------------------------------------------------------------------------
 *  bits2prm_ld8k - converts serial received bits to  encoder parameter vector
 *----------------------------------------------------------------------------
 */
void bits2prm_ld8k(
 Word16 bits[],            /* input : serial bits (80)                       */
 Word16   prm[]            /* output: decoded parameters (11 parameters)     */
)
{
   //Word16 i;
   //for (i = 0; i < PRM_SIZE; i++)
   //  {
   //     prm[i] = bin2int(bitsno[i], bits);
   //     bits  += bitsno[i];
   //  }

	Word16 i;
	int nBits = 0;
	for (i = 0; i < PRM_SIZE; i++)
	{
		prm[i] = bit2int(bitsno[i], bits,nBits);
		nBits +=bitsno[i];
	}

}

/*----------------------------------------------------------------------------
* bit2int - read specified bits from bit array  and convert to integer value
*----------------------------------------------------------------------------
*/
static Word16 bit2int(       /* output: decimal value of bit pattern */
					  Word16 no_of_bits,          /* input : number of bits to read       */
					  Word16 *bitstream,          /* input : array containing bits        */
					  int nBits			    	  /* input : begin bits in stream         */	
					  )
{
	Word16 value = 0;
	do 
	{
		Word16 nBitsPack;
		Word16 packvalue;

		Word16 temVal = bitstream[nBits/16];
		Word16 nMask = -1;
		int nRest = nBits%16;
		nRest = 16 - nRest;//剩余比特数

		nBitsPack = min(nRest,no_of_bits);//可用比特数

		packvalue = temVal >>(nRest - nBitsPack);

		//Clear MSB
		
		nMask<<=nBitsPack;
		nMask = ~nMask;
		value <<=nBitsPack;
		value += (nMask&packvalue);

		nBits += nBitsPack;
		no_of_bits -= nBitsPack;

		if(no_of_bits == 0)
		{
			break;
		}
	} while(1);
	
	return(value);
}



/*----------------------------------------------------------------------------
 * bin2int - read specified bits from bit array  and convert to integer value
 *----------------------------------------------------------------------------
 */
static Word16 bin2int(       /* output: decimal value of bit pattern */
 Word16 no_of_bits,          /* input : number of bits to read       */
 Word16 *bitstream           /* input : array containing bits        */
)
{
   Word16   value, i;
   Word16 bit;

   value = 0;
   for (i = 0; i < no_of_bits; i++)
   {
     value <<= 1;
     bit = *bitstream++;
     if (bit == BIT_1)  value += 1;
   }
   return(value);
}

