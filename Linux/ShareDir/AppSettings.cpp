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
		printf("\n fopen(\"%s\", \"r\") failed!\n", filename);
		return FALSE;
	}
	
	size_t ret = fread(szValueData, 1, *lpdwLen - 1, fp);
	fclose(fp);
	if (ret > 0 && ret <= *lpdwLen - 1) {
		szValueData[ret] = '\0';
		*lpdwLen = ret + 1;
		return TRUE;
	}
	else {
		printf("\n fread(%s, %d)=%d failed!\n", szValueName, *lpdwLen - 1, ret);
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
	if((fp = fopen(filename, "w")) == NULL) {//打开只写文件，若文件存在则长度清为0，即该文件内容消失，若不存在则创建该文件
		printf("\n fopen(\"%s\", \"w\") failed!\n", filename);
		return FALSE;
	}
	
	fwrite(szValueData, 1, strlen(szValueData), fp);
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
