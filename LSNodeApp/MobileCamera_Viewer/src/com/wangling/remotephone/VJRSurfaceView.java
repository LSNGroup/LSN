package com.wangling.remotephone;

import java.io.IOException;

import android.app.LauncherActivity;
import android.app.Service;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.graphics.PorterDuff.Mode;
import android.graphics.Rect;
import android.graphics.RectF;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Vibrator;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import com.lsngroup.live.R;

public class VJRSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
	
	private SurfaceHolder mHolder = null;
	private int mWidth;
	private int mHeight;
	private SensorManager mSensorManager = null;
	private Sensor mSensorAcce = null;
	private OnSensorEventListener mOnSensorEventListener = new OnSensorEventListener();
	private int mSensorAcceValX = 0;
	private int mSensorAcceValY = 0;
	private int mSensorAcceValZ = 0;
	private Vibrator mVibrator = null;
	private Paint mPaint = null;
	private Point mLeftCtrlPoint = null;  //左摇杆中心位置
	private Point mRightCtrlPoint = null; //右摇杆中心位置
	private Point mLeftRockerPosition = null;  //左摇杆位置
	private Point mRightRockerPosition = null; //右摇杆位置
	private int mLeftPointerId;
	private int mRightPointerId;
	private boolean mShowLeftVJR = false;
	private boolean mShowRightVJR = false;
	private boolean mMoveLeftVJR = false;
	private boolean mMoveRightVJR = false;
	private boolean mLeftButtonADown = false;//左下角的按钮是否按下
	private boolean mRightButtonADown = false;//右下角的按钮是否按下
	private boolean mLeftButtonBFlag = false;
	private boolean mRightButtonBFlag = false;
	public static final int mMainColor = Color.argb(255, 240, 133, 25);
	public int mRoundButtonRadius = 26;//圆形按钮半径
	public int mRoundButtonDisdance = 30;//圆形按钮圆心到边缘的距离
	private int mRudderRadius = 15;//摇杆半径
	private int mWheelRadius = 58;//摇杆活动范围半径
	private static final int mMoveMinRadius = 5;
	private static final int mMoveMinTime = 200;
	private long mLastMoveTime = 0;
	private VJRListener listener = null; //事件回调接口
	private boolean mUseLeftThrottle = true;//美国手
	private int mLeftAlwaysShow = 0; //1：一直显示，-1：一直不显示
	private int mRightAlwaysShow = 0;//1：一直显示，-1：一直不显示
	private boolean mQuitPostVJRThread;
	
	private Bitmap bmp_back = null;
	private Bitmap bmp_operate = null;
	private Bitmap bmp_gsensor = null;
	private Bitmap bmp_gsensor2 = null;
	private Bitmap bmp_speaker = null;
	private Bitmap bmp_speaker2 = null;
	private Bitmap bmp_vjr_l1 = null;
	private Bitmap bmp_vjr_l2 = null;
	private Bitmap bmp_vjr_l2_2 = null;
	private Bitmap bmp_vjr_r1 = null;
	private Bitmap bmp_vjr_r2 = null;
	private Bitmap bmp_vjr_r2_2 = null;
	
	
	public VJRSurfaceView(Context context) {
		super(context);
		// TODO Auto-generated constructor stub
		do_init(context);
	}
	
	public VJRSurfaceView(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
		do_init(context);
	}
	
	public VJRSurfaceView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		// TODO Auto-generated constructor stub
		do_init(context);
	}
	
	private void do_init(Context context) {
		
		DisplayMetrics metric = new DisplayMetrics();
		((AvCtrlActivity)context).getWindowManager().getDefaultDisplay().getMetrics(metric);
		mRoundButtonRadius = (int)(mRoundButtonRadius * metric.density);
		mRoundButtonDisdance = (int)(mRoundButtonDisdance * metric.density);
		mRudderRadius = (int)(mRudderRadius * metric.density);
		mWheelRadius = (int)(mWheelRadius * metric.density);
		
		
		mHolder = getHolder();
		mHolder.addCallback(this);
		mHolder.setFormat(PixelFormat.TRANSPARENT);//设置背景透明
		mPaint = new Paint();
		mPaint.setColor(Color.GREEN);
		mPaint.setAntiAlias(true);//抗锯齿
		mPaint.setStyle(Style.STROKE);//绘制空心圆或 空心矩形
		
		mSensorManager = (SensorManager) getContext().getSystemService(Context.SENSOR_SERVICE);
        mSensorAcce = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		
		mVibrator = (Vibrator) getContext().getSystemService(Service.VIBRATOR_SERVICE); 
		
		mLeftPointerId = -1;
		mRightPointerId = -1;
		
		mShowLeftVJR = false;
		mShowRightVJR = false;
		mMoveLeftVJR = false;
		mMoveRightVJR = false;
		
		mLeftButtonADown = false;
		mRightButtonADown = false;
		
		mLeftButtonBFlag = false;
		mRightButtonBFlag = false;
		
		mLastMoveTime = 0;
		
		this.setKeepScreenOn(true);
		this.setFocusable(true);
		this.setFocusableInTouchMode(true);
		this.setZOrderOnTop(true);
		
		bmp_back = BitmapFactory.decodeResource(getResources(), R.drawable.ui_back);
		bmp_operate = BitmapFactory.decodeResource(getResources(), R.drawable.ui_operate);
		bmp_gsensor = BitmapFactory.decodeResource(getResources(), R.drawable.ui_gsensor);
		bmp_gsensor2 = BitmapFactory.decodeResource(getResources(), R.drawable.ui_gsensor2);
		bmp_speaker = BitmapFactory.decodeResource(getResources(), R.drawable.ui_speaker);
		bmp_speaker2 = BitmapFactory.decodeResource(getResources(), R.drawable.ui_speaker2);
		bmp_vjr_l1 = BitmapFactory.decodeResource(getResources(), R.drawable.vjr_l1);
		bmp_vjr_l2 = BitmapFactory.decodeResource(getResources(), R.drawable.vjr_l2);
		bmp_vjr_l2_2 = BitmapFactory.decodeResource(getResources(), R.drawable.vjr_l2_2);
		bmp_vjr_r1 = BitmapFactory.decodeResource(getResources(), R.drawable.vjr_r1);
		bmp_vjr_r2 = BitmapFactory.decodeResource(getResources(), R.drawable.vjr_r2);
		bmp_vjr_r2_2 = BitmapFactory.decodeResource(getResources(), R.drawable.vjr_r2_2);
		
		mQuitPostVJRThread = false;
		new Thread(new Runnable() {
			public void run()
			{
				while (!mQuitPostVJRThread)
				{
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {}
					
					
					int len;
					int L;
					
					if (mLeftCtrlPoint != null && mLeftRockerPosition != null)
					{
						len = getLength(mLeftCtrlPoint.x, mLeftCtrlPoint.y, mLeftRockerPosition.x, mLeftRockerPosition.y);
						if(len <= mWheelRadius) {//如果手指在摇杆活动范围内，则摇杆处于手指触摸位置
							L = len * 65535 / mWheelRadius;
						}
						else {//设置摇杆位置，使其处于手指触摸方向的摇杆活动范围边缘
							L = 65535;
						}
						if(len > mMoveMinRadius && listener != null) {
							float radian = getRadian(mLeftCtrlPoint, mLeftRockerPosition);
							listener.onLeftWheelChanged(getAngleCouvert(radian), L);
						}
					}
						
					if (mRightCtrlPoint != null && mRightRockerPosition != null)
					{
						len = getLength(mRightCtrlPoint.x, mRightCtrlPoint.y, mRightRockerPosition.x, mRightRockerPosition.y);
						if(len <= mWheelRadius) {//如果手指在摇杆活动范围内，则摇杆处于手指触摸位置
							L = len * 65535 / mWheelRadius;
						}
						else {//设置摇杆位置，使其处于手指触摸方向的摇杆活动范围边缘
							L = 65535;
						}
						if(len > mMoveMinRadius && listener != null) {
							float radian = getRadian(mRightCtrlPoint, mRightRockerPosition);
							listener.onRightWheelChanged(getAngleCouvert(radian), L);
						}
					}
				}//while
			}
		}).start();
	}

	public void do_uninit()
	{
		mQuitPostVJRThread = true;
		
		try {
			bmp_back.recycle();
			bmp_operate.recycle();
			bmp_gsensor.recycle();
			bmp_gsensor2.recycle();
			bmp_speaker.recycle();
			bmp_speaker2.recycle();
			bmp_vjr_l1.recycle();
			bmp_vjr_l2.recycle();
			bmp_vjr_l2_2.recycle();
			bmp_vjr_r1.recycle();
			bmp_vjr_r2.recycle();
			bmp_vjr_r2_2.recycle();
			
			if (mSensorManager != null) {
				mSensorManager.unregisterListener(mOnSensorEventListener);
			}
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	//设置回调接口
	public void setVJRListener(VJRListener vjrListener) {
		listener = vjrListener;
	}
	
	//设置油门操作方式
	public void setUseLeftThrottle(boolean bUseLeftThrottle) {
		mUseLeftThrottle = bUseLeftThrottle;
	}
	
	//设置初始油门值
	public void setInitThrottle(int rc3)
	{
		float n = (1500 - rc3)*mWheelRadius/(550*0.9f);
		if (n > mWheelRadius) {
			n = mWheelRadius;
		}
		
		if (mUseLeftThrottle)
		{
			mLeftRockerPosition.set(mLeftCtrlPoint.x, (int)n + mLeftCtrlPoint.y);
		}
		else
		{
			mRightRockerPosition.set(mRightCtrlPoint.x, (int)n + mRightCtrlPoint.y);
		}
		DrawCanvas();
	}
	
	//设置是否一直显示左摇杆
	public void setLeftAlwaysShow(int nLeftAlwaysShow) {
		mLeftAlwaysShow = nLeftAlwaysShow;
		mLeftCtrlPoint = new Point(mWheelRadius*3, mHeight - mWheelRadius*2);
		mLeftRockerPosition = new Point(mLeftCtrlPoint.x, mLeftCtrlPoint.y);
		if (1 == mLeftAlwaysShow)
		{
			mShowLeftVJR = true;
			DrawCanvas();
		}
		if (-1 == mLeftAlwaysShow)
		{
			mShowLeftVJR = false;
			DrawCanvas();
		}
	}
	
	//设置是否一直显示右摇杆
	public void setRightAlwaysShow(int nRightAlwaysShow) {
		mRightAlwaysShow = nRightAlwaysShow;
		mRightCtrlPoint = new Point(mWidth - mWheelRadius*3, mHeight - mWheelRadius*2);
		mRightRockerPosition = new Point(mRightCtrlPoint.x, mRightCtrlPoint.y);
		if (1 == mRightAlwaysShow)
		{
			mShowRightVJR = true;
			DrawCanvas();
		}
		if (-1 == mRightAlwaysShow)
		{
			mShowRightVJR = false;
			DrawCanvas();
		}
	}
	
	//振动
	private void callVibrator() {
		if (mVibrator != null) {
			mVibrator.vibrate(30);
		}
	}
	
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
		// TODO Auto-generated method stub
		mWidth = width;
		mHeight = height;
		DrawCanvas();
	}
	
	public void surfaceCreated(SurfaceHolder holder) {
		// TODO Auto-generated method stub
		mWidth = this.getWidth();
		mHeight = this.getHeight();
		DrawCanvas();
	}
	
	public void surfaceDestroyed(SurfaceHolder holder) {
		// TODO Auto-generated method stub
		if (mSensorManager != null) {
			mSensorManager.unregisterListener(mOnSensorEventListener);
		}
		if (mVibrator != null) {
			mVibrator.cancel();
		}
	}
	
	private class OnSensorEventListener implements SensorEventListener
	{
		private long  lLastTime = 0;
		
		public void onAccuracyChanged(Sensor sensor, int accuracy) {
			// TODO Auto-generated method stub
			
		}

		//////////
		public void onSensorChanged(SensorEvent event) 
		{
			long curTime = System.currentTimeMillis();
			if ((curTime - lLastTime) < mMoveMinTime) {
				return;
			}
			else {
				lLastTime = curTime;
			}
			
			if (Sensor.TYPE_ACCELEROMETER == event.sensor.getType())
			{				
		        int y = (int)(event.values[SensorManager.DATA_X] * 10);////X,Y互换
		        int x = (int)(event.values[SensorManager.DATA_Y] * 10);////X,Y互换
		        int z = (int)(event.values[SensorManager.DATA_Z] * 10);
		        
		        if (mSensorAcceValX == 0 && mSensorAcceValY == 0 && mSensorAcceValZ == 0)
		        {
			        mSensorAcceValX = x;
			        mSensorAcceValY = y;
			        mSensorAcceValZ = z;
		        }
		        else {
		        	int len = getLength(mSensorAcceValX, mSensorAcceValY, x, y);
		    		if(len > mMoveMinRadius && listener != null) {
		    			float radian = getRadian(new Point(mSensorAcceValX, mSensorAcceValY), new Point(x, y));
		    			int L = len * 65535 / (98 / 2);
		    			if (L > 65535) L = 65535;
		    			if (false == mUseLeftThrottle) {
		    				listener.onLeftWheelChanged(getAngleCouvert(radian), L);
		    			}
		    			else {
		    				listener.onRightWheelChanged(getAngleCouvert(radian), L);
		    			}
		    			Log.d("Acce Debug", "getAngleCouvert=" + getAngleCouvert(radian));////Debug
		    		}
		        }
			}
			
		}/* onSensorChanged */
	}
	
    //获取两点间直线距离
    public int getLength(float x1, float y1, float x2, float y2) {
        return (int)Math.sqrt(Math.pow(x1-x2, 2) + Math.pow(y1-y2, 2));
    }
    
    /**
     * 获取线段上某个点的坐标，长度为a.x - cutRadius
     * @param a 点A
     * @param b 点B
     * @param cutRadius 截断距离
     * @return 截断点
     */
    public Point getBorderPoint(Point a, Point b, int cutRadius) {
        float radian = getRadian(a, b);
        return new Point(a.x + (int)(cutRadius * Math.cos(radian)), a.y + (int)(cutRadius * Math.sin(radian)));
    }
    
    //获取水平线夹角弧度
    public float getRadian(Point a, Point b) {
        float lenA = b.x-a.x;
        float lenB = b.y-a.y;
        float lenC = (float)Math.sqrt(lenA*lenA+lenB*lenB);
        float ang = (float)Math.acos(lenA/lenC);
        ang = ang * (b.y < a.y ? -1 : 1);
        //Log.d("VJR Debug", "getAngle()=" + getAngleCouvert(ang));////Debug
        return ang;
    }
    
    //获取摇杆偏移角度 0-360°
    private int getAngleCouvert(float radian) {
        int tmp = (int)Math.round(radian/Math.PI*180);
        if(tmp < 0) {
            return -tmp;
        }else{
            return 180 + (180 - tmp);
        }
    }
	
    //判断点是否在圆内
    private boolean isPointInCircle(Point pt, Point c, int radius)
	{
		if (getLength(pt.x, pt.y, c.x, c.y) < radius) {
			return true;
		}
		else {
			return false;
		}
	}
    
	private void DrawCanvas() {
		Canvas canvas = null;
		try {
			canvas = mHolder.lockCanvas();
			canvas.drawColor(Color.TRANSPARENT, Mode.CLEAR);//清除屏幕
			
			//画左边VJR
			if (mShowLeftVJR)
			{
				mPaint.setColor(mMainColor);
				mPaint.setAlpha(80);
				mPaint.setStyle(Style.FILL);
				canvas.drawCircle(mLeftCtrlPoint.x, mLeftCtrlPoint.y, mWheelRadius, mPaint);
				mPaint.setStyle(Style.STROKE);
				
				final int st_len = mWheelRadius/6;
				mPaint.setColor(Color.GREEN);
				mPaint.setAlpha(100);
				mPaint.setStrokeWidth(2);
				
				//上箭头
				canvas.drawLine(mLeftCtrlPoint.x, mLeftCtrlPoint.y - mWheelRadius + st_len, mLeftCtrlPoint.x + st_len, mLeftCtrlPoint.y - mWheelRadius + 2*st_len, mPaint);
				canvas.drawLine(mLeftCtrlPoint.x, mLeftCtrlPoint.y - mWheelRadius + st_len, mLeftCtrlPoint.x - st_len, mLeftCtrlPoint.y - mWheelRadius + 2*st_len, mPaint);
				canvas.drawLine(mLeftCtrlPoint.x + st_len, mLeftCtrlPoint.y - mWheelRadius + 2*st_len, mLeftCtrlPoint.x - st_len, mLeftCtrlPoint.y - mWheelRadius + 2*st_len, mPaint);
				
				//下箭头
				canvas.drawLine(mLeftCtrlPoint.x, mLeftCtrlPoint.y + mWheelRadius - st_len, mLeftCtrlPoint.x + st_len, mLeftCtrlPoint.y + mWheelRadius - 2*st_len, mPaint);
				canvas.drawLine(mLeftCtrlPoint.x, mLeftCtrlPoint.y + mWheelRadius - st_len, mLeftCtrlPoint.x - st_len, mLeftCtrlPoint.y + mWheelRadius - 2*st_len, mPaint);
				canvas.drawLine(mLeftCtrlPoint.x + st_len, mLeftCtrlPoint.y + mWheelRadius - 2*st_len, mLeftCtrlPoint.x - st_len, mLeftCtrlPoint.y + mWheelRadius - 2*st_len, mPaint);
				
				//左箭头
				canvas.drawLine(mLeftCtrlPoint.x - mWheelRadius + st_len, mLeftCtrlPoint.y, mLeftCtrlPoint.x - mWheelRadius + 2*st_len, mLeftCtrlPoint.y - st_len, mPaint);
				canvas.drawLine(mLeftCtrlPoint.x - mWheelRadius + st_len, mLeftCtrlPoint.y, mLeftCtrlPoint.x - mWheelRadius + 2*st_len, mLeftCtrlPoint.y + st_len, mPaint);
				canvas.drawLine(mLeftCtrlPoint.x - mWheelRadius + 2*st_len, mLeftCtrlPoint.y - st_len, mLeftCtrlPoint.x - mWheelRadius + 2*st_len, mLeftCtrlPoint.y + st_len, mPaint);
				
				//右箭头
				canvas.drawLine(mLeftCtrlPoint.x + mWheelRadius - st_len, mLeftCtrlPoint.y, mLeftCtrlPoint.x + mWheelRadius - 2*st_len, mLeftCtrlPoint.y + st_len, mPaint);
				canvas.drawLine(mLeftCtrlPoint.x + mWheelRadius - st_len, mLeftCtrlPoint.y, mLeftCtrlPoint.x + mWheelRadius - 2*st_len, mLeftCtrlPoint.y - st_len, mPaint);
				canvas.drawLine(mLeftCtrlPoint.x + mWheelRadius - 2*st_len, mLeftCtrlPoint.y + st_len, mLeftCtrlPoint.x + mWheelRadius - 2*st_len, mLeftCtrlPoint.y - st_len, mPaint);
				
				mPaint.setColor(mMainColor);
				mPaint.setAlpha(100);
				mPaint.setStrokeWidth(1);
				canvas.drawCircle(mLeftCtrlPoint.x, mLeftCtrlPoint.y, mWheelRadius, mPaint);
				
				//油门指示
				if (true == mUseLeftThrottle)
				{
					mPaint.setColor(Color.GREEN);
					mPaint.setAlpha(100);
					mPaint.setStrokeWidth(2);
					canvas.drawRect(mLeftCtrlPoint.x - 5, mLeftCtrlPoint.y - (mWheelRadius - 30), mLeftCtrlPoint.x + 5, mLeftCtrlPoint.y + (mWheelRadius - 30), mPaint);
				}
				
				mPaint.setColor(Color.GREEN);
				mPaint.setAlpha(100);
				mPaint.setStrokeWidth(27);
				canvas.drawCircle(mLeftRockerPosition.x, mLeftRockerPosition.y, mRudderRadius, mPaint);
			}
			
			//画右边VJR
			if (mShowRightVJR)
			{
				mPaint.setColor(mMainColor);
				mPaint.setAlpha(80);
				mPaint.setStyle(Style.FILL);
				canvas.drawCircle(mRightCtrlPoint.x, mRightCtrlPoint.y, mWheelRadius, mPaint);
				mPaint.setStyle(Style.STROKE);
				
				final int st_len = mWheelRadius/6;
				mPaint.setColor(Color.GREEN);
				mPaint.setAlpha(100);
				mPaint.setStrokeWidth(2);
				
				//上箭头
				canvas.drawLine(mRightCtrlPoint.x, mRightCtrlPoint.y - mWheelRadius + st_len, mRightCtrlPoint.x + st_len, mRightCtrlPoint.y - mWheelRadius + 2*st_len, mPaint);
				canvas.drawLine(mRightCtrlPoint.x, mRightCtrlPoint.y - mWheelRadius + st_len, mRightCtrlPoint.x - st_len, mRightCtrlPoint.y - mWheelRadius + 2*st_len, mPaint);
				canvas.drawLine(mRightCtrlPoint.x + st_len, mRightCtrlPoint.y - mWheelRadius + 2*st_len, mRightCtrlPoint.x - st_len, mRightCtrlPoint.y - mWheelRadius + 2*st_len, mPaint);
				
				//下箭头
				canvas.drawLine(mRightCtrlPoint.x, mRightCtrlPoint.y + mWheelRadius - st_len, mRightCtrlPoint.x + st_len, mRightCtrlPoint.y + mWheelRadius - 2*st_len, mPaint);
				canvas.drawLine(mRightCtrlPoint.x, mRightCtrlPoint.y + mWheelRadius - st_len, mRightCtrlPoint.x - st_len, mRightCtrlPoint.y + mWheelRadius - 2*st_len, mPaint);
				canvas.drawLine(mRightCtrlPoint.x + st_len, mRightCtrlPoint.y + mWheelRadius - 2*st_len, mRightCtrlPoint.x - st_len, mRightCtrlPoint.y + mWheelRadius - 2*st_len, mPaint);
				
				//左箭头
				canvas.drawLine(mRightCtrlPoint.x - mWheelRadius + st_len, mRightCtrlPoint.y, mRightCtrlPoint.x - mWheelRadius + 2*st_len, mRightCtrlPoint.y - st_len, mPaint);
				canvas.drawLine(mRightCtrlPoint.x - mWheelRadius + st_len, mRightCtrlPoint.y, mRightCtrlPoint.x - mWheelRadius + 2*st_len, mRightCtrlPoint.y + st_len, mPaint);
				canvas.drawLine(mRightCtrlPoint.x - mWheelRadius + 2*st_len, mRightCtrlPoint.y - st_len, mRightCtrlPoint.x - mWheelRadius + 2*st_len, mRightCtrlPoint.y + st_len, mPaint);
				
				//右箭头
				canvas.drawLine(mRightCtrlPoint.x + mWheelRadius - st_len, mRightCtrlPoint.y, mRightCtrlPoint.x + mWheelRadius - 2*st_len, mRightCtrlPoint.y + st_len, mPaint);
				canvas.drawLine(mRightCtrlPoint.x + mWheelRadius - st_len, mRightCtrlPoint.y, mRightCtrlPoint.x + mWheelRadius - 2*st_len, mRightCtrlPoint.y - st_len, mPaint);
				canvas.drawLine(mRightCtrlPoint.x + mWheelRadius - 2*st_len, mRightCtrlPoint.y + st_len, mRightCtrlPoint.x + mWheelRadius - 2*st_len, mRightCtrlPoint.y - st_len, mPaint);
				
				mPaint.setColor(mMainColor);
				mPaint.setAlpha(100);
				mPaint.setStrokeWidth(1);
				canvas.drawCircle(mRightCtrlPoint.x, mRightCtrlPoint.y, mWheelRadius, mPaint);
				
				//油门指示
				if (false == mUseLeftThrottle)
				{
					mPaint.setColor(Color.GREEN);
					mPaint.setAlpha(100);
					mPaint.setStrokeWidth(2);
					canvas.drawRect(mRightCtrlPoint.x - 5, mRightCtrlPoint.y - (mWheelRadius - 30), mRightCtrlPoint.x + 5, mRightCtrlPoint.y + (mWheelRadius - 30), mPaint);
				}
				
				mPaint.setColor(Color.GREEN);
				mPaint.setAlpha(100);
				mPaint.setStrokeWidth(27);
				canvas.drawCircle(mRightRockerPosition.x, mRightRockerPosition.y, mRudderRadius, mPaint);
			}
			
			//画左下角按钮 mLeftButtonA
			mPaint.setColor(mMainColor);
			mPaint.setAlpha(100);
			mPaint.setStrokeWidth(2);
			canvas.drawCircle(mRoundButtonDisdance, mHeight - mRoundButtonDisdance, mRoundButtonRadius, mPaint);
			mPaint.setAlpha(200);
			if (false == mUseLeftThrottle)
			{
				canvas.drawBitmap(bmp_gsensor, 
						null, 
						new Rect(mRoundButtonDisdance - 35, mHeight - mRoundButtonDisdance - 35, mRoundButtonDisdance + 35, mHeight - mRoundButtonDisdance + 35), 
						mPaint);
			}
			else {
				canvas.drawBitmap(bmp_speaker, 
						null, 
						new Rect(mRoundButtonDisdance - 35, mHeight - mRoundButtonDisdance - 35, mRoundButtonDisdance + 35, mHeight - mRoundButtonDisdance + 35), 
						mPaint);
			}
			if (mLeftButtonADown) {
				mPaint.setColor(Color.GREEN);
				mPaint.setAlpha(100);
				mPaint.setStrokeWidth(15);
				canvas.drawCircle(mRoundButtonDisdance, mHeight - mRoundButtonDisdance, mRoundButtonRadius - 6, mPaint);
				if (false == mUseLeftThrottle)
				{
					canvas.drawBitmap(bmp_gsensor2, 
							null, 
							new Rect(mWidth/2 - 80, mHeight - 160, mWidth/2 + 80, mHeight), 
							mPaint);
				}
				else {
					canvas.drawBitmap(bmp_speaker2, 
							null, 
							new Rect(mWidth/2 - 80, mHeight - 160, mWidth/2 + 80, mHeight), 
							mPaint);
				}
			}
			else {
				mPaint.setColor(Color.GREEN);
				mPaint.setAlpha(100);
				mPaint.setStrokeWidth(2);
				canvas.drawCircle(mRoundButtonDisdance, mHeight - mRoundButtonDisdance, mRoundButtonRadius - 6, mPaint);
			}
			
			//画右下角按钮 mRightButtonA
			mPaint.setColor(mMainColor);
			mPaint.setAlpha(100);
			mPaint.setStrokeWidth(2);
			canvas.drawCircle(mWidth - mRoundButtonDisdance, mHeight - mRoundButtonDisdance, mRoundButtonRadius, mPaint);
			mPaint.setAlpha(200);
			if (false == mUseLeftThrottle)
			{
				canvas.drawBitmap(bmp_speaker, 
						null, 
						new Rect(mWidth - mRoundButtonDisdance - 35, mHeight - mRoundButtonDisdance - 35, mWidth - mRoundButtonDisdance + 35, mHeight - mRoundButtonDisdance + 35), 
						mPaint);
			}
			else {
				canvas.drawBitmap(bmp_gsensor, 
						null, 
						new Rect(mWidth - mRoundButtonDisdance - 35, mHeight - mRoundButtonDisdance - 35, mWidth - mRoundButtonDisdance + 35, mHeight - mRoundButtonDisdance + 35), 
						mPaint);
			}
			if (mRightButtonADown) {
				mPaint.setColor(Color.GREEN);
				mPaint.setAlpha(100);
				mPaint.setStrokeWidth(15);
				canvas.drawCircle(mWidth - mRoundButtonDisdance, mHeight - mRoundButtonDisdance, mRoundButtonRadius - 6, mPaint);
				if (false == mUseLeftThrottle)
				{
					canvas.drawBitmap(bmp_speaker2, 
							null, 
							new Rect(mWidth/2 - 80, mHeight - 160, mWidth/2 + 80, mHeight), 
							mPaint);
				}
				else {
					canvas.drawBitmap(bmp_gsensor2, 
							null, 
							new Rect(mWidth/2 - 80, mHeight - 160, mWidth/2 + 80, mHeight), 
							mPaint);
				}
			}
			else {
				mPaint.setColor(Color.GREEN);
				mPaint.setAlpha(100);
				mPaint.setStrokeWidth(2);
				canvas.drawCircle(mWidth - mRoundButtonDisdance, mHeight - mRoundButtonDisdance, mRoundButtonRadius - 6, mPaint);
			}

			//画左上角方形按钮
			mPaint.setColor(mMainColor);
			mPaint.setAlpha(100);
			mPaint.setStrokeWidth(2);
			canvas.drawRect(mRoundButtonDisdance/2, mRoundButtonDisdance/2, mRoundButtonDisdance/2 + mRoundButtonRadius*3/2, mRoundButtonDisdance/2 + mRoundButtonRadius*3/2, mPaint);
			//mPaint.setAlpha(200);
			canvas.drawBitmap(bmp_back, 
					null, 
					new Rect(mRoundButtonDisdance/2 + 4, mRoundButtonDisdance/2 + 4, mRoundButtonDisdance/2 + mRoundButtonRadius*3/2 - 4, mRoundButtonDisdance/2 + mRoundButtonRadius*3/2 - 4), 
					mPaint);
			
			//画右上角方形按钮
			mPaint.setColor(mMainColor);
			mPaint.setAlpha(100);
			mPaint.setStrokeWidth(2);
			canvas.drawRect(mWidth - mRoundButtonDisdance/2 - mRoundButtonRadius*3/2, mRoundButtonDisdance/2, mWidth - mRoundButtonDisdance/2, mRoundButtonDisdance/2 + mRoundButtonRadius*3/2, mPaint);
			//mPaint.setAlpha(200);
			canvas.drawBitmap(bmp_operate, 
					null, 
					new Rect(mWidth - mRoundButtonDisdance/2 - mRoundButtonRadius*3/2 + 4, mRoundButtonDisdance/2 + 4, mWidth - mRoundButtonDisdance/2 - 4, mRoundButtonDisdance/2 + mRoundButtonRadius*3/2 - 4), 
					mPaint);
			
			//画左边中上按钮，两状态
			mPaint.setColor(mMainColor);
			mPaint.setAlpha(100);
			mPaint.setStrokeWidth(2);
			canvas.drawCircle(mRoundButtonDisdance, mHeight-7*mRoundButtonDisdance, mRoundButtonRadius, mPaint);
			mPaint.setAlpha(200);
			Bitmap bmp_power = null;
			if (false == mLeftButtonBFlag) {
				bmp_power = bmp_vjr_l2;
			}
			else {
				bmp_power = bmp_vjr_l2_2;
			}
			canvas.drawBitmap(bmp_power, 
					null, 
					new Rect(mRoundButtonDisdance - 35, mHeight-7*mRoundButtonDisdance - 35, mRoundButtonDisdance + 35, mHeight-7*mRoundButtonDisdance + 35), 
					mPaint);
			
			//画左边中下按钮
			mPaint.setColor(mMainColor);
			mPaint.setAlpha(100);
			mPaint.setStrokeWidth(2);
			canvas.drawCircle(mRoundButtonDisdance, mHeight-4*mRoundButtonDisdance, mRoundButtonRadius, mPaint);
			mPaint.setAlpha(200);
			canvas.drawBitmap(bmp_vjr_l1, 
					null, 
					new Rect(mRoundButtonDisdance - 35, mHeight-4*mRoundButtonDisdance - 35, mRoundButtonDisdance + 35, mHeight-4*mRoundButtonDisdance + 35), 
					mPaint);
			
			//画右边中上按钮，两状态
			mPaint.setColor(mMainColor);
			mPaint.setAlpha(100);
			mPaint.setStrokeWidth(2);
			canvas.drawCircle(mWidth - mRoundButtonDisdance, mHeight-7*mRoundButtonDisdance, mRoundButtonRadius, mPaint);
			mPaint.setAlpha(200);
			Bitmap bmp_light = null;
			if (false == mRightButtonBFlag) {
				bmp_light = bmp_vjr_r2;
			}
			else {
				bmp_light = bmp_vjr_r2_2;
			}
			canvas.drawBitmap(bmp_light, 
					null, 
					new Rect(mWidth - mRoundButtonDisdance - 35, mHeight-7*mRoundButtonDisdance - 35, mWidth - mRoundButtonDisdance + 35, mHeight-7*mRoundButtonDisdance + 35), 
					mPaint);
			
			//画右边中下按钮
			mPaint.setColor(mMainColor);
			mPaint.setAlpha(100);
			mPaint.setStrokeWidth(2);
			canvas.drawCircle(mWidth - mRoundButtonDisdance, mHeight-4*mRoundButtonDisdance, mRoundButtonRadius, mPaint);
			mPaint.setAlpha(200);
			canvas.drawBitmap(bmp_vjr_r1, 
					null, 
					new Rect(mWidth - mRoundButtonDisdance - 35, mHeight-4*mRoundButtonDisdance - 35, mWidth - mRoundButtonDisdance + 35, mHeight-4*mRoundButtonDisdance + 35), 
					mPaint);
		}
		catch (Exception e) {
			e.printStackTrace();
		}
		finally {
			if(canvas != null) {
				mHolder.unlockCanvasAndPost(canvas);
			}
		}
	}
	
	private boolean buttonClickDownCheck(int x, int y)
	{
		//onLeftButtonADown
		if (isPointInCircle(new Point(x, y), new Point(mRoundButtonDisdance, mHeight - mRoundButtonDisdance), mRoundButtonRadius))
		{
			callVibrator();
			if (!mLeftButtonADown) {
				mLeftButtonADown = true;
				DrawCanvas();
				if(listener != null) {
					listener.onLeftButtonADown();
				}
				if (false == mUseLeftThrottle)
				{
					if (null != mSensorManager && null != mSensorAcce) {
						mSensorAcceValX = 0;
						mSensorAcceValY = 0;
						mSensorAcceValZ = 0;
			        	mSensorManager.registerListener(mOnSensorEventListener, mSensorAcce, SensorManager.SENSOR_DELAY_GAME);
			        }
				}
			}
			return true;
		}
		//onRightButtonADown
		else if (isPointInCircle(new Point(x, y), new Point(mWidth - mRoundButtonDisdance, mHeight - mRoundButtonDisdance), mRoundButtonRadius))
		{
			callVibrator();
			if (!mRightButtonADown) {
				mRightButtonADown = true;
				DrawCanvas();
				if(listener != null) {
					listener.onRightButtonADown();
				}
				if (true == mUseLeftThrottle)
				{
					if (null != mSensorManager && null != mSensorAcce) {
						mSensorAcceValX = 0;
						mSensorAcceValY = 0;
						mSensorAcceValZ = 0;
			        	mSensorManager.registerListener(mOnSensorEventListener, mSensorAcce, SensorManager.SENSOR_DELAY_GAME);
			        }
				}
			}
			return true;
		}
		//onLeftTopButtonClick
		else if (isPointInCircle(new Point(x, y), new Point(mRoundButtonDisdance, mRoundButtonDisdance), mRoundButtonRadius))
		{
			callVibrator();
			if(listener != null) {
				listener.onLeftTopButtonClick();
			}
			return true;
		}
		//onRightTopButtonClick
		else if (isPointInCircle(new Point(x, y), new Point(mWidth - mRoundButtonDisdance, mRoundButtonDisdance), mRoundButtonRadius))
		{
			callVibrator();
			if(listener != null) {
				listener.onRightTopButtonClick();
			}
			return true;
		}
		//onLeftButtonCClick
		else if (isPointInCircle(new Point(x, y), new Point(mRoundButtonDisdance, mHeight-4*mRoundButtonDisdance), mRoundButtonRadius))
		{
			callVibrator();
			if(listener != null) {
				listener.onLeftButtonCClick();
			}
			return true;
		}
		//onRightButtonCClick
		else if (isPointInCircle(new Point(x, y), new Point(mWidth - mRoundButtonDisdance, mHeight-4*mRoundButtonDisdance), mRoundButtonRadius))
		{
			callVibrator();
			if(listener != null) {
				listener.onRightButtonCClick();
			}
			return true;
		}
		//LeftButtonB
		else if (isPointInCircle(new Point(x, y), new Point(mRoundButtonDisdance, mHeight-7*mRoundButtonDisdance), mRoundButtonRadius))
		{
			callVibrator();
			if (false == mLeftButtonBFlag) {
				if(listener != null) {
					listener.onLeftButtonBClick1();
				}
				mLeftButtonBFlag = true;
				DrawCanvas();
			}
			else {
				if(listener != null) {
					listener.onLeftButtonBClick2();
				}
				mLeftButtonBFlag = false;
				DrawCanvas();
			}
			return true;
		}
		//RightButtonB
		else if (isPointInCircle(new Point(x, y), new Point(mWidth - mRoundButtonDisdance, mHeight-7*mRoundButtonDisdance), mRoundButtonRadius))
		{
			callVibrator();
			if (false == mRightButtonBFlag) {
				if(listener != null) {
					listener.onRightButtonBClick1();
				}
				mRightButtonBFlag = true;
				DrawCanvas();
			}
			else {
				if(listener != null) {
					listener.onRightButtonBClick2();
				}
				mRightButtonBFlag = false;
				DrawCanvas();
			}
			return true;
		}
		return false;
	}
	
	private boolean buttonClickUpCheck(int x, int y)
	{
		if (mLeftButtonADown 
				&& isPointInCircle(new Point(x, y), new Point(mRoundButtonDisdance, mHeight - mRoundButtonDisdance), mRoundButtonRadius)
				) {
			mLeftButtonADown = false;
			DrawCanvas();
			if(listener != null) {
				listener.onLeftButtonAUp();
			}
			if (false == mUseLeftThrottle)
			{
				if(listener != null) {
					listener.onLeftWheelChanged(0, 0);
				}
				if (mSensorManager != null) {
					mSensorManager.unregisterListener(mOnSensorEventListener);
				}
			}
			return true;
		}
		else if (mRightButtonADown 
				&& isPointInCircle(new Point(x, y), new Point(mWidth - mRoundButtonDisdance, mHeight - mRoundButtonDisdance), mRoundButtonRadius)
				) {
			mRightButtonADown = false;
			DrawCanvas();
			if(listener != null) {
				listener.onRightButtonAUp();
			}
			if (true == mUseLeftThrottle)
			{
				if(listener != null) {
					listener.onRightWheelChanged(0, 0);
				}
				if (mSensorManager != null) {
					mSensorManager.unregisterListener(mOnSensorEventListener);
				}
			}
			return true;
		}
		return false;
	}
	
	private boolean isTouchLeftPart(int x, int y)
	{
		if (x < mWidth/2)
		{
			return true;
		}
		else {
			return false;
		}
	}
	
	private void onLeftVJRDown(int x, int y)
	{
		if (mLeftAlwaysShow == -1)
		{
			mShowLeftVJR = false;
			mMoveLeftVJR = false;
		}
		else if (mLeftAlwaysShow == 1)
		{
			mShowLeftVJR = true;
			if (null == mLeftCtrlPoint || null == mLeftRockerPosition) {
				mLeftCtrlPoint = new Point(x, y);
				mLeftRockerPosition = new Point(x, y);
				mMoveLeftVJR = true;
				callVibrator();
			}
			else if (getLength(mLeftRockerPosition.x, mLeftRockerPosition.y, x, y) <= mRudderRadius) {
				mLeftRockerPosition = new Point(x, y);
				mMoveLeftVJR = true;
				callVibrator();
			}//点中摇杆
		}
		else {
			mShowLeftVJR = true;
			mLeftCtrlPoint = new Point(x, y);
			mLeftRockerPosition = new Point(x, y);
			mMoveLeftVJR = true;
			callVibrator();
		}
		DrawCanvas();
	}
	
	private void onLeftVJRMove(int x, int y)
	{
		if (!mShowLeftVJR) {
			return;
		}
		if (!mMoveLeftVJR) {
			return;
		}
		
		int len = getLength(mLeftCtrlPoint.x, mLeftCtrlPoint.y, x, y);
		int L;
		if(len <= mWheelRadius) {//如果手指在摇杆活动范围内，则摇杆处于手指触摸位置
			mLeftRockerPosition.set(x, y);
			L = len * 65535 / mWheelRadius;
		}
		else {//设置摇杆位置，使其处于手指触摸方向的摇杆活动范围边缘
			mLeftRockerPosition = getBorderPoint(mLeftCtrlPoint, new Point(x, y), mWheelRadius);
			L = 65535;
		}
		if(len > mMoveMinRadius && listener != null) {
			float radian = getRadian(mLeftCtrlPoint, new Point(x, y));
			listener.onLeftWheelChanged(getAngleCouvert(radian), L);
		}
		DrawCanvas();
	}
	
	private void onLeftVJRUp(int x, int y)
	{
		mMoveLeftVJR = false;
		if (mLeftAlwaysShow == 1 || mLeftAlwaysShow == -1)
		{
			return;
		}
		if (!mShowLeftVJR) {
			return;
		}
		mLeftRockerPosition.set(mLeftCtrlPoint.x, mLeftCtrlPoint.y);
		if(listener != null) {
			listener.onLeftWheelChanged(0, 0);
		}
		mShowLeftVJR = false;
		DrawCanvas();
	}
	
	private void onRightVJRDown(int x, int y)
	{
		if (mRightAlwaysShow == -1)
		{
			mShowRightVJR = false;
			mMoveRightVJR = false;
		}
		else if (mRightAlwaysShow == 1)
		{
			mShowRightVJR = true;
			if (null == mRightCtrlPoint || null == mRightRockerPosition) {
				mRightCtrlPoint = new Point(x, y);
				mRightRockerPosition = new Point(x, y);
				mMoveRightVJR = true;
				callVibrator();
			}
			else if (getLength(mRightRockerPosition.x, mRightRockerPosition.y, x, y) <= mRudderRadius) {
				mRightRockerPosition = new Point(x, y);
				mMoveRightVJR = true;
				callVibrator();
			}//点中摇杆
		}
		else {
			mShowRightVJR = true;
			mRightCtrlPoint = new Point(x, y);
			mRightRockerPosition = new Point(x, y);
			mMoveRightVJR = true;
			callVibrator();
		}
		DrawCanvas();
	}
	
	private void onRightVJRMove(int x, int y)
	{
		if (!mShowRightVJR) {
			return;
		}
		if (!mMoveRightVJR) {
			return;
		}
		
		int len = getLength(mRightCtrlPoint.x, mRightCtrlPoint.y, x, y);
		int L;
		if(len <= mWheelRadius) {//如果手指在摇杆活动范围内，则摇杆处于手指触摸位置
			mRightRockerPosition.set(x, y);
			L = len * 65535 / mWheelRadius;
		}
		else {//设置摇杆位置，使其处于手指触摸方向的摇杆活动范围边缘
			mRightRockerPosition = getBorderPoint(mRightCtrlPoint, new Point(x, y), mWheelRadius);
			L = 65535;
		}
		if(len > mMoveMinRadius && listener != null) {
			float radian = getRadian(mRightCtrlPoint, new Point(x, y));
			listener.onRightWheelChanged(getAngleCouvert(radian), L);
		}
		DrawCanvas();
	}
	
	private void onRightVJRUp(int x, int y)
	{
		mMoveRightVJR = false;
		if (mRightAlwaysShow == 1 || mRightAlwaysShow == -1)
		{
			return;
		}
		if (!mShowRightVJR) {
			return;
		}
		mRightRockerPosition.set(mRightCtrlPoint.x, mRightCtrlPoint.y);
		if(listener != null) {
			listener.onRightWheelChanged(0, 0);
		}
		mShowRightVJR = false;
		DrawCanvas();
	}
	
	@Override
	public boolean onTouchEvent(MotionEvent event) {
		int index = event.getActionIndex();
		int x = (int) event.getX(index);
		int y = (int) event.getY(index);
		int pointerCount = event.getPointerCount();
		
		switch (event.getActionMasked()) {
		case MotionEvent.ACTION_DOWN:
			Log.d("VJR Debug", "ACTION_DOWN  index=" + index + ", id=" + event.getPointerId(index));
			if (false == buttonClickDownCheck(x, y))
			{
				if (-1 == mLeftPointerId && -1 == mRightPointerId)
				{
					if (isTouchLeftPart(x, y))
					{
						mLeftPointerId = event.getPointerId(index);
						onLeftVJRDown(x, y);
					}
					else {
						mRightPointerId = event.getPointerId(index);
						onRightVJRDown(x, y);
					}
				}
				else {
					Log.d("VJR Debug", "ACTION_DOWN Error!!!!!!!!!!!!!");
				}
			}
			break;
			
		case MotionEvent.ACTION_UP:
			Log.d("VJR Debug", "ACTION_UP  index=" + index + ", id=" + event.getPointerId(index));
			if (event.getPointerId(index) == mLeftPointerId)
			{
				onLeftVJRUp(x, y);
				mLeftPointerId = -1;
			}
			else if (event.getPointerId(index) == mRightPointerId)
			{
				onRightVJRUp(x, y);
				mRightPointerId = -1;
			}
			else {
				buttonClickUpCheck(x, y);
			}
			break;
			
		case MotionEvent.ACTION_MOVE:
			long currTime = System.currentTimeMillis();
			if (currTime - mLastMoveTime < mMoveMinTime) {
				return true;
			}
			else {
				mLastMoveTime = currTime;
			}
			//Log.d("VJR Debug", "ACTION_MOVE  index=" + event.getActionIndex() + ", count=" + pointerCount);
			if (-1 != mLeftPointerId || -1 != mRightPointerId)
			{
				int id = event.getPointerId(0);
				x = (int) event.getX(0);
				y = (int) event.getY(0);
				int id2 = -1;
				int x2 = Integer.MAX_VALUE;
				int y2 = Integer.MAX_VALUE;
				if (pointerCount > 1)
				{
					id2 = event.getPointerId(1);
					x2 = (int) event.getX(1);
					y2 = (int) event.getY(1);
				}
				if (id == mLeftPointerId)
				{
					onLeftVJRMove(x, y);
				}
				else if (id == mRightPointerId)
				{
					onRightVJRMove(x, y);
				}
				if (id2 == mLeftPointerId)
				{
					if (x2 != Integer.MAX_VALUE && y2 != Integer.MAX_VALUE) {
						onLeftVJRMove(x2, y2);
					}
				}
				else if (id2 == mRightPointerId)
				{
					if (x2 != Integer.MAX_VALUE && y2 != Integer.MAX_VALUE) {
						onRightVJRMove(x2, y2);
					}
				}
			}
			break;
			
		case MotionEvent.ACTION_POINTER_DOWN:
			Log.d("VJR Debug", "ACTION_POINTER_DOWN  index=" + index + ", id=" + event.getPointerId(index));
			if (false == buttonClickDownCheck(x, y))
			{
				if (-1 == mLeftPointerId && -1 == mRightPointerId)
				{
					if (isTouchLeftPart(x, y))
					{
						mLeftPointerId = event.getPointerId(index);
						onLeftVJRDown(x, y);
					}
					else {
						mRightPointerId = event.getPointerId(index);
						onRightVJRDown(x, y);
					}
				}
				else if (-1 == mRightPointerId)
				{
					mRightPointerId = event.getPointerId(index);
					onRightVJRDown(x, y);
				}
				else if (-1 == mLeftPointerId)
				{
					mLeftPointerId = event.getPointerId(index);
					onLeftVJRDown(x, y);
				}
				else {
					Log.d("VJR Debug", "ACTION_POINTER_DOWN Error!!!!!!!!!!!!!");
				}
			}
			break;
			
		case MotionEvent.ACTION_POINTER_UP:
			Log.d("VJR Debug", "ACTION_POINTER_UP  index=" + index + ", id=" + event.getPointerId(index));
			if (event.getPointerId(index) == mRightPointerId)
			{
				onRightVJRUp(x, y);
				mRightPointerId = -1;
			}
			else if(event.getPointerId(index) == mLeftPointerId)
			{
				onLeftVJRUp(x, y);
				mLeftPointerId = -1;
			}
			else {
				buttonClickUpCheck(x, y);
			}
			break;
		}
		return true;
	}
}
