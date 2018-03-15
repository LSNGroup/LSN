package com.wangling.remotephone;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageButton;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import com.wangling.remotephone.R;


public class AvParamActivity extends Activity {
	
	private int comments_id;
	private int conn_type;
	private int audio_channels;
	private int video_channels;
	
	private byte av_video_size;
	private byte av_video_framerate;
	private int av_audio_channel;
	private int av_video_channel;

	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.avparam);
        
        Bundle extras = getIntent().getExtras();
        comments_id = extras.getInt("comments_id");
        conn_type = extras.getInt("conn_type");
        audio_channels = extras.getInt("audio_channels");
        video_channels = extras.getInt("video_channels");
        
                
        int tmpVal;
        
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_AUDIOENABLE, 1);
        ((CheckBox)findViewById(R.id.id_check_audio_enable)).setChecked(1 == tmpVal);
        
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOENABLE, 0);
        ((CheckBox)findViewById(R.id.id_check_video_enable)).setChecked(1 == tmpVal);
        
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_AUDIOREDUNDANCE, 0);
        ((CheckBox)findViewById(R.id.id_check_audioredundance)).setChecked(1 == tmpVal);
        
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEORELIABLE, 0);
        ((CheckBox)findViewById(R.id.id_check_videoreliable)).setChecked(1 == tmpVal);
        
        
    	av_video_size = (byte) AppSettings.GetSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOSIZE, SharedFuncLib.AV_VIDEO_SIZE_VGA);
    	switch (av_video_size)
    	{
    	case SharedFuncLib.AV_VIDEO_SIZE_QVGA:
    		((RadioGroup)findViewById(R.id.radiogroup_videosize)).check(R.id.radio_videosize_qvga);
    		break;
    	case SharedFuncLib.AV_VIDEO_SIZE_480x320:
    		((RadioGroup)findViewById(R.id.radiogroup_videosize)).check(R.id.radio_videosize_480x320);
    		break;
    	case SharedFuncLib.AV_VIDEO_SIZE_VGA:
    		((RadioGroup)findViewById(R.id.radiogroup_videosize)).check(R.id.radio_videosize_vga);
    		break;
    	default:
    		((RadioGroup)findViewById(R.id.radiogroup_videosize)).check(R.id.radio_videosize_vga);
    		break;
    	}
    	
    	
    	av_video_framerate = (byte) AppSettings.GetSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_FRAMERATE, 25);
    	switch (av_video_framerate)
    	{
    	case 20:
    		((RadioGroup)findViewById(R.id.radiogroup_framerate)).check(R.id.radio_framerate_a);
    		break;
    	case 25:
    		((RadioGroup)findViewById(R.id.radiogroup_framerate)).check(R.id.radio_framerate_b);
    		break;
    	case 30:
    		((RadioGroup)findViewById(R.id.radiogroup_framerate)).check(R.id.radio_framerate_c);
    		break;
    	default:
    		((RadioGroup)findViewById(R.id.radiogroup_framerate)).check(R.id.radio_framerate_b);
    		break;
    	}
    	
    	
    	av_audio_channel   = AppSettings.GetSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_AUDIODEVICE, 0);
    	av_video_channel   = AppSettings.GetSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEODEVICE, 0);
    	
    	if (av_audio_channel >= audio_channels) {
    		av_audio_channel = 0;
    	}
    	if (av_video_channel >= video_channels) {
    		av_video_channel = 0;
    	}
    	((TextView)findViewById(R.id.id_text_audiodevice)).setText(String.format(getResources().getString(R.string.ui_avparam_audio_device_format), av_audio_channel));
    	((TextView)findViewById(R.id.id_text_videodevice)).setText(String.format(getResources().getString(R.string.ui_avparam_video_device_format), av_video_channel));
    	
        
    	checkButtonState();        
        
        ((CheckBox)findViewById(R.id.id_check_audio_enable)).setOnCheckedChangeListener(  
        		new CompoundButton.OnCheckedChangeListener() {  
        			public void onCheckedChanged(CompoundButton buttonView,  boolean isChecked) 
        			{
        				checkButtonState();
        			}
        });
        
        ((CheckBox)findViewById(R.id.id_check_video_enable)).setOnCheckedChangeListener(  
        		new CompoundButton.OnCheckedChangeListener() {  
        			public void onCheckedChanged(CompoundButton buttonView,  boolean isChecked) 
        			{
        				checkButtonState();
        			}
        });
        
        
        findViewById(R.id.ok_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnOk();
        	}
        });
        
        findViewById(R.id.cancel_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnCancel();
        	}
        });
        
        findViewById(R.id.audio_device_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnAudioDevice();
        	}
        });
        
        findViewById(R.id.video_device_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnVideoDevice();
        	}
        });
    }
    
    
    static final int DIALOG_AUDIO_DEVICES = 1;
    static final int DIALOG_VIDEO_DEVICES = 2;
    
    @Override
    protected Dialog onCreateDialog(int id)
    {
		Builder builder = new AlertDialog.Builder(this);
    	Dialog dialog = null;
    	int i;
    	String[] strItems = null;
    	
    	switch (id)
    	{
    	case DIALOG_AUDIO_DEVICES:
    		final TextView textAudioDevice = (TextView)findViewById(R.id.id_text_audiodevice);
    		strItems = new String[audio_channels];
    		for(i = 0; i < audio_channels; i++)
    		{
    			strItems[i] = String.format(getResources().getString(R.string.ui_avparam_audio_device_format), i);
    		}
    		builder.setTitle(getResources().getString(R.string.ui_avparam_audio_device_title));
    		builder.setItems(strItems, new DialogInterface.OnClickListener(){
	    			public void onClick(DialogInterface dialog, int which)
	    			{
	    				if (which < audio_channels)
	    				{
	    					av_audio_channel = which;
	    					textAudioDevice.setText(String.format(getResources().getString(R.string.ui_avparam_audio_device_format), av_audio_channel));
	    				}
	    			}
    			});
    		dialog = builder.create();
    		break;
    		
    	case DIALOG_VIDEO_DEVICES:
    		final TextView textVideoDevice = (TextView)findViewById(R.id.id_text_videodevice);
    		strItems = new String[video_channels];
    		for(i = 0; i < video_channels; i++)
    		{
    			strItems[i] = String.format(getResources().getString(R.string.ui_avparam_video_device_format), i);
    		}
    		builder.setTitle(getResources().getString(R.string.ui_avparam_video_device_title));
    		builder.setItems(strItems, new DialogInterface.OnClickListener(){
	    			public void onClick(DialogInterface dialog, int which)
	    			{
	    				if (which < video_channels)
	    				{
	    					av_video_channel = which;
	    					textVideoDevice.setText(String.format(getResources().getString(R.string.ui_avparam_video_device_format), av_video_channel));
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
    
    private void onBtnAudioDevice()
    {
    	//this.dismissDialog(id);
		this.showDialog(DIALOG_AUDIO_DEVICES);
    }
	
    private void onBtnVideoDevice()
    {
    	//this.dismissDialog(id);
    	this.showDialog(DIALOG_VIDEO_DEVICES);
    }
    
    private void onBtnOk()
    {
    	int tmpVal;
    	
    	tmpVal = ((CheckBox)findViewById(R.id.id_check_audio_enable)).isChecked() ? 1 : 0;
    	AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_AUDIOENABLE, tmpVal);
    	
    	tmpVal = ((CheckBox)findViewById(R.id.id_check_video_enable)).isChecked() ? 1 : 0;
    	AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOENABLE, tmpVal);
    
    	tmpVal = ((CheckBox)findViewById(R.id.id_check_audioredundance)).isChecked() ? 1 : 0;
    	AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_AUDIOREDUNDANCE, tmpVal);
    
    	tmpVal = ((CheckBox)findViewById(R.id.id_check_videoreliable)).isChecked() ? 1 : 0;
    	AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEORELIABLE, tmpVal);
    
    
    	int tmpId;
    	
    	tmpId = ((RadioGroup)findViewById(R.id.radiogroup_videosize)).getCheckedRadioButtonId();
    	switch (tmpId)
    	{
    	case R.id.radio_videosize_qvga:
    		AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOSIZE, SharedFuncLib.AV_VIDEO_SIZE_QVGA);
    		break;
    	case R.id.radio_videosize_480x320:
    		AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOSIZE, SharedFuncLib.AV_VIDEO_SIZE_480x320);
    		break;
    	case R.id.radio_videosize_vga:
    		AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEOSIZE, SharedFuncLib.AV_VIDEO_SIZE_VGA);
    		break;
    	}
    	
    	
    	tmpId = ((RadioGroup)findViewById(R.id.radiogroup_framerate)).getCheckedRadioButtonId();
    	switch (tmpId)
    	{
    	case R.id.radio_framerate_a:
    		AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_FRAMERATE, 20);
    		break;
    	case R.id.radio_framerate_b:
    		AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_FRAMERATE, 25);
    		break;
    	case R.id.radio_framerate_c:
    		AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_FRAMERATE, 30);
    		break;
    	}
    	
    	
    	/* av_audio_channel, av_video_channel */
    	AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_AUDIODEVICE, av_audio_channel);
    	AppSettings.SaveSoftwareKeyDwordValue(this, "" + comments_id + AppSettings.STRING_REGKEY_NAME_CAM_AVPARAM_VIDEODEVICE, av_video_channel);
    	
    	
    	this.setResult(RESULT_OK);
		this.finish();
    }
    
    private void onBtnCancel()
    {
    	this.setResult(RESULT_CANCELED);
		this.finish();
    }
    
    @Override  
    public boolean onKeyDown(int keyCode, KeyEvent event) {   
           
        if(keyCode == KeyEvent.KEYCODE_BACK){   
        	onBtnCancel();
            return true;   
        }else{         
            return super.onKeyDown(keyCode, event);   
        }   
    }
    
    private void checkButtonState()
    {
    	boolean isChecked;
    	
    	if (audio_channels == 0)
    	{
    		((CheckBox)findViewById(R.id.id_check_audio_enable)).setChecked(false);
			((CheckBox)findViewById(R.id.id_check_audio_enable)).setEnabled(false);
			((ImageButton)findViewById(R.id.audio_device_btn)).setEnabled(false);
			((CheckBox)findViewById(R.id.id_check_audioredundance)).setEnabled(false);
    	}
    	
		if (video_channels == 0)
    	{
    		((CheckBox)findViewById(R.id.id_check_video_enable)).setChecked(false);
			((CheckBox)findViewById(R.id.id_check_video_enable)).setEnabled(false);
			((ImageButton)findViewById(R.id.video_device_btn)).setEnabled(false);
			((RadioGroup)findViewById(R.id.radiogroup_videosize)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_videosize_vga)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_videosize_qvga)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_videosize_480x320)).setEnabled(false);
			((RadioGroup)findViewById(R.id.radiogroup_framerate)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_framerate_a)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_framerate_b)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_framerate_c)).setEnabled(false);
			((CheckBox)findViewById(R.id.id_check_videoreliable)).setEnabled(false);
    	}
    	
    	isChecked = ((CheckBox)findViewById(R.id.id_check_audio_enable)).isChecked();
		if (isChecked)
		{
			((ImageButton)findViewById(R.id.audio_device_btn)).setEnabled(true);
			((CheckBox)findViewById(R.id.id_check_audioredundance)).setEnabled(true);
		}
		else {
			((ImageButton)findViewById(R.id.audio_device_btn)).setEnabled(false);
			((CheckBox)findViewById(R.id.id_check_audioredundance)).setEnabled(false);
		}
		
		isChecked = ((CheckBox)findViewById(R.id.id_check_video_enable)).isChecked();
		if (isChecked)
		{
			((ImageButton)findViewById(R.id.video_device_btn)).setEnabled(true);
			((RadioGroup)findViewById(R.id.radiogroup_videosize)).setEnabled(true);
			((RadioButton)findViewById(R.id.radio_videosize_vga)).setEnabled(true);
			((RadioButton)findViewById(R.id.radio_videosize_qvga)).setEnabled(true);
			((RadioButton)findViewById(R.id.radio_videosize_480x320)).setEnabled(true);
			((RadioGroup)findViewById(R.id.radiogroup_framerate)).setEnabled(true);
			((RadioButton)findViewById(R.id.radio_framerate_a)).setEnabled(true);
			((RadioButton)findViewById(R.id.radio_framerate_b)).setEnabled(true);
			((RadioButton)findViewById(R.id.radio_framerate_c)).setEnabled(true);
			((CheckBox)findViewById(R.id.id_check_videoreliable)).setEnabled(true);
		}
		else {
			((ImageButton)findViewById(R.id.video_device_btn)).setEnabled(false);
			((RadioGroup)findViewById(R.id.radiogroup_videosize)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_videosize_vga)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_videosize_qvga)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_videosize_480x320)).setEnabled(false);
			((RadioGroup)findViewById(R.id.radiogroup_framerate)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_framerate_a)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_framerate_b)).setEnabled(false);
			((RadioButton)findViewById(R.id.radio_framerate_c)).setEnabled(false);
			((CheckBox)findViewById(R.id.id_check_videoreliable)).setEnabled(false);
		}
	   	
        if (SharedFuncLib.SOCKET_TYPE_TCP == conn_type)
        {
        	((CheckBox)findViewById(R.id.id_check_audioredundance)).setChecked(false);
        	((CheckBox)findViewById(R.id.id_check_audioredundance)).setEnabled(false);
        	((CheckBox)findViewById(R.id.id_check_videoreliable)).setChecked(true);
        	((CheckBox)findViewById(R.id.id_check_videoreliable)).setEnabled(false);
        }
    }
}
