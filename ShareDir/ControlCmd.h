#ifndef _CONTROLCMD_H
#define _CONTROLCMD_H


/*

Control Channel:

2 Bytes, Command Code.
4 Bytes, Data Length.
N Bytes, Data.

*/

#define CTRLCMD_MAX_PACKET_SIZE		2048  /* in bytes */


/* Definition of Result Code */
#define CTRLCMD_RESULT_NG			0x0000
#define CTRLCMD_RESULT_OK			0x0001


/* Definition of AV flags */
#define AV_FLAGS_VIDEO_ENABLE		0x01  /* bit 0 */
#define AV_FLAGS_AUDIO_ENABLE		0x02  /* bit 1 */
#define AV_FLAGS_VIDEO_RELIABLE		0x04  /* bit 2 */
#define AV_FLAGS_AUDIO_REDUNDANCE	0x08  /* bit 3 */
#define AV_FLAGS_VIDEO_HWACCE		0x10  /* bit 4 */
#define AV_FLAGS_AUDIO_HWACCE		0x20  /* bit 5 */
#define AV_FLAGS_VIDEO_H264			0x40  /* bit 6 */
#define AV_FLAGS_AUDIO_G729A		0x80  /* bit 7 */


/* Definition of AV video mode */
#define AV_VIDEO_MODE_NULL			0x00
#define AV_VIDEO_MODE_X264			0x01
#define AV_VIDEO_MODE_FF263			0x02
#define AV_VIDEO_MODE_FF264			0x03


/* Definition of AV video size */
#define AV_VIDEO_SIZE_NULL			0x00
#define AV_VIDEO_SIZE_QCIF			0x01  /* 176 x 144 */
#define AV_VIDEO_SIZE_QVGA			0x02  /* 320 x 240 */
#define AV_VIDEO_SIZE_CIF			0x03  /* 352 x 288 */
#define AV_VIDEO_SIZE_480x320		0x04  /* 480 x 320 */
#define AV_VIDEO_SIZE_VGA			0x05  /* 640 x 480 */


/* Definition of AV Control */
#define AV_CONTRL_TURN_CENTER		0x0000
#define AV_CONTRL_TURN_UP			0x0001
#define AV_CONTRL_TURN_DOWN			0x0002
#define AV_CONTRL_TURN_LEFT			0x0003
#define AV_CONTRL_TURN_RIGHT		0x0004
#define AV_CONTRL_ZOOM_IN			0x0005
#define AV_CONTRL_ZOOM_OUT			0x0006
#define AV_CONTRL_RECORDVOL_UP		0x0007
#define AV_CONTRL_RECORDVOL_DOWN	0x0008
#define AV_CONTRL_RECORDVOL_SET		0x0009  /* contrl_param */
#define AV_CONTRL_FLASH_ON			0x000a
#define AV_CONTRL_FLASH_OFF			0x000b
#define AV_CONTRL_LEFT_SERVO		0x000c  /* contrl_param */
#define AV_CONTRL_RIGHT_SERVO		0x000d  /* contrl_param */
#define AV_CONTRL_SEARCH_POWER		0x000e
#define AV_CONTRL_MOVE_ADVANCE			0x0011  /* contrl_param */
#define AV_CONTRL_MOVE_BACK				0x0012  /* contrl_param */
#define AV_CONTRL_MOVE_ADVANCE_LEFT		0x0013  /* contrl_param */
#define AV_CONTRL_MOVE_ADVANCE_RIGHT	0x0014  /* contrl_param */
#define AV_CONTRL_MOVE_BACK_LEFT		0x0015  /* contrl_param */
#define AV_CONTRL_MOVE_BACK_RIGHT		0x0016  /* contrl_param */
#define AV_CONTRL_MOVE_STOP				0x0017
#define AV_CONTRL_SYSTEM_SHUTDOWN		0x0021
#define AV_CONTRL_SYSTEM_REBOOT			0x0022


/* Definition of func_flags */
#define FUNC_FLAGS_AV		0x01
#define FUNC_FLAGS_VNC		0x02
#define FUNC_FLAGS_FT		0x04
#define FUNC_FLAGS_ADB		0x08
#define FUNC_FLAGS_WEBMONI	0x10
#define FUNC_FLAGS_SHELL	0x20
#define FUNC_FLAGS_HASROOT		0x40
#define FUNC_FLAGS_ACTIVATED	0x80


/* Command Code Definiton */

#define CMD_CODE_FAKERTP_RESP	0x0000  /* CMD_CODE_FAKERTP_RESP | n | FakeRtp Packet */

#define CMD_CODE_END			0x00FF  /* CMD_CODE_END  | 0 */

#define CMD_CODE_NULL			0x00EE  /* CMD_CODE_NULL  | 0 */

#define CMD_CODE_HELLO_REQ		0x0001  /* CMD_CODE_HELLO_REQ | 6+4+256 | client_node_id | client_version | password */
#define CMD_CODE_HELLO_RESP		0x8001  /* CMD_CODE_HELLO_RESP | 6+4+1+2 | server_node_id | server_version | func_flags | result_code */

