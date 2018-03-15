/*
droid VNC server  - a vnc server for android
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

#include "input.h"

int inputfd = -1;
// keyboard code modified from remote input by http://www.math.bme.hu/~morap/RemoteInput/

// q,w,e,r,t,y,u,i,o,p,a,s,d,f,g,h,j,k,l,z,x,c,v,b,n,m
int qwerty[] = {30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44};
//  ,!,",#,$,%,&,',(,),*,+,,,-,.,/
int spec1[] = {57,2,40,4,5,6,8,40,10,11,9,13,51,12,52,52};
int spec1sh[] = {0,1,1,1,1,1,1,0,1,1,1,1,0,0,0,1};
// :,;,<,=,>,?,@
int spec2[] = {39,39,227,13,228,53,215};
int spec2sh[] = {1,0,1,1,1,1,0};
// [,\,],^,_,`
int spec3[] = {26,43,27,7,12,399};
int spec3sh[] = {0,0,0,1,1,0};
// {,|,},~
int spec4[] = {26,43,27,215,14};
int spec4sh[] = {1,1,1,1,0};


void initInput()
{
  L("---Initializing uinput...---\n");
  struct input_id id = {
    BUS_VIRTUAL, /* Bus type. */
    1, /* Vendor id. */
    1, /* Product id. */
    1 /* Version id. */
  }; 

  if((inputfd = suinput_open("qwerty", &id)) == -1)////Changed by Gaohua
  {
  	  L("cannot create virtual kbd device...qwerty\n");
  	  if((inputfd = suinput_open("Generic", &id)) == -1)
  	  {
		    L("cannot create virtual kbd device...Generic\n");
		    sendMsgToGui("~SHOW|Cannot create virtual input device!\n");
		    //  exit(EXIT_FAILURE); do not exit, so we still can see the framebuffer
      }
  }
}

void input_enable()
{
	  if (inputfd == -1) {
	    initInput();
	  }
}

void input_disable()
{
	  if (inputfd != -1) {
	    L("---Uninitializing uinput...---\n");
	    suinput_close(inputfd);
	    inputfd = -1;
	  }
}

static void android_input_keyevent(int k)
{
#if defined(FOR_WL_YKZ)
	char szCmd[256];
	sprintf(szCmd, "su -c \"input keyevent %d\"", k);
	system(szCmd);
#elif (defined(FOR_51HZ_GUAJI) || defined(FOR_MAYI_GUAJI))
	char szCmd[256];
	sprintf(szCmd, "am broadcast -a ADB_INPUT_CODE --ei code %d", k);
	system(szCmd);
#endif
}

static void android_input_text(char c)
{
#if defined(FOR_WL_YKZ)
	char tmp = c;
	
	char szCmd[256];
	sprintf(szCmd, "su -c \"input text \'%c\'\"", tmp);
	system(szCmd);
#elif (defined(FOR_51HZ_GUAJI) || defined(FOR_MAYI_GUAJI))
	char szCmd[256];
	sprintf(szCmd, "am broadcast -a ADB_INPUT_TEXT --es msg \'%c\'", c);
	system(szCmd);
#endif
}

