package com.wangling.remotephone;

import java.io.File;
import java.lang.reflect.Method;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;

import com.lsngroup.live.R;

import android.app.Activity;
import android.app.KeyguardManager;
import android.app.KeyguardManager.KeyguardLock;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaPlayer;
import android.media.MediaRecorder;
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
import android.os.Vibrator;
import android.provider.Settings;
import android.util.Base64;
import android.util.Log;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;


public class MobileCameraActivity extends Activity {
	
	
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
				;
				;
				Message send_msg = _instance.mWorkerHandler.obtainMessage(WORK_MSG_CHECK);
                _instance.mWorkerHandler.sendMessageDelayed(send_msg, 15000);
				break;
			}
			
			super.handleMessage(msg);
		}
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
				
				_instance.setTitle("" + _instance.getTitle() + " [ID:" + comments_id + "]");
				SharedFuncLib.MyMessageTip(_instance, "Online (ID:" + comments_id + ")");
				
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
	
	
	private static MobileCameraActivity _instance = null;//////////////////
	
	private static final String TAG = "MobileCameraActivity";
	private PowerManager.WakeLock m_wl = null;
	private WorkerHandler mWorkerHandler = null;
	private MainHandler mMainHandler = null;
	
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
    
	private boolean m_isClientConnected = false;
	private boolean m_isCaptureRunning = false;
    
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.mobilecamera);
        
    	Log.d(TAG, "onCreate~~~");
        
        Log.d(TAG, "Acquiring wake lock");
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        m_wl = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE, "MobileCameraActivity SCREEN_BRIGHT_WAKE_LOCK");
        m_wl.acquire();
        
        //设置WiFi永不休眠
        try {
        	Settings.System.putInt(getContentResolver(),
                    android.provider.Settings.System.WIFI_SLEEP_POLICY,
                    Settings.System.WIFI_SLEEP_POLICY_NEVER);
        } catch (Exception e) {
			e.printStackTrace();
		}
        
        
        Worker worker = new Worker("MobileCameraActivity Worker");
        mWorkerHandler = new WorkerHandler(worker.getLooper());
        mMainHandler = new MainHandler();
        
		
        m_sfv = (SurfaceView)findViewById(R.id.preview_surfaceview);
        m_sfh = m_sfv.getHolder();
        m_sfh.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);//For camera preview!!!
        m_sfh.addCallback(new MySurfaceHolderCallback());
        
        
    	//AudioManager audioManager = (AudioManager)getApplication().getSystemService(Context.AUDIO_SERVICE);
    	//int max = audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
    	//audioManager.setSpeakerphoneOn(true);
    	//audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, max, 0);
    	
        
        _instance = this;//////////////////
        
        Message send_msg = _instance.mWorkerHandler.obtainMessage(WORK_MSG_CHECK);
        _instance.mWorkerHandler.sendMessageDelayed(send_msg, 100);
        
        
        findViewById(R.id.push_start_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnPushStart();
        	}
        });
        
        findViewById(R.id.push_end_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnPushEnd();
        	}
        });
        
        findViewById(R.id.push_start_btn).setEnabled(true);
        findViewById(R.id.push_end_btn).setEnabled(false);
        
        
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
				
			}
		}).start();
		
        /* Start native main... */
        SetThisObject();
        StartNativeMain("utf8", getResources().getString(R.string.app_lang), getPackageName());
    }
    
    @Override
    protected void onDestroy() {
    	
    	Log.d(TAG, "onDestroy~~~");
    	
    	_instance = null;//////////////////
    	
        Log.d(TAG, "Release wake lock");
    	m_wl.release();
    	
    	mWorkerHandler.getLooper().quit();
    	
    	m_bQuitRecord = true;
    	
    	StopDoConnection();
    	ForceDisconnect();
    	
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
        
    	super.onDestroy();
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
    
    private void onBtnPushStart()
    {
    	findViewById(R.id.push_start_btn).setEnabled(false);
    	
    	_instance.setTitle(_instance.getResources().getString(R.string.app_name));
    	SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_please_wait));
    	StartDoConnection();
    }
    
    private void onBtnPushEnd()
    {
    	_instance.setTitle(_instance.getResources().getString(R.string.app_name));
    	SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_please_wait));
    	StopDoConnection();
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
    
    public static int j_hw264_capture_start(int width, int height, int fps, int channel)
    {
    	return -1;
    }
    
    public static void j_hw264_capture_stop()
    {
    	
    }
    
    public static int j_hw263_capture_start(int width, int height, int fps, int channel)
    {
    	return -1;
    }
    
    public static void j_hw263_capture_stop()
    {
    	
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
	    		Log.d(TAG, "MaxZoom=" + max + ", curr=" + value);
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
	    		Log.d(TAG, "MaxZoom=" + max + ", curr=" + value);
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
    	
    	return strOS;
    }
	
    public static void j_on_push_result(int ret, int joined_channel_id)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (ret < 0)
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_communication_error));
    				_instance.findViewById(R.id.push_start_btn).setEnabled(true);
    				_instance.findViewById(R.id.push_end_btn).setEnabled(false);
    			}
    		});
    	}
    	else if (joined_channel_id == 0)
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_push_start_failed));
    				_instance.findViewById(R.id.push_start_btn).setEnabled(true);
    				_instance.findViewById(R.id.push_end_btn).setEnabled(false);
    			}
    		});
    	}
    	else if (joined_channel_id > 0)
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				_instance.findViewById(R.id.push_start_btn).setEnabled(false);
    				_instance.findViewById(R.id.push_end_btn).setEnabled(true);
    			}
    		});
    		
	    	int comments_id = joined_channel_id;
	    	boolean approved = true;
	    	Message msg = _instance.mMainHandler.obtainMessage(UI_MSG_DISPLAY_CAMERA_ID, comments_id, (approved ? 1 : 0));
	    	_instance.mMainHandler.sendMessage(msg);
    	}
    }
    
    public static void j_on_register_network_error()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	_instance.mMainHandler.post(new Runnable(){
			@Override
			public void run() {
				SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_communication_error));
				_instance.findViewById(R.id.push_start_btn).setEnabled(true);
				_instance.findViewById(R.id.push_end_btn).setEnabled(false);
			}
		});
    }
    
    public static void j_on_client_connected()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.m_isClientConnected = true;
    	
    	//_instance.startVNCServer();
    	
    	if (true)
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_on_client_connected));
    				_instance.findViewById(R.id.push_start_btn).setEnabled(false);
    				_instance.findViewById(R.id.push_end_btn).setEnabled(true);
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
    	
    	if (true)
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_on_client_disconnected));
    				_instance.findViewById(R.id.push_start_btn).setEnabled(true);
    				_instance.findViewById(R.id.push_end_btn).setEnabled(false);
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
    public native void ForceDisconnect();
    
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
