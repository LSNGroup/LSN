#include "stdafx.h"
#include "Repeater.h"
#include "PacketRepeater.h"

#include "Shiyong.h"
#include "Guaji.h"

#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#include "LogMsg.h"



static BYTE CheckNALUType(BYTE bNALUHdr)
{
	return ((bNALUHdr & 0x1f) >> 0);
}

static BYTE CheckNALUNri(BYTE bNALUHdr)
{
	return ((bNALUHdr & 0x60) >> 5);
}

void fake_rtp_recv_fn(void *ctx, int payload_type, unsigned long rtptimestamp, unsigned char *data, int len)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)ctx;
	if (NULL == pViewerNode) {
		return;
	}

	if (PAYLOAD_VIDEO == payload_type)
	{
		BYTE bType = CheckNALUType(data[8]);
#if 0////Debug
		if (bType != 28 && bType != NALU_TYPE_SLICE_NOPART && bType != NALU_TYPE_SLICE_PARTA && bType != NALU_TYPE_SLICE_PARTB && bType != NALU_TYPE_SLICE_PARTC)
		{
			char szMsg[MAX_PATH];
			sprintf(szMsg, "PacketRepeater: Recv H.264 NALU, type=%d", bType);
			log_msg(szMsg, LOG_LEVEL_INFO);
		}
#endif
		if (0 == pViewerNode->m_sps_len && NALU_TYPE_SEQ_SET == bType)
		{
			memcpy(pViewerNode->m_sps_buff, data, len);
			pViewerNode->m_sps_len = len;
			printf("ViewerNode[%d]: H.264 SPS(%d bytes) 已获取！\n", pViewerNode->nID, len);
		}
		if (0 == pViewerNode->m_pps_len && NALU_TYPE_PIC_SET == bType)
		{
			memcpy(pViewerNode->m_pps_buff, data, len);
			pViewerNode->m_pps_len = len;
			printf("ViewerNode[%d]: H.264 PPS(%d bytes) 已获取！\n", pViewerNode->nID, len);
		}
	}

	if (g_pShiyong != NULL &&  g_pShiyong->currentSourceIndex != -1 && g_pShiyong->currentSourceIndex == pViewerNode->nID) {

			for (int i = 0; i < MAX_SERVER_NUM; i++)
			{
				if (arrServerProcesses[i].m_bAVStarted == FALSE) {
					continue;
				}

				if (payload_type == PAYLOAD_TLV && arrServerProcesses[i].m_bTLVEnable == FALSE) {
					continue;
				}
				else if (payload_type == PAYLOAD_AUDIO && arrServerProcesses[i].m_bAudioEnable == FALSE) {
					continue;
				}
				else if (payload_type == PAYLOAD_VIDEO && arrServerProcesses[i].m_bVideoEnable == FALSE) {
					continue;
				}
				
				//此处如果用CtrlCmd_Send_FAKERTP_RESP_NOMUTEX()，会出错！！！
				int ret = CtrlCmd_Send_FAKERTP_RESP(SOCKET_TYPE_TCP, arrServerProcesses[i].m_fhandle, data, len);
				if (ret < 0) {
					log_msg("PacketRepeater: CtrlCmd_Send_FAKERTP_RESP() failed!", LOG_LEVEL_ERROR);
				}
			}
	}
}
