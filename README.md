# AVC Android Camera Video Analytics SDK

[![Build Status](https://api.travis-ci.org/Androvideo/AVC_VA_SDK.svg?branch=master)](https://travis-ci.org/Androvideo/AVC_VA_SDK/)

This repository contains native C/C++ VA SDK, Java helper library, and a Motion-Track example module. With this SDK, you can build your own VA modules for AndroVideo camera devices.


## Native C/C++ VA SDK

The AVC native C/C++ VA SDK contains code that facilitate building video analytics modules for AndroVideo camera devices.

- [Native VA SDK Template](https://github.com/Androvideo/AVC_VA_SDK/tree/master/Native_VA_SDK) (An Empty template for a video analytics  module)
- [Native VA MotionTrack Sample](https://github.com/Androvideo/AVC_VA_SDK/tree/master/VA_Motion_Sample) (An example with motion-track video analytics sample code)

### Getting Started


- Download the [Android NDK package](https://developer.android.com/ndk/downloads/index.html) (Recommend to download the latest stable version - r16b)
- Download the [Native VA MotionTrack Sample](https://github.com/Androvideo/AVC_VA_SDK/tree/master/VA_Motion_Sample)
- Setup PATH env

```sh
$ export PATH=$PATH:YOUR_NDK_PATH
```

- Go into the working directory and Run ndk-build command

```sh
$ ndk-build
#You will find libva_motion_sample.so in libs/armeabi-v7a
```

### Install your VA module

- Connect camera device via adb

```sh
$ adb connect 192.168.0.10
$ adb devices
List of devices attached
192.168.0.10:5555    device
```

- Remount system before installing the VA module

```sh
$ adb remount
```

- Push VA module to device

```sh
$ adb push libs/armeabi-v7a/libva_motion_sample.so /vendor/lib/libva_motion_sample.so
```

- Update new VA config file

```sh
$ adb push va_config.txt /data/misc/media/va_config.txt

# Set va config path once
$ adb shell setprop persist.va.config /data/misc/media/va_config.txt

# Reboot device to enable new setting
$ adb reboot
```

- After rebooting, you can see your VA lib is loaded via logcat log

```sh
$ adb logcat
D/VABroker(  425 ): int32_t qipcam::VABroker::init(): VAEngines[2].libName = /vendor/lib/libva_motion_sample.so
D/VABroker(  425 ): int32_t qipcam::VABroker::init(): module_api_version = 0x1001
```


## Build Android App with Java VA helper library

The AVC Java help library helps you to build Android applications that can enable/disable your own VA modules and receive your own VA events for further processing.

- [Java Helper Lib](https://github.com/Androvideo/AVC_VA_SDK/tree/master/VAHelper/VAHelperLib)
- [Android Sample App](https://github.com/Androvideo/AVC_VA_SDK/tree/master/VAHelper/VASampleApp)

### Getting Started

- Prepare an Android App development environment (https://developer.android.com)
- Download the [Android Sample App](https://github.com/Androvideo/AVC_VA_SDK/tree/master/VAHelper/VASampleApp)
- Build and install the App to device
- You can see your VA module is enable via logcat log
```sh
$ adb logcat
I/AVCLog.IpcamBroadcastRecv( 2418 ): onReceive com.androvideo.va.action.USER_CONFIG
D/AVCIPCam( 2418 ): int32_t qipcam::AVCIPCamService::enableVA(android::String8, int): Enter MotionSample 1
I/va_motion_sample(  423 ): int32_t motiontrack_init(void**):
I/va_motion_sample(  423 ): int32_t motiontrack_get_supported_events(void*, int32_t*, int32_t*):

```

## FAQ
