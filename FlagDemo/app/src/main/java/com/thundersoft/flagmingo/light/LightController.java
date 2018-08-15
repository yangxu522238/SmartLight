package com.thundersoft.flagmingo.light;

import com.wilddog.client.DataSnapshot;
import com.wilddog.client.SyncError;
import com.wilddog.client.ValueEventListener;

public class LightController {
    private static  LightController instance;

    private LightUIFragment mLightUIFragment;
    private LightWilddogController mLightWilddogController;

    public static LightController getInstance()
    {
        if(null==instance)
            instance=new LightController();
        return instance;
    }

    public LightUIFragment getLightUIFragment() {
        return mLightUIFragment;
    }

    public void OpenLight(){
        mLightWilddogController.OpenLight();
    }
    public void CloseLight()
    {
        mLightWilddogController.CloseLight();
    }

    private LightController()
    {
        mLightUIFragment =new LightUIFragment();
        mLightWilddogController = new LightWilddogController();
        mLightWilddogController.AddValueEventListener(mLightValueListener);
    }
    private ValueEventListener mLightValueListener= new ValueEventListener() {
        @Override
        public void onDataChange(DataSnapshot dataSnapshot) {
            String value =dataSnapshot.getValue()+"";
            mLightUIFragment.UpdateLightControlBtn(value.equals(mLightWilddogController.OPEN_LIGHT_CODE));
        }

        @Override
        public void onCancelled(SyncError syncError) {

        }
    };

}
