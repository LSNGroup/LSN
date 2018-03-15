package com.wangling.remotephone;

import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

import android.content.Context;
import android.net.ConnectivityManager;


public class MobileDataUtility {

	private static void toggleMobileData1(Context context, boolean enabled)
	{
		ConnectivityManager connectivityManager =  (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
		Method setMobileDataEnabl;
		try {
			setMobileDataEnabl = connectivityManager.getClass().getDeclaredMethod("setMobileDataEnabled", boolean.class);
			setMobileDataEnabl.invoke(connectivityManager, enabled);
		} catch (Exception e) {
            e.printStackTrace();
        }
	}
	
	private static void toggleMobileData2(Context context, boolean enabled)
	{
		 ConnectivityManager conMgr = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
			
		 Class<?> conMgrClass = null; // ConnectivityManager��
		 Field iConMgrField = null; // ConnectivityManager���е��ֶ�
		 Object iConMgr = null; // IConnectivityManager�������
		 Class<?> iConMgrClass = null; // IConnectivityManager��
		 Method setMobileDataEnabledMethod = null; // setMobileDataEnabled����
		
		 try {
		  // ȡ��ConnectivityManager��
		  conMgrClass = Class.forName(conMgr.getClass().getName());
		  // ȡ��ConnectivityManager���еĶ���mService
		  iConMgrField = conMgrClass.getDeclaredField("mService");
		  // ����mService�ɷ���
		  iConMgrField.setAccessible(true);
		  // ȡ��mService��ʵ������IConnectivityManager
		  iConMgr = iConMgrField.get(conMgr);
		  // ȡ��IConnectivityManager��
		  iConMgrClass = Class.forName(iConMgr.getClass().getName());
		  // ȡ��IConnectivityManager���е�setMobileDataEnabled(boolean)����
		  setMobileDataEnabledMethod = iConMgrClass.getDeclaredMethod("setMobileDataEnabled", Boolean.TYPE);
		  // ����setMobileDataEnabled�����ɷ���
		  setMobileDataEnabledMethod.setAccessible(true);
		  // ����setMobileDataEnabled����
		  setMobileDataEnabledMethod.invoke(iConMgr, enabled);
		 } catch (Exception e) {
			 e.printStackTrace();
		 }
	}
	
	private static void toggleMobileData_root(Context context, boolean enabled)
	{
		if (enabled) {
			try {
				//Runtime.getRuntime().exec("su -c \"svc data enable\"");
				MobileCameraService.runNativeShellCmd("su -c \"svc data enable\"");
			} catch (Exception e) {}
		}
		else {
			try {
				//Runtime.getRuntime().exec("su -c \"svc data disable\"");
				MobileCameraService.runNativeShellCmd("su -c \"svc data disable\"");
			} catch (Exception e) {}
		}
	}
	
	public static void toggleMobileData(Context context, boolean enabled)
	{
		toggleMobileData_root(context, enabled);
		toggleMobileData1(context, enabled);
		toggleMobileData2(context, enabled);
	}
}
