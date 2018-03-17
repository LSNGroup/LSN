package com.wangling.remotephone;

import com.lsngroup.live.R;


public class ANYPC_NODE
{
	public boolean bLanNode;  /* LAN_NODE_SUPPORT */
	public String node_id_str;
	public String node_name;
	public int version;
	public String ip_str;
	public String port_str;
	public String pub_ip_str;
	public String pub_port_str;
	public boolean no_nat;
	public int nat_type;
	public boolean is_admin;
	public boolean is_busy;
	public int audio_channels;
	public int video_channels;
	public String os_info;
	public byte func_flags;
	public String device_uuid;
	public int comments_id;
	public String location;
	
	public boolean isAnypcNode()
	{
		String temp = "@ANYPC@";
		return device_uuid.contains(temp);
	}
	
	public boolean isYkzNode()
	{
		String temp = "@YKZ@";
		return device_uuid.contains(temp);
	}
	
	public boolean isRobNode()
	{
		String temp = "@ROB@";
		return device_uuid.contains(temp);
	}
	
	public boolean isUavNode()
	{
		String temp = "@UAV@";
		return device_uuid.contains(temp);
	}
	
	public boolean isYcamNode()
	{
		String temp = "@YCAM@";
		return device_uuid.contains(temp);
	}
	
	public boolean isOnline()
	{
		return location.equals("") ? false : true;
	}
	
	public boolean isLanOnly()
	{
		return comments_id > 0 ? false : true;
	}
}
