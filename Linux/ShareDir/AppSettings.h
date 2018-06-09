#ifndef __APP_SETTINGS_H__
#define __APP_SETTINGS_H__


#define  APP_SETTINGS_DIR  "./.settings"

BOOL GetSoftwareKeyValue(const char *szValueName, BYTE *szValueData, DWORD *lpdwLen);
BOOL SaveSoftwareKeyValue(const char *szValueName, const char *szValueData);
BOOL GetSoftwareKeyDwordValue(const char *szValueName, DWORD *lpdwValue);
BOOL SaveSoftwareKeyDwordValue(const char *szValueName, DWORD dwValue);


#endif //__APP_SETTINGS_H__
