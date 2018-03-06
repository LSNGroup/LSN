#ifndef _PLATFORM_H_
#define _PLATFORM_H_


typedef	unsigned char	byte;
typedef	unsigned short	word;
typedef	unsigned long	dword;


#define snprintf  _snprintf



/* See Application.mk */
#define _SXDEF_CPU_80X86		1
#define _SXDEF_LITTLE_ENDIAN	1



/* Sasaki's byte order/word align conversion macros ------------------------ */
#if _SXDEF_CPU_80X86

/* Little-Endian, byte-swappable CPU */
word	bswap_func(word w);
dword	wswap_func(dword dw);

#define bswap(w)			bswap_func(w)
#define wswap(dw)			wswap_func(dw)

#define pf_set_byte(dst, val)	(*((byte*)(dst)) = (byte)(val))
#define pf_set_word(dst, val)	(*((word*)(dst)) = (word)(val))
#define pf_set_dword(dst, val)	(*((dword*)(dst)) = (dword)(val))
#define pf_get_byte(src)		(*((byte*)(src)))
#define pf_get_word(src)		(*((word*)(src)))
#define pf_get_dword(src)		(*((dword*)(src)))

#else /* _SXDEF_CPU_80X86 */

/* Big-Endian, non-byte-swappable CPUs */
word	bswap_func(word w);
dword	wswap_func(dword dw);
void	set_word_func(byte *dst, word val);
word	get_word_func(byte *dst);
void	set_dword_func(byte *dst, dword val);
dword	get_dword_func(byte *dst);

#define bswap(w)			bswap_func(w)
#define wswap(dw)			wswap_func(dw)
#define pf_set_byte(dst, val)	(*((byte*)(dst)) = (byte)(val))
#define pf_set_word(dst, val)	set_word_func((byte*)(dst), (word)(val))
#define pf_set_dword(dst, val)	set_dword_func((byte*)(dst), (dword)(val))
#define pf_get_byte(src)		(*((byte*)(src)))
#define pf_get_word(src)		get_word_func((byte*)(src))
#define pf_get_dword(src)		get_dword_func((byte*)(src))

#endif /* _SXDEF_CPU_80X86 */


#endif /* _PLATFORM_H_ */
