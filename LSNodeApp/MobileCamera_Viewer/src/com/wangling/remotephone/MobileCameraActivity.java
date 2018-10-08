package com.wangling.remotephone;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;
import java.util.UUID;

import org.droidplanner.MAVLink.MavLinkArm;
import org.droidplanner.MAVLink.MavLinkHeartbeat;
import org.droidplanner.MAVLink.MavLinkModes;
import org.droidplanner.MAVLink.MavLinkSetRelay;
import org.droidplanner.MAVLink.MavLinkSetServo;
import org.droidplanner.MAVLink.MavLinkStreamRates;
import org.droidplanner.connection.FT311Connection;
import org.droidplanner.connection.MAVLinkConnection;
import org.droidplanner.connection.MAVLinkConnection.MavLinkConnectionListener;
import org.droidplanner.connection.SerialConnection;
import org.droidplanner.helpers.RcOutput;
import org.droidplanner.mission.MAVLinkClient;
import org.droidplanner.mission.WaypointMananger;

import com.MAVLink.Messages.ApmModes;
import com.MAVLink.Messages.MAVLinkMessage;
import com.MAVLink.Messages.MAVLinkPacket;
import com.MAVLink.Messages.ardupilotmega.*;
import com.MAVLink.Messages.enums.MAV_MODE_FLAG;
import com.MAVLink.Messages.enums.MAV_STATE;
import com.MAVLink.Messages.enums.MAV_TYPE;

import com.lsngroup.live.R;

import android.app.Activity;
import android.app.KeyguardManager;
import android.app.KeyguardManager.KeyguardLock;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
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
import android.os.SystemClock;
import android.os.Vibrator;
import android.provider.Settings;
import android.util.Base64;
import android.util.Log;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Toast;


public class MobileCameraActivity extends Activity  implements MavLinkConnectionListener, RtspStreamReceiver {
	
	
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
	
	/////////////////////////////////////////////////////////////////////////////////////

	class AlarmCenter
	{
		public void input_red_alarm(int redType)
		{
		}
		
		public void input_acce_alarm(float x, float y, float z)
		{
		}
		
	}//class AlarmCenter
	
	
	public class BluetoothClient
	{
		private Handler _handler = new Handler();
		private BluetoothAdapter _bluetooth = BluetoothAdapter.getDefaultAdapter();
		private BluetoothSocket btSocket = null;
		private OutputStream outputStream = null;
		private InputStream inputStream = null;
		public int servoH_curr = -1;
		public int servoV_curr = -1;
		public int servoL_curr = -1;
		public int servoR_curr = -1;
		private double voltage_curr = -1.0f;
		private double mqx_curr = -1.0f;
		private double temp_curr = -1.0f;
		private double humi_curr = -1.0f;
		private int relay_curr = -1;
		private int red_curr = -1;
		
		
		private volatile boolean _discoveryStop = true;
		
		final Runnable auto_select_runnable = new Runnable() {
			public void run()
			{
				if (_instance != null)
				{
					_instance.mMainHandler.post(new Runnable() {
						public void run() 
						{
							SharedFuncLib.MyMessageTip(_instance, 
				    				_instance.getResources().getString(R.string.msg_bt_search_start));
						}
					});
				}
				try	{
					/* Start search device */
					_bluetooth.startDiscovery();
					Log.d("EF-BTBee", ">>Starting Discovery");
				} catch (Exception e) {e.printStackTrace();}
			}
		};
		
		/**
		 * Receiver
		 * When the discovery finished be called.
		 */
		private BroadcastReceiver _foundReceiver = new BroadcastReceiver() {
			public void onReceive(Context context, Intent intent) {
				/* get the search results */
				BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
				String saved = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_BT_ADDRESS, "");
				if (true == device.getAddress().equalsIgnoreCase(saved))
				{
					_discoveryStop = true;
					
					unregisterReceiver(_foundReceiver);
					unregisterReceiver(_discoveryReceiver);
					
					_bluetooth.cancelDiscovery();
					
					Intent result = new Intent();
					result.putExtra(BluetoothDevice.EXTRA_DEVICE, device);
					btActivityResult(result);
				}
			}
		};
		
		private BroadcastReceiver _discoveryReceiver = new BroadcastReceiver() {

			@Override
			public void onReceive(Context context, Intent intent)  
			{
				_handler.removeCallbacks(auto_select_runnable);
				if (_instance != null && _discoveryStop == false)
				{
					_handler.postDelayed(auto_select_runnable, 50);
				}
				else {
					unregisterReceiver(_foundReceiver);
					unregisterReceiver(_discoveryReceiver);
				}
			}
		};
		
