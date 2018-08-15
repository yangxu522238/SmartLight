package com.thundersoft.flagmingo.smog;

import android.app.Service;
import android.content.Intent;
import android.media.MediaPlayer;
import android.os.IBinder;
import android.support.annotation.Nullable;

import com.thundersoft.flagmingo.main.R;

import java.io.IOException;

public class PlayService extends Service {

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {


        return START_STICKY_COMPATIBILITY;
    }
}
