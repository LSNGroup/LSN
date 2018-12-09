package com.wangling.remotephone;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.util.Base64;
import android.util.Log;

import com.lsngroup.live.R;


public class AppSettings {

	private static final String fileName = "remotephone_settings";
	private static final String backupfilepath = "/sdcard/rp_settings";
	private static final String savedmacfilepath = "/sdcard/rp_savedmac";
	
	/* For MobileCamera */
	public static final String STRING_REGKEY_NAME_CAMID = "CamId";//int:val
	public static final String STRING_REGKEY_NAME_NODENAME = "NodeName";
	public static final String STRING_REGKEY_NAME_NODEID = "NodeId";
	public static final String STRING_REGKEY_NAME_PASSWORD = "Password";
	public static final String STRING_REGKEY_NAME_SAVED_MAC = "SavedMac";
	
	/* For Viewer */
	public static final String STRING_REGKEY_NAME_VIEWERNODEID = "ViewerNodeId";
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
	
	public static final String STRING_REGKEY_NAME_WITHUAV = "WithUAV";//int:0,1
	public static final String STRING_REGKEY_NAME_TAILSITTER = "TailSitter";//int:0,1
	public static final String STRING_REGKEY_NAME_TAILSITTER_SW_GPSALTI = "SW_GPSALTI";//int Ã×
	public static final String STRING_REGKEY_NAME_TAILSITTER_SW_GNDSPEED = "SW_GNDSPEED";//int ÀåÃ×Ã¿Ãë
	
	public static final String STRING_REGKEY_NAME_VIDEOENC = "VideoEnc";//int:0,1,2
	public static final String STRING_REGKEY_NAME_VIDEOUV = "VideoUV";//int:0,1
	
	public static final String STRING_REGKEY_NAME_BT_ADDRESS = "BtAddress";
	public static final String STRING_REGKEY_NAME_SERIAL_PORT = "SerialPort";//String:/dev/ttyS4
	
	public static final String STRING_REGKEY_NAME_AUTO_START = "AutoStart";//int:0,1
	
	
	private static void customBufferStreamCopy(File source, File target)
	{  
		InputStream fis = null;  
		OutputStream fos = null;  
		try {  
			fis = new FileInputStream(source);  
			fos = new FileOutputStream(target);  
			byte[] buf = new byte[4096];  
			int i;  
			while ((i = fis.read(buf)) != -1) {  
				fos.write(buf, 0, i);  
			}  
		}  
		catch (Exception e) {  
			e.printStackTrace();  
		} finally {  
			try {
				fis.close();
			} catch (IOException e) {}  
			try {
				fos.close();
			} catch (IOException e) {}  
		}  
	} 
	
	private static void BeforeReadSettings(Context context)
	{
		try {
			String file_path = "/data/data/" + context.getPackageName() + "/shared_prefs/" + fileName + ".xml";
			File f = new File(file_path);
			if (false == f.exists()) {
				File backup = new File(backupfilepath);
				if (backup.exists())
				{
					File dir = new File("/data/data/" + context.getPackageName() + "/shared_prefs");
		    		if (false == dir.exists()) {
		    			dir.mkdir();
		    		}
		    		Log.d("AppSettings", "Copy backup to file...");
		    		//Runtime.getRuntime().exec("cat " + backupfilepath + " > " + file_path);
		    		customBufferStreamCopy(backup, f);
				}
			}
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	private static void AfterWriteSettings(Context context)
	{
		try {
			File backup = new File(backupfilepath);
    		if (backup.exists()) {
    			backup.delete();
    		}
    		String file_path = "/data/data/" + context.getPackageName() + "/shared_prefs/" + fileName + ".xml";
    		File f = new File(file_path);
    		Log.d("AppSettings", "Copy file to backup...");
    		//Runtime.getRuntime().exec("cat " + file_path + " > " + backupfilepath);
    		customBufferStreamCopy(f, backup);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public static void delete_backup_settings_file()
	{
		try {
			File backup = new File(backupfilepath);
    		if (backup.exists()) {
    			backup.delete();
    		}
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public static String GetSavedMacFromBackup(Context context)
	{
		File f = new File(savedmacfilepath);
		if (false == f.exists()) {
			return "";
		}
		String s = null;
		String s2 = null;
		try {
			BufferedReader br = new BufferedReader(new FileReader(f));
			s = br.readLine();
			s2 = br.readLine();
			br.close();
			
			s = s.trim();
			String base64 = Base64.encodeToString(s.getBytes(), Base64.DEFAULT);
			if (base64.substring(0, 23).equals(s2.substring(0, 23)) == false)
			{
				s = null;
			}
			
			Log.d("AppSettings", "s=" + s);
			Log.d("AppSettings", "    s2=" + s2);
			Log.d("AppSettings", "base64=" + base64);
		}catch(Exception e){ s = null;}
		
		if (null == s) {
			s = "";
		}
		return s;
	}
	
	public static String GetSoftwareKeyValue(Context context, String keyName, String defValue)
	{
		BeforeReadSettings(context);
		SharedPreferences preferences = context.getSharedPreferences(fileName, Context.MODE_PRIVATE);
		return preferences.getString(keyName, defValue);
	}
	
	public static int GetSoftwareKeyDwordValue(Context context, String keyName, int defValue)
	{
		BeforeReadSettings(context);
		SharedPreferences preferences = context.getSharedPreferences(fileName, Context.MODE_PRIVATE);
		return preferences.getInt(keyName, defValue);
	}
	
	public static long GetSoftwareKeyLongValue(Context context, String keyName, long defValue)
	{
		BeforeReadSettings(context);
		SharedPreferences preferences = context.getSharedPreferences(fileName, Context.MODE_PRIVATE);
		return preferences.getLong(keyName, defValue);
	}
	
	public static void SaveSoftwareKeyValue(Context context, String keyName, String value)
	{
		BeforeReadSettings(context);
		SharedPreferences preferences = context.getSharedPreferences(fileName, Context.MODE_PRIVATE);
		Editor editor = preferences.edit();
		editor.putString(keyName, value);
		editor.commit();
		AfterWriteSettings(context);
	}
	
	public static void SaveSoftwareKeyDwordValue(Context context, String keyName, int value)
	{
		BeforeReadSettings(context);
		SharedPreferences preferences = context.getSharedPreferences(fileName, Context.MODE_PRIVATE);
		Editor editor = preferences.edit();
		editor.putInt(keyName, value);
		editor.commit();
		AfterWriteSettings(context);
	}
	
	public static void SaveSoftwareKeyLongValue(Context context, String keyName, long value)
	{
		BeforeReadSettings(context);
		SharedPreferences preferences = context.getSharedPreferences(fileName, Context.MODE_PRIVATE);
		Editor editor = preferences.edit();
		editor.putLong(keyName, value);
		editor.commit();
		AfterWriteSettings(context);
	}
}
