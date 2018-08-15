package com.thundersoft.flagmingo.curtain;

import android.app.Activity;
import android.util.Log;
import android.widget.Toast;

import com.thundersoft.flagmingo.main.FragmentsController;
import com.thundersoft.flagmingo.main.R;
import com.wilddog.client.ChildEventListener;
import com.wilddog.client.DataSnapshot;
import com.wilddog.client.SyncError;
import com.wilddog.client.SyncReference;
import com.wilddog.client.ValueEventListener;


public class CurtainController {
    private static final String TAG = CurtainController.class.getSimpleName();
    private static  CurtainController mInstance=null;
    public  static  CurtainController getInstance()
    {
        if(null==mInstance)
            mInstance=new CurtainController();
        return mInstance;
    }


    private Activity mActivity;

    private CurtainUIFragment mCurtainUIFragment;

    private CurtainWilddogController mCurtainWilddogManager;

    private MissionAdapter mMissionAdapter =null;

    public CurtainUIFragment getCurtainUIFragment() {
        if(null==mCurtainUIFragment)
            mCurtainUIFragment = new CurtainUIFragment();
        return mCurtainUIFragment;
    }

    private CurtainController()
    {
        mActivity = FragmentsController.getMainActivity();
    }

    public CurtainWilddogController getCurtainWilddogController() {
        if(null==mCurtainWilddogManager)
            mCurtainWilddogManager = new CurtainWilddogController();
        return mCurtainWilddogManager;
    }

    public MissionAdapter getMissionAdapter() {
        if(null==mMissionAdapter)
            mMissionAdapter = new MissionAdapter(mActivity, R.layout.mission_item,CurtainDate.mCurtainMisstionList);
        return mMissionAdapter;
    }

    public void AddWilddogListeners()
    {
        getCurtainWilddogController().AddChildEventListener(mission_list_listener);
        getCurtainWilddogController().AddValueEventListener(control_value_listener);
    }
    public void RemoveWilddogListeners()
    {
        getCurtainWilddogController().RemoveChildEventListener(mission_list_listener);
        getCurtainWilddogController().RemoveValueEventListener(control_value_listener);
    }


    public void AddCurtainMissionList(final String missionId,final Mission mission)
    {
        if(CurtainDate.mCurtainMisstionList.contains(mission))
            return;
        mCurtainWilddogManager.AddCurtainMissionList(missionId, mission, new SyncReference.CompletionListener() {
            @Override
            public void onComplete(SyncError syncError, SyncReference syncReference) {
                if(null==syncError)
                    getCurtainUIFragment().HideMissionSettingPlane();
                else
                    Toast.makeText(mActivity,"AddCurtainMissionList Failed  :"+syncError.getErrCode(),Toast.LENGTH_LONG).show();
            }
        });
    }

    public void RemoveCurtainMissionlist(final Mission mission)
    {
        int index = CurtainDate.mCurtainMisstionList.indexOf(mission);
        final String keyStr = CurtainDate.mCurtainMissionListKey.get(index);
        mCurtainWilddogManager.RemoveCurtainMissonList(keyStr, new SyncReference.CompletionListener() {
            @Override
            public void onComplete(SyncError syncError, SyncReference syncReference) {
                if(null==syncError)
                {
                    mMissionAdapter.remove(mission);
                    CurtainDate.mCurtainMissionListKey.remove(keyStr);
                }
                else
                    Toast.makeText(mActivity,"Remove Curtain MissonList Failed :"+syncError.getErrCode(),Toast.LENGTH_LONG).show();
            }
        });
    }

    public  void  ChangeCurtainValue(int targetValue)
    {
        mCurtainWilddogManager.UpdateCurtainValue(targetValue, new SyncReference.CompletionListener() {
            @Override
            public void onComplete(SyncError syncError, SyncReference syncReference) {
                if(null==syncError)
                    Toast.makeText(mActivity,"Update Curtain Value Success :",Toast.LENGTH_LONG).show();
                else
                    Toast.makeText(mActivity,"RUpdate Curtain Value Failed :"+syncError.getErrCode(),Toast.LENGTH_LONG).show();
            }
        });
    }

    private ChildEventListener mission_list_listener=new ChildEventListener() {
        @Override
        public void onChildAdded(DataSnapshot dataSnapshot, String s) {
            String mMissionTime = dataSnapshot.child("mMissionTime").getValue().toString();
            String mMissionValue = dataSnapshot.child("mMissionValue").getValue().toString();
            Mission model = new Mission(mMissionTime,mMissionValue);
            String key = dataSnapshot.getKey();
            if(!CurtainDate.mCurtainMisstionList.contains(model))
            {
                CurtainDate.mCurtainMisstionList.add(model);
                CurtainDate.mCurtainMissionListKey.add(key);
                mMissionAdapter.notifyDataSetChanged();
            }
        }

        @Override
        public void onChildChanged(DataSnapshot dataSnapshot, String s) {

        }

        @Override
        public void onChildRemoved(DataSnapshot dataSnapshot) {

        }

        @Override
        public void onChildMoved(DataSnapshot dataSnapshot, String s) {

        }

        @Override
        public void onCancelled(SyncError syncError) {

        }
    };
    private ValueEventListener control_value_listener= new ValueEventListener() {
        @Override
        public void onDataChange(DataSnapshot dataSnapshot) {
            Log.d(TAG, "control_value_listener onDataChange: ");
            Object object = dataSnapshot.getValue();
            if (null==object)
            {
                Toast.makeText(mActivity,"获取窗帘位置失败 ",Toast.LENGTH_SHORT);
            }
            else
            {
                Toast.makeText(mActivity,"获取窗帘位置成功 "+object,Toast.LENGTH_SHORT);
                String value =""+object;
                getCurtainUIFragment().UpdateCurtainControlSliderValue(value);
            }
        }

        @Override
        public void onCancelled(SyncError syncError) {

        }
    };
}
