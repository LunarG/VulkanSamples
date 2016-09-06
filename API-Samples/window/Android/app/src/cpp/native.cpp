#include "native.h"

android_app* Android_App=0;                //Android native-actvity state

/*
//--Window event handler--
static void handle_cmd(struct android_app* app, int32_t cmd) {
    //printf(" -> handle_cmd");
    switch(cmd){
        case APP_CMD_INPUT_CHANGED  : printf("APP_CMD_INPUT_CHANGED");  break;
        case APP_CMD_INIT_WINDOW    : printf("APP_CMD_INIT_WINDOW");    break;
        case APP_CMD_TERM_WINDOW    : printf("APP_CMD_TERM_WINDOW");    break;
        //case APP_CMD_WINDOW_RESIZED       : printf("APP_CMD_WINDOW_RESIZED");       break;
        //case APP_CMD_WINDOW_REDRAW_NEEDED : printf("APP_CMD_WINDOW_REDRAW_NEEDED"); break;
        //case APP_CMD_CONTENT_RECT_CHANGED : printf("APP_CMD_CONTENT_RECT_CHANGED"); break;
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
*/
//====================Main====================
int main(int argc, char *argv[]);          //Forward declaration of main function

void android_main(struct android_app* state) {
    printf("Native Activity\n");
    app_dummy();                           // Make sure glue isn't stripped
    //state->onAppCmd     = handle_cmd;      // Register window event callback
    //state->onInputEvent = handle_input;    // Register input event callback
    Android_App=state;                     // Pass android app state to window_andoid.cpp

    int success=InitVulkan();
    printf("InitVulkan : %s\n",success ? "SUCCESS" : "FAILED");

    main(0,NULL);

    printf("Exiting.\n");
    ANativeActivity_finish(state->activity);
}
//============================================