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

#include "common.h"
#include "framebuffer.h"
#include "adb.h"

#include "gui.h"
#include "input.h"
#include "flinger.h"

#include "libvncserver/scale.h"
#include "rfb/rfb.h"
#include "rfb/keysym.h"
#include "suinput.h"


#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#define CONCAT3(a,b,c) a##b##c
#define CONCAT3E(a,b,c) CONCAT3(a,b,c)


//不要使用TRUE和FALSE，定义不一致，导致文件传输不能用！！！
static int hasRoot()
{
	char buff[128];
	int ret = 1;
	
	if (0 != access("/system/xbin/su", F_OK)) {
		if (0 != access("/system/bin/su", F_OK)) {
			if (0 != access("/su/bin/su", F_OK)) {
				ret = 0;
			}
		}
	}
	
	if (1 == ret) {
		//vncserver中不能运行system()，异常退出
		//system("su > /data/local/tmp/root.txt");//wait...
	}
	
	if (1 == ret) {
		//vncserver中不能运行system()，异常退出
		//system("su > /sdcard/root.txt");//wait...
	}
	
	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "vncserver hasRoot() = %d\n", ret);
	return ret;
}


char DVNC_LIB_PATH[256] = "/data/data/com.wangling.remotephone/lib/";

int ANDROID_API_LEVEL = 15;
int LIB_API_LEVEL = 15;

int init_kbd_device = 0;

char VNC_PASSWORD[256] = "";
/* Android already has 5900 bound natively in some devices. */
int VNC_PORT=5901;

unsigned int *cmpbuf;
unsigned int *vncbuf;

static rfbScreenInfoPtr vncscr;

uint32_t idle = 0;
uint32_t standby = 1;
uint16_t rotation = 0;
uint16_t scaling = 100;
uint8_t display_rotate_180 = 0;

//reverse connection
char *rhost = NULL;
int rport = 5500;

void (*update_screen)(void)=NULL;

enum method_type {AUTO,FRAMEBUFFER,ADB,FLINGER};
enum method_type method=AUTO;

#define PIXEL_TO_VIRTUALPIXEL_FB(i,j) ((j+scrinfo.yoffset)*scrinfo.xres_virtual+i+scrinfo.xoffset)
#define PIXEL_TO_VIRTUALPIXEL(i,j) ((j*screenformat.width)+i)

#define OUT 8 
#include "updateScreen.c" 
#undef OUT

#define OUT 16
#include "updateScreen.c"
#undef OUT

//#define OUT 24
#include "updateScreen24.c"
//#undef OUT

#define OUT 32 
#include "updateScreen.c"
#undef OUT

inline int getCurrentRotation()
{
  return rotation;
}

void setIdle(int i)
{
  idle=i; 
}

ClientGoneHookPtr clientGone(rfbClientPtr cl)
{
  sendMsgToGui("~DISCONNECTED|\n");
  return 0;
}

rfbNewClientHookPtr clientHook(rfbClientPtr cl)
{
  if (scaling!=100)
  {
    rfbScalingSetup(cl, vncscr->width*scaling/100.0, vncscr->height*scaling/100.0);
    L("Scaling to w=%d  h=%d\n",(int)(vncscr->width*scaling/100.0), (int)(vncscr->height*scaling/100.0));
    //rfbSendNewScaleSize(cl);
  }

  cl->clientGoneHook=(ClientGoneHookPtr)clientGone;

  char *header="~CONNECTED|";
  char *msg=malloc(sizeof(char)*((strlen(cl->host)) + strlen(header)+1));
  msg[0]='\0';
  strcat(msg,header);
  strcat(msg,cl->host);
  strcat(msg,"\n");
  sendMsgToGui(msg);
  free (msg);

  return RFB_CLIENT_ACCEPT;
}


#if defined(FOR_WL_YKZ)

void CutText(char* str,int len, struct _rfbClientRec* cl)
{
  str[len]='\0';
  char *header="~CLIP|\n";
  char *msg=malloc(sizeof(char)*(strlen(str) + strlen(header)+1));
  msg[0]='\0';
  strcat(msg,header);
  strcat(msg,str);
  strcat(msg,"\n");
  sendMsgToGui(msg);
  free(msg);
}

#elif (defined(FOR_51HZ_GUAJI) || defined(FOR_MAYI_GUAJI))

#include <sys/ioctl.h>     
#include <sys/types.h>     
#include <sys/socket.h>     
#include <sys/un.h>
#include <netinet/in.h>     
#include <netdb.h>     
#include <arpa/inet.h>     
#include <net/if.h>    

