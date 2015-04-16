/* IN DEVELOPMENT.  DO NOT SHIP. */

#ifndef __VKWSIX11EXT_H__
#define __VKWSIX11EXT_H__

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include "vulkan.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef struct _VK_WSI_X11_CONNECTION_INFO {
    xcb_connection_t*                           pConnection;
    xcb_window_t                                root;
    xcb_randr_provider_t                        provider;
} VK_WSI_X11_CONNECTION_INFO;

typedef struct _VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO
{
    VkFormat          format;
    VkFlags           usage;           // VkImageUsageFlags
    VkExtent2D        extent;
    VkFlags           flags;
} VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO;

typedef struct _VK_WSI_X11_PRESENT_INFO
{
    /* which window to present to */
    xcb_window_t destWindow;
    VkImage srcImage;

    /**
     * After the command buffers in the queue have been completed, if the MSC
     * of \p crtc is less than or equal to \p target_msc, wait until it
     * reaches \p target_msc.
     *
     * If the current MSC of \p crtc is greater than \p target_msc, adjust
     * \p target_msc as following:
     *
     *   if (divisor) {
     *       target_msc = crtc_msc - (crtc_msc % divisor) + remainder;
     *       if (target_msc < crtc_msc)
     *           target_msc += divisor;
     *   } else {
     *       target_msc = crtc_msc;
     *   }
     *
     * In other words, either set \p target_msc to an absolute value (require
     * vkWsiX11GetMSC(), potentially a round-trip to the server, to get the
     * current MSC first), or set \p target_msc to zero and set a "swap
     * interval".
     *
     * \p crtc can be XCB_NONE.  In that case, a suitable CRTC is picked based
     * on \p destWindow.
     */
    xcb_randr_crtc_t crtc;
    uint64_t target_msc;
    uint64_t divisor;
    uint64_t remainder;

    /**
     * After waiting for the current and target MSCs to match, the
     * presentation is scheduled.  When \p async is false, it will occur the
     * next time current MSC is incremented.  When \p async is true, it will
     * occur as soon as possible.
     */
    bool32_t async;

    /**
     * When \p flip is false, the contents of \p srcImage are copied to
     * \p destWindow when the presentation occurs.  When \p flip is true,
     * \p srcImage is made the front buffer of \p destWindow.
     *
     * An error may be returned if \p flip is true but \p destWindow can not
     * be flipped to.
     */
    bool32_t flip;
} VK_WSI_X11_PRESENT_INFO;

typedef VkResult (VKAPI *PFN_vkWsiX11AssociateConnection)(VkPhysicalGpu gpu, const VK_WSI_X11_CONNECTION_INFO* pConnectionInfo);
typedef VkResult (VKAPI *PFN_vkWsiX11GetMSC)(VkDevice device, xcb_window_t window, xcb_randr_crtc_t crtc, uint64_t* pMsc);
typedef VkResult (VKAPI *PFN_vkWsiX11CreatePresentableImage)(VkDevice device, const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, VkImage* pImage, VkGpuMemory* pMem);
typedef VkResult (VKAPI *PFN_vkWsiX11QueuePresent)(VkQueue queue, const VK_WSI_X11_PRESENT_INFO* pPresentInfo, VkFence fence);

/**
 * Associate an X11 connection with a GPU.  This should be done before device
 * creation.  If the device is already created,
 * VK_ERROR_DEVICE_ALREADY_CREATED is returned.
 *
 * Truth is, given a connection, we could find the associated GPU.  But
 * without having a GPU as the first parameter, the loader could not find the
 * dispatch table.
 *
 * This function is available when vkGetGlobalExtensionInfo says "VK_WSI_X11"
 * is supported.
 */
VkResult VKAPI vkWsiX11AssociateConnection(
    VkPhysicalGpu                            gpu,
    const VK_WSI_X11_CONNECTION_INFO*          pConnectionInfo);

/**
 * Return the current MSC (Media Stream Counter, incremented for each vblank)
 * of \p crtc.  If crtc is \p XCB_NONE, a suitable CRTC is picked based on \p
 * win.
 */
VkResult VKAPI vkWsiX11GetMSC(
    VkDevice                                  device,
    xcb_window_t                                window,
    xcb_randr_crtc_t                            crtc,
    uint64_t*                                   pMsc);

/**
 * Create an VkImage that can be presented.  An VkGpuMemory is created
 * and bound automatically.  The memory returned can only be used in
 * vkQueue[Add|Remove]MemReference.  Destroying the memory or binding another memory to the
 * image is not allowed.
 */
VkResult VKAPI vkWsiX11CreatePresentableImage(
    VkDevice                                  device,
    const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    VkImage*                                  pImage,
    VkGpuMemory*                             pMem);

/**
 * Present an image to an X11 window.  The presentation always occurs after
 * the command buffers in the queue have been completed, subject to other
 * parameters specified in VK_WSI_X11_PRESENT_INFO.
 *
 * Fence is reached when the presentation occurs.
 */
VkResult VKAPI vkWsiX11QueuePresent(
    VkQueue                                   queue,
    const VK_WSI_X11_PRESENT_INFO*             pPresentInfo,
    VkFence                                   fence);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __VKWSIX11EXT_H__
