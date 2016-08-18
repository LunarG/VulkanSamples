#include <stdio.h>
#include <WSIWindow.h>

using namespace std;

void MouseEvent(eMouseAction action, int16_t x, int16_t y, uint8_t btn){  //Mouse event handler
    const char* type[]={"move","down","up  "};
    printf("Mouse %s %d x %d Btn:%d\n",type[action],x,y,btn); fflush(stdout);
}

void KeyEvent(eKeyAction action,uint8_t keycode){                         //Keyboard event handler
    const char* type[]={"down","up  "};
    printf("Key %s keycode:%d\n",type[action],keycode); fflush(stdout);
}

void TextEvent(const char* str){                                          //Text typed event handler
    printf("Text: %s\n",str); fflush(stdout);
}

void ShapeEvent(int16_t x, int16_t y, uint16_t width, uint16_t height){   //Window move/resize event handler
    printf("Shape: x=%4d y=%4d width=%4d height=%4d\n",x,y,width, height); fflush(stdout);
}

int main(int argc, char *argv[]){
    printf("WSI-Window\n");

    CInstance Inst;                              //Create a Vulkan Instance
    WSIWindow Window(Inst,"LunarG",640,480);     //Create a Vulkan window
    Window.OnMouseEvent=MouseEvent;              //Register callback function for mouse events
    Window.OnKeyEvent  =KeyEvent;                //Register callback function for keyboard events
    Window.OnTextEvent =TextEvent;               //Register callback function for text typed
    Window.OnShapeEvent=ShapeEvent;              //Register callback function for window move/resize

    while(Window.ProcessEvents()){
        bool KeyPressed = Window.GetKeyState(KEY_LeftShift);
        if (KeyPressed) printf("LEFT SHIFT PRESSED\r");
         //printf(".");  fflush(stdout);
    }


    return 0;
}
