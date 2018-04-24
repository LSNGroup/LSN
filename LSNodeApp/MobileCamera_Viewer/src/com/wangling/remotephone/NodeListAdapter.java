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
    private List<CHANNEL_NODE> nodeList = null;
    
    public class ViewHolder{   
        ImageView imageViewIcon;   
        TextView textViewName;   
        ImageView imageViewArm;   
        TextView textViewInfo;   
    }
    
    
    public NodeListAdapter(Context _context, List<CHANNEL_NODE> _nodeList)
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
        final CHANNEL_NODE anypcNode = (CHANNEL_NODE)getItem(position);   
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
        	{/* if (anypcNode.isYcamNode()) */
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
        	{/* if (anypcNode.isYcamNode()) */
        		viewHolder.imageViewIcon.setImageResource(R.drawable.ycam_offline);
        	}
        	viewHolder.textViewInfo.setText(R.string.msg_mobcam_offline);
        }


       	viewHolder.textViewName.setText("[ID:" + anypcNode.channel_id + "] " + anypcNode.channel_comments);
        //viewHolder.imageViewArm.setImageResource(anypcNode.comments_id);
        
        return convertView;   
    }   
	
}
