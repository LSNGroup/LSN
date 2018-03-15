package com.wangling.remotephone;


import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Paint.Style;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
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
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;


public class AvCtrlActivity extends Activity implements VJRListener {
	
	static final int WORK_MSG_MAV_TLV = 1;
	
	private void ParseTLV(byte[] tlv_buff)
	{
		int tlv_len = tlv_buff.length;
		int offset = 0;
		
		while (offset < tlv_len)
		{
			short t;
			short l;
			int v;//不能是DWORD
			double df;

			t = SharedFuncLib.getUint16Val(tlv_buff, offset);
			offset += 2;
			l = SharedFuncLib.getUint16Val(tlv_buff, offset);
			offset += 2;
			v = SharedFuncLib.getUint32Val(tlv_buff, offset);
			offset += 4;

			df = (double)v / SharedFuncLib.TLV_VALUE_TIMES;
			mSensorData[t] = df;
		}
	}
	
	class WorkerHandler extends Handler {
		
		public WorkerHandler() {
			
		}
		
		public WorkerHandler(Looper l) {
			super(l);
		}
		
		@Override
		public void handleMessage(Message msg) {
			
			int ret;
			Message send_msg = null;
			int what = msg.what;
			
			switch(what)
			{
			case WORK_MSG_MAV_TLV:
				if (null != _instance && true == _instance.m_bQuitPlay)
				{
					int[] arrReturn = new int[1];
					byte[] tlv_data = SharedFuncLib.CtrlCmdMAVTLV(conn_type, conn_fhandle, arrReturn);
					if (arrReturn[0] == 0 && tlv_data != null && tlv_data.length > 0) {
						ParseTLV(tlv_data);
						
						int is_armed = (int)(mSensorData[SharedFuncLib.TLV_TYPE_USER_A]) & 0xff;
						if (is_armed == 1 && device_uuid.contains("@UAV@"))
						{
							_instance.mMainHandler.post(new Runnable() {
								public void run() 
								{
									findViewById(R.id.hud_view_layout).setVisibility(View.VISIBLE);
									
									float r = (float) mSensorData[SharedFuncLib.TLV_TYPE_ORIE_X];
									float p = (float) mSensorData[SharedFuncLib.TLV_TYPE_ORIE_Y];
									float y = (float) mSensorData[SharedFuncLib.TLV_TYPE_ORIE_Z];
									if (y<0) {
										y = 360+y;
									}
									try {
										hud.setAttitude(r, p, y);
									}
						            catch (Exception e){}
								}
							});
						}
						else {
							_instance.mMainHandler.post(new Runnable() {
								public void run() 
								{
									findViewById(R.id.hud_view_layout).setVisibility(View.INVISIBLE);
								}
							});
						}
						
						Canvas canvas = null;
		        		try {
		        			canvas = m_sfh2.lockCanvas();//If pass rect, rect may be modified!!!
			        		if (canvas != null) {
			        			
			        			canvas.drawARGB(255, 30, 30, 30);
			        			
		        				if (device_uuid.contains("@UAV@"))
		        				{
		        					DrawOsdData(canvas, mSensorData);
		        				}
		        				else {
		        					DrawSensorData(canvas, mSensorData[SharedFuncLib.TLV_TYPE_BATTERY1_VOLTAGE], mSensorData[SharedFuncLib.TLV_TYPE_TEMP], mSensorData[SharedFuncLib.TLV_TYPE_HUMI], mSensorData[SharedFuncLib.TLV_TYPE_MQX], mSensorData[SharedFuncLib.TLV_TYPE_OOO]);
		        				}
			        		}
		        		} finally {
			        		if (canvas != null) m_sfh2.unlockCanvasAndPost(canvas);
		        		}
					}
				}
				mWorkerHandler.sendEmptyMessageDelayed(WORK_MSG_MAV_TLV, 500);
				break;
				
			default:
				break;	
			}
			
			super.handleMessage(msg);
		}
	}

	
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
					for (int i = 0; i < SharedFuncLib.TLV_TYPE_COUNT; i++)
					{
						mSensorData[i] = SharedFuncLib.TLVRecvGetData(i);
					}
				}
				mMainHandler.sendEmptyMessageDelayed(UI_MSG_DISPLAY_SENSOR, 500);
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
        	
        	Paint paint = new Paint();
        	paint.setAntiAlias(true);//抗锯齿
        	paint.setFilterBitmap(true);//位图滤波
        	paint.setDither(true);
        	
       		SharedFuncLib.FF264RecvStart();
        	
        	while (false == m_bQuitPlay)
        	{
        		int[] data = null;        		
        		
            	data = SharedFuncLib.FF264RecvGetData(arrBreak);
        		
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
        			
                	v_width = m_sfv2.getWidth();
                	v_height = m_sfv2.getHeight();
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
        			canvas = m_sfh2.lockCanvas();//If pass rect, rect may be modified!!!
	        		if (canvas != null) {

	        			canvas.drawARGB(255, 0, 0, 0);
	        			
	        			fr_bmp = Bitmap.createBitmap(data, fr_width, fr_height, Bitmap.Config.RGB_565);
	        			//Log.d(TAG, "fr_bmp.width=" + fr_bmp.getWidth() + " fr_bmp.height=" + fr_bmp.getHeight());
	        			bmp = Bitmap.createScaledBitmap(fr_bmp, disp_width, disp_height, false);
	        			canvas.drawBitmap(bmp, left, top, paint);
	        			
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
	        			
	        			if (mSensorData != null) {
	        				if (device_uuid.contains("@UAV@"))
	        				{
	        					DrawOsdData(canvas, mSensorData);
	        				}
	        				else {
	        					DrawSensorData(canvas, mSensorData[SharedFuncLib.TLV_TYPE_BATTERY1_VOLTAGE], mSensorData[SharedFuncLib.TLV_TYPE_TEMP], mSensorData[SharedFuncLib.TLV_TYPE_HUMI], mSensorData[SharedFuncLib.TLV_TYPE_MQX], mSensorData[SharedFuncLib.TLV_TYPE_OOO]);
	        				}
	        			}
	        		}
        		} finally {
	        		if (canvas != null) m_sfh2.unlockCanvasAndPost(canvas);
	        		if (bmp != null) bmp.recycle();
	        		if (fr_bmp != null) fr_bmp.recycle();
        		}
            	
        	}//while
        	
        	SharedFuncLib.FF264RecvStop();
        }
    }
    
	class MySurfaceHolderCallback2 implements SurfaceHolder.Callback {

		 public void surfaceCreated(SurfaceHolder holder) {
		     // The Surface has been created, acquire the camera and tell it where
		     // to draw.
			 Log.d(TAG, "surfaceCreated(2)");
		 }

		 public void surfaceDestroyed(SurfaceHolder holder) {
		     // Surface will be destroyed when we return, so stop the preview.
		     // Because the CameraDevice object is not a shared resource, it's very
		     // important to release it when the activity is paused.
			 Log.d(TAG, "surfaceDestroyed(2)");
		 }

		 public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		     // Now that the size is known, set up the camera parameters and begin
		     // the preview.
			 Log.d(TAG, "surfaceChanged(2: " + w + ", " + h +")");
			 
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
     			canvas = m_sfh2.lockCanvas();//If pass rect, rect may be modified!!!
        		if (canvas != null) {
        			canvas.drawBitmap(bmp, left, top, null);
        		}
     		} finally {
	        		if (canvas != null) m_sfh2.unlockCanvasAndPost(canvas);
	        		if (bmp != null) bmp.recycle();
	        		if (fr_bmp != null) fr_bmp.recycle();
     		}
		 }
	}
	
	void DrawOsdData(Canvas canvas, double[] tlv_values)
	{
		if (m_sfv2 == null || m_sfh2 == null || canvas == null) {
			return;
		}
		
		int v_width = m_sfv2.getWidth();
    	int v_height = m_sfv2.getHeight();
    	
    	int greenColor = Color.argb(255, 0, 200, 0);
    	int redColor = Color.argb(255, 200, 0, 0);
    	//默认1280x720分辨率
    	float smallTextSize = 27;
    	float bigTextSize = 65;
    	float xFactor = 1.0f;
    	float yFactor = 1.0f;
    	
    	if (v_width < 1280 || v_height < 720)
    	{
    		smallTextSize = 18;
    		bigTextSize = 47;
    	}
    	else if (v_width > 1280 || v_height > 720)
    	{
    		smallTextSize = 42;
    		bigTextSize = 85;
    	}
    	xFactor = (float)v_width/1280.0f;
    	yFactor = (float)v_height/720.0f;
    	//精确匹配
    	if (v_width == 1920 && v_height == 1080)
    	{
    		smallTextSize = 37;
    		bigTextSize = 78;
    	}
    	
    	String strText;
		Paint mPaint = new Paint();
		mPaint.setAntiAlias(true);//抗锯齿
		mPaint.setAlpha(255);
		mPaint.setStyle(Style.FILL);
		
		mPaint.setColor(greenColor);
		mPaint.setTextSize(smallTextSize);
		
		
		int gps_num = (int)(tlv_values[SharedFuncLib.TLV_TYPE_GPS_COUNT]) & 0xff;
		int gps_fix = ((int)(tlv_values[SharedFuncLib.TLV_TYPE_GPS_COUNT]) >> 8) & 0xff;
		
		strText = String.format(_instance.getResources().getString(R.string.ui_text_gps_num_format), (int)gps_num);
		canvas.drawText(strText, xFactor * 120, yFactor * 30, mPaint);
		
		
		if (tlv_values[SharedFuncLib.TLV_TYPE_GPS_LATI] >= 0) {
			strText = String.format(_instance.getResources().getString(R.string.ui_text_north_lati_format), tlv_values[SharedFuncLib.TLV_TYPE_GPS_LATI]);
		}
		else {
			strText = String.format(_instance.getResources().getString(R.string.ui_text_south_lati_format), -1.0f * tlv_values[SharedFuncLib.TLV_TYPE_GPS_LATI]);
		}
		canvas.drawText(strText, xFactor * 120, yFactor * 60, mPaint);
		
		
		if (tlv_values[SharedFuncLib.TLV_TYPE_GPS_LONG] >= 0) {
			strText = String.format(_instance.getResources().getString(R.string.ui_text_east_long_format), tlv_values[SharedFuncLib.TLV_TYPE_GPS_LONG]);
		}
		else {
			strText = String.format(_instance.getResources().getString(R.string.ui_text_west_long_format), -1.0f * tlv_values[SharedFuncLib.TLV_TYPE_GPS_LONG]);
		}
		canvas.drawText(strText, xFactor * 120, yFactor * 90, mPaint);
		
		
		if (3 == gps_fix) {
			strText = "GPS: 3D fix";
			canvas.drawText(strText, xFactor * 120, yFactor * 120, mPaint);
		}
		else if (2 == gps_fix) {
			strText = "GPS: 2D fix";
			canvas.drawText(strText, xFactor * 120, yFactor * 120, mPaint);
		}
		else if (4 == gps_fix) {
			strText = "GPS: RTK FIX";
			canvas.drawText(strText, xFactor * 120, yFactor * 120, mPaint);
		}
		else if (5 == gps_fix) {
			strText = "GPS: RTK FLOAT";
			canvas.drawText(strText, xFactor * 120, yFactor * 120, mPaint);
		}
		else {
			strText = "GPS: Not fix";
			mPaint.setColor(redColor);
			canvas.drawText(strText, xFactor * 120, yFactor * 120, mPaint);
			mPaint.setColor(greenColor);
		}
		
		
		strText = String.format(_instance.getResources().getString(R.string.ui_text_dongli_battery_format), tlv_values[SharedFuncLib.TLV_TYPE_BATTERY1_VOLTAGE], tlv_values[SharedFuncLib.TLV_TYPE_BATTERY1_CURRENT], (int)(tlv_values[SharedFuncLib.TLV_TYPE_BATTERY1_REMAIN]));
		if ((int)(tlv_values[SharedFuncLib.TLV_TYPE_BATTERY1_REMAIN]) < 50) {
			mPaint.setColor(redColor);
			canvas.drawText(strText, xFactor * 120, yFactor * (720 - 10), mPaint);
			mPaint.setColor(greenColor);
		}
		else {
			canvas.drawText(strText, xFactor * 120, yFactor * (720 - 10), mPaint);
		}
		
		
		strText = String.format(_instance.getResources().getString(R.string.ui_text_tuchuan_battery_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_BATTERY2_REMAIN]));
		if ((int)(tlv_values[SharedFuncLib.TLV_TYPE_BATTERY2_REMAIN]) < 50) {
			mPaint.setColor(redColor);
			canvas.drawText(strText, xFactor * 120, yFactor * (720 - 40), mPaint);
			mPaint.setColor(greenColor);
		}
		else {
			canvas.drawText(strText, xFactor * 120, yFactor * (720 - 40), mPaint);
		}
		
		
		strText = String.format(_instance.getResources().getString(R.string.ui_text_total_time_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_TOTAL_TIME]) / 60, (int)(tlv_values[SharedFuncLib.TLV_TYPE_TOTAL_TIME]) % 60);
		canvas.drawText(strText, xFactor * 120, yFactor * (720 - 90), mPaint);
		
		
		strText = String.format(_instance.getResources().getString(R.string.ui_text_home_dist_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_DIST]));
		canvas.drawText(strText, xFactor * 120, yFactor * (720 - 120), mPaint);
		
		
		strText = _instance.getResources().getString(R.string.ui_text_network_type_0);
		if ((int)(tlv_values[SharedFuncLib.TLV_TYPE_USER_C]) == 1) {
			strText = _instance.getResources().getString(R.string.ui_text_network_type_1);
		}
		else if ((int)(tlv_values[SharedFuncLib.TLV_TYPE_USER_C]) == 2) {
			strText = _instance.getResources().getString(R.string.ui_text_network_type_2);
		}
		else if ((int)(tlv_values[SharedFuncLib.TLV_TYPE_USER_C]) == 3) {
			strText = _instance.getResources().getString(R.string.ui_text_network_type_3);
		}
		else if ((int)(tlv_values[SharedFuncLib.TLV_TYPE_USER_C]) == 4) {
			strText = _instance.getResources().getString(R.string.ui_text_network_type_4);
		}
		canvas.drawText(strText, xFactor * 120, yFactor * (720 - 170), mPaint);
		
		strText = String.format(_instance.getResources().getString(R.string.ui_text_network_signal_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_SIGNAL_STRENGTH]));
		if ((int)(tlv_values[SharedFuncLib.TLV_TYPE_SIGNAL_STRENGTH]) < 50) {
			mPaint.setColor(redColor);
			canvas.drawText(strText, xFactor * 120, yFactor * (720 - 200), mPaint);
			mPaint.setColor(greenColor);
		}
		else {
			canvas.drawText(strText, xFactor * 120, yFactor * (720 - 200), mPaint);
		}
		
		
		//右侧
		strText = String.format(_instance.getResources().getString(R.string.ui_text_gnd_speed_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_GND_SPEED]));
		canvas.drawText(strText, xFactor * (960), yFactor * 30, mPaint);
		strText = String.format(_instance.getResources().getString(R.string.ui_text_air_speed_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_AIR_SPEED]));
		canvas.drawText(strText, xFactor * (960), yFactor * 60, mPaint);
		strText = String.format(_instance.getResources().getString(R.string.ui_text_climb_rate_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_CLIMB_RATE]));
		canvas.drawText(strText, xFactor * (960), yFactor * 90, mPaint);
		strText = String.format(_instance.getResources().getString(R.string.ui_text_gps_alti_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_GPS_ALTI]));
		canvas.drawText(strText, xFactor * (960), yFactor * 120, mPaint);
		strText = String.format(_instance.getResources().getString(R.string.ui_text_height_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_HEIGHT]));
		canvas.drawText(strText, xFactor * (960), yFactor * 150, mPaint);
		
		strText = String.format(_instance.getResources().getString(R.string.ui_text_roll_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_ORIE_X]));
		canvas.drawText(strText, xFactor * (1010), yFactor * 200, mPaint);
		strText = String.format(_instance.getResources().getString(R.string.ui_text_pitch_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_ORIE_Y]));
		canvas.drawText(strText, xFactor * (1010), yFactor * 230, mPaint);
		strText = String.format(_instance.getResources().getString(R.string.ui_text_yaw_format), (int)(tlv_values[SharedFuncLib.TLV_TYPE_ORIE_Z]));
		canvas.drawText(strText, xFactor * (1010), yFactor * 260, mPaint);
		
		strText = String.format("RC1: %d", (int)(tlv_values[SharedFuncLib.TLV_TYPE_RC1]));
		canvas.drawText(strText, xFactor * (1010), yFactor * 320, mPaint);
		strText = String.format("RC2: %d", (int)(tlv_values[SharedFuncLib.TLV_TYPE_RC2]));
		canvas.drawText(strText, xFactor * (1010), yFactor * 350, mPaint);
		strText = String.format("RC3: %d", (int)(tlv_values[SharedFuncLib.TLV_TYPE_RC3]));
		canvas.drawText(strText, xFactor * (1010), yFactor * 380, mPaint);
		strText = String.format("RC4: %d", (int)(tlv_values[SharedFuncLib.TLV_TYPE_RC4]));
		canvas.drawText(strText, xFactor * (1010), yFactor * 410, mPaint);
		
		//未解锁
		int is_armed = (int)(tlv_values[SharedFuncLib.TLV_TYPE_USER_A]) & 0xff;
		int is_failsafe = ((int)(tlv_values[SharedFuncLib.TLV_TYPE_USER_A]) >> 8) & 0xff;
		if (is_armed != 1)
		{
			mPaint.setTextSize(bigTextSize);
			mPaint.setColor(redColor);
			strText = _instance.getResources().getString(R.string.ui_text_apm_disarmed);
			canvas.drawText(strText, xFactor * (510), yFactor * (330), mPaint);
		}
		if (is_failsafe == 1)
		{
			mPaint.setTextSize(bigTextSize);
			mPaint.setColor(redColor);
			strText = "FailSafe";
			canvas.drawText(strText, xFactor * (510), yFactor * (240), mPaint);
		}
		
		//使用帮助
		if (is_armed != 1)
		{
			mPaint.setTextSize(smallTextSize);
			mPaint.setColor(greenColor);
			strText = _instance.getResources().getString(R.string.ui_text_l2_r2_intro);
			canvas.drawText(strText, xFactor * (340), yFactor * (390), mPaint);
			strText = _instance.getResources().getString(R.string.ui_text_l1_r1_intro);
			canvas.drawText(strText, xFactor * (340), yFactor * (420), mPaint);
		}
		
		//飞行模式
		int mode_type = (int)(tlv_values[SharedFuncLib.TLV_TYPE_USER_B]) & 0xff;
		int mode_number = ((int)(tlv_values[SharedFuncLib.TLV_TYPE_USER_B]) >> 8) & 0xff;
		ApmModes apm_mode = ApmModes.getMode(mode_number, mode_type);
		mPaint.setTextSize(smallTextSize);
		mPaint.setColor(greenColor);
		strText = apm_mode.getName();
		canvas.drawText(strText, xFactor * (860), yFactor * (720 - 10), mPaint);
		
		//000
		if ((int)(tlv_values[SharedFuncLib.TLV_TYPE_OOO]) == 7) {
			mPaint.setTextSize(smallTextSize);
			mPaint.setColor(redColor);
			strText = _instance.getResources().getString(R.string.ui_text_telem_error);
			canvas.drawText(strText, xFactor * (520), yFactor * (30), mPaint);
		}
	}
	
	void DrawSensorData(Canvas canvas, double power, double temp, double humi, double mqx, double ooo)
	{
		if (m_sfv2 == null || m_sfh2 == null || canvas == null) {
			return;
		}
		
		int v_width = m_sfv2.getWidth();
    	int v_height = m_sfv2.getHeight();
    	
		Paint mPaint = new Paint();
		mPaint.setAntiAlias(true);//抗锯齿
		
		if (ooo < 0 || ooo > 7) {
			ooo = 0;
		}
		
		if (((int)ooo & 0x01) != 0)//Center
		{
			mPaint.setColor(Color.RED);
			mPaint.setAlpha(200);
		}
		else {
			mPaint.setColor(Color.GREEN);
			mPaint.setAlpha(100);
		}
		mPaint.setStyle(Style.FILL);
		canvas.drawRect(v_width/2 - 25, 10, v_width/2 + 25, 10 + 25, mPaint);
		mPaint.setColor(Color.BLACK);
		mPaint.setAlpha(150);
		mPaint.setStyle(Style.STROKE);
		mPaint.setStrokeWidth(8);
		canvas.drawRect(v_width/2 - 25, 10, v_width/2 + 25, 10 + 25, mPaint);
		
		if (((int)ooo & 0x02) != 0)//Left
		{
			mPaint.setColor(Color.RED);
			mPaint.setAlpha(200);
		}
		else {
			mPaint.setColor(Color.GREEN);
			mPaint.setAlpha(100);
		}
		mPaint.setStyle(Style.FILL);
		canvas.drawRect(v_width/2 - 25 - 50, 7, v_width/2 + 25 - 50, 7 + 20, mPaint);
		mPaint.setColor(Color.BLACK);
		mPaint.setAlpha(150);
		mPaint.setStyle(Style.STROKE);
		mPaint.setStrokeWidth(8);
		canvas.drawRect(v_width/2 - 25 - 50, 7, v_width/2 + 25 - 50, 7 + 20, mPaint);
		
		if (((int)ooo & 0x04) != 0)//Right
		{
			mPaint.setColor(Color.RED);
			mPaint.setAlpha(200);
		}
		else {
			mPaint.setColor(Color.GREEN);
			mPaint.setAlpha(100);
		}
		mPaint.setStyle(Style.FILL);
		canvas.drawRect(v_width/2 - 25 + 50, 7, v_width/2 + 25 + 50, 7 + 20, mPaint);
		mPaint.setColor(Color.BLACK);
		mPaint.setAlpha(150);
		mPaint.setStyle(Style.STROKE);
		mPaint.setStrokeWidth(8);
		canvas.drawRect(v_width/2 - 25 + 50, 7, v_width/2 + 25 + 50, 7 + 20, mPaint);
		
		if (mqx >= 0 && mqx < 10000) {
			mPaint.setColor(VJRSurfaceView.mMainColor);
			mPaint.setAlpha(255);
			mPaint.setStyle(Style.FILL);
			mPaint.setTextSize(27);
			canvas.drawText(String.format(_instance.getResources().getString(R.string.ui_text_mqx_format), (int)mqx) + " (" + String.format("%.1f", power) + "V)", 170, 28, mPaint);
		}
		
		if (temp >= -1000 && temp <= 1000) {
			mPaint.setColor(VJRSurfaceView.mMainColor);
			mPaint.setAlpha(255);
			mPaint.setStyle(Style.FILL);
			mPaint.setTextSize(27);
			canvas.drawText(String.format(_instance.getResources().getString(R.string.ui_text_temp_format), (int)temp), v_width/2 + 120, 28, mPaint);
		}
		
		if (humi >= 0 && humi <= 100) {
			mPaint.setColor(VJRSurfaceView.mMainColor);
			mPaint.setAlpha(255);
			mPaint.setStyle(Style.FILL);
			mPaint.setTextSize(27);
			canvas.drawText(String.format(_instance.getResources().getString(R.string.ui_text_humi_format), (int)humi), v_width/2 + 300, 28, mPaint);
		}
	}
	
	/////////////////////////////////////////////////////////////////////
	//实现接口VJRListener
	
	public void onLeftWheelChanged(int angle, int L) {
		// TODO Auto-generated method stub
		int param = ((L << 16) & 0xffff0000) | (angle & 0x0000ffff);
		if (false == mUseLeftThrottle) {
			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_JOYSTICK1, param);
		}
		else {
			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_JOYSTICK2, param);
		}
	}
	
	public void onRightWheelChanged(int angle, int L) {
		// TODO Auto-generated method stub
		int param = ((L << 16) & 0xffff0000) | (angle & 0x0000ffff);
		if (false == mUseLeftThrottle) {
			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_JOYSTICK2, param);
		}
		else {
			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_JOYSTICK1, param);
		}
	}
	
	public void onLeftButtonADown() {
		// TODO Auto-generated method stub
		if (false == mUseLeftThrottle) {
			btnPressSound(R.raw.turn);
		}
		else {
			onBtnShoutDown();
		}
	}
	
	public void onLeftButtonAUp() {
		// TODO Auto-generated method stub
		if (false == mUseLeftThrottle) {
			btnPressSound(R.raw.turn);
		}
		else {
			onBtnShoutUp();
		}
	}
	
	public void onRightButtonADown() {
		// TODO Auto-generated method stub
		if (false == mUseLeftThrottle) {
			onBtnShoutDown();
		}
		else {
			btnPressSound(R.raw.turn);
		}
	}
	
	public void onRightButtonAUp() {
		// TODO Auto-generated method stub
		if (false == mUseLeftThrottle) {
			onBtnShoutUp();
		}
		else {
			btnPressSound(R.raw.turn);
		}
	}
	
	public void onLeftTopButtonClick() {
		// TODO Auto-generated method stub
		onBtnStop();
	}
	
	public void onRightTopButtonClick() {
		// TODO Auto-generated method stub
		onBtnOperate();
	}
	
	public void onLeftButtonCClick()//L1
	{
		btnPressSound(R.raw.turn);
		
		if (device_uuid.contains("@UAV@"))
		{
			AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
	        builder.setTitle(_instance.getResources().getString(R.string.app_name));
	        
	        int mode_type = (int)(mSensorData[SharedFuncLib.TLV_TYPE_USER_B]) & 0xff;
	        if (mode_type == MAV_TYPE.MAV_TYPE_QUADROTOR)
	        {
	        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_land_mode_or_not));
	        }
	        else if (mode_type == MAV_TYPE.MAV_TYPE_FIXED_WING)
	        {
	        	builder.setMessage(_instance.getResources().getString(R.string.msg_can_not_land_mode));
	        }
	        else if (mode_type == MAV_TYPE.MAV_TYPE_GROUND_ROVER)
	        {
	        	builder.setMessage("Are you sure? - L1");
	        }
	        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button1");
	                    	
	                    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_L1, 0);
	                    	
	                    	dialog.dismiss();
	                    }
	                });
	        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button3");
	                    	dialog.dismiss();
	                    }
	                });
	        builder.show();
		}
		else {
			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_L1, 0);
		}
	}
	
	public void onRightButtonCClick()//R1
	{
		btnPressSound(R.raw.turn);
		
		if (device_uuid.contains("@UAV@"))
		{
			AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
	        builder.setTitle(_instance.getResources().getString(R.string.app_name));
	        builder.setMessage(_instance.getResources().getString(R.string.msg_enter_rtl_mode_or_not));
	        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button1");
	                    	
	                    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_R1, 0);
	                    	
	                    	dialog.dismiss();
	                    }
	                });
	        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button3");
	                    	dialog.dismiss();
	                    }
	                });
	        builder.show();
		}
		else {
			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_R1, 0);
		}
	}
	
	private void onUavL2()
	{
		int is_armed = (int)(mSensorData[SharedFuncLib.TLV_TYPE_USER_A]) & 0xff;
		if (is_armed == 0)
		{
			AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
	        builder.setTitle(_instance.getResources().getString(R.string.app_name));
	        builder.setMessage(_instance.getResources().getString(R.string.msg_apm_arm_or_not));
	        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button1");
	                    	
	                    	int gps_num = (int)(mSensorData[SharedFuncLib.TLV_TYPE_GPS_COUNT]) & 0xff;
	            			int gps_fix = ((int)(mSensorData[SharedFuncLib.TLV_TYPE_GPS_COUNT]) >> 8) & 0xff;
	            			
	                    	if ((int)(mSensorData[SharedFuncLib.TLV_TYPE_RC3]) < 975) {
	                    		SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_arm_failed_throttle_too_low));
	                    	}
	                    	else if ((int)(mSensorData[SharedFuncLib.TLV_TYPE_RC3]) > 1600) {
	                    		SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_arm_failed_throttle_too_high));
	                    	}
	                    	else {
	                    		if (gps_fix < 2) {
		                    		SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_arm_failed_gps_not_fix));
		                    	}
	                    		SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_L2, 1);
	                    	}
	                    	
	                    	dialog.dismiss();
	                    }
	                });
	        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button3");
	                    	dialog.dismiss();
	                    }
	                });
	        builder.show();
		}
		else {
			AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
	        builder.setTitle(_instance.getResources().getString(R.string.app_name));
	        builder.setMessage(_instance.getResources().getString(R.string.msg_apm_disarm_or_not));
	        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button1");
	                    	
	                    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_L2, 0);
	                    	
	                    	dialog.dismiss();
	                    }
	                });
	        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button3");
	                    	dialog.dismiss();
	                    }
	                });
	        builder.show();
		}
	}
	
	public 	void onLeftButtonBClick1()//L2,down
	{
		btnPressSound(R.raw.turn);
		if (device_uuid.contains("@UAV@"))
		{
			onUavL2();
		}
		else {
			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_L2, 1);
		}
	}
	
	public void onLeftButtonBClick2()//L2,up
	{
		btnPressSound(R.raw.turn);
		if (device_uuid.contains("@UAV@"))
		{
			onUavL2();
		}
		else {
			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_L2, 0);
		}
	}
	
	public void onRightButtonBClick1()//R2,down
	{
		btnPressSound(R.raw.turn);
		if (device_uuid.contains("@UAV@"))
		{
			AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
	        builder.setTitle(_instance.getResources().getString(R.string.app_name));
	        builder.setMessage(_instance.getResources().getString(R.string.msg_enable_joystick_or_not));
	        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button1");
	                    	
	                    	if ((int)(mSensorData[SharedFuncLib.TLV_TYPE_RC3]) > 1500
	                				|| Math.abs((int)(mSensorData[SharedFuncLib.TLV_TYPE_RC1]) - 1500) > 50
	                				|| Math.abs((int)(mSensorData[SharedFuncLib.TLV_TYPE_RC2]) - 1500) > 50
	                				|| Math.abs((int)(mSensorData[SharedFuncLib.TLV_TYPE_RC4]) - 1500) > 50 )
	                		{
	                    		SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_rc_channel_abnormal));
	                		}
	                    	else {
	                    		
	                    		if (mUseLeftThrottle)
	                    		{
		                    		m_vjrSurfaceView.setLeftAlwaysShow(1);
		                        	m_vjrSurfaceView.setRightAlwaysShow(0);
	                    		}
	                    		else {
		                    		m_vjrSurfaceView.setLeftAlwaysShow(0);
		                        	m_vjrSurfaceView.setRightAlwaysShow(1);
	                    		}
	                    		m_vjrSurfaceView.setInitThrottle((int)(mSensorData[SharedFuncLib.TLV_TYPE_RC3]));
	                    		SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_R2, 1);
	                    	}
	                    	
	                    	dialog.dismiss();
	                    }
	                });
	        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button3");
	                    	dialog.dismiss();
	                    }
	                });
	        builder.show();
		}
		else {
			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_R2, 1);
		}
	}
	
	public void onRightButtonBClick2()//R2,up
	{
		btnPressSound(R.raw.turn);
		if (device_uuid.contains("@UAV@"))
		{
			AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
	        builder.setTitle(_instance.getResources().getString(R.string.app_name));
	        builder.setMessage(_instance.getResources().getString(R.string.msg_disable_joystick_or_not));
	        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button1");
	                    	
	                    	m_vjrSurfaceView.setLeftAlwaysShow(-1);
	                    	m_vjrSurfaceView.setRightAlwaysShow(-1);
	                    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_R2, 0);
	                    	
	                    	dialog.dismiss();
	                    }
	                });
	        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
	                new DialogInterface.OnClickListener() {
	                    public void onClick(DialogInterface dialog, int whichButton) {
	                        //setTitle("点击了对话框上的Button3");
	                    	dialog.dismiss();
	                    }
	                });
	        builder.show();
		}
		else {
			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_R2, 0);
		}
	}
	
	
	///////////////////////////////////////////////////////////
	
	private static AvCtrlActivity _instance = null;//////////////////
	
	private static final String TAG = "AvCtrlActivity";
	private PowerManager.WakeLock m_wl = null;
	
	private WorkerHandler mWorkerHandler = null;
	private MainHandler mMainHandler = null;
	
	private SurfaceView m_sfv2 = null;
	private SurfaceHolder m_sfh2 = null;
	
	private VJRSurfaceView m_vjrSurfaceView = null;
	private boolean mUseLeftThrottle = true;//美国手
	
	private boolean m_bSnapPic = false;
	
	private boolean m_bQuitPlay = true;
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
	
	private double[] mSensorData = new double[SharedFuncLib.TLV_TYPE_COUNT];
	
	private newHUD hud;
	
	private boolean m_bQuitVoice = true;
	
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.avctrl);
        
        Log.d(TAG, "Acquiring wake lock");
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        m_wl = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "AvCtrlActivity SCREEN_DIM_WAKE_LOCK");
        m_wl.acquire();
        
        Worker worker = new Worker("AvCtrlActivity Worker");
        mWorkerHandler = new WorkerHandler(worker.getLooper());
        mMainHandler = new MainHandler();
        
        Bundle extras = getIntent().getExtras();
        comments_id = extras.getInt("comments_id");
        conn_type = extras.getInt("conn_type");
        conn_fhandle = extras.getInt("conn_fhandle");
        audio_channels = extras.getInt("audio_channels");
        video_channels = extras.getInt("video_channels");
        device_uuid = extras.getString("device_uuid");
        
        
        hud = (newHUD)findViewById(R.id.hudView);
        findViewById(R.id.hud_view_layout).setVisibility(View.INVISIBLE);
        
        m_sfv2 = (SurfaceView)findViewById(R.id.video_surfaceview);
        m_sfh2 = m_sfv2.getHolder();
        m_sfh2.addCallback(new MySurfaceHolderCallback2());
        
        m_vjrSurfaceView = (VJRSurfaceView)findViewById(R.id.vjr_surfaceview);
        m_vjrSurfaceView.setZOrderOnTop(true);
        m_vjrSurfaceView.setVJRListener(this);
        
        int tmp_val = AppSettings.GetSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_LEFT_THROTTLE, 1);
		mUseLeftThrottle = (1 == tmp_val);
		m_vjrSurfaceView.setUseLeftThrottle(mUseLeftThrottle);
        
		if (device_uuid.contains("@UAV@"))
		{
			m_vjrSurfaceView.setLeftAlwaysShow(-1);
			m_vjrSurfaceView.setRightAlwaysShow(-1);
		}
		
        _instance = this;//////////////////
        
        findViewById(R.id.start_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnStart();
        	}
        });
        
        m_bQuitPlay = true;
        
        SharedFuncLib.CtrlCmdMAVSTART(conn_type, conn_fhandle);
        
    	mWorkerHandler.sendEmptyMessageDelayed(WORK_MSG_MAV_TLV, 500);
    }
    
    @Override
    protected void onDestroy() {
    	
    	_instance = null;//////////////////
    	
        Log.d(TAG, "Release wake lock");
    	m_wl.release();
    	
    	mWorkerHandler.getLooper().quit();
    	
    	m_vjrSurfaceView.do_uninit();
    	
    	super.onDestroy();
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
    	
    	//触发远程拍照
    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_TAKE_PICTURE, 0);
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
    
    private void onBtnStart()
    {
    	findViewById(R.id.start_btn_layout).setVisibility(View.GONE);
    	findViewById(R.id.hud_view_layout).setVisibility(View.GONE);
    	
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
    	av_video_size = (byte) AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOSIZE, SharedFuncLib.AV_VIDEO_SIZE_QVGA);
    	av_video_framerate = (byte) AppSettings.GetSoftwareKeyDwordValue(_instance, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_FRAMERATE, 10);
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
    	
    	//AvCtrl视频遥控，至少接收视频
    	av_flags |= SharedFuncLib.AV_FLAGS_VIDEO_ENABLE;
    	
    	
		new Thread(new Runnable()
		{
			public void run()
			{
				m_bQuitPlay = false;
				//避免CtrlCmd_MAV_TLV再执行
				try {
					Thread.sleep(500);
				} catch (InterruptedException e) {}
				
		    	if (SharedFuncLib.CtrlCmdAVSTART(conn_type, conn_fhandle, av_flags, av_video_size, av_video_framerate, av_audio_channel, av_video_channel) < 0)
		    	{//failed!
		    		m_bQuitPlay = true;
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
		    	}
			}
		}).start();
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
                    	
                    	SharedFuncLib.CtrlCmdMAVSTOP(conn_type, conn_fhandle);
                    	
                    	try {
                			Thread.sleep(1500);
                		} catch (InterruptedException e) {
                			// TODO Auto-generated catch block
                			e.printStackTrace();
                		}
                    	
                    	dialog.dismiss();
                    	
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
        AlertDialog alertDialog = builder.create();
        Window wnd = alertDialog.getWindow();
        WindowManager.LayoutParams lp = wnd.getAttributes();
        lp.alpha = 0.65f;
        wnd.setAttributes(lp);
        alertDialog.show();
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
    
    private void onUavA()
    {
    	AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
        builder.setTitle(_instance.getResources().getString(R.string.app_name));
        
        int mode_type = (int)(mSensorData[SharedFuncLib.TLV_TYPE_USER_B]) & 0xff;
        if (mode_type == MAV_TYPE.MAV_TYPE_QUADROTOR) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_stabilize_mode_or_not));
        }
        else if (mode_type == MAV_TYPE.MAV_TYPE_FIXED_WING) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_stabilize_mode_or_not));
        }
        else if (mode_type == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_manual_mode_or_not));
        }
        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button1");
                    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_A, 0);
                    	
                    	dialog.dismiss();
                    }
                });
        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button3");
                    	dialog.dismiss();
                    }
                });
        builder.show();
    }
    
    private void onUavB()
    {
    	AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
        builder.setTitle(_instance.getResources().getString(R.string.app_name));
        
        int mode_type = (int)(mSensorData[SharedFuncLib.TLV_TYPE_USER_B]) & 0xff;
        if (mode_type == MAV_TYPE.MAV_TYPE_QUADROTOR) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_althold_mode_or_not));
        }
        else if (mode_type == MAV_TYPE.MAV_TYPE_FIXED_WING) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_fbwa_mode_or_not));
        }
        else if (mode_type == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_steering_mode_or_not));
        }
        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button1");
                    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_B, 0);
                    	
                    	dialog.dismiss();
                    }
                });
        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button3");
                    	dialog.dismiss();
                    }
                });
        builder.show();
    }
    
    private void onUavX()
    {
    	AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
        builder.setTitle(_instance.getResources().getString(R.string.app_name));
        
        int mode_type = (int)(mSensorData[SharedFuncLib.TLV_TYPE_USER_B]) & 0xff;
        if (mode_type == MAV_TYPE.MAV_TYPE_QUADROTOR) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_loiter_mode_or_not));
        }
        else if (mode_type == MAV_TYPE.MAV_TYPE_FIXED_WING) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_loiter_mode_or_not));
        }
        else if (mode_type == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_hold_mode_or_not));
        }
        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button1");
                    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_X, 0);
                    	
                    	dialog.dismiss();
                    }
                });
        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button3");
                    	dialog.dismiss();
                    }
                });
        builder.show();
    }
    
    private void onUavY()
    {
    	AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
        builder.setTitle(_instance.getResources().getString(R.string.app_name));

        int mode_type = (int)(mSensorData[SharedFuncLib.TLV_TYPE_USER_B]) & 0xff;
        if (mode_type == MAV_TYPE.MAV_TYPE_QUADROTOR) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_auto_mode_or_not));
        }
        else if (mode_type == MAV_TYPE.MAV_TYPE_FIXED_WING) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_auto_mode_or_not));
        }
        else if (mode_type == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
        	builder.setMessage(_instance.getResources().getString(R.string.msg_enter_auto_mode_or_not));
        }
        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button1");
                    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_Y, 0);
                    	
                    	dialog.dismiss();
                    }
                });
        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button3");
                    	dialog.dismiss();
                    }
                });
        builder.show();
    }
    
    private void onBtnABXY()
    {
    	AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
    	Dialog dialog = null;
    	
    	View abxy_view = getLayoutInflater().inflate(R.layout.avctrl_abxy, null);
    	
    	abxy_view.findViewById(R.id.btn_vjr_a).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		btnPressSound(R.raw.turn);
        		if (device_uuid.contains("@UAV@"))
        		{
        			onUavA();
        		}
        		else {
        			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_A, 0);
        		}
        	}
        });
    	abxy_view.findViewById(R.id.btn_vjr_b).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		btnPressSound(R.raw.turn);
        		if (device_uuid.contains("@UAV@"))
        		{
        			onUavB();
        		}
        		else {
        			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_B, 0);
        		}
        	}
        });
    	abxy_view.findViewById(R.id.btn_vjr_x).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		btnPressSound(R.raw.turn);
        		if (device_uuid.contains("@UAV@"))
        		{
        			onUavX();
        		}
        		else {
        			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_X, 0);
        		}
        	}
        });
    	abxy_view.findViewById(R.id.btn_vjr_y).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		btnPressSound(R.raw.turn);
        		if (device_uuid.contains("@UAV@"))
        		{
        			onUavY();
        		}
        		else {
        			SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_BUTTON_Y, 0);
        		}
        	}
        });
    	
    	builder.setView(abxy_view);
		dialog = builder.create();
		Window wnd = dialog.getWindow();
        WindowManager.LayoutParams lp = wnd.getAttributes();
        lp.alpha = 0.65f;
        lp.dimAmount = 0.0f;
        wnd.setAttributes(lp);
        wnd.setGravity(Gravity.BOTTOM);

    	dialog.show();
    }
    
    private void onBtnServo()
    {
    	AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
    	Dialog dialog = null;
    	
    	View servo_view = getLayoutInflater().inflate(R.layout.avctrl_servo, null);
    	
    	SeekBar seek_left = (SeekBar)(servo_view.findViewById(R.id.seekbar_left));
    	seek_left.setMax(180);
    	seek_left.setProgress(180/2);
    	
    	seek_left.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

			@Override
			public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
				// TODO Auto-generated method stub
				if (arg1 % 3 == 1) SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_LEFT_SERVO, arg1);
			}
			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
			}
			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
			}
        });
    	
    	SeekBar seek_right = (SeekBar)(servo_view.findViewById(R.id.seekbar_right));
    	seek_right.setMax(180);
    	seek_right.setProgress(180/2);
    	
    	seek_right.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

			@Override
			public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
				// TODO Auto-generated method stub
				if (arg1 % 3 == 1) SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_RIGHT_SERVO, arg1);
			}
			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
			}
			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
			}
        });
    	
    	builder.setView(servo_view);
		dialog = builder.create();
		Window wnd = dialog.getWindow();
        WindowManager.LayoutParams lp = wnd.getAttributes();
        lp.alpha = 0.65f;
        lp.dimAmount = 0.0f;
        wnd.setAttributes(lp);
        wnd.setGravity(Gravity.BOTTOM);

    	dialog.show();
    }
    
    private void onBtnOperate()
    {
    	AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
        //builder.setTitle(_instance.getResources().getString(R.string.app_name));
    	builder.setItems(R.array.array_avctrl_operate_items, new DialogInterface.OnClickListener(){
			public void onClick(DialogInterface dialog, int which)
			{
				if (0 == which)
				{
					onBtnABXY();
				}
				else if (1 == which)
				{
					onBtnServo();
				}
				else if (2 == which)
				{//美国手
					if (true == mUseLeftThrottle) {
						SharedFuncLib.MyMessageTip(
								_instance, 
								_instance.getResources().getString(R.string.msg_curr_is_left_throttle));
					}
					else {
						int tmpVal = 1;
						AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_LEFT_THROTTLE, tmpVal);
						mUseLeftThrottle = true;
						m_vjrSurfaceView.setUseLeftThrottle(mUseLeftThrottle);
					}
				}
				else if (3 == which)
				{//日本手
					if (false == mUseLeftThrottle) {
						SharedFuncLib.MyMessageTip(
								_instance, 
								_instance.getResources().getString(R.string.msg_curr_is_right_throttle));
					}
					else {
						int tmpVal = 0;
						AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_LEFT_THROTTLE, tmpVal);
						mUseLeftThrottle = false;
						m_vjrSurfaceView.setUseLeftThrottle(mUseLeftThrottle);
					}
				}
				else if (4 == which)
				{
					onBtnSwitch();
				}
				else if (5 == which)
				{
					onBtnSnap();
				}
			}
		});
    	AlertDialog alertDialog = builder.create();
        Window wnd = alertDialog.getWindow();
        WindowManager.LayoutParams lp = wnd.getAttributes();
        lp.alpha = 0.65f;
        lp.dimAmount = 0.0f;
        wnd.setAttributes(lp);
        alertDialog.show();
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
    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_TURN_LEFT, 0);
    }
    
    private void onBtnTurnRight()
    {
    	btnPressSound(R.raw.turn);
    	SharedFuncLib.CtrlCmdAVCONTRL(conn_type, conn_fhandle, SharedFuncLib.AV_CONTRL_TURN_RIGHT, 0);
    }
    
    private void onBtnSwitch()
    {
    	btnPressSound(R.raw.turn);
    	if (av_video_channel == 0)
    	{
    		av_video_channel = 1;
    	}
    	else {
    		av_video_channel = 0;
    	}
    	SharedFuncLib.CtrlCmdAVSWITCH(conn_type, conn_fhandle, av_video_channel);
    	
    	
    	int w = m_sfv2.getWidth();
    	int h = m_sfv2.getHeight();
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
 		
 		try {
 			canvas = m_sfh2.lockCanvas();//If pass rect, rect may be modified!!!
    		if (canvas != null) {
    			canvas.drawBitmap(bmp, left, top, null);
    		}
 		} finally {
        		if (canvas != null) m_sfh2.unlockCanvasAndPost(canvas);
        		if (bmp != null) bmp.recycle();
        		if (fr_bmp != null) fr_bmp.recycle();
 		}
    }
    
}
