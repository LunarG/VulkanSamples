#ifndef NATIVE_H
#define NATIVE_H

//--- On Android, re-direct printf output to the Android Monitor (LogCat) ---
//#include <jni.h>
//#include <stdlib.h>
//#include <stdio.h>          //include, and then override printf with logcat macro
//#include <android/log.h>
//#define LOG_TAG "WSIWindow"
//#define printf(...)  __android_log_print(ANDROID_LOG_INFO ,LOG_TAG,__VA_ARGS__)
//#define LOGI(...)    __android_log_print(ANDROID_LOG_INFO ,LOG_TAG,__VA_ARGS__)
//#define LOGE(...)    __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
//---------------------------------------------------------------------------

#include <android_native_app_glue.h>
#include <vulkan_wrapper.h>

#include "android_fopen.h"   // redirect fopen, to read files from asset folder

extern android_app* Android_App;               // Native Activity state info
void ShowKeyboard(bool visible, int flags=0);  // Show/hide Android keyboard
int  GetUnicodeChar(int eventType, int keyCode, int metaState);

#endif