package com.androvideo.vasampleapp;

import android.app.Application;

import com.androvideo.vahelperlib.AvcVAManager;

public class MyApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        // Initialize VA manager with application context
        AvcVAManager.getInstance().init(this);
    }
}
