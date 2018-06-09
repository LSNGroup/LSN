
#include "platform.h"


#define SREV(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#define IREV(x) ((SREV(x)<<16) | (SREV((x)>>16)))


word bswap_func(word w)
{
	return (SREV(w));
}

dword wswap_func(dword dw)
{
	return (IREV(dw));
}

#if _SXDEF_LITTLE_ENDIAN

void set_word_func(byte *dst, word val)
{
	pf_set_byte(dst+0, val & 0xff);
	pf_set_byte(dst+1, val >> 8);
}

word get_word_func(byte *dst)
{
	return ((word)pf_get_byte(dst+0) |
	        (word)pf_get_byte(dst+1) << 8);
}

void set_dword_func(byte *dst, dword val)
{
	pf_set_byte(dst+0, val         & 0xff);
	pf_set_byte(dst+1, (val >> 8)  & 0xff);
	pf_set_byte(dst+2, (val >> 16) & 0xff);
	pf_set_byte(dst+3, (val >> 24) & 0xff);
}

dword get_dword_func(byte *dst)
{
	return ((dword)pf_get_byte(dst+0)       |
	        (dword)pf_get_byte(dst+1) << 8  |
	        (dword)pf_get_byte(dst+2) << 16 |
	        (dword)pf_get_byte(dst+3) << 24);
}

#else /* _SXDEF_LITTLE_ENDIAN */

void set_word_func(byte *dst, word val)
{
	pf_set_byte(dst+0, val >> 8);
	pf_set_byte(dst+1, val & 0xff);
}

word get_word_func(byte *dst)
{
	return ((word)pf_get_byte(dst+0) << 8 |
	        (word)pf_get_byte(dst+1));
}

void set_dword_func(byte *dst, dword val)
{
	pf_set_byte(dst+0, (val >> 24) & 0xff);
	pf_set_byte(dst+1, (val >> 16) & 0xff);
	pf_set_byte(dst+2, (val >> 8)  & 0xff);
	pf_set_byte(dst+3,  val        & 0xff);
}

dword get_dword_func(byte *dst)
{
	return ((dword)pf_get_byte(dst+0) << 24 |
	        (dword)pf_get_byte(dst+1) << 16 |
	        (dword)pf_get_byte(dst+2) << 8 |
	        (dword)pf_get_byte(dst+3));
}

#endif /* _SXDEF_LITTLE_ENDIAN */
