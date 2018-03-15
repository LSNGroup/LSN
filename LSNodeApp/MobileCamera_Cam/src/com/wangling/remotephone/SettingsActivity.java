package com.wangling.remotephone;

import android.app.AlertDialog;
import android.app.TabActivity;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Message;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TabHost;
import android.widget.Toast;
import com.ysk.remoteaccess.R;


public class SettingsActivity extends TabActivity {

	private SettingsActivity _instance = null;
	private int comments_id = 0;
	private TabHost mTabHost;
	
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.settings);
        
        Bundle extras = getIntent().getExtras();
        comments_id = extras.getInt("comments_id");
        
        _instance = this;
        
        mTabHost = this.getTabHost();
        LayoutInflater.from(this).inflate(R.layout.settings, mTabHost.getTabContentView(), true);
        
        mTabHost.setBackgroundColor(Color.argb(255, 0x75, 0x75, 0x75));
        
        mTabHost.addTab(
        		mTabHost.newTabSpec("Tab-Basic")
        		.setIndicator(getResources().getString(R.string.ui_tab_basic), getResources().getDrawable(android.R.drawable.ic_menu_info_details))
        		.setContent(R.id.id_layout_tab_basic)
        		);
        
        mTabHost.addTab(
        		mTabHost.newTabSpec("Tab-Phone")
        		.setIndicator(getResources().getString(R.string.ui_tab_phone), getResources().getDrawable(android.R.drawable.ic_menu_call))
        		.setContent(R.id.id_layout_tab_phone)
        		);
        
        
        int tmpVal;
        Button btn = (Button)findViewById(R.id.id_btn_mobcam_id);
        if (null != btn) {
        	if (0 == comments_id) {
        		btn.setText(getResources().getString(R.string.ui_text_mobcam_id_0));
        	}
        	else {
        		btn.setText(String.format(getResources().getString(R.string.ui_text_mobcam_id_1), comments_id));
        	}
        	btn.setEnabled(false);
        }
        
        
        findViewById(R.id.id_checkbox_hide_ui).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		if (0 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ALLOW_HIDE_UI, 1))
            	{
	        		if (((CheckBox)findViewById(R.id.id_checkbox_hide_ui)).isChecked())
	        		{
	        			SharedFuncLib.MyMessageBox(_instance, 
	        					getResources().getString(R.string.app_name), 
	            				getResources().getString(R.string.msg_level_too_low_for_allow_hide));
	        			//((CheckBox)findViewById(R.id.id_checkbox_hide_ui)).setChecked(false);
	        		}
            	}
        	}
        });        
        
        
        /* Tab basic */
        ((EditText)findViewById(R.id.id_edit_mobcam_name)).setText(AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, ""));
        ((EditText)findViewById(R.id.id_edit_mobcam_password)).setText(AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_PASSWORD, ""));
           
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_AUTO_START, 1);
        ((CheckBox)findViewById(R.id.id_checkbox_auto_start)).setChecked(1 == tmpVal);
        
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0);
        ((CheckBox)findViewById(R.id.id_checkbox_hide_ui)).setChecked(1 == tmpVal);
        
        
        String []capMethodArray = new String[4];
        capMethodArray[0] = getResources().getString(R.string.ui_text_cap_method_auto);
        capMethodArray[1] = getResources().getString(R.string.ui_text_cap_method_adb);
        capMethodArray[2] = getResources().getString(R.string.ui_text_cap_method_flinger);
        capMethodArray[3] = getResources().getString(R.string.ui_text_cap_method_fb);
        
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, capMethodArray);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item); 
        
        Spinner spinnerCap = (Spinner)findViewById(R.id.id_spinner_cap_method);
        spinnerCap.setAdapter(adapter);
        spinnerCap.setSelection( AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_CAPMETHOD, 0) );
        
        
        /* Tab phone */
        tmpVal = AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ENABLE_EMAIL, 1);
        ((CheckBox)findViewById(R.id.id_checkbox_enable_email)).setChecked(1 == tmpVal);
        
        ((EditText)findViewById(R.id.id_edit_email_address)).setText(AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, ""));
        ((EditText)findViewById(R.id.id_edit_sms_phone_num)).setText(AppSettings.GetSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SMSPHONENUM, ""));
    }
	
    
    private void saveSettings()
    {
    	int tmpVal;
    	
    	/* Tab basic */
    	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_NODENAME, ((EditText)findViewById(R.id.id_edit_mobcam_name)).getText().toString());
    	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_PASSWORD, ((EditText)findViewById(R.id.id_edit_mobcam_password)).getText().toString());
    	
    	tmpVal = ((CheckBox)findViewById(R.id.id_checkbox_auto_start)).isChecked() ? 1 : 0;
    	AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_AUTO_START, tmpVal);
    	
    	tmpVal = ((CheckBox)findViewById(R.id.id_checkbox_hide_ui)).isChecked() ? 1 : 0;
    	AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, tmpVal);
    	
    	if (1 == AppSettings.GetSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_HIDE_UI, 0))
    	{
    		HomeActivity.DoHideUi(this);
    	}
        else {
        	HomeActivity.DoShowUi(this);
        }
    	
    	
    	Spinner spinnerCap = (Spinner)findViewById(R.id.id_spinner_cap_method);
        AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_CAPMETHOD, spinnerCap.getSelectedItemPosition() );
    	
        
    	/* Tab phone */
    	tmpVal = ((CheckBox)findViewById(R.id.id_checkbox_enable_email)).isChecked() ? 1 : 0;
    	AppSettings.SaveSoftwareKeyDwordValue(_instance, AppSettings.STRING_REGKEY_NAME_ENABLE_EMAIL, tmpVal);
    	
    	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_EMAILADDRESS, ((EditText)findViewById(R.id.id_edit_email_address)).getText().toString());
    	AppSettings.SaveSoftwareKeyValue(_instance, AppSettings.STRING_REGKEY_NAME_SMSPHONENUM, ((EditText)findViewById(R.id.id_edit_sms_phone_num)).getText().toString());
    }
        
    @Override  
    public boolean onKeyDown(int keyCode, KeyEvent event) {   
           
        if(keyCode == KeyEvent.KEYCODE_BACK){   

            AlertDialog.Builder builder = new AlertDialog.Builder(_instance);
            builder.setTitle(_instance.getResources().getString(R.string.app_name));
            builder.setMessage(_instance.getResources().getString(R.string.msg_save_settings_or_not));
            builder.setPositiveButton(_instance.getResources().getString(R.string.ui_yes_btn),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                            //setTitle("点击了对话框上的Button1");
                        	
                        	saveSettings();                        	
                        	dialog.dismiss();
                        	_instance.finish();
                        }
                    });
            builder.setNegativeButton(_instance.getResources().getString(R.string.ui_no_btn),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                            //setTitle("点击了对话框上的Button3");
                        	
                        	dialog.dismiss();
                        	_instance.finish();
                        }
                    });
            builder.show();
        	
            return true;   
        }else{         
            return super.onKeyDown(keyCode, event);
        }   
    }
}

