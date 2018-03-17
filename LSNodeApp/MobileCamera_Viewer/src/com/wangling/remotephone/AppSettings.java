package com.wangling.remotephone;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import com.lsngroup.live.R;


public class AppSettings {

	private static final String fileName = "remotephone_settings";
	
	/* For Viewer */
	public static final String STRING_REGKEY_NAME_VIEWERNODEID = "ViewerNodeId";
	public static final String STRING_REGKEY_NAME_QUERYIDS = "QueryIds";
	public static final String STRING_REGKEY_NAME_CAM_PASSWORD = "_Password";
	
	public static final String STRING_REGKEY_NAME_CAM_AVPARAM_AUDIOENABLE = "_AvParam_AudioEnable";//int:0,1
	public static final String STRING_REGKEY_NAME_CAM_AVPARAM_AUDIODEVICE = "_AvParam_AudioDevice";//int:channel
	public static final String STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOENABLE = "_AvParam_VideoEnable";//int:0,1
	public static final String STRING_REGKEY_NAME_CAM_AVPARAM_VIDEODEVICE = "_AvParam_VideoDevice";//int:channel
	public static final String STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOMODE = "_AvParam_VideoMode";//int:mode_def_val
	public static final String STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOSIZE = "_AvParam_VideoSize";//int:size_def_val
	public static final String STRING_REGKEY_NAME_CAM_AVPARAM_FRAMERATE = "_AvParam_Framerate";//int:fps
	public static final String STRING_REGKEY_NAME_CAM_AVPARAM_AUDIOREDUNDANCE = "_AvParam_AudioRedundance";//int:0,1
	public static final String STRING_REGKEY_NAME_CAM_AVPARAM_VIDEORELIABLE = "_AvParam_VideoReliable";//int:0,1
	
	public static final String STRING_REGKEY_NAME_LEFT_THROTTLE = "UseLeftThrottle";//int:0,1
	
	
	public static String GetSoftwareKeyValue(Context context, String keyName, String defValue)
	{
		SharedPreferences preferences = context.getSharedPreferences(fileName, Context.MODE_PRIVATE);
		return preferences.getString(keyName, defValue);
	}

	public static int GetSoftwareKeyDwordValue(Context context, String keyName, int defValue)
	{
		SharedPreferences preferences = context.getSharedPreferences(fileName, Context.MODE_PRIVATE);
		return preferences.getInt(keyName, defValue);
	}

	public static void SaveSoftwareKeyValue(Context context, String keyName, String value)
	{
		SharedPreferences preferences = context.getSharedPreferences(fileName, Context.MODE_PRIVATE);
		Editor editor = preferences.edit();
		editor.putString(keyName, value);
		editor.commit();
	}

	public static void SaveSoftwareKeyDwordValue(Context context, String keyName, int value)
	{
		SharedPreferences preferences = context.getSharedPreferences(fileName, Context.MODE_PRIVATE);
		Editor editor = preferences.edit();
		editor.putInt(keyName, value);
		editor.commit();
	}
}
