package com.wangling.remotephone;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;


public class SmsComeReceiver extends BroadcastReceiver
{
	private static final String TAG = "SmsComeReceiver";
	
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
