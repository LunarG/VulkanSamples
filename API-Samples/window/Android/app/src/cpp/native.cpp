#include "native.h"

int main(int argc, char *argv[]);  //Forward declaration of main function

void android_main(struct android_app* state) {
    app_dummy(); // Make sure glue isn't stripped
    printf("Native Activity\n");
    int success=InitVulkan();
    printf("InitVulkan : %s\n",success ? "SUCCESS" : "FAILED");

    main(0,NULL);
    printf("Exiting.\n");



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