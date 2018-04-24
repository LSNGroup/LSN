package com.wangling.remotephone;


import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.List;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.util.Base64;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;
import android.widget.ListView;
import com.lsngroup.live.R;


public class MainListActivity extends ListActivity {
	
	
	static final int WORK_MSG_REFRESH = 1;
	static final int WORK_MSG_ON_CONNECTED = 2;
	
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
			case WORK_MSG_REFRESH://obj:ids
				
				send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_PROGRESS_SHOW, _instance.getResources().getString(R.string.msg_please_wait));
		    	_instance.mMainHandler.sendMessage(send_msg);
		    	
		    	_instance.m_nCurrentSelected = -1;
		    	_instance.m_nodesArray.clear();
		    	DoSearchChannels(0, 30);//msg.obj
	    		send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_REFRESH_RESULT);
		    	_instance.mMainHandler.sendMessage(send_msg);
		    	
		    	send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_PROGRESS_CANCEL);
		    	_instance.mMainHandler.sendMessage(send_msg);
		    	
				break;
				
			case WORK_MSG_ON_CONNECTED://arg1:type, arg2:fhandle
				_instance.conn_type = msg.arg1;
				_instance.conn_fhandle = msg.arg2;
				
				//////////////////////////PROGRESS_SHOW
				String strMsgText = null;
				if (_instance.conn_type == SharedFuncLib.SOCKET_TYPE_TCP)
				{
					strMsgText = _instance.getResources().getString(R.string.msg_connect_checking_password_tcp);
				}
				else {
					strMsgText = _instance.getResources().getString(R.string.msg_connect_checking_password);
				}
		    	send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_PROGRESS_SHOW, strMsgText);
		    	_instance.mMainHandler.sendMessage(send_msg);
		    	
		    	String strPass = AppSettings.GetSoftwareKeyValue(_instance, 
		    			"" + _instance.m_nodesArray.get(_instance.m_nCurrentSelected).channel_id + AppSettings.STRING_REGKEY_NAME_CAM_PASSWORD, 
		    			"");
		    	if (false == strPass.equals("")) {
		    		strPass = SharedFuncLib.phpMd5(strPass);
		    	}
		    	
		    	int[] arrResults = new int[3];
		    	ret = SharedFuncLib.CtrlCmdHELLO(_instance.conn_type, _instance.conn_fhandle, strPass, arrResults);
		    	
		    	//////////////////////////PROGRESS_CANCEL
		    	send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_PROGRESS_CANCEL);
		    	_instance.mMainHandler.sendMessage(send_msg);
		    	
		    	if (0 != ret) {
		    		send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGEBOX, _instance.getResources().getString(R.string.msg_connect_checking_password_failed1));
		        	_instance.mMainHandler.sendMessage(send_msg);
		        	DoDisconnect();
		    	}
		    	else if (SharedFuncLib.CTRLCMD_RESULT_NG == arrResults[0]) {
		    		send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, _instance.getResources().getString(R.string.msg_connect_checking_password_retry));
		        	_instance.mMainHandler.sendMessage(send_msg);
		        	
		        	//Show password dialog...
		        	send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_CONNECT_PASSDLG);
		        	_instance.mMainHandler.sendMessage(send_msg);
		    	}
		    	else {// OK, go to AvParam or AvPlay
    		       	send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_CONNECT_RESULT);
		        	_instance.mMainHandler.sendMessage(send_msg);
		    	}
				
				break;
				
			default:
				break;	
			}
			
			super.handleMessage(msg);
		}
	}
	
	
	static final int UI_MSG_AUTO_START = 0;
	static final int UI_MSG_MESSAGEBOX = 1;
	static final int UI_MSG_MESSAGETIP = 2;
	static final int UI_MSG_PROGRESS_SHOW = 3;
	static final int UI_MSG_PROGRESS_CANCEL = 4;
	static final int UI_MSG_REFRESH_RESULT = 5;
	static final int UI_MSG_CONNECT_PASSDLG = 6;
	static final int UI_MSG_CONNECT_RESULT = 7;
	
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
				onBtnRefresh();
				break;
				
			case UI_MSG_MESSAGEBOX://obj
				SharedFuncLib.MyMessageBox(_instance, _instance.getResources().getString(R.string.app_name), (String)(msg.obj));
				break;
				
			case UI_MSG_MESSAGETIP://obj
				SharedFuncLib.MyMessageTip(_instance, (String)(msg.obj));
				break;
				
			case UI_MSG_PROGRESS_SHOW://obj
				if (null == mProgressDialog) {
					mProgressDialog = new ProgressDialog(_instance);
					mProgressDialog.setCancelable(false);
					mProgressDialog.setMessage((String)(msg.obj));
					mProgressDialog.show();
				}
				else {
					mProgressDialog.setMessage((String)(msg.obj));
				}
				break;
			
			case UI_MSG_PROGRESS_CANCEL:
				if (null != mProgressDialog) {
					mProgressDialog.dismiss();
					mProgressDialog = null;
				}
				break;
			
			case UI_MSG_REFRESH_RESULT:
				NodeListAdapter adapter = new NodeListAdapter(_instance, m_nodesArray);
				mListView.setAdapter(adapter);
				break;
			
			case UI_MSG_CONNECT_PASSDLG:
				final View textEntryView = LayoutInflater.from(_instance).inflate(  
		                R.layout.dlgpass, null);
		        final EditText editPass=(EditText)textEntryView.findViewById(R.id.edit_pass);
		        AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
		        builder.setCancelable(false);
		        builder.setTitle(_instance.getResources().getString(R.string.msg_connect_dlgpass_title));
		        builder.setView(textEntryView);
		        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_ok_btn),
		                new DialogInterface.OnClickListener() {  
		                    public void onClick(DialogInterface dialog, int whichButton) {
		                    	
		                    	String strPass = editPass.getText().toString();
		        		    	if (false == strPass.equals("")) {
		        		    		strPass = SharedFuncLib.phpMd5(strPass);
		        		    	}
		        		    	
		        		    	int[] arrResults = new int[3];
		        		    	int ret = SharedFuncLib.CtrlCmdHELLO(_instance.conn_type, _instance.conn_fhandle, strPass, arrResults);
		        		    	
		        		    	if (0 != ret) {
		        		    		Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, _instance.getResources().getString(R.string.msg_connect_checking_password_failed1));
		        		        	_instance.mMainHandler.sendMessage(send_msg);
		        		        	DoDisconnect();
		        		        	onBtnRefresh();
		        		    	}
		        		    	else if (SharedFuncLib.CTRLCMD_RESULT_NG == arrResults[0]) {
		        		    		Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, _instance.getResources().getString(R.string.msg_connect_checking_password_retry));
		        		        	_instance.mMainHandler.sendMessage(send_msg);
		        		        	
		        		        	//Show password dialog...
		        		        	send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_CONNECT_PASSDLG);
		        		        	_instance.mMainHandler.sendMessage(send_msg);
		        		    	}
		        		    	else {// OK, save password and go to AvParam or AvPlay
		        		    		
		        		    		AppSettings.SaveSoftwareKeyValue(_instance, 
		        			    			"" + _instance.m_nodesArray.get(_instance.m_nCurrentSelected).channel_id + AppSettings.STRING_REGKEY_NAME_CAM_PASSWORD, 
		        			    			editPass.getText().toString());
		        		    		
		        		        	Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_CONNECT_RESULT);
		        		        	_instance.mMainHandler.sendMessage(send_msg);
		        		    	}
		        		    	
		        		    	dialog.dismiss();
		                    }
		                });
		        builder.setNegativeButton(_instance.getResources().getString(R.string.ui_cancel_btn),
		                new DialogInterface.OnClickListener() {  
		                    public void onClick(DialogInterface dialog, int whichButton) {  
		    		    		Message send_msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, _instance.getResources().getString(R.string.msg_connect_checking_password_failed2));
		    		        	_instance.mMainHandler.sendMessage(send_msg);
		    		        	DoDisconnect();
		    		        	onBtnRefresh();
		    		        	dialog.dismiss();
		                    }
		                });
		        builder.show();
				break;
			
			case UI_MSG_CONNECT_RESULT:
			  if (_instance.do_func == DO_FUNC_FULL)
			  {
			  }
			  else if (_instance.do_func == DO_FUNC_VNC)
			  {
			  }
			  else if (_instance.do_func == DO_FUNC_AV)
			  {////////
	    			Intent intent = new Intent(_instance, AvPlayActivity.class);
	    	    	Bundle bundle = new Bundle();
	    	    	bundle.putInt("comments_id", m_nodesArray.get(m_nCurrentSelected).channel_id);
	    	    	bundle.putInt("conn_type", _instance.conn_type);
	    	    	bundle.putInt("conn_fhandle", _instance.conn_fhandle);
	    	    	bundle.putInt("audio_channels", 1);
	    	    	bundle.putInt("video_channels", 1);
	    	    	bundle.putString("device_uuid", m_nodesArray.get(m_nCurrentSelected).device_uuid);
	    	    	intent.putExtras(bundle);
	    	    	startActivityForResult(intent, REQUEST_CODE_AVPLAY);
			  }////////
			  
			  mMainHandler.removeCallbacks(auto_send_ctrlnull_runnable);
			  mMainHandler.postDelayed(auto_send_ctrlnull_runnable, 25000);
			  
				break;
				
			default:
				break;				
			}
			
			super.handleMessage(msg);
		}
	}
	
	
	final Runnable auto_send_ctrlnull_runnable = new Runnable() {
		public void run() {
			try {
				//此处本来是要发送CMD_CODE_NULL，由于受控端低版本不能处理CMD_CODE_NULL，暂时屏蔽
				//SharedFuncLib.CtrlCmdSendNULL(conn_type, conn_fhandle);
				if (_instance != null) {
					mMainHandler.postDelayed(auto_send_ctrlnull_runnable, 25000);
				}
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	};
	
	
	private static MainListActivity _instance = null;//////////////////
	
	private static final String TAG = "MainListActivity";
	private PowerManager.WakeLock m_wl = null;
	private WorkerHandler mWorkerHandler = null;
	private MainHandler mMainHandler = null;
	
	private ProgressDialog mProgressDialog = null;
	private int conn_type;
	private int conn_fhandle;
	private int proxy_tcp_port = 10000;
	
	private int do_func;
	
	public static final int DO_FUNC_FULL = 0;
	public static final int DO_FUNC_AV = 1;
	public static final int DO_FUNC_VNC = 2;
	public static final int DO_FUNC_FT = 3;
	public static final int DO_FUNC_DP = 4;//DroidPlanner
	
	private ListView mListView = null;
	private List<CHANNEL_NODE> m_nodesArray = null;
	private int m_nCurrentSelected;
	
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.mainlist);
        
        
        Log.d(TAG, "Acquiring wake lock");
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        m_wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "MainListActivity PARTIAL_WAKE_LOCK");
        m_wl.acquire();
        
        
        Worker worker = new Worker("MainListActivity Worker");
        mWorkerHandler = new WorkerHandler(worker.getLooper());
        mMainHandler = new MainHandler();
        
        
        mListView = this.getListView();
        m_nodesArray = new ArrayList<CHANNEL_NODE>();
    	m_nCurrentSelected = -1;
        
     
        findViewById(R.id.addcam_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnAddCam();
        	}
        });
        findViewById(R.id.refresh_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnRefresh();
        	}
        });
        findViewById(R.id.exitapp_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnExitApp();
        	}
        });
        
        
        
        
        _instance = this;//////////////////
        
        mMainHandler.sendEmptyMessageDelayed(UI_MSG_AUTO_START, 500);
        
        /* Start native... */
        SetThisObject();
        StartNative("utf8", getResources().getString(R.string.app_lang));
        
        int ver = SharedFuncLib.getAppVersion();
        String ver_str = String.format("%s Ver %d.%d.%d", 
        		_instance.getResources().getString(R.string.app_name),
        		(ver & 0xff000000)>>24,
        		(ver & 0x00ff0000)>>16,
        		(ver & 0x0000ff00)>>8);
        _instance.setTitle(ver_str);
    }
    
    @Override
    protected void onDestroy() {
    	
    	mMainHandler.removeCallbacks(auto_send_ctrlnull_runnable);
    	
    	_instance = null;//////////////////
    	
    	if (null != mProgressDialog) {
    		mProgressDialog.dismiss();
    		mProgressDialog = null;
    	}
    	
        Log.d(TAG, "Release wake lock");
    	m_wl.release();
    	
    	mWorkerHandler.getLooper().quit();
    	
    	m_nCurrentSelected = -1;
    	m_nodesArray.clear();    	
    	
    	
    	StopNative();
    	
    	super.onDestroy();
    }
    
    private void onBtnAddCam()
    {
    	Intent intent = new Intent(this, AddCamActivity.class);
    	startActivityForResult(intent, REQUEST_CODE_ADDCAM);
    }
    
    private void onBtnRefresh()
    {
    	String ids = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_QUERYIDS, "");
    	//if (ids.equals("")) {
    	//	SharedFuncLib.MyMessageTip(_instance, _instance.getResources().getString(R.string.msg_empty_addcam_first));
    	//	return;
    	//}
    	
    	Message send_msg = mWorkerHandler.obtainMessage(WORK_MSG_REFRESH, ids);
    	mWorkerHandler.sendMessage(send_msg);
    }
    
    private void onBtnExitApp()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
        builder.setTitle(_instance.getResources().getString(R.string.app_name));
        builder.setMessage(_instance.getResources().getString(R.string.msg_exit_app_or_not));
        builder.setPositiveButton(_instance.getResources().getString(R.string.ui_yes_btn),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        //setTitle("点击了对话框上的Button1");
                    	dialog.dismiss();
                    	_instance.finish();
                    	android.os.Process.killProcess(android.os.Process.myPid());
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
        	onBtnExitApp();
            return true;   
        }else{         
            return super.onKeyDown(keyCode, event);   
        }   
    }   
    
    
    static final int REQUEST_CODE_ADDCAM = 1;
    static final int REQUEST_CODE_AVPLAY = 2;
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
    	switch (requestCode)
    	{
    	case REQUEST_CODE_ADDCAM:
    		if (RESULT_OK == resultCode)
    		{
    			onBtnRefresh();
    		}
    		break;
    		
    	case REQUEST_CODE_AVPLAY:
    		mMainHandler.removeCallbacks(auto_send_ctrlnull_runnable);
    		SharedFuncLib.CtrlCmdBYE(conn_type, conn_fhandle);
    		DoDisconnect();
    		onBtnRefresh();
    		break;
    	}
    }

    @Override 
    protected void onListItemClick(ListView l, View v, int position, long id)
    {
    	m_nCurrentSelected = position;
    	if (m_nCurrentSelected >= 0) {
    		if (m_nodesArray.get(m_nCurrentSelected).isOnline())
    		{
				//this.dismissDialog(id);
				this.showDialog(DIALOG_ONLINE_OPERATIONS);
    		}
    		else
    		{
    			//this.dismissDialog(id);
    			this.showDialog(DIALOG_OFFLINE_OPERATIONS);
    		}
    	}
    }
    
    
    static final int DIALOG_OFFLINE_OPERATIONS = 0;
    static final int DIALOG_ONLINE_OPERATIONS = 1;
    
    @Override
    protected Dialog onCreateDialog(int id)
    {
		Builder builder = new AlertDialog.Builder(_instance); 
    	Dialog dialog = null;
    	
    	switch (id)
    	{
    	case DIALOG_ONLINE_OPERATIONS:  
    		builder.setItems(R.array.array_select_operation_items, new DialogInterface.OnClickListener(){
	    			public void onClick(DialogInterface dialog, int which)
	    			{
	    				if (0 == which)
	    				{
	    					do_func = DO_FUNC_AV;
	    					onMenuItemConnect();
	    				}
	    				else if (1 == which)
	    				{
	    					onMenuItemSetParams();
	    				}
	    				else if (2 == which)
	    				{
	    					onMenuItemRemove();
	    				}
	    			}
    			});
    		dialog = builder.create();
    		break;
    		
    	case DIALOG_OFFLINE_OPERATIONS:
    		builder.setItems(R.array.array_select_operation_offline_items, new DialogInterface.OnClickListener(){
	    			public void onClick(DialogInterface dialog, int which)
	    			{
	    				if (0 == which)
	    				{
	    					onMenuItemSetParams();
	    				}
	    				else if (1 == which)
	    				{
	    					onMenuItemRemove();
	    				}
	    			}
    			});
    		dialog = builder.create();
    		break;
    		
    	default:
    		break;
    	}
    	
    	return dialog;
    }
    
    private void autoAddCam(int cam_id)
    {
    	String ids = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_QUERYIDS, "");
    	String ids2 = "-" + ids + "-";
    	
    	String strCamId = String.format("%d", cam_id);
    	
    	if (strCamId == null || strCamId.equals(""))
    	{
    		//SharedFuncLib.MyMessageBox(this, 
    		//		getResources().getString(R.string.app_name), 
    		//		getResources().getString(R.string.msg_mobcam_id_empty));
    		return;
    	}
    	
    	if (ids2.contains("-" + strCamId + "-"))
    	{
    		//SharedFuncLib.MyMessageBox(this, 
    		//		getResources().getString(R.string.app_name), 
    		//		getResources().getString(R.string.msg_mobcam_id_exists));
    		return;
    	}
    	
    	String new_ids = null;
    	if (ids.equals("")) {
    		new_ids = strCamId;
    	}
    	else {
    		new_ids = ids + "-" + strCamId;
    	}
    	AppSettings.SaveSoftwareKeyValue(this, AppSettings.STRING_REGKEY_NAME_QUERYIDS, new_ids);
    }
    
    private void onMenuItemConnect()
    {
    	autoAddCam(m_nodesArray.get(m_nCurrentSelected).channel_id);
    	
    	if (false == m_nodesArray.get(m_nCurrentSelected).isOnline()) {
    		return;
    	}
    	
    	DoConnect("123456",	m_nodesArray.get(m_nCurrentSelected).channel_id);
    }
    
    private void onMenuItemSetParams()
    {
    }
    
    private void onMenuItemRemove()
    {
    	String ids = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_QUERYIDS, "");
    	if (ids.equals("")) {
    		return;
    	}
    	
    	String ids2 = "-" + ids + "-";
    	String target = String.format("-%d-", m_nodesArray.get(m_nCurrentSelected).channel_id);
    	ids2 = ids2.replace(target, "-");
    	if (ids2.equals("-") || ids2.equals("--")) {
    		ids = "";
    	}
    	else {
    		ids = ids2.substring(1, ids2.length() - 1);
    	}
    	
    	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_QUERYIDS, ids);
    	
    	onBtnRefresh();
    }
    
    
    public void FillChannelNode(int index, int channel_id, String channel_comments, 
    		String device_uuid,	String node_id_str, String pub_ip_str, String location 		)
    {
    	if (null == m_nodesArray) {
    		return;
    	}
    	
    	if (0 == index) {
    		m_nodesArray.clear();
    	}
    	
    	CHANNEL_NODE node = new CHANNEL_NODE();
    	
    	node.channel_id = channel_id;
    	node.channel_comments = channel_comments;
    	node.device_uuid = device_uuid;
    	node.node_id_str = node_id_str;
    	node.pub_ip_str = pub_ip_str;
    	node.location = location;
    	
    	m_nodesArray.add(node);
    }
    
    public static String j_get_viewer_nodeid()
    {
    	if (_instance == null) {
    		return null;
    	}
    
    	String nodeid = AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_VIEWERNODEID, "");
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
    		
    		AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_VIEWERNODEID, newid);
    		return newid;
    	}
    	else {
    		return nodeid;
    	}
    }
    
    public static void j_show_text(String text)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	Message msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, text);
    	_instance.mMainHandler.sendMessage(msg);
    	Message msg2 = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, text);
    	_instance.mMainHandler.sendMessageDelayed(msg2, 3000);
    	Message msg3 = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, text);
    	_instance.mMainHandler.sendMessageDelayed(msg3, 6000);
    }
    
    public static void j_messagebox(int msg_rid)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	Message msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGEBOX, _instance.getResources().getString(msg_rid));
    	_instance.mMainHandler.sendMessage(msg);
    }
    
    public static void j_messagetip(int msg_rid)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	Message msg = _instance.mMainHandler.obtainMessage(UI_MSG_MESSAGETIP, _instance.getResources().getString(msg_rid));
    	_instance.mMainHandler.sendMessage(msg);
    }
    
    public static void j_progress_show(int msg_rid)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	Message msg = _instance.mMainHandler.obtainMessage(UI_MSG_PROGRESS_SHOW, _instance.getResources().getString(msg_rid));
    	_instance.mMainHandler.sendMessage(msg);
    }    
    
    public static void j_progress_show_format1(int msg_rid, int arg1)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	String obj = String.format(_instance.getResources().getString(msg_rid), arg1);
    	Message msg = _instance.mMainHandler.obtainMessage(UI_MSG_PROGRESS_SHOW, obj);
    	_instance.mMainHandler.sendMessage(msg);
    }
    
    public static void j_progress_show_format2(int msg_rid, int arg1, int arg2)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	String obj = String.format(_instance.getResources().getString(msg_rid), arg1, arg2);
    	Message msg = _instance.mMainHandler.obtainMessage(UI_MSG_PROGRESS_SHOW, obj);
    	_instance.mMainHandler.sendMessage(msg);
    }
    
    public static void j_progress_cancel()
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	Message msg = _instance.mMainHandler.obtainMessage(UI_MSG_PROGRESS_CANCEL);
    	_instance.mMainHandler.sendMessage(msg);
    }
    
    public static void j_on_connected(int type, int fhandle)
    {
    	if (_instance == null) {
    		return;
    	}
    	
    	Message msg = _instance.mWorkerHandler.obtainMessage(WORK_MSG_ON_CONNECTED, type, fhandle);
    	_instance.mWorkerHandler.sendMessage(msg);
    }
    
    
    ///////////////////////////////////////////////////////////////////////////////////////
    public native void SetThisObject();
    public native int StartNative(String str_client_charset, String str_client_lang);
    public native void StopNative();
    public native int DoSearchChannels(int page_offset, int page_rows);
    public native void DoConnect(String password_str, int channel_id);
    public native void DoDisconnect();
    
    ///////////////////////////////////////////////////////////////////////////////////////
    static {
        System.loadLibrary("up2p"); //The first
        System.loadLibrary("shdir");//The second
        System.loadLibrary("avrtp");//The third
    }
}