#define CMD_CODE_RUN_REQ		0x0002  /* CMD_CODE_RUN_REQ | n | run_exe */
#define CMD_CODE_RUN_RESP		0x8002  /* CMD_CODE_RUN_RESP | n | result string */

#define CMD_CODE_PROXY_REQ		0x0003  /* CMD_CODE_PROXY_REQ | 2 | tcp_port */

#define CMD_CODE_PROXY_DATA		0x0004  /* CMD_CODE_PROXY_DATA | n | n bytes data */

#define CMD_CODE_PROXY_END		0x0005  /* CMD_CODE_PROXY_END | 0 */

#define CMD_CODE_ARM_REQ		0x0006  /* CMD_CODE_ARM_REQ | 0 */

#define CMD_CODE_DISARM_REQ		0x0007  /* CMD_CODE_DISARM_REQ | 0 */

#define CMD_CODE_AV_START_REQ	0x0008  /* CMD_CODE_AV_START_REQ | 1+1+1+4+4 | flags | video_size | video_framerate | audio_channel | video_channel */

#define CMD_CODE_AV_STOP_REQ	0x0009  /* CMD_CODE_AV_STOP_REQ | 0 */

#define CMD_CODE_AV_SWITCH_REQ	0x000A  /* CMD_CODE_AV_SWITCH_REQ | 4 | video_channel */

#define CMD_CODE_AV_CONTRL_REQ	0x000B  /* CMD_CODE_AV_CONTRL_REQ | 2+4 | contrl | contrl_param */

#define CMD_CODE_VOICE_REQ		0x000C  /* CMD_CODE_VOICE_REQ | n | G729a Data */

#define CMD_CODE_TEXT_REQ		0x000D  /* CMD_CODE_TEXT_REQ | 512 | Text Data */

#define CMD_CODE_BYE_REQ		0x000E  /* CMD_CODE_BYE_REQ | 0 */



int  CtrlCmd_Init();
void CtrlCmd_Uninit();


//#ifdef JNI_FOR_MOBILECAMERA
int CtrlCmd_Send_FAKERTP_RESP(SOCKET_TYPE type, SOCKET fhandle, BYTE *packet, int len);
int CtrlCmd_Send_FAKERTP_RESP_NOMUTEX(SOCKET_TYPE type, SOCKET fhandle, BYTE *packet, int len);
int CtrlCmd_Send_HELLO_RESP(SOCKET_TYPE type, SOCKET fhandle, BYTE *server_node_id, DWORD server_version, BYTE func_flags, WORD result_code);
int CtrlCmd_Send_RUN_RESP(SOCKET_TYPE type, SOCKET fhandle, const char *result_str);

int CtrlCmd_Send_NULL(SOCKET_TYPE type, SOCKET fhandle);

int CtrlCmd_Send_END(SOCKET_TYPE type, SOCKET fhandle);

void CtrlCmd_Recv_AV_END(SOCKET_TYPE type, SOCKET fhandle);



//#else
int CtrlCmd_HELLO(SOCKET_TYPE type, SOCKET fhandle, BYTE *client_node_id, DWORD client_version, const char *password, BYTE *server_node_id, DWORD *server_version, BYTE *func_flags, WORD *result_code);////Send/Recv
int CtrlCmd_RUN(SOCKET_TYPE type, SOCKET fhandle, const char *exe_cmd, char *result_buf, int buf_size);////Send/Recv

int CtrlCmd_PROXY(SOCKET_TYPE type, SOCKET fhandle, WORD wTcpPort);
int CtrlCmd_PROXY_DATA(SOCKET_TYPE type, SOCKET fhandle, BYTE *data, int len);
int CtrlCmd_PROXY_END(SOCKET_TYPE type, SOCKET fhandle);

int CtrlCmd_ARM(SOCKET_TYPE type, SOCKET fhandle);
int CtrlCmd_DISARM(SOCKET_TYPE type, SOCKET fhandle);
int CtrlCmd_AV_START(SOCKET_TYPE type, SOCKET fhandle, BYTE flags, BYTE video_size, BYTE video_framerate, DWORD audio_channel, DWORD video_channel);
int CtrlCmd_AV_STOP(SOCKET_TYPE type, SOCKET fhandle);
int CtrlCmd_AV_SWITCH(SOCKET_TYPE type, SOCKET fhandle, DWORD video_channel);
int CtrlCmd_AV_CONTRL(SOCKET_TYPE type, SOCKET fhandle, WORD contrl, DWORD contrl_param);
int CtrlCmd_VOICE(SOCKET_TYPE type, SOCKET fhandle, BYTE *data, int len);
int CtrlCmd_TEXT(SOCKET_TYPE type, SOCKET fhandle, BYTE *data, int len);
int CtrlCmd_BYE(SOCKET_TYPE type, SOCKET fhandle);
//#endif


#endif /* _CONTROLCMD_H */
