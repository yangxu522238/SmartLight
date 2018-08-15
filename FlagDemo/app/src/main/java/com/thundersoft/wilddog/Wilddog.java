package com.thundersoft.wilddog;

import android.app.Activity;
import android.app.FragmentContainer;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;
import android.widget.Toast;

import com.thundersoft.flagmingo.curtain.CurtainDate;
import com.thundersoft.flagmingo.main.FragmentsController;
import com.wilddog.client.ChildEventListener;
import com.wilddog.client.DataSnapshot;
import com.wilddog.client.Query;
import com.wilddog.client.SyncError;
import com.wilddog.client.SyncReference;
import com.wilddog.client.ValueEventListener;
import com.wilddog.client.WilddogSync;
import com.wilddog.wilddogcore.WilddogApp;
import com.wilddog.wilddogcore.WilddogOptions;

public class Wilddog {

    private static Wilddog instance =null;
    public static Wilddog getInstance()
    {
        if(null==instance)
            instance=new Wilddog(FragmentsController.getMainActivity(), CurtainDate.SYNC_URL);
        return instance;
    }
    private Wilddog(Activity activity,final String syncUrl)
    {
        mActivity = activity;
        mWilddogOptions = new WilddogOptions.Builder().setSyncUrl(syncUrl).build();
        mWilddogApp =WilddogApp.initializeApp(mActivity, mWilddogOptions);

    }

    private Activity mActivity;
    private static final String WILDDOG_DATA_UPDATE_SUCCESS="Data upload successful!";
    private static final String WILDDOG_DATA_UPDATE_FAIL = "Data upload failed!";
    private WilddogOptions mWilddogOptions=null;
    private WilddogApp mWilddogApp=null;

    private SyncReference.CompletionListener completionListener=new SyncReference.CompletionListener() {
        @Override
        public void onComplete(SyncError syncError, SyncReference syncReference) {
            if(null!=syncError)
                Toast.makeText(mActivity,WILDDOG_DATA_UPDATE_FAIL +" Error Code:"+syncError.getErrCode(),Toast.LENGTH_LONG).show();
            else
                Toast.makeText(mActivity,WILDDOG_DATA_UPDATE_SUCCESS,Toast.LENGTH_LONG).show();
        }
    };
    private boolean isNetworkAvailable(Context context) {
        ConnectivityManager manager = (ConnectivityManager) context
                .getApplicationContext().getSystemService(
                        Context.CONNECTIVITY_SERVICE);
        if (manager == null) {
            return false;
        }
        NetworkInfo networkinfo = manager.getActiveNetworkInfo();
        if (networkinfo == null || !networkinfo.isAvailable()) {
            return false;
        }
        return true;
    }

    public void SetNodeValue(String path, Object object, SyncReference.CompletionListener listener)
    {
        if(path.equals(""))
            return;
        if(null==object)
            return;
        SyncReference sync= WilddogSync.getInstance().getReference().child(path);
        if(null!=sync)
        {
            if(null==listener)
                sync.setValue(object,completionListener);
            else
                sync.setValue(object,listener);
        }
    }

    public void RemoveNodeValue(String path,SyncReference.CompletionListener listener) {
        SyncReference sync = WilddogSync.getInstance().getReference().child(path);
        if (null != listener)
            sync.removeValue(listener);
        else
            sync.removeValue(completionListener);
    }

    public void AddChildEventListener(String nodePath, ChildEventListener listener)
    {
        if(null==listener)
            return;
        SyncReference sync = WilddogSync.getInstance().getReference().child(nodePath);
        sync.addChildEventListener(listener);
    }

    public void AddValueEventListener(String nodePath,ValueEventListener listener)
    {
        if(null==listener)
            return;
        SyncReference sync = WilddogSync.getInstance().getReference().child(nodePath);
        sync.addValueEventListener(listener);
    }

    public void RemoveEventListener(String nodePath,ChildEventListener listener)
    {
        if(null==listener)
            return;
        SyncReference sync=WilddogSync.getInstance().getReference().child(nodePath);
        sync.removeEventListener(listener);
    }

    public void RemoveEventListener(String nodePath,ValueEventListener listener)
    {
        if(null==listener)
            return;
        SyncReference sync=WilddogSync.getInstance().getReference().child(nodePath);
        sync.removeEventListener(listener);
    }
}
