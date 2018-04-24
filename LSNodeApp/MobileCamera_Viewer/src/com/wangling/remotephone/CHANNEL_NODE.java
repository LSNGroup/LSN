package com.wangling.remotephone;

import com.lsngroup.live.R;


public class CHANNEL_NODE
{
	public int channel_id;
	public String channel_comments;
	public String device_uuid;
	public String node_id_str;
	public String pub_ip_str;
	public String location;
	
	public boolean isOnline()
	{
		return location.equals("") ? false : true;
	}
}
