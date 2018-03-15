package com.wangling.remotephone;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.widget.EditText;
import com.wangling.remotephone.R;


public class AddCamActivity extends Activity {
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.addcam);
        
        findViewById(R.id.add_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnAdd();
        	}
        });
        
        findViewById(R.id.cancel_btn).setOnClickListener(new View.OnClickListener() {
        	public void onClick(View v) {
        		onBtnCancel();
        	}
        });
    }
    
    private void onBtnAdd()
    {
    	String ids = AppSettings.GetSoftwareKeyValue(this, AppSettings.STRING_REGKEY_NAME_QUERYIDS, "");
    	String ids2 = "-" + ids + "-";
    	
    	EditText editCamId = (EditText)findViewById(R.id.id_edit_mobcam_id);
    	String strCamId = editCamId.getText().toString();
    	EditText editPass = (EditText)findViewById(R.id.id_edit_password);
    	String strPass = editPass.getText().toString();
    	
    	if (strCamId == null || strCamId.equals(""))
    	{
    		SharedFuncLib.MyMessageBox(this, 
    				getResources().getString(R.string.app_name), 
    				getResources().getString(R.string.msg_mobcam_id_empty));
    		return;
    	}
    	
    	try {
    		int temp = Integer.parseInt(strCamId);
    		strCamId = String.format("%d", temp);
    	} catch (NumberFormatException e) {
    		SharedFuncLib.MyMessageBox(this, 
    				getResources().getString(R.string.app_name), 
    				getResources().getString(R.string.msg_mobcam_id_invalid));
			return;
		}
    	
    	if (ids2.contains("-" + strCamId + "-"))
    	{
    		SharedFuncLib.MyMessageBox(this, 
    				getResources().getString(R.string.app_name), 
    				getResources().getString(R.string.msg_mobcam_id_exists));
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
    	
    	if (strPass != null) {
    		AppSettings.SaveSoftwareKeyValue(this, strCamId + AppSettings.STRING_REGKEY_NAME_CAM_PASSWORD, strPass);
    	}
    	
    	
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
    
}
