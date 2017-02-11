/*
*--------------------------------------------------------------------------
* Copyright (c) 2015-2016 Valve Corporation
* Copyright (c) 2015-2016 LunarG, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Author: Rene Lindsay <rene@lunarg.com>
*
*/

//----------------------------------------------------------------------------------------------------
//  The android_main() function is the entry-point for Android, and calls the user's main() function.
//  But first, it initialises the asset manager, so that fopen can be used to read assets from the APK.
//
//  The ShowKeyboard() function displays the Android soft-keyboard.
//  The GetUnicodeChar() function converts key-stroke input to text.
//
//  (Please excuse the uncommented code... This unit is a work in progress.)
//----------------------------------------------------------------------------------------------------

#include "native.h"

//----------------------------------------printf for Android----------------------------------------
// Uses a 256 byte buffer to allow concatenating multiple printf's onto one log line.
// The buffer gets flushed when the printf string ends in a '\n', or the buffer is full.
// Alternative with no concatenation:
//   #define printf(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG,__VA_ARGS__)

struct printBuf {
    static const int SIZE = 256;
    char buf[SIZE];
    printBuf() { clear(); }
    printBuf(const char* c) {
        memset(buf, 0, SIZE);
        strncpy(buf, c, SIZE - 1);
    }
    printBuf& operator+=(const char* c) {
        strncat(buf, c, SIZE - len() - 1);
        if (len() >= SIZE - 1) flush();
        return *this;
    }
    int len() { return strlen(buf); }
    void clear() { memset(buf, 0, SIZE); }
    void flush() {
        __android_log_print(ANDROID_LOG_INFO, "WSIWindow", "%s", buf);
        clear();
    }
} printBuf;

int printf(const char* format, ...) {  // printf for Android
    char buf[printBuf.SIZE];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(buf, sizeof(buf), format, argptr);
    va_end(argptr);
    printBuf += buf;
    int len = strlen(buf);
    if ((len >= printBuf.SIZE - 1) || (buf[len - 1] == '\n')) printBuf.flush();  // flush on
    return strlen(buf);
}
//--------------------------------------------------------------------------------------------------

android_app* Android_App = 0;  // Android native-actvity state
/*
//--------------------TEMP------------------------
//--Window event handler--
static void handle_cmd(struct android_app* app, int32_t cmd) {
    //printf(" -> handle_cmd");
    switch(cmd){
        case APP_CMD_INPUT_CHANGED  : printf("APP_CMD_INPUT_CHANGED");  break;
        case APP_CMD_INIT_WINDOW    : printf("APP_CMD_INIT_WINDOW");    break;
        case APP_CMD_TERM_WINDOW    : printf("APP_CMD_TERM_WINDOW");    break;
        case APP_CMD_WINDOW_RESIZED       : printf("APP_CMD_WINDOW_RESIZED");       break;
        case APP_CMD_WINDOW_REDRAW_NEEDED : printf("APP_CMD_WINDOW_REDRAW_NEEDED"); break;
        case APP_CMD_CONTENT_RECT_CHANGED : printf("APP_CMD_CONTENT_RECT_CHANGED"); break;
        case APP_CMD_GAINED_FOCUS   : printf("APP_CMD_GAINED_FOCUS");   break;
        case APP_CMD_LOST_FOCUS     : printf("APP_CMD_LOST_FOCUS");     break;
        case APP_CMD_CONFIG_CHANGED : printf("APP_CMD_CONFIG_CHANGED"); break;
        case APP_CMD_LOW_MEMORY     : printf("APP_CMD_LOW_MEMORY");     break;
        case APP_CMD_START          : printf("APP_CMD_START");          break;
        case APP_CMD_RESUME         : printf("APP_CMD_RESUME");         break;
        case APP_CMD_SAVE_STATE     : printf("APP_CMD_SAVE_STATE");     break;
        case APP_CMD_PAUSE          : printf("APP_CMD_PAUSE");          break;
        case APP_CMD_STOP           : printf("APP_CMD_STOP");           break;
        case APP_CMD_DESTROY        : printf("APP_CMD_DESTROY");        break;
        default : printf("handle_cmd : UNKNOWN EVENT");
    }
}

//--Input event handler--
static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    //printf(" -> handle_input\n");
    int32_t type=AInputEvent_getType(event);
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        float mx = AMotionEvent_getX(event, 0);
        float my = AMotionEvent_getY(event, 0);
        printf("%f x %f\n",mx,my);
        return 1;
    }
    return 0;
}
//------------------------------------------------
*/
//====================Main====================
int main(int argc, char *argv[]);  // Forward declaration of main function

