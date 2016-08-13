#include <stdio.h>
#include <WSIWindow.h>

using namespace std;

int main(int argc, char *argv[]){
    printf("WSI-Window\n");

    CInstance Inst;                              //Create a Vulkan Instance
    WSIWindow Window(Inst,"LunarG",640,480);     //Create a Vulkan window

    while(Window.PollEvent()){                   //Window message loop, for handling keyboard & mouse events
        bool KeyPressed = Window.GetKeyState(KEY_LeftShift);
        if (KeyPressed) printf("LEFT SHIFT PRESSED\r");
    }
    return 0;
}