int keysym2scancode(rfbBool down, rfbKeySym c, rfbClientPtr cl, int *sh, int *alt)
{
  L("keysym2scancode(): c=0x%lx\n", c);
  
  int real=1;
  if ('a' <= c && c <= 'z')
  {
	if (down) android_input_text(c);
	return 0;
  }
  if ('A' <= c && c <= 'Z')
  {
	if (down) android_input_text(c);
	return 0;
  }
  if ('0' <= c && c <= '9')
  {
	if (down) android_input_keyevent(c - '0' + 7);
	return 0;
  }
  
  if ('+' == c)
  {
	if (down) android_input_keyevent(81);
	return 0;
  }
  else if ('-' == c)
  {
	if (down) android_input_keyevent(69);
	return 0;
  }
  else if ('*' == c)
  {
	if (down) android_input_keyevent(17);
	return 0;
  }
  else if ('/' == c)
  {
	if (down) android_input_keyevent(76);
	return 0;
  }
  else if ('=' == c)
  {
	if (down) android_input_keyevent(70);
	return 0;
  }
  else if ('@' == c)
  {
	if (down) android_input_keyevent(77);
	return 0;
  }
  else if ('#' == c)
  {
	if (down) android_input_keyevent(18);
	return 0;
  }
  else if ('\'' == c)
  {
	if (down) android_input_keyevent(75);
	return 0;
  }
  else if ('\\' == c)
  {
	if (down) android_input_keyevent(73);
	return 0;
  }
  else if (',' == c)
  {
	if (down) android_input_keyevent(55);
	return 0;
  }
  else if ('.' == c)
  {
	if (down) android_input_keyevent(56);
	return 0;
  }
  else if ('[' == c)
  {
	if (down) android_input_keyevent(71);
	return 0;
  }
  else if (']' == c)
  {
	if (down) android_input_keyevent(72);
	return 0;
  }
  else if (';' == c)
  {
	if (down) android_input_keyevent(74);
	return 0;
  }
  else if ('`' == c)
  {
	if (down) android_input_keyevent(68);
	return 0;
  }
  else if ('\t' == c)
  {
	if (down) android_input_keyevent(61);
	return 0;
  }
  else if (' ' == c)
  {
	if (down) android_input_keyevent(62);
	return 0;
  }
  
  if ('_' == c || '|' == c || '(' == c || ')' == c || '&' == c || '^' == c || '%' == c || '$' == c || '!' == c || '<' == c || '>' == c || '{' == c || '}' == c || '?' == c)
  {
	if (down) android_input_text(c);
	return 0;
  }
  
  
  if (32 <= c && c <= 47)
  { 
    (*sh) = spec1sh[c-32];
    return spec1[c-32];
  }
  if (58 <= c && c <= 64)
  {
    (*sh) = spec2sh[c-58];
    return spec2[c-58];
  } 
  if (91 <= c && c <= 96)
  { 
    (*sh) = spec3sh[c-91];
    return spec3[c-91];
  }   
  if (123 <= c && c <= 127)
  {
    (*sh) = spec4sh[c-123]; 
    return spec4[c-123];
  } 

  switch(c)
  {
    case 0xff08: if (down) system("su -c \"input keyevent 67\""); return 0;// backspace -> KEYCODE_DEL
    case 0xff09: if (down) system("su -c \"input keyevent 61\""); return 0;// tab -> KEYCODE_TAB
    case 1: (*alt)=1; return 34;// ctrl+a
    case 3: (*alt)=1; return 46;// ctrl+c  
    case 4: (*alt)=1; return 32;// ctrl+d
    case 18: (*alt)=1; return 31;// ctrl+r
    case 0xff0D: if (down) system("su -c \"input keyevent 66\""); return 0;// enter -> KEYCODE_ENTER
    case 0xff1B: if (down) system("input keyevent 4"); return 0;// esc -> back
    case 0xFF51: if (down) rotate(90);  return 0;// left -> DPAD_LEFT  
    case 0xFF53: if (down) rotate(270); return 0;// right -> DPAD_RIGHT 
    case 0xFF54: if (down) rotate(180); return 0;// down -> DPAD_DOWN  
    case 0xFF52: if (down) rotate(0);   return 0;// up -> DPAD_UP
    // 		case 360: return 232;// end -> DPAD_CENTER (ball click)
    case 0xff50: if (down) system("su -c \"input keyevent 3\""); return 0;// home 
    case 0xFFC8: if (down) system("su -c \"input keyevent 26\""); return 0;//F11 -> KEYCODE_POWER
    case 0xFFC9: if (down) system("su -c \"input keyevent 164\""); return 0;//F10 -> KEYCODE_VOLUME_MUTE
    case 0xffc1: down?rotate(-1):0; return 0; // F4 rotate 
    case 0xffff: if (down) system("su -c \"input keyevent 4\""); return 0;// del -> back
    case 0xff55: if (down) system("su -c \"input keyevent 24\""); return 0;// PgUp -> KEYCODE_VOLUME_UP
    case 0xffcf: if (down) system("su -c \"input keyevent 84\""); return 0;// F2 -> search
    // 		case 0xffe3: return 127;// left ctrl -> search
    case 0xff56: if (down) system("su -c \"input keyevent 25\""); return 0;// PgDown -> KEYCODE_VOLUME_DOWN
    case 0xff57: if (down) system("su -c \"input keyevent 82\""); return 0;// End -> KEYCODE_MENU
    case 0xffc2: if (down) input_enable(); return 0;// F5 -> input_enable
    case 0xffc3: if (down) input_disable(); return 0;// F6 -> input_disable
    case 0xffc4: if (down) system("su -c \"input keyevent 5\""); return 0;// F7 -> KEYCODE_CALL
    case 0xffc5: if (down) system("su -c \"input keyevent 6\""); return 0;// F8 -> KEYCODE_ENDCALL

    case 50081:
    case 225: (*alt)=1;
    if (real) return 48; //a with acute
    return 30; //a with acute -> a with ring above
    case 50049: 
    case 193:(*sh)=1; (*alt)=1; 
    if (real) return 48; //A with acute 
    return 30; //A with acute -> a with ring above
    case 50089:
    case 233: (*alt)=1; return 18; //e with acute
    case 50057:  
    case 201:(*sh)=1; (*alt)=1; return 18; //E with acute
    case 50093:
    case 0xffbf: (*alt)=1; 
    if (real) return 36; //i with acute 
    return 23; //i with acute -> i with grave
    case 50061:
    case 205: (*sh)=1; (*alt)=1; 
    if (real) return 36; //I with acute 
return 23; //I with acute -> i with grave
    case 50099: 
      case 243:(*alt)=1; 
      if (real) return 16; //o with acute 
      return 24; //o with acute -> o with grave
    case 50067: 
      case 211:(*sh)=1; (*alt)=1; 
      if (real) return 16; //O with acute 
      return 24; //O with acute -> o with grave
    case 50102:
      case 246: (*alt)=1; return 25; //o with diaeresis
      case 50070:
        case 214: (*sh)=1; (*alt)=1; return 25; //O with diaeresis
        case 50577: 
          case 245:(*alt)=1; 
          if (real) return 19; //Hungarian o 
          return 25; //Hungarian o -> o with diaeresis
    case 50576:
      case 213: (*sh)=1; (*alt)=1; 
      if (real) return 19; //Hungarian O 
      return 25; //Hungarian O -> O with diaeresis
    case 50106:
      // 		case 0xffbe: (*alt)=1; 
      // 			if (real) return 17; //u with acute 
      // 			return 22; //u with acute -> u with grave
      case 50074:
        case 218: (*sh)=1; (*alt)=1; 
        if (real) return 17; //U with acute 
        return 22; //U with acute -> u with grave
    case 50108:
      case 252: (*alt)=1; return 47; //u with diaeresis
      case 50076: 
        case 220:(*sh)=1; (*alt)=1; return 47; //U with diaeresis
        case 50609:
          case 251: (*alt)=1; 
          if (real) return 45; //Hungarian u 
          return 47; //Hungarian u -> u with diaeresis
    case 50608:
      case 219: (*sh)=1; (*alt)=1; 
      if (real) return 45; //Hungarian U 
      return 47; //Hungarian U -> U with diaeresis

    }
    return 0;
}


void keyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
  int code;
  //      L("Got keysym: %04x (down=%d)\n", (unsigned int)key, (int)down);

  setIdle(0);
  int sh = 0;
  int alt = 0;

  if ((code = keysym2scancode(down, key, cl,&sh,&alt)))
  {
	  if ( inputfd == -1 )
	    return;
  }
}



void ptrEvent(int buttonMask, int x, int y, rfbClientPtr cl)
{

  static int leftClicked=0,rightClicked=0,middleClicked=0;

  if ( inputfd == -1 )
    return;
  
  setIdle(0);
  transformTouchCoordinates(&x,&y,cl->screen->width,cl->screen->height);

  if((buttonMask & 1)&& leftClicked) {//left btn clicked and moving
                                      static int i=0;
                                      i=i+1;

                                      if (i%10==1)//some tweak to not report every move event
                                      {
                                        suinput_write(inputfd, EV_ABS, ABS_X, x);
                                        suinput_write(inputfd, EV_ABS, ABS_Y, y);
                                        suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
                                      }
                                     }
  else if (buttonMask & 1)//left btn clicked
  {
    leftClicked=1;

    suinput_write(inputfd, EV_ABS, ABS_X, x);
    suinput_write(inputfd, EV_ABS, ABS_Y, y);
    suinput_write(inputfd,EV_KEY,BTN_TOUCH,1);
    suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);


  }
  else if (leftClicked)//left btn released
  {
    leftClicked=0;
    suinput_write(inputfd, EV_ABS, ABS_X, x);
    suinput_write(inputfd, EV_ABS, ABS_Y, y);
    suinput_write(inputfd,EV_KEY,BTN_TOUCH,0);
    suinput_write(inputfd, EV_SYN, SYN_REPORT, 0);
  }

  if (buttonMask & 4)//right btn clicked
  {
    rightClicked=1;
    suinput_press(inputfd,158); //back key
  }
  else if (rightClicked)//right button released
  {
    rightClicked=0;
    suinput_release(inputfd,158);
  }

  if (buttonMask & 2)//mid btn clicked
  {
    middleClicked=1;
    suinput_press( inputfd,KEY_END);
  }
    else if (middleClicked)// mid btn released
    {
      middleClicked=0;
      suinput_release( inputfd,KEY_END);
    }
    }


inline void transformTouchCoordinates(int *x, int *y,int width,int height)
{
  int scale=4096.0;
  int old_x=*x,old_y=*y;
  int rotation=getCurrentRotation();

  if (rotation==0)
  {  
    *x = old_x*scale/width-scale/2.0;
    *y = old_y*scale/height-scale/2.0;
  }
  else if (rotation==90)
  {
    *x =old_y*scale/height-scale/2.0;
    *y = (width - old_x)*scale/width-scale/2.0;
  }
  else if (rotation==180)
  {
    *x =(width - old_x)*scale/width-scale/2.0;
    *y =(height - old_y)*scale/height-scale/2.0;
  }
  else if (rotation==270)
  {
    *y =old_x*scale/width-scale/2.0; 
    *x =(height - old_y)*scale/height-scale/2.0;
  }

}


void cleanupInput()
{
  if(inputfd != -1)
  {
    suinput_close(inputfd);
  }
}
