#include <stdio.h>
#include "WSIWindow/WSIWindow.h"

using namespace std;

//-- USING VIRTUAL FUNCTION EVENT HANDLERS --
class MyWindow : public WSIWindow{
    using WSIWindow::WSIWindow;     //Inherit base constructor

    //--Mouse event handler--
    void OnMouseEvent(eMouseAction action, int16_t x, int16_t y, uint8_t btn){
        const char* type[]={"move","down","up  "};
        printf("Mouse: %s %d x %d Btn:%d\n",type[action],x,y,btn);
    }

    //--Keyboard event handler--
    void OnKeyEvent(eKeyAction action,uint8_t keycode){
        const char* type[]={"down","up  "};
        printf("Key: %s keycode:%d\n",type[action],keycode);
    }

    //--Text typed event handler--
    void OnTextEvent(const char* str){
        printf("Text: %s\n",str);
    }

    //--Window move/resize event handler--
    void OnShapeEvent(int16_t x, int16_t y, uint16_t width, uint16_t height){
        printf("Shape: x=%4d y=%4d width=%4d height=%4d\n",x,y,width, height);
    }

};


int main(int argc, char *argv[]){
    setbuf(stdout, NULL);          //Prevent printf buffering in QtCreator
    printf("WSI-Window\n");

    CInstance Inst;                             //Create a Vulkan Instance
    MyWindow Window(Inst,"LunarG",640,480);     //Create a Vulkan window

    while(Window.ProcessEvents()){
        bool KeyPressed = Window.GetKeyState(KEY_LeftShift);
        if (KeyPressed) printf("LEFT SHIFT PRESSED\r");
    }
    return 0;
}
