#ifndef __FFCONFIG_H__
#define __FFCONFIG_H__


#define EXTERN_PREFIX  ""
#define EXTERN_ASM

#if defined(WIN32) || defined(_M_IX86)
#define ARCH_ARM  0
#define ARCH_X86  1
#define HAVE_ARMV5TE  0
#define HAVE_ARMV6    0
#define HAVE_ARMV6T2  0
#define HAVE_ARMVFP   0
#define HAVE_IWMMXT   0
#define HAVE_NEON     0
#else //defined(WIN32) || defined(_M_IX86)
#define ARCH_ARM  1
#define ARCH_X86  0
#ifdef PLATFORM_ARMV7
#define HAVE_ARMV5TE  1
#define HAVE_ARMV6    1
#define HAVE_ARMV6T2  0
#define HAVE_ARMVFP   1
#define HAVE_IWMMXT   0
#define HAVE_NEON     1
#else
#define HAVE_ARMV5TE  1
#define HAVE_ARMV6    0
#define HAVE_ARMV6T2  0
#define HAVE_ARMVFP   0
#define HAVE_IWMMXT   0
#define HAVE_NEON     0
#endif
#endif //defined(WIN32) || defined(_M_IX86)


#define ARCH_AVR32  0
#define ARCH_SH4    0
#define ARCH_MIPS   0
#define ARCH_PPC    0
#define ARCH_BFIN   0
#define ARCH_TOMI   0
#define ARCH_ALPHA  0

#define HAVE_MMX      0
#define HAVE_MMI      0
#define HAVE_VIS      0
#define HAVE_ALTIVEC  0

#define HAVE_INLINE_ASM  0

#define HAVE_THREADS       0
#define HAVE_BEOSTHREADS   0
#define HAVE_OS2THREADS    0
#define HAVE_PTHREADS      0
#define HAVE_W32THREADS    0

#define HAVE_BIGENDIAN  0
#define HAVE_FAST_UNALIGNED  0

#ifdef WIN32
#define LIBMPEG2_BITSTREAM_READER     1
#else
#define A32_BITSTREAM_READER          1
#endif

#ifdef WIN32
#define CONFIG_WIN32  1
#endif

#define CONFIG_PIC   1

#define CONFIG_MLIB  0

#ifndef WIN32
#define HAVE_EXP2    1
#define HAVE_EXP2F   1
#define HAVE_LLRINT  1
#define HAVE_LLRINTF 1
#define HAVE_LOG2    1
#define HAVE_LOG2F   1
#define HAVE_LRINT   1
#define HAVE_LRINTF  1
#define HAVE_ROUND   1
#define HAVE_ROUNDF  1
#define HAVE_TRUNCF  1
#define HAVE_MKSTEMP 1
#endif


#define CONFIG_H264DSP  1
#define CONFIG_GOLOMB   1
#define CONFIG_FFT      0
#define CONFIG_MDCT     0
#define CONFIG_RDFT     0

#define CONFIG_VP3_DECODER   0
#define CONFIG_VP5_DECODER   0
#define CONFIG_VP6_DECODER   0
#define CONFIG_MPEG_XVMC_DECODER  0
#define CONFIG_H261_ENCODER 0
#define CONFIG_H261_DECODER 0
#define CONFIG_H263_ENCODER   0
#define CONFIG_H263_DECODER   1
#define CONFIG_H263I_DECODER  1
#define CONFIG_H264_ENCODER   0
#define CONFIG_H264_DECODER   1
#define CONFIG_H264_DXVA2_HWACCEL  0
#define CONFIG_H264_VAAPI_HWACCEL  0
#define CONFIG_MPEG4_DECODER     0
#define CONFIG_MSMPEG4V1_DECODER 0
#define CONFIG_MSMPEG4V2_DECODER 0
#define CONFIG_MSMPEG4V3_DECODER 0
#define CONFIG_VC1_DECODER       0
#define CONFIG_FLV_DECODER       0
#define CONFIG_WMV2_DECODER      0
#define CONFIG_WMV2_ENCODER      0
#define CONFIG_GRAY   0
#define CONFIG_SVQ3_DECODER  0
#define CONFIG_SMALL  0
#define CONFIG_H264_VDPAU_DECODER  0
#define CONFIG_MPEG4_VDPAU_DECODER 0
#define CONFIG_EATGQ_DECODER   0
#define CONFIG_BINK_DECODER    0
#define CONFIG_VORBIS_DECODER  0

#define CONFIG_H263_PARSER  1
#define CONFIG_H264_PARSER  1


#endif /* __FFCONFIG_H__ */