		public void bluetoothStart() {
			
			if (_bluetooth == null) {
				SharedFuncLib.MyMessageTip(_instance, "No Bluetooth Adapter!");
				return;
			}
			
			String saved = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_BT_ADDRESS, "");
			if (saved.equals(""))
			{
				SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_bt_device_not_set));
				return;
			}
			
			if (!_bluetooth.isEnabled()) {
				_bluetooth.enable();				
				do {
					try {
						Thread.sleep(200);
					} catch (InterruptedException e) {}
				} while (!_bluetooth.isEnabled());
			}
			
			{
				/* Register Receiver*/
				IntentFilter discoveryFilter = new IntentFilter(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
				registerReceiver(_discoveryReceiver, discoveryFilter);
				IntentFilter foundFilter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
				registerReceiver(_foundReceiver, foundFilter);
				
				_discoveryStop = false;
				
				_handler.postDelayed(auto_select_runnable, 50);
			}
		}
		
		public void bluetoothStop() {
			if (btSocket != null) {
				try {
					btSocket.close();
					btSocket = null;
					Log.d("EF-BTBee", ">>Client Socket Close");
				} catch (Exception e) {	}
			}
			
			if (_bluetooth != null && _bluetooth.isEnabled()) {
				_bluetooth.disable();
			}
		}
		
		/* after select, connect to device */
		public void btActivityResult(Intent data) {
			
			//If android device reboot...
			new Thread() {
				public void run() {
					while (outputStream == null)
					{
						try {
							Thread.sleep(1000);
						} catch (InterruptedException e) {}
						
						if (_instance == null || _instance.mBluetoothClient == null)
						{
							return;
						}
					}
					/*
					if (_instance != null && _instance.mBluetoothClient != null && _instance.m_isArmed == false)
					{
						_instance.mMainHandler.post(new Runnable() {
							public void run() {				
								try {
									_instance.onBtnArm();
								} catch (Exception e) {
									// TODO Auto-generated catch block
									e.printStackTrace();
								}
							}
						});
					}
					*/
					//Arduino Rx/Tx recovery...
					while (_instance != null && _instance.mBluetoothClient != null)
					{
						_instance.mBluetoothClient.send_control_str("\n");
						
						try {
							Thread.sleep(2000);
						} catch (InterruptedException e) {}
					}
				}
			}.start();
			
			final BluetoothDevice device = data.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
			new Thread() {
				public void run() {
					while (_instance != null)
					{
						connect(device);
						
						servoH_curr = -1;
						servoV_curr = -1;
						servoL_curr = -1;
						servoR_curr = -1;
						voltage_curr = -1.0f;
						mqx_curr = -1.0f;
						temp_curr = -1.0f;
						humi_curr = -1.0f;
						relay_curr = -1;
						red_curr = -1;
						
						try {
							_bluetooth.disable();
							for (int i = 0; i < 8; i++) {
								if (_instance == null) return;
								Thread.sleep(1000);
							}
							_bluetooth.enable();
							for (int i = 0; i < 8; i++) {
								if (_instance == null) return;
								Thread.sleep(1000);
							}
						} catch (Exception e) {}
					}
				}
			}.start();
		}
		
		private void connect(BluetoothDevice device) {
			try {
				//Create a Socket connection: need the server's UUID number of registered
				btSocket = device.createRfcommSocketToServiceRecord(UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"));
				
				_bluetooth.cancelDiscovery();
				btSocket.connect();
				inputStream = btSocket.getInputStream();														
				outputStream = btSocket.getOutputStream();
				Log.d("EF-BTBee", ">>Bluetooth Client connectted!");
				
				if (_instance != null) {
					_instance.mMainHandler.post(new Runnable() {
						public void run() 
						{
							SharedFuncLib.MyMessageTip(_instance, 
				    				_instance.getResources().getString(R.string.msg_bt_connect_ok));
						}
					});
					_instance.onBluetoothConnected();
				}
				
				int read = -1;
				byte[] bytes = new byte[1];
				while (_instance != null && (read = inputStream.read(bytes, 0, 1)) != -1) {
					String ch = new String(bytes);
					int num = -1;
					double fNum = -1.0f;
					
					if (read != 1) {
						Thread.sleep(100);
						continue;
					}
					
					if (ch.equals("k"))//空气质量传感器
					{
						fNum = readNumber(inputStream);
						if (fNum < 0 || fNum > 1000) {
							break;
						}
						Log.d("EF-BTBee", "Read: k" + fNum);////Debug
						mqx_curr = fNum;
						
						_instance.mSensorMQXVal = mqx_curr;
					}
					else if (ch.equals("o"))//左中右3位障碍物传感器
					{
						fNum = readNumber(inputStream);
						if (fNum < 0 || fNum > 7) {
							break;
						}
						Log.d("EF-BTBee", "Read: o" + fNum);////Debug
						if (_instance != null) {
							_instance.mSensorOOOVal = fNum;
						}
					}
					else if (ch.equals("w"))//温度传感器
					{
						fNum = readNumber(inputStream);
						if (fNum < -100 || fNum > 200) {
							break;
						}
						Log.d("EF-BTBee", "Read: w" + fNum);////Debug
						temp_curr = fNum;
						
						_instance.mSensorTempVal = temp_curr;
					}
					else if (ch.equals("s"))//湿度传感器
					{
						fNum = readNumber(inputStream);
						if (fNum < 0 || fNum > 100) {
							break;
						}
						Log.d("EF-BTBee", "Read: s" + fNum);////Debug
						humi_curr = fNum;
						
						_instance.mSensorHumiVal = humi_curr;
					}
					else if (ch.equals("H"))//舵机H
					{
						num = (int)readNumber(inputStream);
						if (num == -1 || num < 0 || num > 180) {
							break;
						}
						Log.d("EF-BTBee", "Read: servoH" + num);////Debug
						if (servoH_curr == -1) {
							servoH_curr = num;
						}
					}
					else if (ch.equals("V"))//舵机V
					{
						num = (int)readNumber(inputStream);
						if (num == -1 || num < 0 || num > 180) {
							break;
						}
						Log.d("EF-BTBee", "Read: servoV" + num);////Debug
						if (servoV_curr == -1) {
							servoV_curr = num;
						}
					}
					else if (ch.equals("L"))//舵机L
					{
						num = (int)readNumber(inputStream);
						if (num == -1 || num < 0 || num > 180) {
							break;
						}
						Log.d("EF-BTBee", "Read: servoL" + num);////Debug
						if (servoL_curr == -1) {
							servoL_curr = num;
						}
					}
					else if (ch.equals("R"))//舵机R
					{
						num = (int)readNumber(inputStream);
						if (num == -1 || num < 0 || num > 180) {
							break;
						}
						Log.d("EF-BTBee", "Read: servoR" + num);////Debug
						if (servoR_curr == -1) {
							servoR_curr = num;
						}
					}
					else if (ch.equals("j"))//自带的继电器
					{
						num = (int)readNumber(inputStream);
						if (num == -1 || (num != 0 && num != 1)) {
							break;
						}
						Log.d("EF-BTBee", "Read: j" + num);////Debug
						relay_curr = num;
					}
					else if (ch.equals("M"))//315M RF信号输入
					{
						num = (int)readNumber(inputStream);
						if (num < 0 || num > 100) {
							break;
						}
						Log.d("EF-BTBee", "Read: M" + num);////Debug
						
						////Debug
						final int the_num = num;
						_instance.mMainHandler.post(new Runnable() {
							public void run() 
							{
								Toast toast = Toast.makeText(_instance, "Read: M" + the_num, Toast.LENGTH_SHORT);
								toast.setGravity(Gravity.CENTER, 0, 0);
								toast.show();
							}
						});
    					
						
						if (num == 8)//布防
						{
						}
						else if (num == 4)//撤防
						{
						}
						else if (num == 2)//门铃
						{
						}
						else if (num == 1)//控制继电器
						{
						}
						
						else if (num == (1+2))
						{
							if (_instance.mBluetoothClient != null) {
					    		_instance.mBluetoothClient.control_servo_turndown();
					    	}
						}
						else if (num == (1+4))
						{
							if (_instance.mBluetoothClient != null) {
								_instance.mBluetoothClient.control_servo_turnup();
					    	}
						}
						
						
						else if (num >= 9 && num <= 15)//外部的安防探测器
						{
							MediaPlayer mp = MediaPlayer.create(_instance, R.raw.detect_obj);
	    			    	if (null != mp) {
	    			    		_instance.mSkipAudioCount -= 1;
	    			    		Log.d(TAG, "MediaPlayer start...");
	    			    		mp.start();
	    			    		Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_RELEASE_MP, mp);
	    			        	_instance.mMainHandler.sendMessageDelayed(send_msg, 2000);
	    			    	}
	    			    	
							if (_instance.m_isArmed && _instance.mAlarmCenter != null && _instance.m_isClientConnected == false && _instance.m_isCaptureRunning == false) {
								_instance.mAlarmCenter.input_red_alarm(num - 9);
							}
						}
					}
					else if (ch.equals("U"))//主电池电压
					{
						fNum = readNumber(inputStream);
						if (fNum < 0 || fNum > 100) {
							break;
						}
						Log.d("EF-BTBee", "Read: U" + fNum);////Debug
						voltage_curr = fNum;
						
						_instance.mSensorBattery1Volt = voltage_curr;
					}
					else if (ch.equals("h"))//自带的红外人体感应器
					{
						num = (int)readNumber(inputStream);
						if (num == -1 || (num != 0 && num != 1)) {
							break;
						}
						Log.d("EF-BTBee", "Read: h" + num);////Debug
						red_curr = num;
						
						control_leda_set(red_curr == 1);
						
						
						if (_instance.m_isArmed && _instance.mAlarmCenter != null && _instance.m_isClientConnected == false && _instance.m_isCaptureRunning == false && red_curr == 1) {
							_instance.mAlarmCenter.input_red_alarm(16 - 9);
						}
					}
				}//while
				
			} catch (Exception e) {
				Log.e("EF-BTBee", ">>", e);
			} finally {				
				if (btSocket != null) {
					try {
						inputStream.close();
						inputStream = null;
						outputStream.close();
						outputStream = null;
					} catch (Exception e) {	}
					try {
						btSocket.close();
						btSocket = null;
						Log.d("EF-BTBee", ">>Client Socket Close");
					} catch (Exception e) {	}
					
					if (_instance != null) {
						_instance.onBluetoothDisconnected();
					}
				}//if
			}//finally
		}
		
		private double readNumber(InputStream in) throws IOException
		{
			int dotCount = 0;
			int n = 0;
			double p = 0.0f;
			byte[] bytes = new byte[1];
			
			while (true)
			{
				if (in.read(bytes, 0, 1) == -1) {
					return -1;
				}
				
				if (bytes[0] == '\r') {
					continue;
				}
				else if (bytes[0] == '\n') {
					return (double)n + p;
				}
				else if (bytes[0] == '.') {
					if (dotCount == 0) dotCount = 1;
					continue;
				}
				else {
			         if (dotCount == 0) {
			             n = n*10 + (bytes[0] - '0');
			         }
			         else {
			             double tmp = 1.0f;
			             int i = dotCount;
			             while (i > 0)
			             {
			               tmp *= 0.1f;
			               i -= 1;
			             }
			             p += ((double)(bytes[0] - '0'))*tmp;
			             dotCount += 1;
			         }
				}
			}//while
		}
		
		private int readWord(InputStream in) throws IOException
		{
			int ret = 0;
			byte[] bytes = new byte[1];
			
			while (true)
			{
				if (in.read(bytes, 0, 1) == -1) {
					return -1;
				}
				
				if (bytes[0] == '\r') {
					continue;
				}
				else if (bytes[0] == '\n') {
					return ret;
				}
				else if (bytes[0] == ',') {
					return ret;
				}
				else {
			         if (ret <= 65535) {
			             ret = ret*10 + (bytes[0] - '0');
			         }
				}
			}//while
		}
		
		// Return value:
		// -1: Error
		//  n: n > 0, number of bytes
		private int readWordArray(InputStream in, int[] buf, int bufSize) throws IOException
		{
			  int len = readWord(in);
			  if (len > bufSize)
			  {
			    return -1;
			  }
			  
			  int i;
			  for (i = 0; i < len; i++)
			  {
			    buf[i] = readWord(in);
			  }
			  return len;
		}
		
		private void send_control_str(String str)
		{
			if (outputStream != null && str != null && str.length() > 0)
			{
				final byte[] temp_buff = str.getBytes();
				_handler.post(new Runnable() {
					public void run() {				
						try {
							outputStream.write(temp_buff);
						} catch (Exception e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
					}
				});
			}
		}
		
		public void control_servoH_set(int degree)
		{
			String str = "H";
			
			if (degree < 0) {
				degree = 0;
			}
			if (degree > 180) {
				degree = 180;
			}
			str += String.format("%d", degree);
			
			str += "\n";
			send_control_str(str);
			
			servoH_curr = degree;
		}
		
		public void control_servoV_set(int degree)
		{
			String str = "V";
			
			if (degree < 0) {
				degree = 0;
			}
			if (degree > 180) {
				degree = 180;
			}
			str += String.format("%d", degree);
			
			str += "\n";
			send_control_str(str);
			
			servoV_curr = degree;
		}
		
		public void control_servoL_set(int degree)
		{
			String str = "L";
			
			if (degree < 0) {
				degree = 0;
			}
			if (degree > 180) {
				degree = 180;
			}
			str += String.format("%d", degree);
			
			str += "\n";
			send_control_str(str);
			
			servoL_curr = degree;
		}
		
		public void control_servoR_set(int degree)
		{
			String str = "R";
			
			if (degree < 0) {
				degree = 0;
			}
			if (degree > 180) {
				degree = 180;
			}
			str += String.format("%d", degree);
			
			str += "\n";
			send_control_str(str);
			
			servoR_curr = degree;
		}
		
		public void control_servo_turncenter()
		{
			if (servoH_curr == -1 || servoV_curr == -1 || servoL_curr == -1 || servoR_curr == -1) {
				return;
			}
			control_servoH_set(90);
			control_servoV_set(90);
			control_servoL_set(150);
			control_servoR_set(30);
		}
		
		public void control_servo_turnup()
		{
			if (servoV_curr == -1) {
				return;
			}
			control_servoV_set(servoV_curr + 2);
		}
		
		public void control_servo_turndown()
		{
			if (servoV_curr == -1) {
				return;
			}
			control_servoV_set(servoV_curr - 2);
		}
		
		public void control_servo_turnleft()
		{
			if (servoH_curr == -1) {
				return;
			}
			control_servoH_set(servoH_curr - 2);
		}
		
		public void control_servo_turnright()
		{
			if (servoH_curr == -1) {
				return;
			}
			control_servoH_set(servoH_curr + 2);
		}
		
		public void control_relay_set(boolean isOn)
		{
			String str = "j";
			
			if (isOn) {
				str += "1";
			}
			else {
				str += "0";
			}
			
			str += "\n";
			send_control_str(str);
		}
		
		public void control_leda_set(boolean isOn)//指示灯LED a
		{
			String str = "a";
			
			if (isOn) {
				str += "1";
			}
			else {
				str += "0";
			}
			
			str += "\n";
			send_control_str(str);
		}
		
		public void control_ledb_set(boolean isOn)//指示灯LED b
		{
			String str = "b";
			
			if (isOn) {
				str += "1";
			}
			else {
				str += "0";
			}
			
			str += "\n";
			send_control_str(str);
		}
		
		public void control_move_start(int left, int right)
		{
			String str = "Z";
			
			str += String.format("%d", left);
			str += String.format(",%d", right);
			
			str += "\n";
			send_control_str(str);
			Log.d("EF-BTBee", ">>" + str);////Debug
		}
		
		public void control_move_stop()
		{
			String str = "T";
			
			str += "0,1.0,1.0";
			
			str += "\n";
			send_control_str(str);
		}
		
		public void control_move_stop_and_set(int base_speed, float a_ratio, float b_ratio)
		{
			String str = "T";
			
			str += String.format("%d", base_speed);
			str += String.format(",%.2f", a_ratio);
			str += String.format(",%.2f", b_ratio);
			
			str += "\n";
			send_control_str(str);
		}
		
		public void control_rf_send2(int rf_type, int rf_value)
		{
			int rf_M = 0;
			int rf_R = 0;
			int rf_codec = 0;
			int rf_addr = 0;
			int rf_data = 0;
			
			if (1 == rf_type)
			{
				rf_M = SharedFuncLib.RF_315M;
				rf_R = SharedFuncLib.RF_R2262_4M7;
				rf_codec = SharedFuncLib.RF_CODEC_2262;
			}
			else if (2 == rf_type)
			{
				rf_M = SharedFuncLib.RF_315M;
				rf_R = SharedFuncLib.RF_R2262_3M3;
				rf_codec = SharedFuncLib.RF_CODEC_2262;
			}
			else if (3 == rf_type)
			{
				rf_M = SharedFuncLib.RF_315M;
				rf_R = SharedFuncLib.RF_R2262_2M0;
				rf_codec = SharedFuncLib.RF_CODEC_2262;
			}
			else if (4 == rf_type)
			{
				rf_M = SharedFuncLib.RF_315M;
				rf_R = SharedFuncLib.RF_R2262_1M2;
				rf_codec = SharedFuncLib.RF_CODEC_2262;
			}
			else if (5 == rf_type)
			{
				rf_M = SharedFuncLib.RF_433M;
				rf_R = SharedFuncLib.RF_R2262_4M7;
				rf_codec = SharedFuncLib.RF_CODEC_2262;
			}
			else if (6 == rf_type)
			{
				rf_M = SharedFuncLib.RF_433M;
				rf_R = SharedFuncLib.RF_R2262_3M3;
				rf_codec = SharedFuncLib.RF_CODEC_2262;
			}
			else if (7 == rf_type)
			{
				rf_M = SharedFuncLib.RF_433M;
				rf_R = SharedFuncLib.RF_R2262_2M0;
				rf_codec = SharedFuncLib.RF_CODEC_2262;
			}
			else if (8 == rf_type)
			{
				rf_M = SharedFuncLib.RF_433M;
				rf_R = SharedFuncLib.RF_R2262_1M2;
				rf_codec = SharedFuncLib.RF_CODEC_2262;
			}
			else if ((SharedFuncLib.RF0_TYPE_OFFSET + 1) == rf_type)
			{
				rf_M = SharedFuncLib.RF_315M;
				rf_R = SharedFuncLib.RF_R1527_330K;
				rf_codec = SharedFuncLib.RF_CODEC_1527;
			}
			else if ((SharedFuncLib.RF0_TYPE_OFFSET + 2) == rf_type)
			{
				rf_M = SharedFuncLib.RF_433M;
				rf_R = SharedFuncLib.RF_R1527_330K;
				rf_codec = SharedFuncLib.RF_CODEC_1527;
			}
			else {
				return;
			}
			
			for (int i = 0; i < SharedFuncLib.str_rf_addr.length() ; i++)
			{
				char c = SharedFuncLib.str_rf_addr.charAt(i);
				if ('F' == c)
				{
					rf_addr = rf_addr * 3 + 2;
				}
				else if ('H' == c)
				{
					rf_addr = rf_addr * 3 + 1;
				}
				else /* 'L' '?' */
				{
					rf_addr = rf_addr * 3;
				}
			}
			
			if (rf_codec == SharedFuncLib.RF_CODEC_1527)
			{
				rf_addr += 0;
				rf_data = rf_value  - 16;
			}
			else if (rf_codec == SharedFuncLib.RF_CODEC_2262)
			{
				rf_addr += 1;
				rf_data = rf_value;
			}
			
			
			String str = "m";
			
			str += String.format("%d,%d,%d,%d,%d", rf_M, rf_R, rf_codec, rf_addr, rf_data);
			
			str += "\n";
			send_control_str(str);
		}
		
		public void control_rf_send1(int rf_value)
		{
			//rf_type=11, RF_315M, RF_CODEC_1527
			control_rf_send2(11, rf_value);
		}
		
	}//BluetoothClient Class
	
    public void onBluetoothConnected()
    {
    	mBtConnected = true;
    	
    	//If arduino device reboot...
    	mBluetoothClient.control_move_stop_and_set(SharedFuncLib.ARDUINO_SPEED_BASE, 1.0f, 1.0f);
    	
    	if (m_isArmed) {
    		mBluetoothClient.control_ledb_set(true);
    	}
    }
    
    public void onBluetoothDisconnected()
    {
    	mBtConnected = false;
    }
    
    
    /////////////////////////////////////////////////////////////////////////////////
	
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
	
	private MediaCodec mMediaEncoder = null;
	
	private int mSkipAudioCount = 0;// >=0, can do audio
	private AudioRecord mAudioRecord = null;
	private LinkedList<byte[]> mRecordQueue = new LinkedList<byte[]>();
	private boolean m_bQuitRecord;
    
	private boolean m_isArmed = false;
	private boolean m_isClientConnected = false;
	private boolean m_isCaptureRunning = false;
    
	public boolean mBtConnected = false;
	public BluetoothClient mBluetoothClient = null;
	
	private AlarmCenter mAlarmCenter = null;
	
	private double mLocLongitude = 0.0d;//jing du
	private double mLocLatitude = 0.0d; //wei du
	
	private float mSensorAcceValX = 0.0f;
	private float mSensorAcceValY = 0.0f;
	private float mSensorAcceValZ = 0.0f;
	private double mSensorOrienValX = 0.0f;
	private double mSensorOrienValY = 0.0f;
	private double mSensorOrienValZ = 0.0f;
	private double mSensorBattery1Volt = 0.0f;
	private double mSensorBattery1Curr = 0.0f;
	private double mSensorBattery1Val = 0.0f;
	private double mSensorBattery2Val = 0.0f;
	private double mSensorTempVal = 0.0f;
	private double mSensorHumiVal = 0.0f;
	private double mSensorMQXVal  = 0.0f;
	private double mSensorSignalStrength = 0.0f;
	private double mSensorGPSCount = 0.0f;
	private double mSensorGPSLong = 0.0f;
	private double mSensorGPSLati = 0.0f;
	private double mSensorGPSAlti = 0.0f;
	private double mSensorTotalTime = 0.0f;
	private double mSensorAirSpeed = 0.0f;
	private double mSensorGndSpeed = 0.0f;
	private double mSensorClimbRate = 0.0f;
	private double mSensorDistance = 0.0f;
	private double mSensorRelatedHeight = 0.0f;
	private double mSensorUserA_IsArmed = 0.0f;
	private double mSensorUserB_FlyMode = 0.0f;
	private double mSensorUserC_NetworkType = 0.0f;
	private double mSensorOOOVal = 0.0f;
	private double mSensorRC1 = 0.0f;
	private double mSensorRC2 = 0.0f;
	private double mSensorRC3 = 0.0f;
	private double mSensorRC4 = 0.0f;
	
	private boolean m_bQuitSensorCapture;
	
	private OnvifProto onvifProto = null;
	private int numOfOnvifNodes;
	private int currSwitchIndex;
	private boolean bUseLocalStream;
	private boolean bUseRtpStream;
    
	private FT311UARTInterface uartInterface = null;
	private SerialPort serialPort = null;
	private String strSerialPortPath = "";
	
	public boolean m_bIsUAV = false;
	public boolean m_bIsTailSitter = false;
	
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.mobilecamera);
        
    	Log.d(TAG, "onCreate~~~");
        
    	AppSettings.SaveSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_WITHUAV, 1);
    	AppSettings.SaveSoftwareKeyDwordValue(this, AppSettings.STRING_REGKEY_NAME_TAILSITTER, 0);
    	AppSettings.SaveSoftwareKeyValue(this, AppSettings.STRING_REGKEY_NAME_BT_ADDRESS, "");
    	AppSettings.SaveSoftwareKeyValue(this, AppSettings.STRING_REGKEY_NAME_SERIAL_PORT, "");
    	
    	
    	try {
			onvifProto = new OnvifProto(this);
		} catch (Exception e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
    	numOfOnvifNodes = 0;
    	currSwitchIndex = 0;
    	bUseLocalStream = true;
    	bUseRtpStream = false;
    	
    	
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
        
        
        mBluetoothClient = new BluetoothClient();
        
        mAlarmCenter = new AlarmCenter();
        
        m_isArmed = false;
        mBluetoothClient.control_ledb_set(false);
        
        
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
        
        
        if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_TAILSITTER, 0))
    	{
        	m_bIsTailSitter = false;
    	}
    	else {
    		m_bIsTailSitter = true;
    	}
        if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_WITHUAV, 0))
    	{
    		m_bIsUAV = false;
    	}
    	else {
    		m_bIsUAV = true;
    	}
        
        if (false == m_bIsUAV)
        {
        	mBluetoothClient.bluetoothStart();
        }
        else {
        	String strSerialPort = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SERIAL_PORT, "");/////dev/ttyS3
        	if (false == strSerialPort.contains("/dev/")) {
        		uartInterface = new FT311UARTInterface(_instance, null);
        	}
        	else {
        		try {
					serialPort = new SerialPort(new File(strSerialPort), SharedFuncLib.SERIAL_PORT_BAUDRATE, 0);
					_instance.strSerialPortPath = strSerialPort;
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
					serialPort = null;
					_instance.strSerialPortPath = "";
				}
        	}
        }
        
        if (uartInterface != null)  //=======> {new Thread()}
		new Thread(new Runnable() {
			public void run()
			{
				try {
					Thread.sleep(3000);//等待Surface创建完成
				} catch (InterruptedException e) {}
				
				//record_h264_3gp();
				
				int ret = 0;
				byte[] ioBuff = new byte[16*1024];
				int[] actualNumBytes = new int[1];
				//启动UART服务端Socket侦听...
				while (null != _instance)
				{
					final int port = 6666;
					ServerSocket server = null;
					Socket socket = null;
					try {
						server = new ServerSocket(port);
						server.setSoTimeout(5000);
						Log.d(TAG, "UART server.accept()~~~");
						socket = server.accept();
						if (socket == null) {
							throw new Exception("Accept failed!");
						}
						
						socket.setSoTimeout(200);//20->500
						InputStream input = socket.getInputStream();
						OutputStream output = socket.getOutputStream();
						
						
						// 停止那边的Telem通信...
				    	if (m_bMavLinkStarted)
				    	{
					    	MavClient.resetTimeOut();
					    	
					    	m_MAVLinkConnection.disconnect();
					    	try {
								Thread.sleep(500);
							} catch (InterruptedException e) {}
					    	m_MAVLinkConnection = null;
					    	m_bMavLinkStarted = false;
				    	}
						
						//Wait MP socket connection...
						try {Thread.sleep(2000);}catch (Exception e) {}
						
						Log.d(TAG, "UART ResumeAccessory~~~");
						_instance.mMainHandler.post(new Runnable(){
			    			@Override
			    			public void run() {
			    				uartInterface.ResumeAccessory();
			    				uartInterface.ResetBuffer();
			    			}
			    		});
						try {Thread.sleep(200);}catch (Exception e) {}
						uartInterface.SetConfig(57600,(byte)8,(byte)1,(byte)0,(byte)0);
						
						while (null != _instance)
						{
							try {
								ret = 0;
								ret = input.read(ioBuff, 0, 256);//64->256
							}
							catch (InterruptedIOException e) {
								if (ret < 0) {
									ret = 0;
								}
							}
							catch (IOException e) {
								final String tmp_str = "ret=" + ret + ", " + e.toString();
								_instance.mMainHandler.post(new Runnable(){
					    			@Override
					    			public void run() {
					    				SharedFuncLib.MyMessageTip(_instance, tmp_str);
					    			}
					    		});
								ret = -1;
							}
							if (ret < 0) {
								break;
							}
							if (ret > 0) {
								uartInterface.SendDataEx(ret, 0, ioBuff);
							}
							
							
							actualNumBytes[0] = 0;
							byte status = uartInterface.ReadData(16*1024, ioBuff, actualNumBytes);
							if (status == 0x00 && actualNumBytes[0] > 0)
							{
								output.write(ioBuff, 0, actualNumBytes[0]);
							}
							if (status == 0x02) {
								_instance.mMainHandler.post(new Runnable(){
					    			@Override
					    			public void run() {
					    				SharedFuncLib.MyMessageTip(_instance, "status == 0x02, break~~~");
					    			}
					    		});
								break;
							}
						}
						
						_instance.mMainHandler.post(new Runnable(){
			    			@Override
			    			public void run() {
			    				uartInterface.DestroyAccessory(true);
			    			}
			    		});
						Log.d(TAG, "UART DestroyAccessory~~~");
						
						
						input.close();input=null;
						output.close();output=null;
						socket.close();socket=null;
						server.close();server=null;
						
						_instance.mMainHandler.post(new Runnable(){
			    			@Override
			    			public void run() {
			    				SharedFuncLib.MyMessageTip(_instance, "UART socket closed!!!");
			    			}
			    		});
						
					} catch (Exception e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
						
						if (null != socket) {
							try {
								socket.close();
							} catch (IOException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
							socket = null;
						}
						if (null != server) {
							try {
								server.close();
							} catch (IOException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
							server = null;
						}
					}
				}
			}
		}).start();
		
        
        if (serialPort != null)  //=======> {new Thread()}
		new Thread(new Runnable() {
			public void run()
			{
				try {
					Thread.sleep(3000);//等待Surface创建完成
				} catch (InterruptedException e) {}
				
				//record_h264_3gp();
				
				int ret = 0;
				byte[] ioBuff = new byte[16*1024];
				//启动UART服务端Socket侦听...
				while (null != _instance)
				{
					final int port = 6666;
					ServerSocket server = null;
					Socket socket = null;
					try {
						server = new ServerSocket(port);
						server.setSoTimeout(5000);
						Log.d(TAG, "UART server.accept()~~~");
						socket = server.accept();
						if (socket == null) {
							throw new Exception("Accept failed!");
						}
						
						socket.setSoTimeout(200);//20->500
						InputStream input = socket.getInputStream();
						OutputStream output = socket.getOutputStream();
						
						
						// 停止那边的Telem通信...
				    	if (m_bMavLinkStarted)
				    	{
					    	MavClient.resetTimeOut();
					    	
					    	m_MAVLinkConnection.disconnect();
					    	try {
								Thread.sleep(500);
							} catch (InterruptedException e) {}
					    	m_MAVLinkConnection = null;
					    	m_bMavLinkStarted = false;
				    	}
						
						//Wait MP socket connection...
						try {Thread.sleep(2000);}catch (Exception e) {}
						
						serialPort.close();
						serialPort = null;
						serialPort = new SerialPort(new File(_instance.strSerialPortPath), SharedFuncLib.SERIAL_PORT_BAUDRATE, 0);
						InputStream serial_input = serialPort.getInputStream();
						OutputStream serial_output = serialPort.getOutputStream();
						
						while (null != _instance)
						{
							try {
								ret = 0;
								ret = input.read(ioBuff, 0, 2048);//64->
							}
							catch (InterruptedIOException e) {
								if (ret < 0) {
									ret = 0;
								}
							}
							catch (IOException e) {
								final String tmp_str = "ret=" + ret + ", " + e.toString();
								_instance.mMainHandler.post(new Runnable(){
					    			@Override
					    			public void run() {
					    				SharedFuncLib.MyMessageTip(_instance, tmp_str);
					    			}
					    		});
								ret = -1;
							}
							//Log.d("SerialPort", "tcp_input.read()=" + ret);////Debug
							if (ret < 0) {
								break;
							}
							if (ret > 0) {
								serial_output.write(ioBuff, 0, ret);
							}
							
							
							try {
								ret = 0;
								ret = serial_input.read(ioBuff, 0, 2048);//64->
							}
							catch (InterruptedIOException e) {
								if (ret < 0) {
									ret = 0;
								}
							}
							catch (IOException e) {
								final String tmp_str = "ret=" + ret + ", " + e.toString();
								_instance.mMainHandler.post(new Runnable(){
					    			@Override
					    			public void run() {
					    				SharedFuncLib.MyMessageTip(_instance, tmp_str);
					    			}
					    		});
								ret = -1;
							}
							//Log.d("SerialPort", "serial_input.read()=" + ret);////Debug
							if (ret < 0) {
								break;
							}
							if (ret > 0) {
								output.write(ioBuff, 0, ret);
							}
						}
						
						serial_input.close(); serial_input = null;
						serial_output.close(); serial_output = null;
						serialPort.close(); serialPort = null;
						
						input.close();input=null;
						output.close();output=null;
						socket.close();socket=null;
						server.close();server=null;
						
						_instance.mMainHandler.post(new Runnable(){
			    			@Override
			    			public void run() {
			    				SharedFuncLib.MyMessageTip(_instance, "UART socket closed!!!");
			    			}
			    		});
						
					} catch (Exception e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
						
						if (null != socket) {
							try {
								socket.close();
							} catch (IOException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
							socket = null;
						}
						if (null != server) {
							try {
								server.close();
							} catch (IOException e) {
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
							server = null;
						}
					}
				}
			}
		}).start();
        

		File dir = new File("/data/data/" + getPackageName() + "/remotephone");
		if (false == dir.exists()) {
			dir.mkdir();
		}
		
		//try {
		//	Thread.sleep(3000);//等待Surface创建完成
		//} catch (InterruptedException e) {}
		
		//record_h264_3gp();
		
		mMainHandler.sendEmptyMessageDelayed(UI_MSG_AUTO_START, 100);
		
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
    	
    	if (uartInterface != null) {
    		uartInterface.DestroyAccessory(true);
    		uartInterface = null;
    	}
    	
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
        
    	if (mBluetoothClient != null) {
        	mBluetoothClient.control_leda_set(false);
        	mBluetoothClient.control_ledb_set(false);
        	mBluetoothClient.bluetoothStop();
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
		
		int video_enc = AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_VIDEOENC, 0);
		int video_uv = AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_VIDEOUV, 0);
		
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
	        	if (bUseLocalStream) {
	        		if (mMediaEncoder != null)
	        		{
	        			//是否进行UV调换
	        			if (video_uv == 1) {
	        				int plane = size.width * size.height;
	        				if (video_enc == 1)//YUV420SP
	        				{
	        					for (int i = 0; i < plane/4; i++)
	        					{
	        						byte tmp = data[plane + 2*i];
	        						data[plane + 2*i] = data[plane + 2*i + 1];
	        						data[plane + 2*i + 1] = tmp;
	        					}
	        				}
	        				else if (video_enc == 2)//YUV420P
	        				{
	        					for (int i = 0; i < plane/4; i++)
	        					{
	        						byte tmp = data[plane + i];
	        						data[plane + i] = data[plane + i + plane/4];
	        						data[plane + i + plane/4] = tmp;
	        					}
	        				}
	        			}
	        			
		        		ByteBuffer[] inputBuffers = mMediaEncoder.getInputBuffers();
		        		ByteBuffer[] outputBuffers = mMediaEncoder.getOutputBuffers();
		        		int inputBufferIndex = mMediaEncoder.dequeueInputBuffer(-1);
		                if (inputBufferIndex >= 0)
		                {
		                    ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
		                    inputBuffer.clear();
		                    inputBuffer.put(data);
		                    mMediaEncoder.queueInputBuffer(inputBufferIndex, 0, data.length, 0, 0);
		                }
		                
		                MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
		                int outputBufferIndex = mMediaEncoder.dequeueOutputBuffer(bufferInfo, 0);
		                while (outputBufferIndex >= 0)
		                {
		                	ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
		                	byte[] outData = new byte[bufferInfo.size];
		                	outputBuffer.get(outData);
		                	PutRtpVideoData(outData, 0, outData.length, 0);
		                	
		                	mMediaEncoder.releaseOutputBuffer(outputBufferIndex, false);
		                	outputBufferIndex = mMediaEncoder.dequeueOutputBuffer(bufferInfo, 0);
		                }
	        		}
	        		else {
	        			PutVideoData(data, data.length, format, size.width, size.height, fps);
	        		}
	        	}
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
    	
    	
    	numOfOnvifNodes = 0;
    	currSwitchIndex = 0;
    	bUseLocalStream = true;
    	bUseRtpStream = false;
    	
    	int video_enc = AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_VIDEOENC, 0);
    	if (video_enc != 0 && Build.VERSION.SDK_INT >= 16)
    	{
	    	try {
		    	mMediaEncoder = MediaCodec.createEncoderByType("video/avc");
		    	MediaFormat mediaFormat = MediaFormat.createVideoFormat("video/avc", width, height);
		        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, (width * height * 3));//bps
		        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, fps);
		        if (video_enc == 1) {
		        	mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar);
		        }
		        else if (video_enc == 2){
		        	mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar);//高通
		        }
		        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1);//关键帧间隔时间 单位s
		        mMediaEncoder.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);  
		        mMediaEncoder.start();
	    	}
	        catch (Exception e){
	        	final int _w = width;
				final int _h = height;
				final int _f = fps;
				final String _s = e.toString() + Log.getStackTraceString(e);
				_instance.mMainHandler.post(new Runnable() {
					public void run() 
					{
						SharedFuncLib.MyMessageTip(_instance, "MediaEncoder open(" + _w + "x" + _h + ", fps=" + _f + ") failed: " + _s);////Debug
					}
				});
				
	        	if (mMediaEncoder != null) {
	        		mMediaEncoder.release();
	        	}
	        	mMediaEncoder = null;
	        }
    	}
    	
    	
    	try {
    		if (Integer.parseInt(Build.VERSION.SDK) >= 9) {
    			mCamera = Camera.open(SharedFuncLib.USE_CAMERA_ID);
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
			//params.setPictureSize(1920, 1080);
			//params.setPreviewFormat(ImageFormat.YV12);////Debug
			if (video_enc == 1) {
				params.setPreviewFormat(ImageFormat.NV21);//OMX_COLOR_FormatYVU420SemiPlanar 
			}
			else if (video_enc == 2) {
				params.setPreviewFormat(ImageFormat.YV12);//OMX_COLOR_FormatYVU420Planar
			}
			mFpsTimeInterval = 100;
			if (0 != fps) {
				mFpsTimeInterval = 1000/fps;
				params.setPreviewFrameRate(fps);
			}
			if (0 != width && 0 != height) {
				params.setPreviewSize(width, height);
			}
			
			//if (Integer.parseInt(Build.VERSION.SDK) >= 8) {
			//	setDisplayOrientation(mCamera, 90);
			//}
			//else {
			//	params.set("orientation", "portrait");
			//	params.set("rotation", 90);
			//}
			
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
    		mCamera.stopPreview();
    		mCamera.setPreviewCallback(null);
    		mCamera.release();
    		mCamera = null;
    	}
    	
    	
    	onvifProto.leaveOnvifNode();
    	
    	
    	if (Build.VERSION.SDK_INT >= 16)
    	{
	    	try {
	    		if (mMediaEncoder != null) {
	    			mMediaEncoder.stop();
	    			mMediaEncoder.release();
	    		}
	    	}
	        catch (Exception e){
	        	e.printStackTrace();
	        }
	    	mMediaEncoder = null;
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
    
    private void localCameraSwitch(int channel)
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
    	
		mCamera.stopPreview();
		mCamera.setPreviewCallback(null);
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
    
    private void video_channel_switch(int channel)
    {
    	if (m_isCaptureRunning == false) {
    		return;
    	}
    	
    	if (mCamera == null) {
    		return;
    	}
    	
    	currSwitchIndex += 1;
    	numOfOnvifNodes = onvifProto.getNumOfOnvifNodes();
    	
    	if (numOfOnvifNodes == 0 && currSwitchIndex > 1)
    	{
    		currSwitchIndex = 0;
    	}
    	if (currSwitchIndex > numOfOnvifNodes + 1)
    	{
    		currSwitchIndex = 0;
    	}
    	if (currSwitchIndex == 0)
    	{
    		bUseRtpStream = false;
    		onvifProto.leaveOnvifNode();
    		localCameraSwitch(1);
    		if (!bUseLocalStream) AvSwitchToLocalStream();
    		bUseLocalStream = true;
    	}
    	if (currSwitchIndex == 1)
    	{
    		bUseRtpStream = false;
    		onvifProto.leaveOnvifNode();
    		localCameraSwitch(0);
    		if (!bUseLocalStream) AvSwitchToLocalStream();
    		bUseLocalStream = true;
    	}
    	if (currSwitchIndex > 1 && currSwitchIndex <= numOfOnvifNodes + 1)
    	{
    		bUseLocalStream = false;
    		bUseRtpStream = true;
    		AvSwitchToRtpStream();
    		onvifProto.switchToOnvifNode(currSwitchIndex - 2);
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
    	
    	onvifProto.leaveOnvifNode();
    }
    
    public static void j_audio_record_stop()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.audio_record_stop();
    }
    
    private int sensor_capture_start()
    {
    	m_bQuitSensorCapture = false;

    	new Thread(){
            @Override
            public void run(){
            	while (false == m_bQuitSensorCapture)
            	{
            		if (false == _instance.m_bIsUAV)
            		{
            		TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_BATTERY1_VOLTAGE, _instance.mSensorBattery1Volt, false);
            		TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_TEMP, _instance.mSensorTempVal, false);
            		TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_HUMI, _instance.mSensorHumiVal, false);
            		TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_MQX, _instance.mSensorMQXVal, false);
            		TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_GPS_LONG, _instance.mLocLongitude, false);
            		TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_GPS_LATI, _instance.mLocLatitude, false);
            		TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_OOO, _instance.mSensorOOOVal, true);//Send
            		}
            		else
            		{
            			_instance.getWiFiSignalLevel();
            			_instance.mSensorTotalTime = _instance.getFlightTime();
            			_instance.mSensorDistance = _instance.getDroneDistanceToHome();
            			
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_BATTERY1_REMAIN, _instance.mSensorBattery1Val, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_BATTERY2_REMAIN, _instance.mSensorBattery2Val, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_TEMP,            _instance.mSensorTempVal, false);
                		TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_HUMI,            _instance.mSensorHumiVal, false);
                		TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_MQX,             _instance.mSensorMQXVal, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_SIGNAL_STRENGTH, _instance.mSensorSignalStrength, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_GPS_COUNT,       _instance.mSensorGPSCount, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_GPS_LONG,        _instance.mSensorGPSLong, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_GPS_LATI,        _instance.mSensorGPSLati, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_GPS_ALTI,        _instance.mSensorGPSAlti, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_TOTAL_TIME,      _instance.mSensorTotalTime, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_AIR_SPEED,       _instance.mSensorAirSpeed, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_GND_SPEED,       _instance.mSensorGndSpeed, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_DIST,            _instance.mSensorDistance, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_HEIGHT,          _instance.mSensorRelatedHeight, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_CLIMB_RATE,      _instance.mSensorClimbRate, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_BATTERY1_VOLTAGE,_instance.mSensorBattery1Volt, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_BATTERY1_CURRENT,_instance.mSensorBattery1Curr, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_ORIE_X,          _instance.mSensorOrienValX, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_ORIE_Y,          _instance.mSensorOrienValY, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_ORIE_Z,          _instance.mSensorOrienValZ, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_USER_A,          _instance.mSensorUserA_IsArmed, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_USER_B,          _instance.mSensorUserB_FlyMode, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_USER_C,          _instance.mSensorUserC_NetworkType, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_RC1,             _instance.mSensorRC1, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_RC2,             _instance.mSensorRC2, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_RC3,             _instance.mSensorRC3, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_RC4,             _instance.mSensorRC4, false);
            			TLVSendUpdateValue(SharedFuncLib.TLV_TYPE_OOO,             _instance.mSensorOOOVal, true);//Send
            		}
            		try {
						Thread.sleep(500);
					} catch (InterruptedException e) {}
            	}
            }
        }.start();
        
        return 0;
    }
    
    public static int j_sensor_capture_start()
    {
    	if (_instance == null) {
    		return -1;
    	}
    	return _instance.sensor_capture_start();
    }
    
    private void sensor_capture_stop()
    {
    	m_bQuitSensorCapture = true;
    }
    
    public static void j_sensor_capture_stop()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.sensor_capture_stop();
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
    	
    	if (_instance.m_bIsUAV)
    	{
    		new Thread(new Runnable() {
    			public void run()
    			{
    				if (_instance.isFishFlashOn) {
    					return;
    				}
    				_instance.isFishFlashOn = true;
    				
    				MavLinkSetRelay.sendSetRelayMessage(_instance.m_MAVLinkConnection, 1, false);//RELAY_PIN2
    				try {
    					Thread.sleep(6000);
    				} catch (InterruptedException e) {}
    				
    				MavLinkSetServo.sendSetServoMessage(_instance.m_MAVLinkConnection, 11, SharedFuncLib.APM_SERVO_MAX_PWM);
    				
    			}
    		}).start();
    	}
    }
    
    public static void j_contrl_flash_off()
    {
    	if (_instance == null) {
    		return;
    	}
    	_instance.setFlashlightEnabled(false);
    	
    	if (_instance.m_bIsUAV)
    	{
    		new Thread(new Runnable() {
    			public void run()
    			{
    				if (false == _instance.isFishFlashOn) {
    					return;
    				}
    				_instance.isFishFlashOn = false;
    				
    				MavLinkSetServo.sendSetServoMessage(_instance.m_MAVLinkConnection, 11, SharedFuncLib.APM_SERVO_MIN_PWM);
    				try {
    					Thread.sleep(5000);
    				} catch (InterruptedException e) {}
    				
    				MavLinkSetRelay.sendSetRelayMessage(_instance.m_MAVLinkConnection, 1, true);//RELAY_PIN2
    				try {
    					Thread.sleep(6000);
    				} catch (InterruptedException e) {}
    			}
    		}).start();
    	}
    }
    
    public static void j_contrl_turn_up()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	//if (_instance.bUseRtpStream) _instance.onvifProto.ptzTurnUp();
    	if (_instance.m_bIsUAV)
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				Toast.makeText(_instance, "Move Forward (" + (_instance.m_bMavLinkStarted ? "S" : "-") + ")(" + (_instance.m_bRcOverride ? "O" : "-") + ")", Toast.LENGTH_SHORT)
    				.show();
    			}
    		});
    		
    		if (false == _instance.m_bRcOverride) {
	    		_instance.m_bRcOverride = true;
	    		_instance.m_RcOutput.enableRcOverride();
    		}
    		
    		if (_instance.m_bMavLinkStarted == false) {
        		return;
        	}
        	
        	if (_instance.m_bRcOverride == true)
        	{
        		_instance.m_RcOutput.setRcChannel(RcOutput.AILERON, 0.0f);//roll
        		_instance.m_RcOutput.setRcChannel(RcOutput.ELEVATOR, -1*0.0f);//pitch
        		_instance.m_RcOutput.setRcChannel(RcOutput.TROTTLE, 0.1f);//throttle
        		_instance.m_RcOutput.setRcChannel(RcOutput.RUDDER, 0.0f);//yaw
        	}
        	
        	//_instance.TryToSendHearbeat();
    	}
    	else
    	{
	    	if (_instance.mBluetoothClient != null) {
	    		_instance.mBluetoothClient.control_servo_turnup();
	    	}
    	}
    }
    
    public static void j_contrl_turn_down()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	//if (_instance.bUseRtpStream) _instance.onvifProto.ptzTurnDown();
    	if (_instance.m_bIsUAV)
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				Toast.makeText(_instance, "Move Stop (" + (_instance.m_bMavLinkStarted ? "S" : "-") + ")(" + (_instance.m_bRcOverride ? "O" : "-") + ")", Toast.LENGTH_SHORT)
    				.show();
    			}
    		});
    		
    		if (false == _instance.m_bRcOverride) {
	    		_instance.m_bRcOverride = true;
	    		_instance.m_RcOutput.enableRcOverride();
    		}
    		
    		if (_instance.m_bMavLinkStarted == false) {
        		return;
        	}
        	
        	if (_instance.m_bRcOverride == true)
        	{
        		_instance.m_RcOutput.setRcChannel(RcOutput.AILERON, 0.0f);//roll
        		_instance.m_RcOutput.setRcChannel(RcOutput.ELEVATOR, -1*0.0f);//pitch
        		_instance.m_RcOutput.setRcChannel(RcOutput.TROTTLE, -1.0f);//throttle
        		_instance.m_RcOutput.setRcChannel(RcOutput.RUDDER, 0.0f);//yaw
        	}
        	
        	//_instance.TryToSendHearbeat();
    	}
    	else
    	{
	    	if (_instance.mBluetoothClient != null) {
	    		_instance.mBluetoothClient.control_servo_turndown();
	    	}
    	}
    }
    
    public static void j_contrl_turn_left()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	//if (_instance.bUseRtpStream) _instance.onvifProto.ptzTurnLeft();
    	if (_instance.m_bIsUAV)
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				Toast.makeText(_instance, "Turn Left (" + (_instance.m_bMavLinkStarted ? "S" : "-") + ")(" + (_instance.m_bRcOverride ? "O" : "-") + ")", Toast.LENGTH_SHORT)
    				.show();
    			}
    		});
    		
    		if (false == _instance.m_bRcOverride) {
	    		_instance.m_bRcOverride = true;
	    		_instance.m_RcOutput.enableRcOverride();
    		}
    		
    		if (_instance.m_bMavLinkStarted == false) {
        		return;
        	}
        	
        	if (_instance.m_bRcOverride == true)
        	{
        		_instance.m_RcOutput.setRcChannel(RcOutput.AILERON, -0.8f);//roll
        		_instance.m_RcOutput.setRcChannel(RcOutput.ELEVATOR, -1*0.0f);//pitch
        		_instance.m_RcOutput.setRcChannel(RcOutput.TROTTLE, 0.1f);//throttle
        		_instance.m_RcOutput.setRcChannel(RcOutput.RUDDER, 0.0f);//yaw
        	}
        	
        	//_instance.TryToSendHearbeat();
    	}
    	else
    	{
	    	if (_instance.mBluetoothClient != null) {
	    		_instance.mBluetoothClient.control_servo_turnleft();
	    	}
    	}
    }
    
    public static void j_contrl_turn_right()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	//if (_instance.bUseRtpStream) _instance.onvifProto.ptzTurnRight();
    	if (_instance.m_bIsUAV)
    	{
    		_instance.mMainHandler.post(new Runnable(){
    			@Override
    			public void run() {
    				Toast.makeText(_instance, "Turn Right (" + (_instance.m_bMavLinkStarted ? "S" : "-") + ")(" + (_instance.m_bRcOverride ? "O" : "-") + ")", Toast.LENGTH_SHORT)
    				.show();
    			}
    		});
    		
    		if (false == _instance.m_bRcOverride) {
	    		_instance.m_bRcOverride = true;
	    		_instance.m_RcOutput.enableRcOverride();
    		}
    		
    		if (_instance.m_bMavLinkStarted == false) {
        		return;
        	}
        	
        	if (_instance.m_bRcOverride == true)
        	{
        		_instance.m_RcOutput.setRcChannel(RcOutput.AILERON, 0.8f);//roll
        		_instance.m_RcOutput.setRcChannel(RcOutput.ELEVATOR, -1*0.0f);//pitch
        		_instance.m_RcOutput.setRcChannel(RcOutput.TROTTLE, 0.1f);//throttle
        		_instance.m_RcOutput.setRcChannel(RcOutput.RUDDER, 0.0f);//yaw
        	}
        	
        	//_instance.TryToSendHearbeat();
    	}
    	else
    	{
	    	if (_instance.mBluetoothClient != null) {
	    		_instance.mBluetoothClient.control_servo_turnright();
	    	}
    	}
    }
    
    private void rob_contrl_left_servo(int n)
    {
    	if (_instance.mBluetoothClient == null) {
    		return;
    	}
    	_instance.mBluetoothClient.control_servoL_set(n);
    }
    
    public static void j_contrl_left_servo(int n)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_left_servo(n);
    	}
    	else {
    		_instance.uav_contrl_left_servo(n);
    	}
    }
    
    private void rob_contrl_right_servo(int n)
    {
    	if (_instance.mBluetoothClient == null) {
    		return;
    	}
    	_instance.mBluetoothClient.control_servoR_set(n);
    }
    
    public static void j_contrl_right_servo(int n)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_right_servo(n);
    	}
    	else {
    		_instance.uav_contrl_right_servo(n);
    	}
    }
    
    static final Runnable auto_move_stop_runnable = new Runnable() {
		public void run() {
			if (_instance == null) {
	    		return;
	    	}
	    	if (_instance.mBluetoothClient == null) {
	    		return;
	    	}
	    	_instance.mBluetoothClient.control_move_stop();
		}
	};
	
    private void rob_contrl_joystick1(int param)
    {
    	int L = (param & 0xffff0000) >>> 16;
    	int angle = param & 0x0000ffff;
    	if (_instance == null) {
    		return;
    	}
    	if (_instance.mBluetoothClient == null) {
    		return;
    	}
    	
    	final int speed_base = SharedFuncLib.ARDUINO_SPEED_BASE;
    	
    	if (L < 65535/5)//靠近原点
    	{
    		_instance.mBluetoothClient.control_move_stop();
    	}
    	else if (angle >= 0 && angle <= 90)
    	{
    		_instance.mBluetoothClient.control_move_start(speed_base, speed_base*angle/45 - speed_base);
    		_instance.mMainHandler.removeCallbacks(auto_move_stop_runnable);
        	_instance.mMainHandler.postDelayed(auto_move_stop_runnable, 5000);
    	}
    	else if (angle > 90 && angle <= 180)
    	{
    		_instance.mBluetoothClient.control_move_start(-1 * speed_base*angle/45 + 3 * speed_base, speed_base);
    		_instance.mMainHandler.removeCallbacks(auto_move_stop_runnable);
        	_instance.mMainHandler.postDelayed(auto_move_stop_runnable, 5000);
    	}
    	else if (angle > 180 && angle <= 270)
    	{
    		_instance.mBluetoothClient.control_move_start(-1 * speed_base, -1 * speed_base*angle/45 + 5 * speed_base);
    		_instance.mMainHandler.removeCallbacks(auto_move_stop_runnable);
        	_instance.mMainHandler.postDelayed(auto_move_stop_runnable, 5000);
    	}
    	else if (angle > 270 && angle <= 360)
    	{
    		_instance.mBluetoothClient.control_move_start(speed_base*angle/45 - 7 * speed_base, -1 * speed_base);
    		_instance.mMainHandler.removeCallbacks(auto_move_stop_runnable);
        	_instance.mMainHandler.postDelayed(auto_move_stop_runnable, 5000);
    	}
    }
    
    public static void j_contrl_joystick1(int param)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_joystick1(param);
    	}
    	else {
    		_instance.uav_contrl_joystick1(param);
    	}
    }
    
    private void rob_contrl_joystick2(int param)
    {
    	int L = (param & 0xffff0000) >>> 16;
    	int angle = param & 0x0000ffff;
    	if (_instance == null) {
    		return;
    	}
    	//if (_instance.mBluetoothClient == null) {
    	//	return;
    	//}
    	
    	if (L > 65535/5 && (angle > 60 && angle < 120)) {
    		j_contrl_turn_up();
    	}
    	else if (L > 65535/5 && (angle > 150 && angle < 210)) {
    		j_contrl_turn_left();
    	}
    	else if (L > 65535/5 && (angle > 240 && angle < 300)) {
    		j_contrl_turn_down();
    	}
    	else if (L > 65535/5 && (angle > 330 || angle < 30)) {
    		j_contrl_turn_right();
    	}
    }
    
    public static void j_contrl_joystick2(int param)//油门
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_joystick2(param);
    	}
    	else {
    		_instance.uav_contrl_joystick2(param);
    	}
    }
    
    private void rob_contrl_button_a()
    {
    	if (_instance.mBluetoothClient == null) {
    		return;
    	}
    	
    	//控制智能插座1：开
    	_instance.mBluetoothClient.control_rf_send1(2);
    }
    
    public static void j_contrl_button_a()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_button_a();
    	}
    	else {
    		_instance.uav_contrl_button_a();
    	}
    }
    
    private void rob_contrl_button_b()
    {
    	if (_instance.mBluetoothClient == null) {
    		return;
    	}
    	
    	//控制智能插座1：关
    	_instance.mBluetoothClient.control_rf_send1(5);
    }
    
    public static void j_contrl_button_b()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_button_b();
    	}
    	else {
    		_instance.uav_contrl_button_b();
    	}
    }
    
    private void rob_contrl_button_x()
    {
    	if (_instance.mBluetoothClient == null) {
    		return;
    	}
    	
    	//控制智能插座2：开
    	_instance.mBluetoothClient.control_rf_send1(8);
    }
    
    public static void j_contrl_button_x()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_button_x();
    	}
    	else {
    		_instance.uav_contrl_button_x();
    	}
    }
    
    private void rob_contrl_button_y()
    {
    	if (_instance.mBluetoothClient == null) {
    		return;
    	}
    	
    	//控制智能插座2：关
    	_instance.mBluetoothClient.control_rf_send1(11);
    }
    
    public static void j_contrl_button_y()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_button_y();
    	}
    	else {
    		_instance.uav_contrl_button_y();
    	}
    }
    
    private void rob_contrl_button_l1(int param)
    {
    	if (_instance.mBluetoothClient == null) {
    		return;
    	}
    	
    }
    
    public static void j_contrl_button_l1(int param)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_button_l1(param);
    	}
    	else {
    		_instance.uav_contrl_button_l1(param);
    	}
    }
    
    private void rob_contrl_button_l2(int param)
    {
    	if (_instance.mBluetoothClient == null) {
    		return;
    	}
    	
    }
    
    public static void j_contrl_button_l2(int param)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_button_l2(param);
    	}
    	else {
    		_instance.uav_contrl_button_l2(param);
    	}
    }
    
    private void rob_contrl_button_r1(int param)
    {
    	if (_instance.mBluetoothClient == null) {
    		return;
    	}
    	
    }
    
    public static void j_contrl_button_r1(int param)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_button_r1(param);
    	}
    	else {
    		_instance.uav_contrl_button_r1(param);
    	}
    }
    
    private void rob_contrl_button_r2(int param)
    {
    	boolean isOn = (param == 1);
    	_instance.setFlashlightEnabled(isOn);
    	if (_instance.mBluetoothClient != null) {
    		_instance.mBluetoothClient.control_relay_set(isOn);
    	}
    }
    
    public static void j_contrl_button_r2(int param)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	if (false == _instance.m_bIsUAV)
    	{
    		_instance.rob_contrl_button_r2(param);
    	}
    	else {
    		_instance.uav_contrl_button_r2(param);
    	}
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
    	
    	//香橙派等，没有wifi，会进入死循环。。。直接用fakeMac
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
    	
    	String tmDeviceId = "ROB";
    	if (1 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_WITHUAV, 0)) {
    		tmDeviceId = "UAV";
    	}
    	
    	return "ANDROID" + "@" + tmDeviceId + "@" + macAddr + "@" + "1";
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
    	
    	if (_instance.mBtConnected && _instance.mBluetoothClient != null) {
    		_instance.mBluetoothClient.control_move_stop();
    	}
    	
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
	

    ////////////////////////////////////////////
	public boolean m_bMavLinkStarted = false;
	public MAVLinkConnection m_MAVLinkConnection = null;
	//private FT311Connection m_FT311Connection = null;////
	//private SerialConnection m_SerialConnection = null;////
	private boolean m_bRcOverride = false;
	private RcOutput m_RcOutput = null;
	private boolean m_bFirstHearbeat = true;
	private long m_lLastHearbeatSentTime = 0;
	
	private int state_type = MAV_TYPE.MAV_TYPE_QUADROTOR;
	private boolean state_failsafe = false;
	private boolean state_armed = false;
	private boolean state_isFlying = false;
	private ApmModes state_mode = ApmModes.UNKNOWN;
	
	public WaypointMananger waypointMananger = null;
	public MAVLinkClient MavClient = null;
	private boolean isWaypointGot = false;
	private byte[] waypointData = null;
	private double homeLati = 0.0f;
	private double homeLongi = 0.0f;
	private double homeAlti = 0.0f;
	
	private boolean isFishFlashOn = true;
	
	/* For tail-sitter... */
	// 0:未解锁; 1:垂直起飞; 2:APM平飞; 3:垂直降落。 
	// 注意mavlink_start()/mavlink_stop()不影响这个状态！只能在控制端连接中改变这个状态。
	private int tailsitter_state = 0;
	
	private double mSensorGPSAlti_Saved;
	
	private int m_SW_GPSALTI = 0;
	private int m_SW_GNDSPEED = 0;
	
    public static final String APP_KEY_default = "532924";//"请输入appKey";
    public static final String APP_SECRET_default = "a02b942c407a5ceb4c82a287a8ecc8a91b2ad7dbd9f45c388971fb4cac83bea5"; //"请输入appSecret"
    public static final String DEVICE_TYPE = "m8p";//"请输入deviceType";
    public static final String DEVICE_ID = "m8p:001";//"请输入deviceId";
    
	
	public void onMissionReceived(List<msg_mission_item> msgs)
	{
		if (msgs != null)
		{
			final int item_num = msgs.size();
			_instance.mMainHandler.post(new Runnable(){
				@Override
				public void run() {
					Toast.makeText(_instance, "Waypoints received: " + item_num + " items", Toast.LENGTH_LONG)
					.show();
				}
			});
			
			uav_setHome(msgs.get(0));
			//msgs.remove(0); // Remove Home waypoint
			waypointData = new byte[msgs.size()*20];
			int offset = 0;
			for (msg_mission_item msg : msgs)
			{
				Gps bd09 = PositionUtil.gps84_To_Bd09(msg.x, msg.y);
				double bd_lati = 0.0f;
				double bd_longi = 0.0f;
				if (null != bd09) {
					bd_lati = bd09.getWgLat();
					bd_longi = bd09.getWgLon();
				}
				
				if (0 == offset) {
					bd_lati = _instance.homeLati;
					bd_longi = _instance.homeLongi;
				}
				
				SharedFuncLib.setUint32Val(0, waypointData, offset);
				offset += 4;
				SharedFuncLib.setUint32Val((int)(msg.command), waypointData, offset);
				offset += 4;
				SharedFuncLib.setUint32Val((int)(bd_lati * SharedFuncLib.TLV_VALUE_TIMES), waypointData, offset);
				offset += 4;
				SharedFuncLib.setUint32Val((int)(bd_longi * SharedFuncLib.TLV_VALUE_TIMES), waypointData, offset);
				offset += 4;
				SharedFuncLib.setUint32Val((int)(msg.z * SharedFuncLib.TLV_VALUE_TIMES), waypointData, offset);
				offset += 4;
			}
			isWaypointGot = true;
		}
	}
	
	public void onWriteWaypoints(msg_mission_ack msg)
	{
		_instance.mMainHandler.post(new Runnable(){
			@Override
			public void run() {
				Toast.makeText(_instance, "Waypoints sent", Toast.LENGTH_LONG)
				.show();
			}
		});
	}
	
	
	private void on_mavlink_start()
	{
		_instance.m_SW_GPSALTI  = AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_TAILSITTER_SW_GPSALTI, 0);
		_instance.m_SW_GNDSPEED = AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_TAILSITTER_SW_GNDSPEED, 0);
		
		if (Math.abs(mSensorOOOVal - 7.0f) < 1)
		{
			if (m_bMavLinkStarted)
			{
				MavClient.resetTimeOut();
		    	
				m_MAVLinkConnection.disconnect();
		    	try {
					Thread.sleep(500);
				} catch (InterruptedException e) {}
		    	m_MAVLinkConnection = null;
		    	m_bMavLinkStarted = false;
			}
			
			tailsitter_state = 0;//未解锁
		}
		
		if (false == m_bMavLinkStarted)
		{
			m_bMavLinkStarted = true;
			m_bFirstHearbeat = true;
			
			state_type = MAV_TYPE.MAV_TYPE_QUADROTOR;
			state_failsafe = false;
			state_armed = false;
			state_isFlying = false;
			state_mode = ApmModes.UNKNOWN;
			
			resetFlightTimer();
			
			waypointMananger = new WaypointMananger(_instance);
			MavClient = new MAVLinkClient(_instance, waypointMananger);
			isWaypointGot = false;
			//homeLati = 0.0f;//飞行之中可以断开GCS连接，但是不能改变Home坐标！！！
			//homeLongi = 0.0f;
			//homeAlti = 0.0f;
			
			if (_instance.strSerialPortPath.isEmpty()) {
				m_MAVLinkConnection = new FT311Connection(_instance, _instance.mMainHandler, _instance.uartInterface);
			}
			else {
				try {
					_instance.serialPort = new SerialPort(new File(_instance.strSerialPortPath), SharedFuncLib.SERIAL_PORT_BAUDRATE, 0);
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				m_MAVLinkConnection = new SerialConnection(_instance, _instance.mMainHandler, _instance.serialPort);
			}
			m_bRcOverride = false;
			m_RcOutput = new RcOutput(m_MAVLinkConnection, _instance);
			m_MAVLinkConnection.start();
		}

		
		_instance.mSensorOOOVal = 0.0f;//清除Telem线路异常标志
		
		//Send GCS Heartbeat...
    	MavLinkHeartbeat.sendMavHeartbeat(m_MAVLinkConnection);
    	SendRadioMessage(m_MAVLinkConnection);
	}
	
    public static void j_on_mavlink_start()
    {
    	if (_instance == null) {
    		return;
    	}
    	if (false == _instance.m_bIsUAV)
    	{
    		return;
    	}
    	_instance.on_mavlink_start();
    }
    
    private void on_mavlink_stop()
    {
    	if (false == m_bMavLinkStarted) {
			return;
		}
		
    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING && state_mode != ApmModes.FIXED_WING_AUTO && state_mode != ApmModes.FIXED_WING_LOITER && state_mode != ApmModes.FIXED_WING_GUIDED) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_RTL);
    	}
    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_QUADROTOR && state_mode != ApmModes.ROTOR_AUTO && state_mode != ApmModes.ROTOR_LOITER && state_mode != ApmModes.ROTOR_GUIDED && state_mode != ApmModes.ROTOR_LAND) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROTOR_RTL);
    	}
    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_GROUND_ROVER && state_mode != ApmModes.ROVER_AUTO && state_mode != ApmModes.ROVER_GUIDED) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROVER_HOLD);
    	}
    	
    	m_bRcOverride = false;
    	m_RcOutput.disableRcOverride();
    	
    	
    	/*  //这个地方是导致固定翼Plane固件，Telem通信故障的祸根
		try {
			Thread.sleep(20);
		} catch (InterruptedException e) {}
		MavLinkStreamRates.setupStreamRates(m_MAVLinkConnection, 0, 0, 0, 0, 0, 0, 0, 0);
		try {
			Thread.sleep(20);
		} catch (InterruptedException e) {}
		MavLinkStreamRates.setupStreamRates(m_MAVLinkConnection, 0, 0, 0, 0, 0, 0, 0, 0);
    	*/
    	
    	
    	/* 网灵GCS连接断开，不要停止Telem通信
    	
    	MavClient.resetTimeOut();
    	
    	m_MAVLinkConnection.disconnect();
    	try {
			Thread.sleep(500);
		} catch (InterruptedException e) {}
    	m_MAVLinkConnection = null;
    	m_bMavLinkStarted = false;
    	*/
    }
    
    public static void j_on_mavlink_stop()
    {
    	if (_instance == null) {
    		return;
    	}
    	if (false == _instance.m_bIsUAV)
    	{
    		return;
    	}
    	_instance.on_mavlink_stop();
    }
    
    private void on_mavlink_guid(float lati, float longi, float alti)
    {
    	if (state_armed == false || state_failsafe == true) {
    		return;
    	}
    	
    	if (state_type == MAV_TYPE.MAV_TYPE_QUADROTOR && state_mode != ApmModes.ROTOR_GUIDED) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROTOR_GUIDED);
    	}
    	else if (state_type == MAV_TYPE.MAV_TYPE_FIXED_WING && state_mode != ApmModes.FIXED_WING_GUIDED) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_GUIDED);
    	}
    	else if (state_type == MAV_TYPE.MAV_TYPE_GROUND_ROVER && state_mode != ApmModes.ROVER_GUIDED) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROVER_GUIDED);
    	}
    	
    	Gps gps84 = PositionUtil.bd09_To_Gps84(lati, longi);
    	if (gps84 != null) {
    		MavLinkModes.setGuidedMode(m_MAVLinkConnection, gps84.getWgLat(), gps84.getWgLon(), alti);
    	}
    }
    
    public static void j_on_mavlink_guid(float lati, float longi, float alti)
    {
    	if (_instance == null) {
    		return;
    	}
    	if (false == _instance.m_bIsUAV)
    	{
    		return;
    	}
    	_instance.on_mavlink_guid(lati, longi, alti);
    }
    
    private void getWiFiSignalLevel()
    {
    	ConnectivityManager conMan = (ConnectivityManager) 
        		_instance.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = conMan.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (networkInfo == null || networkInfo.isAvailable() == false || networkInfo.isConnected() == false)
        {
        	return;
        }
        
    	WifiManager wifi_service = (WifiManager)_instance.getSystemService(WIFI_SERVICE); 
    	WifiInfo wifiInfo = wifi_service.getConnectionInfo();
    	int rssi = wifiInfo.getRssi();
    	if (rssi < -85) {////
    		_instance.mSensorUserC_NetworkType = 1;
    		_instance.mSensorSignalStrength = 20;
    	}
    	else if (rssi < -77) {
    		_instance.mSensorUserC_NetworkType = 1;
    		_instance.mSensorSignalStrength = 30;
    	}
    	else if (rssi < -70) {////
    		_instance.mSensorUserC_NetworkType = 1;
    		_instance.mSensorSignalStrength = 40;
    	}
    	else if (rssi < -62) {
    		_instance.mSensorUserC_NetworkType = 1;
    		_instance.mSensorSignalStrength = 50;
    	}
    	else if (rssi < -55) {////
    		_instance.mSensorUserC_NetworkType = 1;
    		_instance.mSensorSignalStrength = 60;
    	}
    	else if (rssi < -47) {
    		_instance.mSensorUserC_NetworkType = 1;
    		_instance.mSensorSignalStrength = 70;
    	}
    	else if (rssi < -40) {////
    		_instance.mSensorUserC_NetworkType = 1;
    		_instance.mSensorSignalStrength = 80;
    	}
    	else if (rssi < -32) {
    		_instance.mSensorUserC_NetworkType = 1;
    		_instance.mSensorSignalStrength = 90;
    	}
    	else {
    		_instance.mSensorUserC_NetworkType = 1;
    		_instance.mSensorSignalStrength = 100;
    	}
    }
    
    private static void setTlvItem(short t, short l, double v, byte[] buff, int offset)
    {
    	SharedFuncLib.setUint16Val(t, buff, offset);
    	SharedFuncLib.setUint16Val(l, buff, offset+2);
    	SharedFuncLib.setUint32Val((int)(v * SharedFuncLib.TLV_VALUE_TIMES), buff, offset+4);
    }
    
    public static byte[] j_get_tlv_data()
    {
    	if (_instance == null) {
    		return null;
    	}
    	
		if (false == _instance.m_bIsUAV)
		{
			byte[] buff = new byte[8 * 7];
			
			setTlvItem(SharedFuncLib.TLV_TYPE_BATTERY1_VOLTAGE, (short)4, _instance.mSensorBattery1Volt, buff, 0);
			setTlvItem(SharedFuncLib.TLV_TYPE_TEMP,             (short)4, _instance.mSensorTempVal, buff, 1*8);
			setTlvItem(SharedFuncLib.TLV_TYPE_HUMI,             (short)4, _instance.mSensorHumiVal, buff, 2*8);
			setTlvItem(SharedFuncLib.TLV_TYPE_MQX,              (short)4, _instance.mSensorMQXVal, buff, 3*8);
			setTlvItem(SharedFuncLib.TLV_TYPE_GPS_LONG,         (short)4, _instance.mLocLongitude, buff, 4*8);
			setTlvItem(SharedFuncLib.TLV_TYPE_GPS_LATI,         (short)4, _instance.mLocLatitude, buff, 5*8);
			setTlvItem(SharedFuncLib.TLV_TYPE_OOO,              (short)4, _instance.mSensorOOOVal, buff, 6*8);
			
			return buff;
		}
		else
		{
		_instance.getWiFiSignalLevel();
		_instance.mSensorTotalTime = _instance.getFlightTime();
		_instance.mSensorDistance = _instance.getDroneDistanceToHome();
    	
    	byte[] buff = new byte[8 * SharedFuncLib.TLV_TYPE_COUNT];
    	
    	setTlvItem(SharedFuncLib.TLV_TYPE_BATTERY1_REMAIN, (short)4, _instance.mSensorBattery1Val, buff, 0);
    	setTlvItem(SharedFuncLib.TLV_TYPE_BATTERY2_REMAIN, (short)4, _instance.mSensorBattery2Val, buff, 1*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_TEMP,            (short)4, _instance.mSensorTempVal, buff, 2*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_HUMI,            (short)4, _instance.mSensorHumiVal, buff, 3*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_MQX,             (short)4, _instance.mSensorMQXVal, buff, 4*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_SIGNAL_STRENGTH, (short)4, _instance.mSensorSignalStrength, buff, 5*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_GPS_COUNT,       (short)4, _instance.mSensorGPSCount, buff, 6*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_GPS_LONG,        (short)4, _instance.mSensorGPSLong, buff, 7*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_GPS_LATI,        (short)4, _instance.mSensorGPSLati, buff, 8*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_GPS_ALTI,        (short)4, _instance.mSensorGPSAlti, buff, 9*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_TOTAL_TIME,      (short)4, _instance.mSensorTotalTime, buff, 10*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_AIR_SPEED,       (short)4, _instance.mSensorAirSpeed, buff, 11*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_GND_SPEED,       (short)4, _instance.mSensorGndSpeed, buff, 12*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_DIST,            (short)4, _instance.mSensorDistance, buff, 13*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_HEIGHT,          (short)4, _instance.mSensorRelatedHeight, buff, 14*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_CLIMB_RATE,      (short)4, _instance.mSensorClimbRate, buff, 15*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_BATTERY1_VOLTAGE, (short)4, _instance.mSensorBattery1Volt, buff, 16*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_BATTERY1_CURRENT, (short)4, _instance.mSensorBattery1Curr, buff, 17*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_ORIE_X,           (short)4, _instance.mSensorOrienValX, buff, 18*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_ORIE_Y,           (short)4, _instance.mSensorOrienValY, buff, 19*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_ORIE_Z,           (short)4, _instance.mSensorOrienValZ, buff, 20*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_USER_A, (short)4, _instance.mSensorUserA_IsArmed, buff, 21*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_USER_B, (short)4, _instance.mSensorUserB_FlyMode, buff, 22*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_USER_C, (short)4, _instance.mSensorUserC_NetworkType, buff, 23*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_RC1,    (short)4, _instance.mSensorRC1, buff, 24*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_RC2,    (short)4, _instance.mSensorRC2, buff, 25*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_RC3,    (short)4, _instance.mSensorRC3, buff, 26*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_RC4,    (short)4, _instance.mSensorRC4, buff, 27*8);
    	setTlvItem(SharedFuncLib.TLV_TYPE_OOO,    (short)4, _instance.mSensorOOOVal, buff, 28*8);
    	
    	return buff;
    	
    	}
    }
    
    public static byte[] j_get_wp_data()
    {
    	if (_instance == null) {
    		return null;
    	}
    	if (false == _instance.m_bIsUAV)
		{
    		return null;
		}
    	
    	if (_instance.isWaypointGot == false) {
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {}
    	}
    	if (_instance.isWaypointGot == false) {
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {}
    	}
    	if (_instance.isWaypointGot == false) {
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {}
    	}
		
		if (_instance.isWaypointGot == false) {
			//TODO
			return null;
		}
		return _instance.waypointData;
    }
    
	private final Runnable client_lost_runnable = new Runnable() {
		public void run() {
			if (_instance == null) {
	    		return;
	    	}
	    	
			if (m_bMavLinkStarted) {
				
				if (state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING && state_mode != ApmModes.FIXED_WING_AUTO && state_mode != ApmModes.FIXED_WING_LOITER && state_mode != ApmModes.FIXED_WING_GUIDED) {
		    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_RTL);
		    	}
		    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_QUADROTOR && state_mode != ApmModes.ROTOR_AUTO && state_mode != ApmModes.ROTOR_LOITER && state_mode != ApmModes.ROTOR_GUIDED && state_mode != ApmModes.ROTOR_LAND) {
		    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROTOR_RTL);
		    	}
		    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_GROUND_ROVER && state_mode != ApmModes.ROVER_AUTO && state_mode != ApmModes.ROVER_GUIDED) {
		    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROVER_RTL);
		    	}
				
		    	m_bRcOverride = false;
				m_RcOutput.disableRcOverride();
			}
			
			ForceDisconnect();
		}
	};
    
	private void SendRadioMessage(MAVLinkConnection conn)
	{
		msg_radio msg = new msg_radio();
		msg.txbuf = 98;
		msg.rxerrors = 0;
		msg.fixed = 0;
		msg.rssi = 98;
		msg.remrssi = 98;
		msg.noise = 1;
		msg.remnoise = 1;
		
		conn.sendMavPacket(msg.pack());
	}
	
    private void TryToSendHearbeat()
    {
    	if (false == m_bMavLinkStarted) {
    		return;
    	}
    	
    	long now_time = System.currentTimeMillis();
    	if (now_time - m_lLastHearbeatSentTime < 1000) {
    		return;
    	}
    	m_lLastHearbeatSentTime = now_time;
    	
    	//Send GCS Heartbeat...冗余发送心跳包
    	MavLinkHeartbeat.sendMavHeartbeat(m_MAVLinkConnection);
    	SendRadioMessage(m_MAVLinkConnection);
    	
    	_instance.mMainHandler.removeCallbacks(client_lost_runnable);
    	if (state_mode == ApmModes.FIXED_WING_AUTO
    			|| state_mode == ApmModes.FIXED_WING_GUIDED
    			|| state_mode == ApmModes.FIXED_WING_LOITER
    			|| state_mode == ApmModes.ROTOR_AUTO
    			|| state_mode == ApmModes.ROTOR_GUIDED
    			|| state_mode == ApmModes.ROTOR_LOITER)
    	{
    		_instance.mMainHandler.postDelayed(client_lost_runnable, 18000);////考虑到rudp无缝切换的时间
    	}
    	else {
    		_instance.mMainHandler.postDelayed(client_lost_runnable, 5000);////非auto/loiter模式，快速反应
    	}
    }
    
    private void tailsitter_switch_to_apm()
    {
    	MavLinkSetRelay.sendSetRelayMessage(m_MAVLinkConnection, 1, true);//RELAY_PIN2
    }
    
    private void tailsitter_switch_to_mwc()
    {
    	MavLinkSetRelay.sendSetRelayMessage(m_MAVLinkConnection, 1, false);//RELAY_PIN2
    }
    
    private void tailsitter_set_aux_takeoff()
    {
    	MavLinkSetServo.sendSetServoMessage(m_MAVLinkConnection, 10, 1900);//a10->aux1
    	MavLinkSetServo.sendSetServoMessage(m_MAVLinkConnection, 11, 1500);//a11->aux2
    }
    
    private void tailsitter_set_aux_fly()
    {
    	MavLinkSetServo.sendSetServoMessage(m_MAVLinkConnection, 10, 1900);//a10->aux1
    	MavLinkSetServo.sendSetServoMessage(m_MAVLinkConnection, 11, 1100);//a11->aux2
    }
    
    private void tailsitter_set_aux_land()
    {
    	MavLinkSetServo.sendSetServoMessage(m_MAVLinkConnection, 10, 1900);//a10->aux1
    	MavLinkSetServo.sendSetServoMessage(m_MAVLinkConnection, 11, 1900);//a11->aux2
    }
    
    private void tailsitter_set_aux_disarm()
    {
    	MavLinkSetServo.sendSetServoMessage(m_MAVLinkConnection, 10, 1100);//a10->aux1
    	MavLinkSetServo.sendSetServoMessage(m_MAVLinkConnection, 11, 1100);//a11->aux2
    }
    
    private void uav_setHome(msg_mission_item msg)
    {
    	Gps bd09 = PositionUtil.gps84_To_Bd09(msg.x, msg.y);
    	if (null != bd09 && bd09.getWgLat() != 0 && bd09.getWgLon() != 0)
    	{
    		homeLati = bd09.getWgLat();
    		homeLongi = bd09.getWgLon();
    		homeAlti = msg.z;
    	}    	
    }
	
	private void uav_checkIsFlying(msg_vfr_hud m_hud) {
		boolean newState = (m_hud.throttle > 0);
		if (newState != state_isFlying) {
			state_isFlying = newState;
			if(state_isFlying){
				startTimer();
			}else{
				stopTimer();
			}
		}
	}

	private void uav_checkFailsafe(msg_heartbeat msg_heart) {
		boolean failsafe2 = msg_heart.system_status == (byte) MAV_STATE.MAV_STATE_CRITICAL;
		state_failsafe = (failsafe2);
		
		int armed = state_armed ? 1 : 0;
		int failsafe = state_failsafe ? 1 : 0;
		_instance.mSensorUserA_IsArmed = (double)(((failsafe & 0xff) << 8) | (armed & 0xff));
	}

	private void uav_checkArmState(msg_heartbeat msg_heart) {
		state_armed = ((msg_heart.base_mode & (byte) MAV_MODE_FLAG.MAV_MODE_FLAG_SAFETY_ARMED) == (byte) MAV_MODE_FLAG.MAV_MODE_FLAG_SAFETY_ARMED);
		
		int armed = state_armed ? 1 : 0;
		int failsafe = state_failsafe ? 1 : 0;
		_instance.mSensorUserA_IsArmed = (double)(((failsafe & 0xff) << 8) | (armed & 0xff));
	}
	
	private void uav_setFlyMode(ApmModes mode)
	{
		state_mode = mode;
		int num = state_mode.getNumber();
		int type = state_mode.getType();
		_instance.mSensorUserB_FlyMode = (double)(((num & 0xff) << 8) | (type & 0xff));
	}
	
	public void uav_setRollPitchYaw(double roll, double pitch, double yaw) {
		Log.d("RequestStream", "uav_setRollPitchYaw()...");
		
		if (_instance.m_bIsTailSitter && state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING)
		{
			if (tailsitter_state == 1 
					&& (_instance.m_SW_GPSALTI == 0 || (_instance.mSensorGPSAlti - _instance.mSensorGPSAlti_Saved) >= _instance.m_SW_GPSALTI) 
					&& (_instance.m_SW_GNDSPEED == 0 || (_instance.mSensorGndSpeed * 100.0f) >= _instance.m_SW_GNDSPEED) 
					&& _instance.mSensorOrienValY >= 45 && pitch <= 45 && state_armed == true) {
				//从垂直起飞阶段切换到APM-FBWA/Loiter模式，可重复执行
				tailsitter_state = 2;
				
				//MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_LOITER);
				if (false == m_bRcOverride) {
					m_RcOutput.enableRcOverride();
				}
	    		m_RcOutput.setRcChannel(RcOutput.ELEVATOR, -1*0.99f);//pitch
	    		m_RcOutput.setRcChannel(RcOutput.TROTTLE, 0.8f);//throttle
	    		m_RcOutput.setRcChannel(RcOutput.AILERON, 0.0f);//roll
	    		m_RcOutput.setRcChannel(RcOutput.RUDDER, 0.0f);//yaw
	    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_FLY_BY_WIRE_A);
	    		
				tailsitter_switch_to_apm();
				tailsitter_set_aux_fly();
				
				////////////////避免遥控器一直推杆会影响loiter
				new Thread(new Runnable() {
					public void run()
					{
			    		for (int i = 0; i < 20; i++)
			    		{
			    			try {
								Thread.sleep(200);
							} catch (InterruptedException e) {}
			    			if (3 == tailsitter_state) {
			    				break;
			    			}
			    		}
			    		
			    		if (3 != tailsitter_state) {
			    			m_RcOutput.setRcChannel(RcOutput.ELEVATOR, 0.0f);//pitch
				    		m_RcOutput.setRcChannel(RcOutput.AILERON, 0.0f);//roll
				    		m_RcOutput.setRcChannel(RcOutput.RUDDER, 0.0f);//yaw
						    if (false == m_bRcOverride) {
								m_RcOutput.disableRcOverride();
							}
						    MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_LOITER);
			    		}
					}
				}).start();
				////////////////////////////////////////////////////////////////
				
				_instance.mMainHandler.post(new Runnable(){
					@Override
					public void run() {
						SharedFuncLib.MyMessageTip(_instance, "MWC Takeoff => APM");
					}
				});
			}
			
			if (tailsitter_state == 2)
			{/*////Debug
				if (pitch <= -90 || roll <= -90 || roll >= 90) {
					tailsitter_state = 3;//紧急切到垂直降落
					
					tailsitter_set_aux_land();
    				tailsitter_switch_to_mwc();
    				MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_MANUAL);
    				
    				final String pitch_str = "" + pitch;
    				final String roll_str  = "" + roll;
    				_instance.mMainHandler.post(new Runnable(){
    					@Override
    					public void run() {
    						SharedFuncLib.MyMessageTip(_instance, "APM pitch("+pitch_str+")/roll("+roll_str+") abnormal => MWC Land");
    					}
    				});
    				
    				MediaPlayer mp = null;
    				if (pitch <= -90) {
    					mp = MediaPlayer.create(_instance, R.raw.record_start);
    				}else {
    					mp = MediaPlayer.create(_instance, R.raw.record_stop);
    				}
			    	if (null != mp) {
			    		_instance.mSkipAudioCount -= 1;
			    		Log.d(TAG, "MediaPlayer start...");
			    		mp.start();
			    		Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_RELEASE_MP, mp);
			        	_instance.mMainHandler.sendMessageDelayed(send_msg, 5000);
			    	}
				}*/
			}
		}
		_instance.mSensorOrienValX = roll;
		_instance.mSensorOrienValY = pitch;
		_instance.mSensorOrienValZ = yaw;
	}
    
	public void uav_setAltitudeGroundAndAirSpeeds(double altitude,
			double groundSpeed, double airSpeed, double climb) {
		Log.d("RequestStream", "uav_setAltitudeGroundAndAirSpeeds()...");
		
		if (_instance.m_bIsTailSitter && state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING)
		{
			if (tailsitter_state == 2)
			{
				if (_instance.m_SW_GPSALTI > 10 && (altitude - _instance.mSensorGPSAlti_Saved) < 10) {
					tailsitter_state = 3;//高度低于最少值，紧急切到垂直降落
					
					tailsitter_set_aux_land();
    				tailsitter_switch_to_mwc();
    				MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_MANUAL);
    				
    				_instance.mMainHandler.post(new Runnable(){
    					@Override
    					public void run() {
    						SharedFuncLib.MyMessageTip(_instance, "APM alt too low => MWC Land");
    					}
    				});
				}
			}
		}
		
		_instance.mSensorGPSAlti = altitude;
		_instance.mSensorGndSpeed = groundSpeed;
		_instance.mSensorAirSpeed = airSpeed;
		_instance.mSensorClimbRate = climb;
	}
	
	public void uav_setGpsState(int fix, int satellites_visible, int eph) {
		_instance.mSensorGPSCount =  (double)(((fix & 0xff) << 8) | (satellites_visible & 0xff));
	}
	
	public void uav_setPosition(double lati, double longi) {
		Gps bd09 = PositionUtil.gps84_To_Bd09(lati, longi);
		if (null != bd09) {
			_instance.mSensorGPSLati = bd09.getWgLat();
			_instance.mSensorGPSLong = bd09.getWgLon();
			if (state_armed == false) {
				_instance.homeLati = _instance.mSensorGPSLati;
				_instance.homeLongi = _instance.mSensorGPSLong;
			}
		}
	}
	
	public void uav_setBatteryState(double battVolt, double battRemain,
			double battCurrent) {
		_instance.mSensorBattery1Volt = battVolt;
		_instance.mSensorBattery1Val = battRemain;
		_instance.mSensorBattery1Curr = battCurrent;
	}
	
	public void uav_setRcInputValues(msg_rc_channels_raw msg) {
		
		if (_instance.m_bIsTailSitter && state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING)
		{
			if (tailsitter_state == 2 && Math.abs(msg.chan3_raw - 1600) < 100 && msg.chan4_raw < 1200) {
				//从APM切换到垂直降落阶段
				tailsitter_state = 3;
				
				_instance.mMainHandler.post(new Runnable(){
					@Override
					public void run() {
						SharedFuncLib.MyMessageTip(_instance, "APM => ...");
					}
				});
				
				new Thread(new Runnable() {
					public void run()
					{
						int i;
						
						if (false == m_bRcOverride) {
							m_RcOutput.enableRcOverride();
						}
						MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_STABILIZE);
			    		m_RcOutput.setRcChannel(RcOutput.ELEVATOR, -1*0.8f);//pitch
			    		m_RcOutput.setRcChannel(RcOutput.TROTTLE, 0.8f);//throttle
			    		
			    		for (i = 0; i < 30; i++)
			    		{
			    			try {
								Thread.sleep(200);
							} catch (InterruptedException e) {}
			    			
			    			if (_instance.mSensorOrienValY > 75) {
			    				tailsitter_set_aux_land();
			    				tailsitter_switch_to_mwc();
			    				MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_MANUAL);
			    				
			    				_instance.mMainHandler.post(new Runnable(){
			    					@Override
			    					public void run() {
			    						SharedFuncLib.MyMessageTip(_instance, "APM => MWC Land");
			    					}
			    				});
			    				
			    				break;
			    			}
			    		}
			    		
			    		if (i == 30) {//回滚到APM平飞状态
			    			m_RcOutput.setRcChannel(RcOutput.ELEVATOR, 0);//pitch
				    		m_RcOutput.setRcChannel(RcOutput.TROTTLE, 0);//throttle
			    			MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_LOITER);
			    			tailsitter_state = 2;
			    		}
						
						if (false == m_bRcOverride) {
							m_RcOutput.disableRcOverride();
						}
					}
				}).start();
			}
		}
		_instance.mSensorRC1 = (double)(msg.chan1_raw);
		_instance.mSensorRC2 = (double)(msg.chan2_raw);
		_instance.mSensorRC3 = (double)(msg.chan3_raw);
		_instance.mSensorRC4 = (double)(msg.chan4_raw);
	}
	
	// flightTimer
	// ----------------
	private long startTime = 0;
	private long elapsedFlightTime = 0;
	
	public void resetFlightTimer() {
		elapsedFlightTime = 0;
		startTime = SystemClock.elapsedRealtime();
	}

	public void startTimer() {
		startTime = SystemClock.elapsedRealtime();
	}

	public void stopTimer() {
		// lets calc the final elapsed timer
		elapsedFlightTime 	+= SystemClock.elapsedRealtime() - startTime;
		startTime 			= SystemClock.elapsedRealtime();
	}

	public long getFlightTime() {//返回秒数
		if(state_isFlying){
			// calc delta time since last checked
			elapsedFlightTime 	+= SystemClock.elapsedRealtime() - startTime;
			startTime 			= SystemClock.elapsedRealtime();
		}
		return elapsedFlightTime / 1000;
	}
	
	//获取离家距离
	public double getDroneDistanceToHome()
	{
		if (Math.abs(homeLati) < 0.001 || Math.abs(homeLongi) < 0.001) {
			return 0.0d;
		}
		if (Math.abs(mSensorGPSLati) < 0.001 || Math.abs(mSensorGPSLong) < 0.001) {
			return 0.0d;
		}
		double dist = 0.0d;
		try {
			dist = PositionUtil.getDistance(homeLati, homeLongi, mSensorGPSLati, mSensorGPSLong);
		} catch (Exception e) {}
		return dist;
	}
	
	@Override
	public void onReceiveMessage(MAVLinkMessage msg) {
		// TODO Auto-generated method stub
		waypointMananger.processMessage(msg);
		
		switch (msg.msgid)
		{
		case msg_attitude.MAVLINK_MSG_ID_ATTITUDE:
			msg_attitude m_att = (msg_attitude) msg;
			uav_setRollPitchYaw(m_att.roll * 180.0 / Math.PI,
					m_att.pitch * 180.0 / Math.PI, m_att.yaw * 180.0 / Math.PI);
			break;
		case msg_vfr_hud.MAVLINK_MSG_ID_VFR_HUD:
			msg_vfr_hud m_hud = (msg_vfr_hud) msg;
			uav_setAltitudeGroundAndAirSpeeds(m_hud.alt, m_hud.groundspeed,m_hud.airspeed, m_hud.climb);
			uav_checkIsFlying(m_hud);
			break;
		case msg_mission_current.MAVLINK_MSG_ID_MISSION_CURRENT:
			break;
		case msg_nav_controller_output.MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT:
			msg_nav_controller_output m_nav = (msg_nav_controller_output) msg;
			break;
		case msg_heartbeat.MAVLINK_MSG_ID_HEARTBEAT:
			msg_heartbeat msg_heart = (msg_heartbeat) msg;
			state_type = msg_heart.type;
			if (state_type == MAV_TYPE.MAV_TYPE_TRICOPTER || state_type == MAV_TYPE.MAV_TYPE_HEXAROTOR || state_type == MAV_TYPE.MAV_TYPE_OCTOROTOR
					|| state_type == MAV_TYPE.MAV_TYPE_HELICOPTER || state_type == MAV_TYPE.MAV_TYPE_COAXIAL )
			{//都当四旋翼类型处理
				state_type = MAV_TYPE.MAV_TYPE_QUADROTOR;
			}
			if (state_type == MAV_TYPE.MAV_TYPE_SURFACE_BOAT) {
				state_type = MAV_TYPE.MAV_TYPE_GROUND_ROVER;
			}
			
			
			uav_checkArmState(msg_heart);
			uav_checkFailsafe(msg_heart);
			ApmModes newMode;
			newMode = ApmModes.getMode(msg_heart.custom_mode, state_type);
			uav_setFlyMode(newMode);
			
			if (m_bFirstHearbeat)
			{
				m_bFirstHearbeat = false;
				
				//Send GCS Heartbeat...
		    	MavLinkHeartbeat.sendMavHeartbeat(m_MAVLinkConnection);
		    	SendRadioMessage(m_MAVLinkConnection);
		    	
				try {
					Thread.sleep(20);
				} catch (InterruptedException e) {}
				MavLinkStreamRates.setupStreamRates(m_MAVLinkConnection, 2, 5, 2, 2, 1, 2, 2, 0);
				try {
					Thread.sleep(20);
				} catch (InterruptedException e) {}
				MavLinkStreamRates.setupStreamRates(m_MAVLinkConnection, 2, 5, 2, 2, 1, 2, 2, 0);
				
				m_bRcOverride = false;
				m_RcOutput.disableRcOverride();
				
				//开始读取航点数据。。。
				waypointMananger.getWaypoints();
			}
			else
			{
				//Send GCS Heartbeat...
		    	MavLinkHeartbeat.sendMavHeartbeat(m_MAVLinkConnection);
		    	SendRadioMessage(m_MAVLinkConnection);
			}
			break;
		case msg_global_position_int.MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
			uav_setPosition(((msg_global_position_int) msg).lat / 1E7,
					((msg_global_position_int) msg).lon / 1E7);
			break;
		case msg_sys_status.MAVLINK_MSG_ID_SYS_STATUS:
			msg_sys_status m_sys = (msg_sys_status) msg;
			uav_setBatteryState(m_sys.voltage_battery / 1000.0,
					m_sys.battery_remaining, m_sys.current_battery / 100.0);
			break;
		case msg_radio.MAVLINK_MSG_ID_RADIO:
			break;
		case msg_gps_raw_int.MAVLINK_MSG_ID_GPS_RAW_INT:
			uav_setGpsState(((msg_gps_raw_int) msg).fix_type,
					((msg_gps_raw_int) msg).satellites_visible,
					((msg_gps_raw_int) msg).eph);
			break;
		case msg_rc_channels_raw.MAVLINK_MSG_ID_RC_CHANNELS_RAW:
			uav_setRcInputValues((msg_rc_channels_raw) msg);
			break;
		case msg_servo_output_raw.MAVLINK_MSG_ID_SERVO_OUTPUT_RAW:
			break;
		}
	}
	
	@Override
	public void onDisconnect() {
		// TODO Auto-generated method stub
		if (m_bIsUAV)
    	{
    		mSensorOOOVal = 7;//无人机用3个红色块表示usb线路松动
    	}
	}
	
	@Override
	public void onComError(String errMsg) {
		// TODO Auto-generated method stub
		if (_instance == null) {
    		return;
    	}
		final String fstr = "MAVLinkConnection: " + errMsg;
		_instance.mMainHandler.post(new Runnable(){
			@Override
			public void run() {
				SharedFuncLib.MyMessageTip(_instance, fstr);
			}
		});
	}
	
    private float getRadianCouvert(int angle)
    {
    	//if (angle >= 0 && angle <= 180)
    	//{
    	//	angle = -1 * angle;
    	//} else {
    	//	angle = 360 - angle;
    	//}
    	return (float) ((float)angle * Math.PI /180.0f);
    }
	
    private void uav_contrl_left_servo(int n)
    {
    	int value = 1500 + (n - 90)*1400/180;
    	MavLinkSetServo.sendSetServoMessage(m_MAVLinkConnection, 10, value);//a10->
    }
    
    private void uav_contrl_right_servo(int n)
    {
    	int value = 1500 + (n - 90)*1400/180;
    	MavLinkSetServo.sendSetServoMessage(m_MAVLinkConnection, 11, value);//a11->钓竿Servo
    }
    
    private void uav_contrl_joystick1(int param)
    {
    	if (_instance == null) {
    		return;
    	}
    	if (_instance.m_bMavLinkStarted == false) {
    		return;
    	}
    	
    	if (m_bRcOverride == true)
    	{
    	int L = (param & 0xffff0000) >>> 16;
    	int angle = param & 0x0000ffff;
    	float radian = getRadianCouvert(angle);
    	double x = L * Math.cos(radian) / 65535;
    	double y = L * Math.sin(radian) / 65535;
    	
    	m_RcOutput.setRcChannel(RcOutput.AILERON, x*0.6f);//roll
    	m_RcOutput.setRcChannel(RcOutput.ELEVATOR, -1*y*0.6f);//pitch
    	}
    	
    	TryToSendHearbeat();
    }
    
    private void uav_contrl_joystick2(int param)
    {
    	if (_instance == null) {
    		return;
    	}
    	if (_instance.m_bMavLinkStarted == false) {
    		return;
    	}
    	
    	if (m_bRcOverride == true)
    	{
    	int L = (param & 0xffff0000) >>> 16;
    	int angle = param & 0x0000ffff;
    	float radian = getRadianCouvert(angle);
    	double x = L * Math.cos(radian) / 65535;
    	double y = L * Math.sin(radian) / 65535;
    	
    	m_RcOutput.setRcChannel(RcOutput.RUDDER, x*0.6f);
    	m_RcOutput.setRcChannel(RcOutput.TROTTLE, y*0.9f);
    	}
    	
    	TryToSendHearbeat();
    }
    
    private void uav_contrl_button_a()
    {
    	if (_instance.m_bIsTailSitter && state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING)
		{
			if (tailsitter_state == 1 || tailsitter_state == 3) {
				//垂直降落阶段，MWC强制切换到Angle+Mag
	    		tailsitter_set_aux_takeoff();
	    		return;
			}
		}
    	
    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_QUADROTOR) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROTOR_STABILIZE);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_STABILIZE);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROVER_MANUAL);
    	}
    }
    
    private void uav_contrl_button_b()
    {
    	if (_instance.m_bIsTailSitter && state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING)
		{
			if (tailsitter_state == 1 || tailsitter_state == 3) {
				//垂直降落阶段，MWC强制切换到Angle+Mag+Baro
	    		tailsitter_set_aux_land();
	    		return;
			}
		}
    	
    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_QUADROTOR) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROTOR_ALT_HOLD);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_FLY_BY_WIRE_A);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROVER_STEERING);
    	}
    }
    
    private void uav_contrl_button_x()
    {
    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_QUADROTOR) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROTOR_LOITER);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_LOITER);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROVER_HOLD);
    	}
    }
    
    private void uav_contrl_button_y()
    {
    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_QUADROTOR) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROTOR_AUTO);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_AUTO);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROVER_AUTO);
    	}
    }
    
    private void uav_contrl_button_l1(int param)
    {
    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_QUADROTOR) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROTOR_LAND);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_LOITER);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
    		//MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROVER_LAND);
    	}
    }
    
    private void uav_contrl_button_l2(int param)
    {
    	if (0 == param) {
    		if (_instance.m_bIsTailSitter && state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING)
    		{
    			if ((tailsitter_state == 1 || tailsitter_state == 3) &&
    					state_isFlying == false && state_armed == true) {
    				//加锁。。。
    				tailsitter_state = 0;
    				MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_MANUAL);
    	    		MavLinkArm.sendArmMessage(m_MAVLinkConnection, false);
    	    		
    	    		tailsitter_set_aux_disarm();
    	    		tailsitter_switch_to_mwc();
    			}
    			return;
    		}
    		
    		if (state_isFlying == false && state_armed == true) {
    			MavLinkArm.sendArmMessage(m_MAVLinkConnection, false);
    		}
    	}
    	else {
    		if (_instance.m_bIsTailSitter && state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING)
    		{
    			if (tailsitter_state == 0 && 
    					state_isFlying == false && state_armed == false) {
    				//解锁。。。准备垂直起飞
    				tailsitter_state = 1;
    				MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_MANUAL);
    	    		MavLinkArm.sendArmMessage(m_MAVLinkConnection, true);
    	    		
    	    		tailsitter_set_aux_takeoff();
    	    		tailsitter_switch_to_mwc();
    	    		
    	    		_instance.mSensorGPSAlti_Saved = _instance.mSensorGPSAlti;
    			}
    			return;
    		}
    		
    		if (state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING) {
    			MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_STABILIZE);
    		}
    		else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_QUADROTOR) {
    			MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROTOR_STABILIZE);
    		}
    		else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
    			MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROVER_MANUAL);
    		}
    		
    		if (state_isFlying == false && state_armed == false) {
    			MavLinkArm.sendArmMessage(m_MAVLinkConnection, true);
    		}
    	}
    }
    
    private void uav_contrl_button_r1(int param)
    {
    	if (state_mode.getType() == MAV_TYPE.MAV_TYPE_FIXED_WING) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.FIXED_WING_RTL);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_QUADROTOR) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROTOR_RTL);
    	}
    	else if (state_mode.getType() == MAV_TYPE.MAV_TYPE_GROUND_ROVER) {
    		MavLinkModes.changeFlightMode(m_MAVLinkConnection, ApmModes.ROVER_RTL);
    	}
    }
    
    private void uav_contrl_button_r2(int param)
    {
    	if (0 == param) {
    		m_bRcOverride = false;
    		m_RcOutput.disableRcOverride();
    	}
    	else {
    		m_bRcOverride = true;
    		m_RcOutput.enableRcOverride();
    	}
    }
	
	
    ////////////////////////////////////////////
    
	public void recvRtpAudioData(byte[] data, int offset, int len, int ptype) {
		// TODO Auto-generated method stub
		if (bUseRtpStream) PutRtpAudioData(data, offset, len, ptype);
	}
	
	public void recvRtpVideoData(byte[] data, int offset, int len, int ptype) {
		// TODO Auto-generated method stub
		if (bUseRtpStream) PutRtpVideoData(data, offset, len, ptype);
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
    
    public native void AvSwitchToLocalStream();
    public native void AvSwitchToRtpStream();
    public native int PutRtpAudioData(byte[] data, int offset, int len, int ptype);
    public native int PutRtpVideoData(byte[] data, int offset, int len, int ptype);
    
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
