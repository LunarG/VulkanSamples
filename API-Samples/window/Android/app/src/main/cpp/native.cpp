#include <jni.h>
#include <string>

#include <android_native_app_glue.h>
#include <android/log.h>
#include <vulkan_wrapper.h>

//--- On Android, re-direct printf output to the Android Monitor (LogCat) ---
#define LOG_TAG "WSIWindow"
#define printf(...)  __android_log_print(ANDROID_LOG_INFO ,LOG_TAG,__VA_ARGS__)
#define LOGI(...)    __android_log_print(ANDROID_LOG_INFO ,LOG_TAG,__VA_ARGS__)
#define LOGE(...)    __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
//---------------------------------------------------------------------------

//#include "android_fopen.h"  //for reading files from the asset folder


void android_main(struct android_app* state) {
    app_dummy(); // Make sure glue isn't stripped

    //__android_log_print(ANDROID_LOG_INFO, "APPNAME", "Native Activity !");
    printf("Native Activity !!");

/*
    ANativeActivity* activity = state->activity;
    JNIEnv* env = activity->env;
    jclass clazz = env->GetObjectClass(activity->clazz);
    jmethodID methodID = env->GetMethodID(clazz, "getPackageCodePath", "()Ljava/lang/String;");
    jobject result = env->CallObjectMethod(activity->clazz, methodID);
    jboolean isCopy;
    //const char* str = env->GetStringUTFChars((jstring)result, &isCopy);
    const char* str = env->GetStringUTFChars((jstring)result, NULL);
    //printf("Name = %s/n",str);
*/
    ANativeActivity_finish(state->activity);
}