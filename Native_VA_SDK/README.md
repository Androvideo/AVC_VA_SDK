# Native SDK

## Build VA module

- Download and setup Android NDK build environment

export PATH=$PATH:<NDK_PATH>

ndk-build

found libs/armeabi-v7a/libxxx.so


## Install VA module

- Connect to adb via usb or tcpip

adb connect 192.168.0.10

- Set va config path once

adb shell setprop persist.va.config /data/misc/media/va_config.txt

- Add your VA module in va_config.txt

- Install lib and config files

adb remount

adb push libs/armeabi-v7a/libxxx.so /vendor/lib/

adb push va_config.txt /data/misc/media/va_config.txt

- Reboot device

adb reboot


## Verify your VA module

- Check your VA lib is loaded

- VA module is default disabled. Use Java VA Helper library to enable your VA module.
