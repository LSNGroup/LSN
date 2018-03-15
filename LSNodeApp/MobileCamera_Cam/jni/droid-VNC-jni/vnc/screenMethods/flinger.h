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

#ifndef FLINGER_H
#define FLINGER_H

#include "screenFormat.h"

#define L(...) do{ __android_log_print(ANDROID_LOG_INFO,"VNCserver",__VA_ARGS__);printf(__VA_ARGS__); } while (0);

int initFlinger(void);
void closeFlinger(void);
unsigned char *readBufferFlinger(void);
screenFormat getScreenFormatFlinger(void);

#endif
