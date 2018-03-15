package com.wangling.remotephone;


import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaPlayer;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.TextView;
import com.wangling.remotephone.R;


public class AvPlayActivity extends Activity {
	
	static final int UI_MSG_MESSAGEBOX = 1;
	static final int UI_MSG_MESSAGETIP = 2;
	static final int UI_MSG_RELEASE_MP = 3;
	static final int UI_MSG_VOICE_TIMEOUT = 4;
	static final int UI_MSG_DISPLAY_SENSOR = 5;
	
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
			case UI_MSG_MESSAGEBOX://obj
				SharedFuncLib.MyMessageBox(_instance, _instance.getResources().getString(R.string.app_name), (String)(msg.obj));
				break;
				
			case UI_MSG_MESSAGETIP://obj
				SharedFuncLib.MyMessageTip(_instance, (String)(msg.obj));
				break;
				
			case UI_MSG_RELEASE_MP://obj
				MediaPlayer mp = (MediaPlayer)(msg.obj);
				mp.stop();
				mp.release();
				Log.d(TAG, "MediaPlayer released!");
				break;
				
			case UI_MSG_VOICE_TIMEOUT:
				onBtnShoutUp();
				break;
				
			case UI_MSG_DISPLAY_SENSOR:
				if (null != _instance && false == _instance.m_bQuitPlay)
				{
					//double[] sensorData = SharedFuncLib.GetSensorData();
					
					mMainHandler.sendEmptyMessageDelayed(UI_MSG_DISPLAY_SENSOR, 500);
				}
				break;
				
