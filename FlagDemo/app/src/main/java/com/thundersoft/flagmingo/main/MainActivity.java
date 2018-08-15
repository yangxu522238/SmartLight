package com.thundersoft.flagmingo.main;

import android.app.Activity;
import android.content.Intent;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;

import com.thundersoft.flagmingo.smog.PlayService;
import com.thundersoft.wilddog.Wilddog;
import com.wilddog.client.ChildEventListener;
import com.wilddog.client.DataSnapshot;
import com.wilddog.client.SyncError;
import com.wilddog.client.SyncReference;
import com.wilddog.client.WilddogSync;

import java.io.IOException;

public class MainActivity extends Activity {

    private FragmentsController mFragmentsController;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_main);
        mFragmentsController = new FragmentsController(this);
        mFragmentsController.CreatBottomNavigationBar();

        Wilddog wilddog = Wilddog.getInstance();
        SyncReference sync = WilddogSync.getInstance().getReference("/Flagmingo/smoke");
        sync.addChildEventListener(new ChildEventListener() {
            @Override
            public void onChildAdded(DataSnapshot dataSnapshot, String s) {
                Log.i("log--yang", "-----" + s);
            }

            @Override
            public void onChildChanged(DataSnapshot dataSnapshot, String s) {
                Log.i("log--yang", "-----" + dataSnapshot.getValue().toString());
                if (dataSnapshot.getValue().toString().contains("108")) {
                    MediaPlayer player = new MediaPlayer();
                    try {
                        player.setDataSource("/sdcard/bluetooth/c.mp3");
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    try {
                        player.prepare();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    player.start();
                }
            }

            @Override
            public void onChildRemoved(DataSnapshot dataSnapshot) {
                Log.i("log--yang", "-----onChildRemoved");
            }

            @Override
            public void onChildMoved(DataSnapshot dataSnapshot, String s) {
                Log.i("log--yang", "-----onChildMoved");
            }

            @Override
            public void onCancelled(SyncError syncError) {
                Log.i("log--yang", "-----onCancelled");
            }
        });

    }


}


