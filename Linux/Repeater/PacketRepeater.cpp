//#include "stdafx.h"
#include "Repeater.h"
#include "PacketRepeater.h"

#include "Shiyong.h"
#include "IpcGuaji.h"

#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#include "LogMsg.h"



BYTE CheckNALUType(BYTE bNALUHdr)
{
	return ((bNALUHdr & 0x1f) >> 0);
}

BYTE CheckNALUNri(BYTE bNALUHdr)
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

			//首先把数据包保存到缓冲队列。。。此处可能会导致重复发送，不过观看端可以正常处理重复包。
			int queue_count = 0;
			pthread_mutex_lock(&(g_pShiyong->arrayCriticalSec[payload_type]));
			queue_count = wps_count_que(g_pShiyong->arrayRecvQueue[payload_type]);
			pthread_mutex_unlock(&(g_pShiyong->arrayCriticalSec[payload_type]));
			if (   (PAYLOAD_AUDIO == payload_type && queue_count >= MAX_AUDIO_RECV_QUEUE_SIZE)
				|| (PAYLOAD_VIDEO == payload_type && queue_count >= MAX_VIDEO_RECV_QUEUE_SIZE)  )
			{
				pthread_mutex_lock(&(g_pShiyong->arrayCriticalSec[payload_type]));
				wps_queue *temp = g_pShiyong->arrayRecvQueue[payload_type];
				RECV_PACKET_SMALL *p = (RECV_PACKET_SMALL *)get_qbody(temp);
				wps_remove_que(&(g_pShiyong->arrayRecvQueue[payload_type]), temp);
				free(p);
				pthread_mutex_unlock(&(g_pShiyong->arrayCriticalSec[payload_type]));
			}

			RECV_PACKET_SMALL *pNewPack = NULL;
			if (len <= RECV_PACKET_SMALL_BUFF_SIZE) {
				pNewPack = (RECV_PACKET_SMALL *)malloc(sizeof(RECV_PACKET_SMALL));
			}
			else if (len <= RECV_PACKET_MEDIUM_BUFF_SIZE) {
				pNewPack = (RECV_PACKET_SMALL *)malloc(sizeof(RECV_PACKET_MEDIUM));
			}
			else if (len <= RECV_PACKET_BIG_BUFF_SIZE) {
				pNewPack = (RECV_PACKET_SMALL *)malloc(sizeof(RECV_PACKET_BIG));
			}

			if (NULL != pNewPack)
			{
				memcpy(pNewPack->szData, data, len);
				pNewPack->nDataSize = len;
				pNewPack->wSeqNum = ntohs(pf_get_word(pNewPack->szData + 2));
				pNewPack->bReserved = *((unsigned char *)(pNewPack->szData) + 1);
				pNewPack->dfRecvTime = (double)rtptimestamp/1000;//借用字段，保存原始时间戳

				g_pShiyong->PutIntoRecvPacketQueue(payload_type, pNewPack);
			}


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
