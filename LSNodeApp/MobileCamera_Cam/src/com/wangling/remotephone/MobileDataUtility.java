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
			
		 Class<?> conMgrClass = null; // ConnectivityManager类
		 Field iConMgrField = null; // ConnectivityManager类中的字段
		 Object iConMgr = null; // IConnectivityManager类的引用
		 Class<?> iConMgrClass = null; // IConnectivityManager类
		 Method setMobileDataEnabledMethod = null; // setMobileDataEnabled方法
		
		 try {
		  // 取得ConnectivityManager类
		  conMgrClass = Class.forName(conMgr.getClass().getName());
		  // 取得ConnectivityManager类中的对象mService
		  iConMgrField = conMgrClass.getDeclaredField("mService");
		  // 设置mService可访问
		  iConMgrField.setAccessible(true);
		  // 取得mService的实例化类IConnectivityManager
		  iConMgr = iConMgrField.get(conMgr);
		  // 取得IConnectivityManager类
		  iConMgrClass = Class.forName(iConMgr.getClass().getName());
		  // 取得IConnectivityManager类中的setMobileDataEnabled(boolean)方法
		  setMobileDataEnabledMethod = iConMgrClass.getDeclaredMethod("setMobileDataEnabled", Boolean.TYPE);
		  // 设置setMobileDataEnabled方法可访问
		  setMobileDataEnabledMethod.setAccessible(true);
		  // 调用setMobileDataEnabled方法
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
