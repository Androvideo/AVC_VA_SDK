package com.androvideo.vasampleapp;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import com.androvideo.vahelperlib.AvcVAManager;

public class VAEventReceiver extends BroadcastReceiver {
    private static final String TAG = "VAEventReceiver";

    private static final String MY_VA_MODULE_NAME = "MotionSample"; // Change your VA module name here
    private static final int MSG_ENABLE_VA = 1;

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "onReceive: " + intent.getAction());

        if (intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) {
            // We started VA module after booting up 5 seconds
            mHandler.sendEmptyMessageDelayed(MSG_ENABLE_VA, 5000);
        } else if (intent.getAction().equals(AvcVAManager.USER_EVENT_ACTION)) {
            Log.d(TAG, "=========== USER_EVENT START ============");
            Log.d(TAG, "algorithm = " + intent.getStringExtra("algorithm"));
            Log.d(TAG, "event_name = " + intent.getStringExtra("event_name"));
            Log.d(TAG, "event_type = " + intent.getIntExtra("event_type", -1));
            Log.d(TAG, "frame_width = " + intent.getIntExtra("frame_width", -1));
            Log.d(TAG, "frame_height = " + intent.getIntExtra("frame_height", -1));

            final Bundle event_box = intent.getBundleExtra("event_box");
            Log.d(TAG, "event_box.left = " + event_box.getFloat("left"));
            Log.d(TAG, "event_box.top = " + event_box.getFloat("top"));
            Log.d(TAG, "event_box.right = " + event_box.getFloat("right"));
            Log.d(TAG, "event_box.bottom = " + event_box.getFloat("bottom"));

            final Bundle event_data = intent.getBundleExtra("event_data");
            if (event_data == null) {
                Log.d(TAG, "event_data is empty");
            } else {
                Log.d(TAG, "event_data.size = " + event_data.getInt("data_size"));
                final byte[] data = event_data.getByteArray("data");
                DataInfo dataInfo = new DataInfo();
                dataInfo.frame_width = byteArrayToInt(data, 0);
                dataInfo.frame_height = byteArrayToInt(data, 4);
                dataInfo.frame_idx = byteArrayToInt(data, 8);
                dataInfo.motion_detected = data[12] > 0;
                dataInfo.last_event = data[13] > 0;
                Log.d(TAG, "event_data.data = " + dataInfo.toString());
            }
            Log.d(TAG, "=========== USER_EVENT END ============");
        }
    }

    private Handler mHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_ENABLE_VA:
                    AvcVAManager.getInstance().enableVA(MY_VA_MODULE_NAME);
                    break;
            }
        }
    };

    private static int byteArrayToInt(byte[] b, int offset) {
        return  b[offset+0] & 0xFF |
                (b[offset+1] & 0xFF) << 8  |
                (b[offset+2] & 0xFF) << 16 |
                (b[offset+3] & 0xFF) << 24;
    }

    private class DataInfo {
        int frame_width;
        int frame_height;
        int frame_idx;
        boolean motion_detected;
        boolean last_event;

        @Override
        public String toString() {
            return "DataInfo{" +
                    "frame_width=" + frame_width +
                    ", frame_height=" + frame_height +
                    ", frame_idx=" + frame_idx +
                    ", motion_detected=" + motion_detected +
                    ", last_event=" + last_event +
                    '}';
        }
    }

}
