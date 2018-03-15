package com.wangling.remotephone;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class MyReceiver extends BroadcastReceiver
{
        public void onReceive(Context context, Intent intent)
        {
        	int tmpVal = AppSettings.GetSoftwareKeyDwordValue(context, AppSettings.STRING_REGKEY_NAME_AUTO_START, 1);
        	if (1 == tmpVal)
        	{
	            Intent i = new Intent(context, MobileCameraService.class);
	            context.startService(i);
        	}
        }
}