			default:
				break;				
			}
			
			super.handleMessage(msg);
		}
	}
	
	
    class VoiceThread implements Runnable { 
        public void run()
        {
        	AudioRecord audioRecord = null;
        	int bufferSize = 16000*20;
        	byte[] buff = new byte[bufferSize];
        	int count = 0;
        	int ret;
        	
        	try {
    	    	int buffSize = AudioRecord.getMinBufferSize(
    	    			8000,
    	    			AudioFormat.CHANNEL_CONFIGURATION_MONO,
    	    			AudioFormat.ENCODING_PCM_16BIT);
    	    	
    	    	audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,
    	    			8000,
    	    			AudioFormat.CHANNEL_CONFIGURATION_MONO,
    	    			AudioFormat.ENCODING_PCM_16BIT,
    	    			buffSize * 8);
        	}
            catch (Exception e){
                return;
            }
        	
        	try {
        		audioRecord.startRecording();
        	}
            catch (Exception e){
            	audioRecord.release();
            	audioRecord = null;
                return;
            }
        	
        	while (false == m_bQuitVoice)
        	{
        		if (count >= bufferSize)
        		{
        			break;
        		}
        		
        		Log.d(TAG, "audioRecord.read( " + (bufferSize - count) + " bytes )");
        		ret = audioRecord.read(buff, count, bufferSize - count);//200ms
        		Log.d(TAG, "audioRecord.read() ==> " + ret + " bytes )");
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
        	
        	if (false == m_bQuitVoice) {
        		Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_VOICE_TIMEOUT);
	        	_instance.mMainHandler.sendMessage(send_msg);
        	}
        	
        	if (count >= 16000) // 1 second
        	{
        		SharedFuncLib.SendVoice(conn_type, conn_fhandle, buff, count);
        	}
        	else {
        		Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, _instance.getResources().getString(R.string.msg_voice_too_short));
	        	_instance.mMainHandler.sendMessage(send_msg);
        	}
        	
        	
        	audioRecord.stop();
        	audioRecord.release();
        	audioRecord = null;
        }
    }
	
    class AudioRecvThread implements Runnable { 
        public void run()
        {
        	SharedFuncLib.AudioRecvStart();
        	
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
        	
        	int[] arrBreak = new int[2];
        	
        	while (false == m_bQuitPlay)
        	{
        		byte[] pcm_data = SharedFuncLib.AudioRecvGetData(arrBreak);
        		if (1 == arrBreak[0]) {
        			break;
        		}
        		m_lStreamTime = arrBreak[1];
        		
        		if (pcm_data != null && pcm_data.length > 0){
        			if (m_bQuitVoice) {
        				audiotrack.write(pcm_data, 0, pcm_data.length);
        			}
        		}
        	}
        	
        	audiotrack.stop();
        	audiotrack.release();
        	
        	SharedFuncLib.AudioRecvStop();
        }
    }
    
    class VideoRecvThread implements Runnable {
        public void run()
        {
        	int v_width;
        	int v_height;
        	float left =0.0f;
        	float top = 0.0f;
        	int disp_width = 0;
        	int disp_height = 0;
        	int fr_width = 0;
        	int fr_height = 0;
        	int fr_timePerFrame = 0;
        	boolean fr_full = false;
        	int[] arrBreak = new int[6];
        	
        	if (0 != (av_flags & SharedFuncLib.AV_FLAGS_VIDEO_H264)) {
        		SharedFuncLib.FF264RecvStart();
        	}
        	else {
        		SharedFuncLib.FF263RecvStart();
        	}
        	
        	while (false == m_bQuitPlay)
        	{
        		int[] data = null;        		
        		
            	if (0 != (av_flags & SharedFuncLib.AV_FLAGS_VIDEO_H264)) {
            		data = SharedFuncLib.FF264RecvGetData(arrBreak);
            	}
            	else {
            		data = SharedFuncLib.FF263RecvGetData(arrBreak);
            	}
        		
        		if (1 == arrBreak[0]) {
        			break;
        		}
        		if (data == null || data.length <= 1) {
        			continue;
        		}
        		
        		fr_timePerFrame = arrBreak[3];
        		fr_full = (arrBreak[4] == 1);
        		long videoTime = arrBreak[5];        		
        		int i;
        		for (i = 0; i < 100; i++)/////////AV
        		{
        			if (videoTime < m_lStreamTime + 10 - 200) {
        				break;
        			}
        			if (m_bQuitPlay) {
        				break;
        			}
        					
	            	try {
	        			Thread.sleep(20);
	        		} catch (Exception e) {
	        			// TODO Auto-generated catch block
	        			e.printStackTrace();
	        		}
        		}
        		if (i >= 100) {
        			m_lStreamTime = Integer.MAX_VALUE;
        			
        			Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, _instance.getResources().getString(R.string.msg_audio_cant_open));
	        		_instance.mMainHandler.sendMessage(send_msg);
        		}
        		
        		////if (0 == fr_width || 0 == fr_height) {
        			fr_width = arrBreak[1];
        			fr_height = arrBreak[2];
        			fr_timePerFrame = arrBreak[3];
                	if (0 >= fr_width || 0 >= fr_height) {
                		continue;
                	}
        			
                	v_width = m_sfv.getWidth();
                	v_height = m_sfv.getHeight();
                	if (0 >= v_width || 0 >= v_height) {
                		continue;
                	}
                	
                	if (fr_width * v_height > fr_height * v_width) {
                		disp_width = v_width;
                		disp_height = fr_height * disp_width / fr_width;
                		left = 0;
                		top = (v_height-disp_height)/2;
                		//rc = new Rect(0, (v_height-disp_height)/2, 0 + disp_width, (v_height-disp_height)/2 + disp_height);
                	}
                	else {
                		disp_height = v_height;
                		disp_width = fr_width * disp_height / fr_height;
                		left = (v_width-disp_width)/2;
                		top = 0;
                		//rc = new Rect((v_width-disp_width)/2, 0, (v_width-disp_width)/2 + disp_width, 0 + disp_height);
                	}
        		////}
        		
        		Canvas canvas = null;
        		Bitmap fr_bmp = null;
        		Bitmap bmp = null;
        		try {
        			canvas = m_sfh.lockCanvas();//If pass rect, rect may be modified!!!
	        		if (canvas != null) {

	        			fr_bmp = Bitmap.createBitmap(data, fr_width, fr_height, Bitmap.Config.RGB_565);
	        			//Log.d(TAG, "fr_bmp.width=" + fr_bmp.getWidth() + " fr_bmp.height=" + fr_bmp.getHeight());
	        			bmp = Bitmap.createScaledBitmap(fr_bmp, disp_width, disp_height, false);
	        			canvas.drawBitmap(bmp, left, top, null);
	        			
	        			if (m_bSnapPic) {
	        				m_bSnapPic = false;
	        				FileOutputStream out = GetPicSaveStream();
	        				if (true == fr_bmp.compress(CompressFormat.JPEG, 95, out))
	        				{
	        					Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, _instance.getResources().getString(R.string.msg_picture_saved_to_sdcard));
	        		        	_instance.mMainHandler.sendMessage(send_msg);
	        				}
	        				try {
	        					out.flush();
								out.close();
							} catch (IOException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
	        			}
	        		}
        		} finally {
	        		if (canvas != null) m_sfh.unlockCanvasAndPost(canvas);
	        		if (bmp != null) bmp.recycle();
	        		if (fr_bmp != null) fr_bmp.recycle();
        		}
            	
        	}//while
        	
        	if (0 != (av_flags & SharedFuncLib.AV_FLAGS_VIDEO_H264)) {
        		SharedFuncLib.FF264RecvStop();
        	}
        	else {
        		SharedFuncLib.FF263RecvStop();
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
			 
			int disp_width = 0;
			int disp_height = 0;
			float left = 0.0f;
			float top = 0.0f;
     		Canvas canvas = null;
     		Bitmap fr_bmp = null;
     		Bitmap bmp = null;
     		
     		if (0 != (av_flags & SharedFuncLib.AV_FLAGS_VIDEO_ENABLE))
    		{
    			bmp = BitmapFactory.decodeResource(getResources(), R.drawable.video_loading);
    			disp_width = bmp.getWidth();   // <= w
    			disp_height = bmp.getHeight(); // <= h
    			if (disp_width > w || disp_height > h) {
    				bmp.recycle();
    				return;
    			}
    			left = (w - disp_width)/2;
    			top = (h - disp_height)/2;
    		}
     		else {
     			fr_bmp = BitmapFactory.decodeResource(getResources(), R.drawable.audio_only);
     			bmp = Bitmap.createScaledBitmap(fr_bmp, w, h, false);
     			left = 0;
     			top = 0;
     		}
     		
     		try {
     			canvas = m_sfh.lockCanvas();//If pass rect, rect may be modified!!!
        		if (canvas != null) {
        			canvas.drawBitmap(bmp, left, top, null);
        		}
     		} finally {
	        		if (canvas != null) m_sfh.unlockCanvasAndPost(canvas);
	        		if (bmp != null) bmp.recycle();
	        		if (fr_bmp != null) fr_bmp.recycle();
     		}
		 }
	}
	
	
	final Runnable auto_send_avctrl_runnable = new Runnable() {
		public void run() {
			try {
				SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_RECORDVOL_SET, 0);
				if (_instance != null) {
					mMainHandler.postDelayed(auto_send_avctrl_runnable, 100000);
				}
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	};
	
	
	private static AvPlayActivity _instance = null;//////////////////
	
	private static final String TAG = "AvPlayActivity";
	private PowerManager.WakeLock m_wl = null;
	
	private MainHandler mMainHandler = null;
	
	private SurfaceView m_sfv = null;
	private SurfaceHolder m_sfh = null;
	
	private boolean m_bSnapPic = false;
	
	private boolean light_is_on = false;
	
	private boolean m_bQuitPlay;
	private long m_lStreamTime;
	
	private byte av_flags;
	private byte av_video_mode;
	private byte av_video_size;
	private byte av_video_framerate;
	private int av_audio_channel;
	private int av_video_channel;
	
	private int comments_id;
	private int conn_type;
	private int conn_fhandle;
	private int audio_channels = 0;
	private int video_channels = 1;
	private String device_uuid = null;
	
	private boolean m_bQuitVoice = true;
	
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.avplay);
        
        Log.d(TAG, "Acquiring wake lock");
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        m_wl = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "AvPlayActivity SCREEN_DIM_WAKE_LOCK");
        m_wl.acquire();
        
        mMainHandler = new MainHandler();
        
        Bundle extras = getIntent().getExtras();
        comments_id = extras.getInt("comments_id");
        conn_type = extras.getInt("conn_type");
        conn_fhandle = extras.getInt("conn_fhandle");
        audio_channels = extras.getInt("audio_channels");
        video_channels = extras.getInt("video_channels");
        device_uuid = extras.getString("device_uuid");
        
        m_sfv = (SurfaceView)findViewById(R.id.video_surfaceview);
        m_sfh = m_sfv.getHolder();
        m_sfh.addCallback(new MySurfaceHolderCallback());
        
        
        findViewById(R.id.id_snap_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnSnap();
        	}
        });
        
        findViewById(R.id.id_shout_btn).setOnTouchListener(new View.OnTouchListener() {
            public boolean onTouch(View arg0, MotionEvent event) {
                // TODO Auto-generated method stub   
                int iAction = event.getAction();
                if (iAction == MotionEvent.ACTION_DOWN) {
                	onBtnShoutDown();
                }
                else if (iAction == MotionEvent.ACTION_UP) {
                	onBtnShoutUp();
                }  
                return false;   // return false表示系统会继续处理
            }
        });
        
        findViewById(R.id.id_stop_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnStop();
        	}
        });
        
        findViewById(R.id.zoom_in_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnZoomIn();
        	}
        });
        
        findViewById(R.id.zoom_out_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnZoomOut();
        	}
        });
        
        findViewById(R.id.turn_left_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnTurnLeft();
        	}
        });
        
        findViewById(R.id.turn_right_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnTurnRight();
        	}
        });
        
        findViewById(R.id.toggle_light_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnToggleLight();
        	}
        });
        
        //video_surfaceview Touch Event
        findViewById(R.id.video_surfaceview).setOnTouchListener(new View.OnTouchListener() {
            public boolean onTouch(View arg0, MotionEvent event) {
                // TODO Auto-generated method stub   
                int iAction = event.getAction();
                if (iAction == MotionEvent.ACTION_DOWN) {
                	onSurfaceViewTouchEvent();
                }
                return false;   // return false表示系统会继续处理
            }
        });
        
        _instance = this;//////////////////
        
        
        
        int tmpVal;
        av_flags = 0;
        
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_AUDIOENABLE, 1);
        if (1 == tmpVal) {
        	av_flags |= SharedFuncLib.AV_FLAGS_AUDIO_ENABLE;
        }
        
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOENABLE, 0);
        if (1 == tmpVal) {
        	av_flags |= SharedFuncLib.AV_FLAGS_VIDEO_ENABLE;
        }
        
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_AUDIOREDUNDANCE, 0);
        if (1 == tmpVal) {
        	av_flags |= SharedFuncLib.AV_FLAGS_AUDIO_REDUNDANCE;
        }
        
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEORELIABLE, 0);
        if (1 == tmpVal) {
        	av_flags |= SharedFuncLib.AV_FLAGS_VIDEO_RELIABLE;
        }
        
        av_video_mode = (byte) AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOMODE, SharedFuncLib.AV_VIDEO_MODE_X264);
    	av_video_size = (byte) AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOSIZE, SharedFuncLib.AV_VIDEO_SIZE_VGA);
    	av_video_framerate = (byte) AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_FRAMERATE, 25);
    	av_audio_channel   = AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_AUDIODEVICE, 0);
    	av_video_channel   = AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEODEVICE, 0);
    	
    	if (SharedFuncLib.SOCKET_TYPE_TCP == conn_type)
        {
    		av_flags &= ~SharedFuncLib.AV_FLAGS_AUDIO_REDUNDANCE;
    		av_flags |= SharedFuncLib.AV_FLAGS_VIDEO_RELIABLE;
        }
    	
    	if (audio_channels == 0)
    	{
    		av_flags &= ~SharedFuncLib.AV_FLAGS_AUDIO_ENABLE;
    	}
    	
    	if (0 == (av_flags & SharedFuncLib.AV_FLAGS_VIDEO_ENABLE))
		{
    		findViewById(R.id.id_snap_btn).setEnabled(false);
		}
    	
    	
    	if (SharedFuncLib.AV_VIDEO_MODE_FF264 == av_video_mode)
    	{
	    	av_flags |= SharedFuncLib.AV_FLAGS_VIDEO_H264;
	    	av_flags |= SharedFuncLib.AV_FLAGS_VIDEO_HWACCE;
    	}
    	else if (SharedFuncLib.AV_VIDEO_MODE_FF263 == av_video_mode)
    	{
	    	av_flags &= ~SharedFuncLib.AV_FLAGS_VIDEO_H264;
	    	av_flags |= SharedFuncLib.AV_FLAGS_VIDEO_HWACCE;
	    	if (SharedFuncLib.AV_VIDEO_SIZE_QCIF != av_video_size) {
	    		av_video_size = SharedFuncLib.AV_VIDEO_SIZE_CIF;
	    	}
    	}
    	else {
    		av_flags |= SharedFuncLib.AV_FLAGS_VIDEO_H264;
    		av_flags &= ~SharedFuncLib.AV_FLAGS_VIDEO_HWACCE;
    	}
    	
    	//云控制PC受控端只支持G729a
		av_flags |= SharedFuncLib.AV_FLAGS_AUDIO_G729A;
    	
    	
    	if (SharedFuncLib.CtrlCmdAVSTART(conn_type, conn_fhandle, av_flags, av_video_size, av_video_framerate, av_audio_channel, av_video_channel) < 0)
    	{//failed!
    		Message send_msg = mMainHandler.obtainMessage(UI_MSG_MESSAGEBOX, _instance.getResources().getString(R.string.msg_communication_error));
        	mMainHandler.sendMessage(send_msg);
    	}
    	else {//succeeded!
    		m_bQuitPlay = false;
    		m_lStreamTime = Integer.MAX_VALUE;
    		
    		if (0 != (av_flags & SharedFuncLib.AV_FLAGS_AUDIO_ENABLE))
    		{
    			m_lStreamTime = 0;
    			new Thread(new AudioRecvThread()).start();
    		}
    		if (0 != (av_flags & SharedFuncLib.AV_FLAGS_VIDEO_ENABLE))
    		{
    			new Thread(new VideoRecvThread()).start();
    		}
    		
    		SharedFuncLib.TLVRecvStart();
    		SharedFuncLib.TLVRecvSetPeriod(500);
    		mMainHandler.sendEmptyMessageDelayed(UI_MSG_DISPLAY_SENSOR, 500);
    		
    		mMainHandler.removeCallbacks(auto_send_avctrl_runnable);
    		mMainHandler.postDelayed(auto_send_avctrl_runnable, 100000);
    	}
    }
    
    @Override
    protected void onDestroy() {
    	
    	mMainHandler.removeCallbacks(auto_send_avctrl_runnable);
    	
    	_instance = null;//////////////////
    	
        Log.d(TAG, "Release wake lock");
    	m_wl.release();
    	
    	super.onDestroy();
    }
	
    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
    	super.onConfigurationChanged(newConfig);
    	
    	if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE)
    	{
    		findViewById(R.id.sub_layout0).setVisibility(View.GONE);
    		findViewById(R.id.sub_layout1).setVisibility(View.GONE);
    		//findViewById(R.id.sub_layout2).setVisibility(View.GONE);
    	}
    	else {
    		findViewById(R.id.sub_layout0).setVisibility(View.VISIBLE);
    		findViewById(R.id.sub_layout1).setVisibility(View.VISIBLE);
    		//findViewById(R.id.sub_layout2).setVisibility(View.VISIBLE);
    	}
    }
    
    private FileOutputStream GetPicSaveStream()
    {
    	FileOutputStream fos = null;
    	final String SD_PATH= Environment.getExternalStorageDirectory().getAbsolutePath();
    	
    	SimpleDateFormat sDateFormat = new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss");
    	String time_str = sDateFormat.format(new java.util.Date());
    	
    	String strFilePath = String.format("%s/%s_%02X%02X.JPG", 
    			SD_PATH, time_str, (byte)(Math.random() * 255), (byte)(Math.random() * 255));
    	
    	File file = new File(strFilePath);
    	
    	try {
    		file.createNewFile();
    		fos = new FileOutputStream(file);
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return null;
		}
    	return fos;
    }
    
    private void onBtnSnap()
    {
    	String status = Environment.getExternalStorageState();
    	if (status.equals(Environment.MEDIA_MOUNTED)) {
    		m_bSnapPic = true;
    	}
    	//else if (status.equals(Environment.MEDIA_MOUNTED_READ_ONLY)) {
    	//
    	//}
    	else {
    		SharedFuncLib.MyMessageBox(
    				this, 
    				this.getResources().getString(R.string.app_name), 
    				this.getResources().getString(R.string.msg_sdcard_not_available));
    	}
    }
    
    private void onBtnShoutDown()
    {
    	MediaPlayer mp = MediaPlayer.create(this, R.raw.record_start);
    	if (null != mp) {
    		Log.d(TAG, "MediaPlayer start...");
    		mp.start();
    		Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_RELEASE_MP, mp);
        	_instance.mMainHandler.sendMessageDelayed(send_msg, 5000);
    	}
    	
    	m_bQuitVoice = false;
    	new Thread(new VoiceThread()).start();
    }
    
    private void onBtnShoutUp()
    {
    	if (false == m_bQuitVoice)
    	{
    		m_bQuitVoice = true;
    		
	    	MediaPlayer mp = MediaPlayer.create(this, R.raw.record_stop);
	    	if (null != mp) {
	    		Log.d(TAG, "MediaPlayer start...");
	    		mp.start();
	    		Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_RELEASE_MP, mp);
	        	_instance.mMainHandler.sendMessageDelayed(send_msg, 5000);
	    	}
    	}
    }
    
    private void onBtnStop()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
        builder.setTitle(_instance.getResources().getString(R.string.app_name));
        builder.setMessage(_instance.getResources().getString(R.string.msg_stop_or_not));
        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_yes_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button1");
                    	
                    	_instance.m_bQuitPlay = true;
                    	SharedFuncLib.TLVRecvStop();
                    	SharedFuncLib.CtrlCmdAVSTOP(_instance.conn_type, _instance.conn_fhandle);
                    	
                    	try {
                			Thread.sleep(1500);
                		} catch (InterruptedException e) {
                			// TODO Auto-generated catch block
                			e.printStackTrace();
                		}
                    	
                    	dialog.dismiss();
                    	
                    	mMainHandler.removeCallbacks(auto_send_avctrl_runnable);
                    	_instance.finish();
                    }
                });
        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_no_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button3");
                    	dialog.dismiss();
                    }
                });
        builder.show();
    }
    
    @Override  
    public boolean onKeyDown(int keyCode, KeyEvent event) {   
           
        if(keyCode == KeyEvent.KEYCODE_BACK){   
        	onBtnStop();
            return true;   
        }else{         
            return super.onKeyDown(keyCode, event);   
        }   
    }
    
    private void btnPressSound(int resid)
    {
    	MediaPlayer mp = MediaPlayer.create(this, resid);
    	if (null != mp) {
    		Log.d(TAG, "MediaPlayer start...");
    		mp.start();
    		Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_RELEASE_MP, mp);
        	_instance.mMainHandler.sendMessageDelayed(send_msg, 3000);
    	}
    }
    
    private void onBtnZoomIn()
    {
    	btnPressSound(R.raw.turn);
    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_ZOOM_IN, 0);
    }
    
    private void onBtnZoomOut()
    {
    	btnPressSound(R.raw.turn);
    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_ZOOM_OUT, 0);
    }
    
    private void onBtnTurnLeft()
    {
    	btnPressSound(R.raw.turn);
    	if (device_uuid.contains("@ANYPC@"))
    	{
    		SharedFuncLib.CtrlCmdAVSWITCH(conn_type, conn_fhandle, 0);
    		SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_video_switching));
    		return;
    	}
    	if (1 == av_video_channel)
    	{
	   		av_video_channel = 0;
	    	SharedFuncLib.CtrlCmdAVSWITCH(conn_type, conn_fhandle, av_video_channel);
	    	SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_video_switching));
    	}
    }
    
    private void onBtnTurnRight()
    {
    	btnPressSound(R.raw.turn);
    	if (device_uuid.contains("@ANYPC@"))
    	{
    		SharedFuncLib.CtrlCmdAVSWITCH(conn_type, conn_fhandle, 0);
    		SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_video_switching));
    		return;
    	}
    	if (0 == av_video_channel)
    	{
    		av_video_channel = 1;
    		SharedFuncLib.CtrlCmdAVSWITCH(conn_type, conn_fhandle, av_video_channel);
    		SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_video_switching));
    	}
    }
    
    private void onBtnToggleLight()
    {
    	ImageButton btn = (ImageButton)findViewById(R.id.toggle_light_btn);
    	
    	btnPressSound(R.raw.toggle_light);
    	
    	if (light_is_on) {
    		if (device_uuid.contains("@ANYPC@")) {
    			SharedFuncLib.FF264RecvSetVflip();
    		}
    		else {
    			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_FLASH_OFF, 0);
    		}
    		light_is_on = false;
    		btn.setImageResource(R.drawable.toggle_light);
    	}
    	else {
    		if (device_uuid.contains("@ANYPC@")) {
    			SharedFuncLib.FF264RecvSetVflip();
    		}
    		else {
    			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_FLASH_ON, 0);
    		}
    		light_is_on = true;
    		btn.setImageResource(R.drawable.toggle_light_on);
    	}
    }
    
    private void onSurfaceViewTouchEvent()
    {
    	if (video_channels <= 1) {
    		return;
    	}
    	
    	btnPressSound(R.raw.turn);
    	
    	if (1 == av_video_channel) {
    		av_video_channel = 0;
    	}
    	else {
    		av_video_channel = 1;
    	}
    	SharedFuncLib.CtrlCmdAVSWITCH(conn_type, conn_fhandle, av_video_channel);
    	SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_video_switching));
    }
}
