package com.androvideo.vahelperlib;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class AvcVAManager {
    private static final String TAG = "AvcVAManager";

    public static final String USER_EVENT_ACTION = "com.androvideo.va.action.USER_EVENT";
    private static final String USER_CONFIG_ACTION = "com.androvideo.va.action.USER_CONFIG";
    private static final String USER_CONFIG_MODULE_NAME = "MODULE_NAME";
    private static final String USER_CONFIG_MODULE_ENABLE = "MODULE_ENABLE";

    private static volatile AvcVAManager sInstance = null;
    private Context mContext = null;

    public static AvcVAManager getInstance() {
        if (sInstance == null) {
            synchronized (AvcVAManager.class) {
                if (sInstance == null) {
                    sInstance = new AvcVAManager();
                }
            }
        }
        return sInstance;
    }

    private AvcVAManager() {
        // for singleton
    }

    public void init(Context ctx) {
        mContext = ctx;
    }

    public void enableVA(String moduleName) {
        if (moduleName == null || moduleName.isEmpty()) {
            Log.e(TAG, "Can't enableVA with null moduleName");
            return;
        }
        setupVA(moduleName, true);
    }

    public void disableVA(String moduleName) {
        if (moduleName == null || moduleName.isEmpty()) {
            Log.e(TAG, "Can't disableVA with null moduleName");
            return;
        }
        setupVA(moduleName, false);
    }

    private void setupVA(String moduleName, boolean enable) {
        if (mContext == null) {
            Log.e(TAG, "Can not configurate VA without context. Do you miss init?");
            return;
        }
        Intent intent = new Intent();
        intent.setAction(USER_CONFIG_ACTION);
        intent.putExtra(USER_CONFIG_MODULE_NAME, moduleName);
        intent.putExtra(USER_CONFIG_MODULE_ENABLE, enable);
        mContext.sendBroadcast(intent);
    }
}
