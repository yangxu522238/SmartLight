package com.thundersoft.flagmingo.curtain;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.TextView;

import com.thundersoft.flagmingo.main.R;

import java.util.List;

public class MissionAdapter extends ArrayAdapter {
    private final int resourceId ;
    public MissionAdapter(Context context, int textViewResourceId, List<Mission> objects ) {
        super(context, textViewResourceId, objects);
        resourceId = textViewResourceId;
    }
    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {
        final Mission mMission = (Mission) getItem(position);
        View view = LayoutInflater.from(getContext()).inflate(resourceId, null);
        TextView mMissionContext=(TextView)view.findViewById(R.id.mission_item_time);
        ImageButton mMissionBtn=(ImageButton) view.findViewById(R.id.mission_item_btn);
        mMissionContext.setText("Time: "+mMission.mMissionTime+" pos: "+ mMission.mMissionValue);

        mMissionBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                CurtainController.getInstance().RemoveCurtainMissionlist(mMission);
            }
        });
        return view;
    }
}
