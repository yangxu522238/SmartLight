package com.thundersoft.flagmingo.light;

import android.annotation.SuppressLint;
import android.app.Fragment;
import android.media.Image;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.LinearLayout;

import com.thundersoft.flagmingo.main.R;

public class LightUIFragment extends Fragment implements View.OnClickListener {
    private ImageButton mLight_control_btn;
    private LinearLayout mLight_bg;
    private boolean mLight_is_open = false;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, Bundle savedInstanceState) {
        View view=inflater.inflate(R.layout.light_fragment,container,false);
        mLight_control_btn =(ImageButton) view.findViewById(R.id.light_control_btn);
        mLight_bg = (LinearLayout)view.findViewById(R.id.light_bg);
        mLight_control_btn.setOnClickListener(this);
        return  view;
    }

    @Override
    public void onClick(View v) {
        if(v.getId()==R.id.light_control_btn){
            if(mLight_is_open)
                LightController.getInstance().CloseLight();
            else
                LightController.getInstance().OpenLight();
        }
    }
    public void UpdateLightControlBtn(boolean isOpen)
    {
        mLight_is_open = isOpen;
        if(isOpen){
            mLight_control_btn.setBackground(getResources().getDrawable(R.drawable.light_open));
            mLight_bg.setBackgroundColor(getResources().getColor(R.color.tab_grey));
        }else{
            mLight_control_btn.setBackground(getResources().getDrawable(R.drawable.light_close));
            mLight_bg.setBackgroundColor(getResources().getColor(R.color.white));
        }
    }

}