void android_main(struct android_app *state) {
    printf("Native Activity\n");
    app_dummy();  // Make sure glue isn't stripped
    // state->onAppCmd     = handle_cmd;      // Register window event callback  (Temporary)
    // state->onInputEvent = handle_input;    // Register input event callback   (Temporary)
    Android_App = state;  // Pass android app state to window_andoid.cpp

    android_fopen_set_asset_manager(state->activity->assetManager);  // Re-direct fopen to read assets from our APK.

    // int success=InitVulkan();
    // printf("InitVulkan : %s\n",success ? "SUCCESS" : "FAILED");

    main(0, NULL);

    printf("Exiting.\n");
    ANativeActivity_finish(state->activity);
}
//============================================

//========================UGLY JNI code for showing the Keyboard========================

#define CALL_OBJ_METHOD(OBJ, METHOD, SIGNATURE, ...) \
    jniEnv->CallObjectMethod(OBJ, jniEnv->GetMethodID(jniEnv->GetObjectClass(OBJ), METHOD, SIGNATURE), __VA_ARGS__)
#define CALL_BOOL_METHOD(OBJ, METHOD, SIGNATURE, ...) \
    jniEnv->CallBooleanMethod(OBJ, jniEnv->GetMethodID(jniEnv->GetObjectClass(OBJ), METHOD, SIGNATURE), __VA_ARGS__)

void ShowKeyboard(bool visible, int flags) {
    // Attach current thread to the JVM.
    JavaVM *javaVM = Android_App->activity->vm;
    JNIEnv *jniEnv = Android_App->activity->env;
    JavaVMAttachArgs Args = {JNI_VERSION_1_6, "NativeThread", NULL};
    javaVM->AttachCurrentThread(&jniEnv, &Args);

    // Retrieve NativeActivity.
    jobject lNativeActivity = Android_App->activity->clazz;

    // Retrieve Context.INPUT_METHOD_SERVICE.
    jclass ClassContext = jniEnv->FindClass("android/content/Context");
    jfieldID FieldINPUT_METHOD_SERVICE = jniEnv->GetStaticFieldID(ClassContext, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jobject INPUT_METHOD_SERVICE = jniEnv->GetStaticObjectField(ClassContext, FieldINPUT_METHOD_SERVICE);

    // getSystemService(Context.INPUT_METHOD_SERVICE).
    jobject lInputMethodManager =
        CALL_OBJ_METHOD(lNativeActivity, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", INPUT_METHOD_SERVICE);

    // getWindow().getDecorView().
    jobject lWindow = CALL_OBJ_METHOD(lNativeActivity, "getWindow", "()Landroid/view/Window;", 0);
    jobject lDecorView = CALL_OBJ_METHOD(lWindow, "getDecorView", "()Landroid/view/View;", 0);
    if (visible) {
        jboolean lResult = CALL_BOOL_METHOD(lInputMethodManager, "showSoftInput", "(Landroid/view/View;I)Z", lDecorView, flags);
    } else {
        jobject lBinder = CALL_OBJ_METHOD(lDecorView, "getWindowToken", "()Landroid/os/IBinder;", 0);
        jboolean lResult =
            CALL_BOOL_METHOD(lInputMethodManager, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z", lBinder, flags);
    }
    // Finished with the JVM.
    javaVM->DetachCurrentThread();
}
//======================================================================================

//===============================Get Unicode from Keyboard==============================
int GetUnicodeChar(int eventType, int keyCode, int metaState) {
    JavaVM *javaVM = Android_App->activity->vm;
    JNIEnv *jniEnv = Android_App->activity->env;

    JavaVMAttachArgs Args = {JNI_VERSION_1_6, "NativeThread", NULL};
    jint result = javaVM->AttachCurrentThread(&jniEnv, &Args);
    if (result == JNI_ERR) return 0;

    jclass class_key_event = jniEnv->FindClass("android/view/KeyEvent");

    jmethodID method_get_unicode_char = jniEnv->GetMethodID(class_key_event, "getUnicodeChar", "(I)I");
    jmethodID eventConstructor = jniEnv->GetMethodID(class_key_event, "<init>", "(II)V");
    jobject eventObj = jniEnv->NewObject(class_key_event, eventConstructor, eventType, keyCode);
    int unicodeKey = jniEnv->CallIntMethod(eventObj, method_get_unicode_char, metaState);

    javaVM->DetachCurrentThread();

    // LOGI("Keycode: %d  MetaState: %d Unicode: %d", keyCode, metaState, unicodeKey);
    return unicodeKey;
}
//======================================================================================