package com.thundersoft.flagmingo.light;

import com.thundersoft.wilddog.Wilddog;
import com.wilddog.client.SyncReference;
import com.wilddog.client.ValueEventListener;

public class LightWilddogController {
    private Wilddog mWilddog;
    private static final String LIGHT_STATE_PATH  = "Flagmingo/Light/LightState";
    public static  final String OPEN_LIGHT_CODE = "1";
    public static final String CLOSE_LIGHT_CODE = "0";
    public LightWilddogController()
    {
        mWilddog = Wilddog.getInstance();
    }

    public void OpenLight()
    {
        mWilddog.SetNodeValue(LIGHT_STATE_PATH,OPEN_LIGHT_CODE,null);
    }
    public void CloseLight()
    {
        mWilddog.SetNodeValue(LIGHT_STATE_PATH,CLOSE_LIGHT_CODE,null);
    }
    public void AddValueEventListener(ValueEventListener listener)
    {
        mWilddog.AddValueEventListener(LIGHT_STATE_PATH, listener);
    }
    public void RemoveValueEventListener(ValueEventListener listener)
    {
        mWilddog.AddValueEventListener(LIGHT_STATE_PATH,listener);
    }
}
