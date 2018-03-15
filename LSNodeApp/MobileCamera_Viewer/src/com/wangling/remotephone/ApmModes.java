package com.wangling.remotephone;

import java.util.ArrayList;
import java.util.List;


public enum ApmModes {
	FIXED_WING_MANUAL (0,"固定翼 Manual",MAV_TYPE.MAV_TYPE_FIXED_WING),
	FIXED_WING_CIRCLE (1,"固定翼 Circle",MAV_TYPE.MAV_TYPE_FIXED_WING),
	FIXED_WING_STABILIZE (2,"固定翼 Stabilize",MAV_TYPE.MAV_TYPE_FIXED_WING),
	FIXED_WING_TRAINING (3,"固定翼 Training",MAV_TYPE.MAV_TYPE_FIXED_WING),
	FIXED_WING_FLY_BY_WIRE_A (5,"固定翼 FBW-A",MAV_TYPE.MAV_TYPE_FIXED_WING),
	FIXED_WING_FLY_BY_WIRE_B (6,"固定翼 FBW-B",MAV_TYPE.MAV_TYPE_FIXED_WING),
	FIXED_WING_AUTO (10,"固定翼 Auto",MAV_TYPE.MAV_TYPE_FIXED_WING),
	FIXED_WING_RTL (11,"固定翼 RTL",MAV_TYPE.MAV_TYPE_FIXED_WING),
	FIXED_WING_LOITER (12,"固定翼 Loiter",MAV_TYPE.MAV_TYPE_FIXED_WING),
	FIXED_WING_GUIDED (15,"固定翼 Guided",MAV_TYPE.MAV_TYPE_FIXED_WING),

	ROTOR_STABILIZE(0, "多旋翼 Stabilize", MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_ACRO(1,"多旋翼 Acro", MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_ALT_HOLD(2, "多旋翼 Alt Hold",MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_AUTO(3, "多旋翼 Auto",MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_GUIDED(4, "多旋翼 Guided",MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_LOITER(5, "多旋翼 Loiter",MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_RTL(6, "多旋翼 RTL",MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_CIRCLE(7, "多旋翼 Circle",MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_POSITION(8, "多旋翼 Pos Hold",MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_LAND(9, "多旋翼 Land",MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_TOY(11, "多旋翼 Drift",MAV_TYPE.MAV_TYPE_QUADROTOR),
	ROTOR_TAKEOFF(13, "多旋翼 Sport",MAV_TYPE.MAV_TYPE_QUADROTOR),

	ROVER_MANUAL(0, "无人车/船 MANUAL", MAV_TYPE.MAV_TYPE_GROUND_ROVER),
	ROVER_LEARNING(2, "无人车/船 LEARNING", MAV_TYPE.MAV_TYPE_GROUND_ROVER),
	ROVER_STEERING(3, "无人车/船 STEERING", MAV_TYPE.MAV_TYPE_GROUND_ROVER),
	ROVER_HOLD(4, "无人车/船 HOLD", MAV_TYPE.MAV_TYPE_GROUND_ROVER),
	ROVER_AUTO(10, "无人车/船 AUTO", MAV_TYPE.MAV_TYPE_GROUND_ROVER),
	ROVER_RTL(11, "无人车/船 RTL", MAV_TYPE.MAV_TYPE_GROUND_ROVER),
	ROVER_GUIDED(15, "无人车/船 GUIDED", MAV_TYPE.MAV_TYPE_GROUND_ROVER),
	ROVER_INITIALIZING(16, "无人车/船 INITIALIZING", MAV_TYPE.MAV_TYPE_GROUND_ROVER),
	
	UNKNOWN(-1, "未知飞行器模式 ",0);



	private final int number;
    private final String name;
	private final int type;

	ApmModes(int number,String name, int type){
		this.number = number;
		this.name = name;
		this.type = type;
	}

	public int getNumber() {
		return number;
	}

	public String getName() {
		return name;
	}

	public int getType() {
		return type;
	}

	public static ApmModes getMode(int i, int type) {
		for (ApmModes mode : ApmModes.values()) {
			if (i == mode.getNumber() & type == mode.getType()) {
				return mode;
			}
		}
		return UNKNOWN;
	}

	public static ApmModes getMode(String str, int type) {
		for (ApmModes mode : ApmModes.values()) {
			if (str.equals(mode.getName()) & type == mode.getType()) {
				return mode;
			}
		}
		return UNKNOWN;
	}

	public static List<ApmModes> getModeList(int type) {
		List<ApmModes> modeList = new ArrayList<ApmModes>();

		if (isCopter(type)) {
			type = MAV_TYPE.MAV_TYPE_QUADROTOR;
		}

		for (ApmModes mode : ApmModes.values()) {
			if (isValid(mode) & mode.getType() == type) {
				modeList.add(mode);
			}
		}
		return modeList;
	}

	public static boolean isValid(ApmModes mode) {
		return mode!=ApmModes.UNKNOWN;
	}


	public static boolean isCopter(int type){
		switch (type) {
		case MAV_TYPE.MAV_TYPE_TRICOPTER:
		case MAV_TYPE.MAV_TYPE_QUADROTOR:
		case MAV_TYPE.MAV_TYPE_HEXAROTOR:
		case MAV_TYPE.MAV_TYPE_OCTOROTOR:
		case MAV_TYPE.MAV_TYPE_HELICOPTER:
			return true;
		case MAV_TYPE.MAV_TYPE_FIXED_WING:
		default:
			return false;
		}
	}

}
