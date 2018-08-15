package com.thundersoft.flagmingo.curtain;


import com.thundersoft.wilddog.Wilddog;
import com.wilddog.client.ChildEventListener;
import com.wilddog.client.SyncReference;
import com.wilddog.client.ValueEventListener;

public class CurtainWilddogController {
    private Wilddog mWilddog = null;
    public  final String CURTAIN_MISSON_LIST_PATH = "Flagmingo/Curtain/MissionList";
    public  final String CURTAIN_CONTROL_VALUE_PATH="Flagmingo/Curtain/ControlValue";

    public CurtainWilddogController()
    {
        mWilddog = Wilddog.getInstance();
    }

    public void UpdateCurtainValue(int value, SyncReference.CompletionListener listener)
    {
        mWilddog.SetNodeValue (CURTAIN_CONTROL_VALUE_PATH,value,listener);
    }
    public void  RemoveCurtainMissonList(String key, SyncReference.CompletionListener completionListener)
    {
        mWilddog.RemoveNodeValue(CURTAIN_MISSON_LIST_PATH+"/"+key,completionListener);
    }
    public void AddCurtainMissionList( String nodeName, Mission mission, SyncReference.CompletionListener completionListener)
    {
        mWilddog.SetNodeValue(CURTAIN_MISSON_LIST_PATH+"/"+nodeName,mission,completionListener);
    }
    public void AddValueEventListener(ValueEventListener listener)
    {
        mWilddog.AddValueEventListener(CURTAIN_CONTROL_VALUE_PATH,listener);
    }
    public void AddChildEventListener(ChildEventListener listener)
    {
        mWilddog.AddChildEventListener(CURTAIN_MISSON_LIST_PATH,listener);
    }
    public void RemoveValueEventListener(ValueEventListener listener)
    {
        mWilddog.RemoveEventListener(CURTAIN_CONTROL_VALUE_PATH,listener);
    }
    public void RemoveChildEventListener(ChildEventListener listener)
    {
        mWilddog.RemoveEventListener(CURTAIN_MISSON_LIST_PATH,listener);
    }
}

