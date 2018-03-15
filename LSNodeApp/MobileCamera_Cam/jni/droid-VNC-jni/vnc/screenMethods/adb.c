/*
droid vnc server - Android VNC server
Copyright (C) 2009 Jose Pereira <onaips@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "adb.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "common.h"


///////////////////////////////////////////////////////////////////////////////

enum {
	PIXEL_FORMAT_UNKNOWN = 0,
	PIXEL_FORMAT_NONE = 0,
	// logical pixel formats used by the SurfaceFlinger -----------------------
	PIXEL_FORMAT_CUSTOM = -4,
	// Custom pixel-format described by a PixelFormatInfo structure
	PIXEL_FORMAT_TRANSLUCENT = -3,
	// System chooses a format that supports translucency (many alpha bits)
	PIXEL_FORMAT_TRANSPARENT = -2,
	// System chooses a format that supports transparency
	// (at least 1 alpha bit)
	PIXEL_FORMAT_OPAQUE = -1,
	
    HAL_PIXEL_FORMAT_RGBA_8888    = 1,  
    HAL_PIXEL_FORMAT_RGBX_8888    = 2,  
    HAL_PIXEL_FORMAT_RGB_888      = 3,  
    HAL_PIXEL_FORMAT_RGB_565      = 4,  
    HAL_PIXEL_FORMAT_BGRA_8888    = 5,  
    HAL_PIXEL_FORMAT_RGBA_5551    = 6,  
    HAL_PIXEL_FORMAT_RGBA_4444    = 7,  
    HAL_PIXEL_FORMAT_YCbCr_422_SP = 0x10,  // NV16
    HAL_PIXEL_FORMAT_YCrCb_420_SP = 0x11,  // NV21 (_adreno)
    HAL_PIXEL_FORMAT_YCbCr_422_P  = 0x12,  // IYUV
    HAL_PIXEL_FORMAT_YCbCr_420_P  = 0x13,  // YUV9
    HAL_PIXEL_FORMAT_YCbCr_422_I  = 0x14,  // YUY2 (_adreno)
    HAL_PIXEL_FORMAT_YCbCr_420_I  = 0x15,  
    HAL_PIXEL_FORMAT_CbYCrY_422_I = 0x16,  // UYVY (_adreno)
    HAL_PIXEL_FORMAT_CbYCrY_420_I = 0x17,  
    HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED = 0x20,  // NV12_adreno_tiled
    HAL_PIXEL_FORMAT_YCbCr_420_SP       = 0x21,  // NV12
    HAL_PIXEL_FORMAT_YCrCb_420_SP_TILED = 0x22,  // NV21_adreno_tiled
    HAL_PIXEL_FORMAT_YCrCb_422_SP       = 0x23,  // NV61
	HAL_PIXEL_FORMAT_YCrCb_422_P        = 0x24,  // YV12 (_adreno)
	HAL_PIXEL_FORMAT_YV12               = HAL_PIXEL_FORMAT_YCrCb_422_P,
};

typedef int32_t PixelFormat;

enum { // components
	PixelFormatInfo_ALPHA = 1,
	PixelFormatInfo_RGB = 2,
	PixelFormatInfo_RGBA = 3,
	PixelFormatInfo_L = 4,
	PixelFormatInfo_LA = 5,
	PixelFormatInfo_OTHER = 0xFF
};

struct PixelFormatInfo
{
	enum {
		INDEX_ALPHA = 0,
		INDEX_RED = 1,
		INDEX_GREEN = 2,
		INDEX_BLUE = 3
	};
	enum { // components
		ALPHA = 1,
		RGB = 2,
		RGBA = 3,
		LUMINANCE = 4,
		LUMINANCE_ALPHA = 5,
		OTHER = 0xFF
	};
	struct szinfo {
		uint8_t h;
		uint8_t l;
	};
	//inline PixelFormatInfo() : version(sizeof(PixelFormatInfo)) { }
	//size_t getScanlineSize(unsigned int width) const;
	//size_t getSize(size_t ci) const {
	//	return (ci <= 3) ? (cinfo[ci].h - cinfo[ci].l) : 0;
	//}
	size_t version;
	PixelFormat format;
	size_t bytesPerPixel;
	size_t bitsPerPixel;
	union {
		struct szinfo cinfo[4];
		struct {
			uint8_t h_alpha;
			uint8_t l_alpha;
			uint8_t h_red;
			uint8_t l_red;
			uint8_t h_green;
			uint8_t l_green;
			uint8_t h_blue;
			uint8_t l_blue;
		};
	};
	uint8_t components;
	uint8_t reserved0[3];
	uint32_t reserved1;
};

static const int COMPONENT_YUV = 0xFF;

struct Info {
    size_t      size;
    size_t      bitsPerPixel;
    struct {
        uint8_t     ah;
        uint8_t     al;
        uint8_t     rh;
        uint8_t     rl;
        uint8_t     gh;
        uint8_t     gl;
        uint8_t     bh;
        uint8_t     bl;
    };
    uint8_t     components;
};

static struct Info const sPixelFormatInfos[] = {
        { 0,  0, { 0, 0,   0, 0,   0, 0,   0, 0 }, 0 },
        { 4, 32, {32,24,   8, 0,  16, 8,  24,16 }, PixelFormatInfo_RGBA },
        { 4, 24, { 0, 0,   8, 0,  16, 8,  24,16 }, PixelFormatInfo_RGB  },
        { 3, 24, { 0, 0,   8, 0,  16, 8,  24,16 }, PixelFormatInfo_RGB  },
        { 2, 16, { 0, 0,  16,11,  11, 5,   5, 0 }, PixelFormatInfo_RGB  },
        { 4, 32, {32,24,  24,16,  16, 8,   8, 0 }, PixelFormatInfo_RGBA },
        { 2, 16, { 1, 0,  16,11,  11, 6,   6, 1 }, PixelFormatInfo_RGBA },
        { 2, 16, { 4, 0,  16,12,  12, 8,   8, 4 }, PixelFormatInfo_RGBA },
        { 1,  8, { 8, 0,   0, 0,   0, 0,   0, 0 }, PixelFormatInfo_ALPHA},
        { 1,  8, { 0, 0,   8, 0,   8, 0,   8, 0 }, PixelFormatInfo_L    },
        { 2, 16, {16, 8,   8, 0,   8, 0,   8, 0 }, PixelFormatInfo_LA   },
        { 1,  8, { 0, 0,   8, 5,   5, 2,   2, 0 }, PixelFormatInfo_RGB  },
};

static const struct Info* gGetPixelFormatTable(size_t* numEntries) {
    if (numEntries) {
        *numEntries = sizeof(sPixelFormatInfos)/sizeof(struct Info);
    }
    return sPixelFormatInfos;
}

int getPixelFormatInfo(PixelFormat format, struct PixelFormatInfo* info)
{
    if (format <= 0)
        return -1;

    //if (info->version != sizeof(PixelFormatInfo))
    //    return INVALID_OPERATION;

    // YUV format from the HAL are handled here
    switch (format) {
    case HAL_PIXEL_FORMAT_YCbCr_422_SP:
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        info->bitsPerPixel = 16;
        goto done;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YV12:
        info->bitsPerPixel = 12;
     done:
        info->format = format;
        info->components = COMPONENT_YUV;
        info->bytesPerPixel = 1;
        info->h_alpha = 0;
        info->l_alpha = 0;
        info->h_red = info->h_green = info->h_blue = 8;
        info->l_red = info->l_green = info->l_blue = 0;
        return 0;
    }

    size_t numEntries;
    const struct Info *i = gGetPixelFormatTable(&numEntries) + format;
    if (!((uint32_t)format < numEntries)) {
        return -1;
    }

    info->format = format;
    info->bytesPerPixel = i->size;
    info->bitsPerPixel  = i->size * 8;//i->bitsPerPixel;
    info->h_alpha       = i->ah;
    info->l_alpha       = i->al;
    info->h_red         = i->rh;
    info->l_red         = i->rl;
    info->h_green       = i->gh;
    info->l_green       = i->gl;
    info->h_blue        = i->bh;
    info->l_blue        = i->bl;
    info->components    = i->components;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////


unsigned int *adbbuf = NULL;
int adbbuf_len;

int initADB(void)
{
  L("Initializing adb access method...\n");
  
  FILE *pipe;
  uint32_t w, h, f;
  unsigned char tmp;
  
  pipe = popen("screencap", "r");//"su -c \"screencap\""
  if (NULL == pipe) {
  	  L("adb access method: error 0\n");
  	  return -1;
  }
  if (1 != fread(&w, 4, 1, pipe)) {
  	  pclose(pipe);
  	  L("adb access method: error 1\n");
  	  return -1;
  }
  if (1 != fread(&h, 4, 1, pipe)) {
  	  pclose(pipe);
  	  L("adb access method: error 2\n");
  	  return -1;
  }
  if (1 != fread(&f, 4, 1, pipe)) {
  	  pclose(pipe);
  	  L("adb access method: error 3\n");
  	  return -1;
  }
  L("adb access method: w=%d, h=%d, f=%d\n", w, h, f);
  
  while (fread(&tmp, 1, 1, pipe) > 0) {;}
  pclose(pipe);
  
  return 0;
}


static void getscreenformat_adb(int w, int h, PixelFormat f)
{
	struct PixelFormatInfo pf;
	getPixelFormatInfo(f, &pf);
	
	screenformat.bitsPerPixel = pf.bitsPerPixel;
	screenformat.stride = w;
	screenformat.width = w;
	screenformat.height = h;
	screenformat.size = w * h * pf.bitsPerPixel / 8;
	screenformat.redShift = pf.l_red;
	screenformat.redMax = pf.h_red;
	screenformat.greenShift = pf.l_green;
	screenformat.greenMax = pf.h_green-pf.h_red;
	screenformat.blueShift = pf.l_blue;
	screenformat.blueMax = pf.h_blue-pf.h_green;
	screenformat.alphaShift = pf.l_alpha;
	screenformat.alphaMax = pf.h_alpha-pf.h_blue;
}

unsigned int *readBufferADB(void)
{
  FILE *pipe;
  uint32_t w, h, f;
  int length;

  pipe = popen("screencap", "r");
  if (NULL == pipe) {
  	  return NULL;
  }
  if (1 != fread(&w, 4, 1, pipe)) {
  	  pclose(pipe);
  	  return NULL;
  }
  if (1 != fread(&h, 4, 1, pipe)) {
  	  pclose(pipe);
  	  return NULL;
  }
  if (1 != fread(&f, 4, 1, pipe)) {
  	  pclose(pipe);
  	  return NULL;
  }
  getscreenformat_adb(w, h, f);
  
  length = screenformat.size;
  if (NULL == adbbuf) {
		adbbuf = (unsigned int *)malloc(length);
		adbbuf_len = length;
  }
  else if (length > adbbuf_len) {
  	  free(adbbuf);
  	  adbbuf = (unsigned int *)malloc(length);
  	  adbbuf_len = length;
  }
  fread(adbbuf, length, 1, pipe);
  pclose(pipe);
  
  return adbbuf;
}

void closeADB(void)
{
	if (NULL != adbbuf) {
		free(adbbuf);
		adbbuf = NULL;
	}
}
