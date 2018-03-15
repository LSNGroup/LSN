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

//this file implements a simple IPC connection with the Java GUI

#include <sys/socket.h>
#include <sys/un.h>
#include "gui.h"
#include "common.h"

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         1024
#define QUEUE_SIZE          1

char unix_13131[] = "unix_13131";
char unix_13132[] = "unix_13132";

static int hServerSocket;  /* handle to socket */
static struct sockaddr_un Address; /* Internet socket address stuct */
static char pBuffer[BUFFER_SIZE];


int sendMsgToGui(char *buffer)
{
  L(buffer);
  return 0;
}  

int bindIPCserver()
{
  L("Starting IPC connection...");

  pthread_t thread;
  pthread_create( &thread,NULL,handle_connections,NULL);

  return 1;
}



void *handle_connections()
{

  /* make a socket */
  int hSocket=socket(AF_UNIX, SOCK_STREAM, 0);

  if(hSocket == SOCKET_ERROR) {
    L("Error creating socket\n");
    return 0;
  }

  /* fill address struct */
  bzero(&Address, sizeof(Address));
  Address.sun_family=AF_UNIX;
  Address.sun_path[0] = '\0';
  strncpy(Address.sun_path + 1, unix_13132, sizeof(Address.sun_path) - 1);
  int length = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(unix_13132);

  /* bind to a port */
  if(bind(hSocket,(struct sockaddr*)&Address,length) == SOCKET_ERROR)
  {
    L("\nCould not connect to IPC gui, another daemon already running?\n");
    close(hSocket);
    close_app();////只运行一个EXE实例
    return 0;
  }
  listen(hSocket,1);
  
  L("\nWaiting for a connection...\n");
  socklen_t client_len = sizeof(struct sockaddr_un);
  struct sockaddr_un client_address;
  hServerSocket = accept(hSocket,(struct sockaddr *)&client_address,(socklen_t *)&client_len);
  close(hSocket);

  int n;

  while (1) {
    n = recv(hServerSocket,pBuffer,BUFFER_SIZE,0);
    if (n < 0) perror("recv");
	pBuffer[n] = '\0';
	
    //L("Recebido: %s\n",pBuffer);

    if (strstr(pBuffer,"~PING|")!=NULL)
    {
      char *resp="~PONG|";
      n = send(hServerSocket,resp,strlen(resp),0);
      if (n  < 0) perror("send");
    }
    else if (strstr(pBuffer,"~KILL|")!=NULL)
    close_app();
  }
}


void unbindIPCserver()
{
  close(hServerSocket);
}
