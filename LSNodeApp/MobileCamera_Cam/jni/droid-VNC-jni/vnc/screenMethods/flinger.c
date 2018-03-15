/*
droid vnc server - Android VNC server
Copyright (C) 2011 Jose Pereira <onaips@gmail.com>

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

#include <dlfcn.h>

#include "flinger.h"
#include "common.h"

void *flinger_lib = NULL;

close_fn_type close_flinger = NULL;
readfb_fn_type readfb_flinger = NULL;
getscreenformat_fn_type getscreenformat_flinger = NULL;
getscreenformat2_fn_type getscreenformat2_flinger = NULL;

int initFlinger(void)
{
  L("Loading flinger native lib, ANDROID_API_LEVEL=%d\n", ANDROID_API_LEVEL);
  char lib_name[256];

	if (ANDROID_API_LEVEL == 23) {
		LIB_API_LEVEL = 23;
	}
	else if (ANDROID_API_LEVEL == 22) {
		LIB_API_LEVEL = 22;
	}
	else if (ANDROID_API_LEVEL == 21) {
		LIB_API_LEVEL = 21;
	}
	else if (ANDROID_API_LEVEL >= 19) {
		LIB_API_LEVEL = 19;
	}
	else if (ANDROID_API_LEVEL == 18) {
		LIB_API_LEVEL = 18;
	}
	else if (ANDROID_API_LEVEL == 17) {
		LIB_API_LEVEL = 17;
	}
	else if (ANDROID_API_LEVEL >= 15) {
		LIB_API_LEVEL = 15;
	}
	else if (ANDROID_API_LEVEL == 14) {
		LIB_API_LEVEL = 14;
	}
	else {
		LIB_API_LEVEL = ANDROID_API_LEVEL;
	}

  {
    sprintf(lib_name, "%s/libdvnc_flinger_sdk%d.so",DVNC_LIB_PATH,LIB_API_LEVEL);

    if (flinger_lib != NULL)
      dlclose(flinger_lib);
    L("Loading lib: %s\n",lib_name);
    flinger_lib = dlopen(lib_name, RTLD_NOW);
    if (flinger_lib == NULL){
      L("Couldnt load flinger library %s! Error string: %s\n",lib_name,dlerror());
        return -1;
    }
  
    init_fn_type init_flinger = dlsym(flinger_lib,"init_flinger");
    if(init_flinger == NULL) {
      L("Couldn't load init_flinger! Error string: %s\n",dlerror());
        return -1;
    }
  
    close_flinger = dlsym(flinger_lib,"close_flinger");
    if(close_flinger == NULL) {
      L("Couldn't load close_flinger! Error string: %s\n",dlerror());
        return -1;
    }

    readfb_flinger = dlsym(flinger_lib,"readfb_flinger");
    if(readfb_flinger == NULL) {
      L("Couldn't load readfb_flinger! Error string: %s\n",dlerror());
        return -1;
    }

if (LIB_API_LEVEL <= 15 || LIB_API_LEVEL >= 21)
{
    getscreenformat2_flinger = NULL;
    getscreenformat_flinger = dlsym(flinger_lib,"getscreenformat_flinger");
    if(getscreenformat_flinger == NULL) {
      L("Couldn't load get_screenformat! Error string: %s\n",dlerror());
        return -1;
    }
}
else {
    getscreenformat_flinger = NULL;
    getscreenformat2_flinger = dlsym(flinger_lib,"getscreenformat_flinger");
    if(getscreenformat2_flinger == NULL) {
      L("Couldn't load get_screenformat2! Error string: %s\n",dlerror());
        return -1;
    }
}
    
    
L("AKI1\n");

    int ret = init_flinger();
    if (ret == -1) {
       L("flinger method not supported by this device!\n");
         return -1;
    }
L("AKI2\n");

    screenformat = getScreenFormatFlinger();
    if ( screenformat.width <= 0 ) {
      L("Error: I have received a bad screen size from flinger.\n");
        return -1;
    }
  
    if ( readBufferFlinger() == NULL) {
      L("Error: Could not read surfaceflinger buffer!\n");
        return -1;
    }
    return 0;
  }//for
}

screenFormat getScreenFormatFlinger(void)
{
  screenFormat f;
  screenFormat2 f2;
  if (getscreenformat_flinger) {
     f = getscreenformat_flinger();
     f.stride = f.width;
  }
  else if (getscreenformat2_flinger) {
     f2 = getscreenformat2_flinger();
     
     f.width = f2.width;
     f.height = f2.height;
     f.bitsPerPixel = f2.bitsPerPixel;
     f.redMax = f2.redMax;
     f.greenMax = f2.greenMax;
     f.blueMax = f2.blueMax;
     f.alphaMax = f2.alphaMax;
     f.redShift = f2.redShift;
     f.greenShift = f2.greenShift;
     f.blueShift = f2.blueShift;
     f.alphaShift = f2.alphaShift;
     f.size = f2.size;
     f.stride = f2.stride;
  }
  
  if (f.size > f.stride * f.height * (f.bitsPerPixel / 8))////Added by Gaohua
  {
  	  L("######: screenformat.stride <== size/(height*bpp)\n");
  	  f.stride = f.size / (f.height * (f.bitsPerPixel / 8));
  }
  if (f.stride > f.width && f.stride < f.width * 2)////Added by Gaohua
  {
  	  L("######: screenformat.width <== screenformat.stride\n");
  	  f.width = f.stride;
  }
  return f;
}

void closeFlinger(void)
{
  if (close_flinger)
    close_flinger();
  if (flinger_lib)
    dlclose(flinger_lib);
}

unsigned char *readBufferFlinger(void)
{
  if (readfb_flinger)
    return readfb_flinger();
  return NULL;
}

