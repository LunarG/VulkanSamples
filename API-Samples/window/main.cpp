#include <stdio.h>
#include <WSIWindow.h>

using namespace std;

int main(int argc, char *argv[])
{
    printf("WSI-Window\n");

    //LunarG::WSIWindow window();
    //WSIWindow window("WSI-Window",640,480);
    //window.Test();

    CInstance Inst;
//    Window_xcb xcb(Inst,"XCB",640,480);
//    while(xcb.PollEvent()){
//    }


    WSIWindow Window(Inst,"LunarG",640,480);
    while(Window.PollEvent()){
    }

    return 0;
}
