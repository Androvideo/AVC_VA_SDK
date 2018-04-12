# Native C/C++ VA Tensorflow Object Detection SDK 
---
This example illustrate how to put google object detection model on device

## Getting Started

* Download Tensorflow object detection model ssd_mobilenet_v1_coco
    ```
    ~$ wget http://download.tensorflow.org/models/object_detection/ssd_mobilenet_v1_coco_2017_11_17.tar.gz
    ~$ tar zxvf ssd_mobilenet_v1_coco_2017_11_17.tar.gz
    ~$ mv ssd_mobilenet_v1_coco_2017_11_17/frozen_inference_graph.pb ~/AVC_VA_SDK/VA_TF_SDK/libs
    ```
* Follow the [tf_libary_build.md](tf_libary_build.md) to generate ```libtensorflow.so``` for object detection model
* Move ```libtensorflow.so``` to ```libs```
    ```bash
    ~/AVC_VA_SDK$ cd VA_TF_SDK
    ~/AVC_VA_SDK/VA_TF_SDK$ mv libtensorflow.so libs
    ~/AVC_VA_SDK/VA_TF_SDK$ cd jni
    ~/AVC_VA_SDK/VA_TF_SDK/jni$ ndk-build
    ```
* Connect camera device via adb and remount system before installing the object detection module
    ```bash
    ~/AVC_VA_SDK/VA_TF_SDK$ adb connect 192.168.0.10
    ~/AVC_VA_SDK/VA_TF_SDK$ adb remount
    ```
* Push object detection module, tensorflow libary and model to device
    ```bash
    ~/AVC_VA_SDK/VA_TF_SDK$ adb push libs/libtensorflow.so /vendor/lib/libtensorflow.so
    ~/AVC_VA_SDK/VA_TF_SDK$ adb push libs/frozen_inference_graph.pb /vendor/lib/frozen_inference_graph.pb
    ~/AVC_VA_SDK/VA_TF_SDK$ adb push libs/armeabi-v7a/libva_tf_mobilenet_ssd.so /vendor/lib/libva_tf_mobilenet_ssd.so
    ```
* Update new config file
    ```bash
    ~/AVC_VA_SDK/VA_TF_SDK$ adb push va_config.txt /data/misc/media/va_config.txt
    ~/AVC_VA_SDK/VA_TF_SDK$ adb shell setprop persist.va.config /data/misc/media/va_config.txt
     ~/AVC_VA_SDK/VA_TF_SDK$ adb reboot
    ```
* Remember to enable object detection modules with Java VA helper library
    form  AVC_VA_SDK/VAHelper/VASampleApp/src/main/java/com/androvideo/vasampleapp/VAEventReceiver.java
    change
    ```java
    private static final String MY_VA_MODULE_NAME = "MotionSample";
    ```
    to
    ```java
    private static final String MY_VA_MODULE_NAME = "TfMobilenetSSD";
    ```
## Now you can check object detection result on your camera screen