package com.wangling.remotephone;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class BootUpReceiver extends BroadcastReceiver
{
        public void onReceive(Context context, Intent intent)
        {
        	if (false == intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) {
        		return;
        	}
	        
        	int tmpVal = AppSettings.GetSoftwareKeyDwordValue(context, AppSettings.STRING_REGKEY_NAME_AUTO_START, 1);
        	if (1 == tmpVal)
        	{
	            Intent i = new Intent(context, MobileCameraActivity.class);
	            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);//注意，必须添加这个标记，否则启动会失败
	            context.startActivity(i);
        	}
        }
}
