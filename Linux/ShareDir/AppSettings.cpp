#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"
#include "AppSettings.h"


BOOL GetSoftwareKeyValue(const char *szValueName, BYTE *szValueData, DWORD *lpdwLen)
{
	FILE *fp;
	char filename[MAX_PATH];
	
	if(NULL == szValueName || NULL == szValueData || NULL == lpdwLen) {
		return FALSE;
	}
	
	snprintf(filename, sizeof(filename), "%s/%s", APP_SETTINGS_DIR, szValueName);
	if((fp = fopen(filename, "r")) == NULL) {
		return FALSE;
	}
	
	size_t ret = fread(szValueData, *lpdwLen - 1, 1, fp);
	fclose(fp);
	if (ret > 0 && ret <= *lpdwLen - 1) {
		szValueData[ret] = '\0';
		*lpdwLen = ret + 1;
		return TRUE;
	}
	else {
		*lpdwLen = 0;
		return FALSE;
	}
}

BOOL SaveSoftwareKeyValue(const char *szValueName, const char *szValueData)
{
	FILE *fp;
	char filename[MAX_PATH];
	
	if(NULL == szValueName || NULL == szValueData) {
		return FALSE;
	}
	
	snprintf(filename, sizeof(filename), "%s/%s", APP_SETTINGS_DIR, szValueName);
	if((fp = fopen(filename, "w")) == NULL) {//��ֻд�ļ������ļ������򳤶���Ϊ0�������ļ�������ʧ�����������򴴽����ļ�
		return FALSE;
	}
	
	fwrite(szValueData, strlen(szValueData), 1, fp);
	fclose(fp);
	return TRUE;
}

BOOL GetSoftwareKeyDwordValue(const char *szValueName, DWORD *lpdwValue)
{
	char szValueData[16];
	DWORD dwDataLen = 16, cntScaned;
	BOOL bResult = FALSE;

	if(NULL == lpdwValue) {
		goto exit;
	}

	if(GetSoftwareKeyValue(szValueName,(BYTE *)szValueData,&dwDataLen) && dwDataLen > 0) {
		cntScaned = sscanf(szValueData,"%d",lpdwValue);
		if(cntScaned > 0) { 
			bResult = TRUE;
		}
	}

exit:
	return bResult;
}

BOOL SaveSoftwareKeyDwordValue(const char *szValueName, DWORD dwValue)
{
	char szValueData[16];

	sprintf(szValueData, "%ld", dwValue);
	return SaveSoftwareKeyValue(szValueName, szValueData);
}
