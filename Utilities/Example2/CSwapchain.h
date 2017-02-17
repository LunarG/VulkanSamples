/*
*  This unit wraps the swapchain.
*
*  WARNING: This unit is a work in progress.
*  Interfaces are highly experimental and very likely to change.
*/

#ifndef CSWAPCHAIN_H
#define CSWAPCHAIN_H

#include "CDevices.h"

#ifdef ANDROID
#define IS_ANDROID true   // ANDROID: default to power-save (limit to 60fps)
#else
#define IS_ANDROID false  // PC: default to low-latency (no fps limit)
#endif

struct BackBuffer {   // Remove this struct?
//    uint32_t    image_index;
    VkSemaphore acquire_semaphore;
    VkSemaphore render_semaphore;
    VkFence     present_fence;  // signaled when this buffer is ready for reuse
};

//struct SwapchainBuffer {
struct CFrameBuffer {
    VkImage       image;
    VkImageView   view;
    VkFramebuffer buffer;
};

class CSwapchain {
    VkPresentModeKHR   mode;
  public:
    CSwapchain(CDevice* device, VkSurfaceKHR surface, uint32_t image_count = 3);
    ~CSwapchain();

    CDevice*           device;
    VkSurfaceKHR       surface;      // TODO: Replace with CSurface, or move to CDevice?
    VkSurfaceFormatKHR format;       // TODO: Move this to CSurface?
    VkSwapchainKHR     swapchain;    //
    VkExtent2D         extent;       // TODO: Move this to CSurface?

    std::vector<CFrameBuffer> framebuffers;

    //std::vector<BackBuffer> back_buffers;  // ?
    //BackBuffer*    acquired_back_buffer = 0;

    bool PresentMode(bool no_tearing, bool powersave = IS_ANDROID);  // ANDROID: default to power-saves (limit to 60fps)
    bool PresentMode(VkPresentModeKHR preferred_mode);               // If mode is not available, returns false and uses FIFO.

    void Init(CDevice* device, VkSurfaceKHR surface, uint32_t image_count = 3);  // default to 3 buffers(tripple-buffering)
    void Resize(uint width = 0, uint height = 0);

    //void CreateBackBuffers(const int count);
    //void DestroyBackBuffers();


    void CreateFramebuffers(const int count);


    //void AcquireBackBuffer();
    void AcquireNextImage();

    void Print();
};

#endif
