package com.wangling.remotephone;

//回调接口
public interface VJRListener {
	void onLeftWheelChanged(int angle, int L);
	void onRightWheelChanged(int angle, int L);
	void onLeftButtonADown();
	void onLeftButtonAUp();
	void onLeftButtonBClick1();
	void onLeftButtonBClick2();
	void onRightButtonADown();
	void onRightButtonAUp();
	void onRightButtonBClick1();
	void onRightButtonBClick2();
	void onLeftTopButtonClick();
	void onRightTopButtonClick();
	void onLeftButtonCClick();
	void onRightButtonCClick();
}
