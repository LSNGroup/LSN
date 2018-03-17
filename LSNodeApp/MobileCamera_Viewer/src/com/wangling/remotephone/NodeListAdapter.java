package com.wangling.remotephone;

import java.util.List;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;
import com.lsngroup.live.R;


public class NodeListAdapter extends BaseAdapter
{
    private Context context = null;
    private List<ANYPC_NODE> nodeList = null;
    
    public class ViewHolder{   
        ImageView imageViewIcon;   
        TextView textViewName;   
        ImageView imageViewArm;   
        TextView textViewInfo;   
    }
    
    
    public NodeListAdapter(Context _context, List<ANYPC_NODE> _nodeList)
    {
    	this.context = _context;
    	this.nodeList = _nodeList;
    }
    
    public int getCount() {   
        return (null == nodeList) ? 0 : nodeList.size();   
    }
    
    public Object getItem(int position) {   
        return (null == nodeList) ? null : nodeList.get(position);   
    }
    
    public long getItemId(int position) {   
        return position;   
    }
    
    public View getView(int position, View convertView, ViewGroup parent) {   
        final ANYPC_NODE anypcNode = (ANYPC_NODE)getItem(position);   
        ViewHolder viewHolder = null;   
        if (convertView == null){   
            Log.d("NodeListAdapter", "New convertView, position="+position);   
            convertView = LayoutInflater.from(context).inflate(   
                    R.layout.node_list_item, null);   
               
            viewHolder = new ViewHolder();   
            viewHolder.imageViewIcon = (ImageView)convertView.findViewById(   
                    R.id.node_icon);   
            viewHolder.textViewName = (TextView)convertView.findViewById(   
                    R.id.node_name);   
            viewHolder.imageViewArm = (ImageView)convertView.findViewById(   
                    R.id.node_arm_img);   
            viewHolder.textViewInfo = (TextView)convertView.findViewById(   
                    R.id.node_info);   
               
            convertView.setTag(viewHolder);   
        }
        else {   
            viewHolder = (ViewHolder)convertView.getTag();   
            Log.d("NodeListAdapter", "Old convertView, position="+position);   
        }   
        
        
        if (anypcNode.isOnline()) {
        	if (anypcNode.isAnypcNode()) {
        		viewHolder.imageViewIcon.setImageResource(R.drawable.anypc_online);
        	}
        	else if (anypcNode.isYkzNode()) {
        		viewHolder.imageViewIcon.setImageResource(R.drawable.ykz_online);
        	}
        	else if (anypcNode.isRobNode()) {
        		viewHolder.imageViewIcon.setImageResource(R.drawable.rob_online);
        	}
        	else if (anypcNode.isUavNode()) {
        		viewHolder.imageViewIcon.setImageResource(R.drawable.uav_online);
        	}
        	else /* if (anypcNode.isYcamNode()) */{
        		viewHolder.imageViewIcon.setImageResource(R.drawable.ycam_online);
        	}
        	if (anypcNode.location.equals(SharedFuncLib.ANYPC_LOCAL_LAN)) {
        		viewHolder.textViewInfo.setText(R.string.msg_local_lan);
        	}
        	else {
        		viewHolder.textViewInfo.setText(anypcNode.location);
        	}
        }
        else {
        	if (anypcNode.isAnypcNode()) {
        		viewHolder.imageViewIcon.setImageResource(R.drawable.anypc_offline);
        	}
        	else if (anypcNode.isYkzNode()) {
        		viewHolder.imageViewIcon.setImageResource(R.drawable.ykz_offline);
        	}
        	else if (anypcNode.isRobNode()) {
        		viewHolder.imageViewIcon.setImageResource(R.drawable.rob_offline);
        	}
        	else if (anypcNode.isUavNode()) {
        		viewHolder.imageViewIcon.setImageResource(R.drawable.uav_offline);
        	}
        	else /* if (anypcNode.isYcamNode()) */{
        		viewHolder.imageViewIcon.setImageResource(R.drawable.ycam_offline);
        	}
        	viewHolder.textViewInfo.setText(R.string.msg_mobcam_offline);
        }

        if (anypcNode.isLanOnly()) {
        	viewHolder.textViewName.setText("[ID:" + "NONE" + "] " + anypcNode.node_name);
        }
        else {
        	viewHolder.textViewName.setText("[ID:" + anypcNode.comments_id + "] " + anypcNode.node_name);
        }
        //viewHolder.imageViewArm.setImageResource(anypcNode.comments_id);
        
        return convertView;   
    }   
	
}