static char clipboard_socket_name[] = "remotephone.clipboard_socket";  // See Java client LocalSocket

void CutText(char* str,int length, struct _rfbClientRec* cl)
{
#if 1
	char szCmd[1024*2];
	sprintf(szCmd, "am broadcast -a ADB_INPUT_TEXT --es msg \'%s\'", str);
	system(szCmd);
#endif

	int data_sock;
	struct sockaddr_un addr;
	int len;
	
	if ((data_sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		__android_log_print(ANDROID_LOG_ERROR, "CutText", "socket(AF_UNIX) failed!\n");
		return;
	}
	
	int snd_size = 64*1024;
	setsockopt(data_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&snd_size, sizeof(snd_size));
	
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = '\0';
    strncpy(addr.sun_path + 1, clipboard_socket_name, sizeof(addr.sun_path) - 1);
    len = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(clipboard_socket_name);
    
    if (connect(data_sock, (struct sockaddr *)&addr, len) < 0) {
    	__android_log_print(ANDROID_LOG_ERROR, "CutText", "connect(%s) failed, wait 1 seconds...\n", clipboard_socket_name);
    	usleep(50000);
	    if (connect(data_sock, (struct sockaddr *)&addr, len) < 0) {
			__android_log_print(ANDROID_LOG_ERROR, "CutText", "connect(%s) failed!\n", clipboard_socket_name);
			close(data_sock);
			return;
		}
	}

	send(data_sock, str, length, 0);
	close(data_sock);
}

#endif


void sendServerStarted(){
  sendMsgToGui("~SERVERSTARTED|\n");
}

void sendServerStopped()
{
  sendMsgToGui("~SERVERSTOPPED|\n");
}

void initVncServer(int argc, char **argv)
{ 

  vncbuf = calloc(screenformat.width * screenformat.height, screenformat.bitsPerPixel/CHAR_BIT);
  cmpbuf = calloc(screenformat.width * screenformat.height, screenformat.bitsPerPixel/CHAR_BIT);

  assert(vncbuf != NULL);
  assert(cmpbuf != NULL);

  if (rotation==0 || rotation==180) 
  vncscr = rfbGetScreen(&argc, argv, screenformat.width , screenformat.height, 0 /* not used */ , 3,  screenformat.bitsPerPixel/CHAR_BIT);
  else
  vncscr = rfbGetScreen(&argc, argv, screenformat.height, screenformat.width, 0 /* not used */ , 3,  screenformat.bitsPerPixel/CHAR_BIT);

  assert(vncscr != NULL);

  vncscr->desktopName = "Android";
  vncscr->frameBuffer =(char *)vncbuf;
  vncscr->port = VNC_PORT;
  vncscr->kbdAddEvent = keyEvent;
  vncscr->ptrAddEvent = ptrEvent;
  vncscr->newClientHook = (rfbNewClientHookPtr)clientHook;
  vncscr->setXCutText = CutText;

  if (strcmp(VNC_PASSWORD,"")!=0)
  {
    char **passwords = (char **)malloc(2 * sizeof(char **));
    passwords[0] = VNC_PASSWORD;
    passwords[1] = NULL;
    vncscr->authPasswdData = passwords;
    vncscr->passwordCheck = rfbCheckPasswordByList;
  } 

  vncscr->httpDir = "webclients/";
//  vncscr->httpEnableProxyConnect = TRUE;
//  vncscr->sslcertfile = "self.pem";

  vncscr->serverFormat.redShift = screenformat.redShift;
  vncscr->serverFormat.greenShift = screenformat.greenShift;
  vncscr->serverFormat.blueShift = screenformat.blueShift;

  vncscr->serverFormat.redMax = (( 1 << screenformat.redMax) -1);
  vncscr->serverFormat.greenMax = (( 1 << screenformat.greenMax) -1);
  vncscr->serverFormat.blueMax = (( 1 << screenformat.blueMax) -1);

  vncscr->serverFormat.trueColour = TRUE; 
  vncscr->serverFormat.bitsPerPixel = screenformat.bitsPerPixel;

  vncscr->alwaysShared = TRUE;
  vncscr->handleEventsEagerly = TRUE;
  vncscr->deferUpdateTime = 5;

  vncscr->permitFileTransfer = TRUE;

  rfbInitServer(vncscr);
  L("rfbInitServer(vncscr) done!\n");

    //assign update_screen depending on bpp
    if (vncscr->serverFormat.bitsPerPixel == 32)
    update_screen=&CONCAT2E(update_screen_,32);
    else if (vncscr->serverFormat.bitsPerPixel == 24)
    update_screen=&CONCAT2E(update_screen_,24);
    else if (vncscr->serverFormat.bitsPerPixel == 16)
    update_screen=&CONCAT2E(update_screen_,16);
    else if (vncscr->serverFormat.bitsPerPixel == 8)
    update_screen=&CONCAT2E(update_screen_,8);
    else {
      L("Unsupported pixel depth: %d\n",
        vncscr->serverFormat.bitsPerPixel);

      sendMsgToGui("~SHOW|Unsupported pixel depth, please send bug report.\n");
      close_app();
      exit(-1);
    }

    /* Mark as dirty since we haven't sent any updates at all yet. */
    rfbMarkRectAsModified(vncscr, 0, 0, vncscr->width, vncscr->height);
}



void rotate(int value)
{

  L("rotate()\n");

  if (value == -1 || 
      ((value == 90 || value == 270) && (rotation == 0 || rotation == 180)) ||
      ((value == 0 || value == 180) && (rotation == 90 || rotation == 270))) {
        int h = vncscr->height;
        int w = vncscr->width;

        vncscr->width = h;
        vncscr->paddedWidthInBytes = h * screenformat.bitsPerPixel / CHAR_BIT;
        vncscr->height = w;

        rfbClientIteratorPtr iterator;
        rfbClientPtr cl;
        iterator = rfbGetClientIterator(vncscr);
        while ((cl = rfbClientIteratorNext(iterator)) != NULL)
        cl->newFBSizePending = 1;
      }

  if (value == -1) {
    rotation += 90;
    rotation %= 360;
  } else {
    rotation = value;  
  }
 
  rfbMarkRectAsModified(vncscr, 0, 0, vncscr->width, vncscr->height);
}


void close_app()
{ 	
  L("Cleaning up...\n");
  if (method == FRAMEBUFFER)
    closeFB();
  else if (method == ADB)
    closeADB();
  else if (method == FLINGER)
    closeFlinger();
  
  if (0 != init_kbd_device) {
  	cleanupInput();
  }
  sendServerStopped();
  unbindIPCserver();
  exit(0); /* normal exit status */
}


void extractReverseHostPort(char *str)
{
  int len = strlen(str);
  char *p;
  /* copy in to host */
  rhost = (char *) malloc(len+1);
  if (! rhost) {
    L("reverse_connect: could not malloc string %d\n", len);
    exit(-1);
  }
  strncpy(rhost, str, len);
  rhost[len] = '\0';

  /* extract port, if any */
  if ((p = strrchr(rhost, ':')) != NULL) {
    rport = atoi(p+1);
    if (rport < 0) {
      rport = -rport;
    } else if (rport < 20) {
      rport = 5500 + rport;
    }
    *p = '\0';
  } 
}

void initGrabberMethod()
{
  if (0 == hasRoot()) {
  	  method = ADB;//fake
  	  return;
  }
  
  if (method == AUTO) {
    L("No grabber method selected, auto-detecting...\n");
    if (initADB() != -1) {
      method = ADB;
      readBufferADB();
    }
    else if (initFlinger() != -1)
      method = FLINGER;
    else if (initFB() != -1) {
      method = FRAMEBUFFER;
    }
  } else if (method == FRAMEBUFFER)
    initFB();
  else if (method == ADB) {
    initADB(); 
    readBufferADB();
  }
  else if (method == FLINGER)
    initFlinger();
}

void printUsage(char **argv)
{
  L("\nandroidvncserver [parameters]\n"
    "-f <device>\t- Framebuffer device (only with -m fb, default is /dev/graphics/fb0)\n"
    "-h\t\t- Print this help\n"
    "-m <method>\t- Display grabber method\n\tfb: framebuffer\n\tgb: gingerbread+ devices\n\tadb: slower, but should be compatible with all devices\n"
    "-p <password>\t- Password to access server\n"
    "-r <rotation>\t- Screen rotation (degrees) (0,90,180,270)\n"
    "-R <host:port>\t- Host for reverse connection\n" 
    "-s <scale>\t- Scale percentage (20,30,50,100,150)\n"
    "-z\t- Rotate display 180º (for zte compatibility)\n\n");
}


#include <time.h>

int main(int argc, char **argv)
{
  //pipe signals
  signal(SIGINT, close_app);
  signal(SIGKILL, close_app);
  signal(SIGILL, close_app);
  long usec;

  if(argc > 1) {
    int i=1;
    int r;
    while(i < argc) {
      if(*argv[i] == '-') {
        switch(*(argv[i] + 1)) {
          case 'h':
          printUsage(argv);
          exit(0); 
          break;
#if 1
          case 'A': 
          i++; 
          ANDROID_API_LEVEL=atoi(argv[i]);
          break;
          
          case 'L': 
          i++; 
          strcpy(DVNC_LIB_PATH,argv[i]);
          break;
          
          case 'I': 
          //i++; 
          init_kbd_device=1;
          break;
#endif
          case 'p': 
          i++; 
          strcpy(VNC_PASSWORD,argv[i]);
          break;
          case 'f': 
          i++; 
          FB_setDevice(argv[i]);
          break;
          case 'z': 
          //i++; 
          display_rotate_180=1;
          break;
          case 'P': 
          i++; 
          VNC_PORT=atoi(argv[i]);
          break;
          case 'r':
          i++; 
          r = atoi(argv[i]);
          if (r==0 || r==90 || r==180 || r==270)
          rotation = r;
          L("rotating to %d degrees\n",rotation);
          break;
          case 's':
          i++;
          r=atoi(argv[i]); 
          if (r >= 1 && r <= 150)
          scaling = r;
          else 
          scaling = 100;
          L("scaling to %d%%\n",scaling);
          break;
          case 'R':
          i++;
          extractReverseHostPort(argv[i]);
          break;
          case 'm':
          i++;
          if (!strcmp(argv[i],"adb")){
            method = ADB;
            L("ADB display grabber selected\n");
          } else if (!strcmp(argv[i],"fb")) {
            method = FRAMEBUFFER;
            L("Framebuffer display grabber selected\n");
          } else if (!strcmp(argv[i],"gralloc")) {
            L("Gralloc -- Not supported\n");
          } else if (!strcmp(argv[i],"flinger")) {
            method = FLINGER;
            L("Flinger display grabber selected\n");
          } else {
            L("Grab method \"%s\" not found, sticking with auto-detection.\n",argv[i]);
          }
          break;
          }
        }
      i++;
      }
    }

    L("Initializing grabber method...\n");
    initGrabberMethod();

	if (0 != init_kbd_device) {
	    L("Initializing virtual keyboard and touch device...\n");
	    initInput();
	}
	else {
		L("Do not initialize virtual keyboard or touch device!!!\n");
	}

    L("Initializing VNC server:\n");
    L("	stride:  %d\n", (int)screenformat.stride);
    L("	width:   %d\n", (int)screenformat.width);
    L("	height:  %d\n", (int)screenformat.height);
    L("	bitsPerPixel: %d\n", (int)screenformat.bitsPerPixel);
    L("	size:    %d\n", (int)screenformat.size);
    L("	port:   %d\n", (int)VNC_PORT);


    L("Colourmap_rgba=%d:%d:%d:%d    lenght=%d:%d:%d:%d\n", screenformat.redShift, screenformat.greenShift, screenformat.blueShift,screenformat.alphaShift,
      screenformat.redMax,screenformat.greenMax,screenformat.blueMax,screenformat.alphaMax);  

	if (screenformat.bitsPerPixel == 0) {
		L("Set fake screenformat...\n");
		screenformat.stride = 320;
		screenformat.width = 320;
		screenformat.height = 240;
		screenformat.bitsPerPixel = 32;
		screenformat.size = screenformat.stride * screenformat.height * screenformat.bitsPerPixel / 8;
		screenformat.redShift = 0;
		screenformat.greenShift = 8;
		screenformat.blueShift = 16;
		screenformat.alphaShift = 24;
		screenformat.redMax = 8;
		screenformat.greenMax = 8;
		screenformat.blueMax = 8;
		screenformat.alphaMax = 8;
	}

    initVncServer(argc, argv);

    bindIPCserver();
    sendServerStarted();

    if (rhost) {
      rfbClientPtr cl;
      cl = rfbReverseConnection(vncscr, rhost, rport);
      if (cl == NULL) {
        char *str=malloc(255*sizeof(char));
        sprintf(str,"~SHOW|Couldn't connect to remote host:\n%s\n",rhost);

        L("Couldn't connect to remote host: %s\n",rhost);
        sendMsgToGui(str);
        free(str);
      } else {
        cl->onHold = FALSE;
        rfbStartOnHoldClient(cl);
      }
    }

	L("Start process loop...\n");
    while (1) {
        usec=(vncscr->deferUpdateTime+standby)*1000;
        //clock_t start = clock();
        rfbProcessEvents(vncscr,usec);
      
      if (idle)
        standby+=2;
      else
        standby=2;
      
      if (vncscr->clientHead == NULL)
      {
        idle=1;
        standby=50;
        continue;
      }

      update_screen(); 
      //printf ( "%f\n", ( (double)clock() - start )*1000 / CLOCKS_PER_SEC );
    }
    close_app();
}
