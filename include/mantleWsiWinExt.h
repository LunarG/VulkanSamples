//
// File: mantleWsiWinExt.h
//
// Copyright 2014 ADVANCED MICRO DEVICES, INC.  All Rights Reserved.
//
// AMD is granting you permission to use this software for reference
// purposes only and not for use in any software product.
//
// You agree that you will not reverse engineer or decompile the Materials,
// in whole or in part, except as allowed by applicable law.
//
// WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND.  AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE, NON-INFRINGEMENT, THAT THE SOFTWARE
// WILL RUN UNINTERRUPTED OR ERROR-FREE OR WARRANTIES ARISING FROM CUSTOM OF
// TRADE OR COURSE OF USAGE.  THE ENTIRE RISK ASSOCIATED WITH THE USE OF THE
// SOFTWARE IS ASSUMED BY YOU.
// Some jurisdictions do not allow the exclusion of implied warranties, so
// the above exclusion may not apply to You.
//
// LIMITATION OF LIABILITY AND INDEMNIFICATION:  AMD AND ITS LICENSORS WILL
// NOT, UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT,
// INCIDENTAL, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF
// THE SOFTWARE OR THIS AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
// In no event shall AMD's total liability to You for all damages, losses,
// and causes of action (whether in contract, tort (including negligence) or
// otherwise) exceed the amount of $100 USD.  You agree to defend, indemnify
// and hold harmless AMD and its licensors, and any of their directors,
// officers, employees, affiliates or agents from and against any and all
// loss, damage, liability and other expenses (including reasonable attorneys'
// fees), resulting from Your use of the Software or violation of the terms and
// conditions of this Agreement.
//
// U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED
// RIGHTS." Use, duplication, or disclosure by the Government is subject to the
// restrictions as set forth in FAR 52.227-14 and DFAR252.227-7013, et seq., or
// its successor.  Use of the Materials by the Government constitutes
// acknowledgement of AMD's proprietary rights in them.
//
// EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as
// stated in the Software License Agreement.
//

#ifndef __MANTLEWSIWINEXT_H__
#define __MANTLEWSIWINEXT_H__

#include <windows.h>
#include "mantle.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

GR_DEFINE_SUBCLASS_HANDLE(GR_WSI_WIN_DISPLAY, GR_OBJECT)

#define GR_MAX_DEVICE_NAME_LEN           32
#define GR_MAX_GAMMA_RAMP_CONTROL_POINTS 1025

typedef enum _GR_WSI_WIN_RESULT_CODE
{
    GR_WSI_WIN_PRESENT_OCCLUDED                  = 0x00210000,
    GR_WSI_WIN_ERROR_FULLSCREEN_UNAVAILABLE      = 0x00210001,
    GR_WSI_WIN_ERROR_DISPLAY_REMOVED             = 0x00210002,
    GR_WSI_WIN_ERROR_INCOMPATIBLE_DISPLAY_MODE   = 0x00210003,
    GR_WSI_WIN_ERROR_MULTI_DEVICE_PRESENT_FAILED = 0x00210004,
    GR_WSI_WIN_ERROR_BLT_PRESENT_UNAVAILABLE     = 0x00210005,
    GR_WSI_WIN_ERROR_INVALID_RESOLUTION          = 0x00210006,
} GR_WSI_WIN_RESULT_CODE;

typedef enum _GR_WSI_WIN_IMAGE_STATE
{
    GR_WSI_WIN_PRESENT_SOURCE_BLT  = 0x00200000,
    GR_WSI_WIN_PRESENT_SOURCE_FLIP = 0x00200001,

    GR_WSI_WIN_IMAGE_STATE_BEGIN_RANGE = GR_WSI_WIN_PRESENT_SOURCE_BLT,
    GR_WSI_WIN_IMAGE_STATE_END_RANGE   = GR_WSI_WIN_PRESENT_SOURCE_FLIP,
    GR_NUM_WSI_WIN_IMAGE_STATE         = (GR_WSI_WIN_IMAGE_STATE_END_RANGE - GR_WSI_WIN_IMAGE_STATE_BEGIN_RANGE + 1),
} GR_WSI_WIN_IMAGE_STATE;

typedef enum _GR_WSI_WIN_ROTATION_ANGLE
{
    GR_WSI_WIN_ROTATION_ANGLE_0   = 0x00200100,
    GR_WSI_WIN_ROTATION_ANGLE_90  = 0x00200101,
    GR_WSI_WIN_ROTATION_ANGLE_180 = 0x00200102,
    GR_WSI_WIN_ROTATION_ANGLE_270 = 0x00200103,

    GR_WSI_WIN_ROTATION_ANGLE_BEGIN_RANGE = GR_WSI_WIN_ROTATION_ANGLE_0,
    GR_WSI_WIN_ROTATION_ANGLE_END_RANGE   = GR_WSI_WIN_ROTATION_ANGLE_270,
    GR_NUM_WSI_WIN_ROTATION_ANGLE         = (GR_WSI_WIN_ROTATION_ANGLE_END_RANGE - GR_WSI_WIN_ROTATION_ANGLE_BEGIN_RANGE + 1),
} GR_WSI_WIN_ROTATION_ANGLE;

typedef enum _GR_WSI_WIN_PRESENT_MODE
{
    GR_WSI_WIN_PRESENT_MODE_BLT  = 0x00200200,
    GR_WSI_WIN_PRESENT_MODE_FLIP = 0x00200201,

    GR_WSI_WIN_PRESENT_MODE_BEGIN_RANGE = GR_WSI_WIN_PRESENT_MODE_BLT,
    GR_WSI_WIN_PRESENT_MODE_END_RANGE   = GR_WSI_WIN_PRESENT_MODE_FLIP,
    GR_NUM_WSI_WIN_PRESENT_MODE         = (GR_WSI_WIN_PRESENT_MODE_END_RANGE - GR_WSI_WIN_PRESENT_MODE_BEGIN_RANGE + 1),
} GR_WSI_WIN_PRESENT_MODE;

typedef enum _GR_WSI_WIN_INFO_TYPE
{
    GR_WSI_WIN_INFO_TYPE_QUEUE_PROPERTIES        = 0x00206800,
    GR_WSI_WIN_INFO_TYPE_DISPLAY_PROPERTIES      = 0x00206801,
    GR_WSI_WIN_INFO_TYPE_GAMMA_RAMP_CAPABILITIES = 0x00206802,

    GR_WSI_WIN_INFO_TYPE_BEGIN_RANGE = GR_WSI_WIN_INFO_TYPE_QUEUE_PROPERTIES,
    GR_WSI_WIN_INFO_TYPE_END_RANGE   = GR_WSI_WIN_INFO_TYPE_GAMMA_RAMP_CAPABILITIES,
    GR_NUM_WSI_WIN_INFO_TYPE         = (GR_WSI_WIN_INFO_TYPE_END_RANGE - GR_WSI_WIN_INFO_TYPE_BEGIN_RANGE + 1),
} GR_WSI_WIN_INFO_TYPE;

typedef enum _GR_WSI_WIN_PRESENT_FLAGS
{
    GR_WSI_WIN_PRESENT_FLIP_DONOTWAIT = 0x00000001,
    GR_WSI_WIN_PRESENT_FLIP_STEREO    = 0x00000002,
} GR_WSI_WIN_PRESENT_FLAGS;

typedef enum _GR_WSI_WIN_IMAGE_CREATE_FLAGS
{
    GR_WSI_WIN_IMAGE_CREATE_FLIPPABLE = 0x00000001,
    GR_WSI_WIN_IMAGE_CREATE_STEREO    = 0x00000002,
} GR_WSI_WIN_IMAGE_CREATE_FLAGS;

typedef enum _GR_WSI_WIN_PRESENT_SUPPORT_FLAGS
{
    GR_WSI_WIN_FLIP_PRESENT_SUPPORTED = 0x00000001,
    GR_WSI_WIN_BLT_PRESENT_SUPPORTED  = 0x00000002,
} GR_WSI_WIN_PRESENT_SUPPORT_FLAGS;

typedef struct _GR_WSI_WIN_QUEUE_PROPERTIES
{
    GR_FLAGS presentSupport;            // GR_WSI_WIN_PRESENT_SUPPORT_FLAGS
} GR_WSI_WIN_QUEUE_PROPERTIES;

typedef struct _GR_WSI_WIN_DISPLAY_PROPERTIES
{
    HMONITOR hMonitor;
    GR_CHAR  displayName[GR_MAX_DEVICE_NAME_LEN];
    GR_RECT  desktopCoordinates;
    GR_ENUM  rotation;                  // GR_WSI_WIN_ROTATION_ANGLE
} GR_WSI_WIN_DISPLAY_PROPERTIES;

typedef struct _GR_RGB_FLOAT
{
    GR_FLOAT red;
    GR_FLOAT green;
    GR_FLOAT blue;
} GR_RGB_FLOAT;

typedef struct _GR_WSI_WIN_GAMMA_RAMP_CAPABILITIES
{
    GR_BOOL  supportsScaleAndOffset;
    GR_FLOAT minConvertedValue;
    GR_FLOAT maxConvertedValue;
    GR_UINT  controlPointCount;
    GR_FLOAT controlPointPositions[GR_MAX_GAMMA_RAMP_CONTROL_POINTS];
} GR_WSI_WIN_GAMMA_RAMP_CAPABILITIES;

typedef struct _GR_WSI_WIN_GAMMA_RAMP
{
    GR_RGB_FLOAT scale;
    GR_RGB_FLOAT offset;
    GR_RGB_FLOAT gammaCurve[GR_MAX_GAMMA_RAMP_CONTROL_POINTS];
} GR_WSI_WIN_GAMMA_RAMP;

typedef struct _GR_WSI_WIN_PRESENTABLE_IMAGE_CREATE_INFO
{
    GR_FORMAT          format;
    GR_FLAGS           usage;           // GR_IMAGE_USAGE_FLAGS
    GR_EXTENT2D        extent;
    GR_WSI_WIN_DISPLAY display;
    GR_FLAGS           flags;           // GR_WSI_WIN_IMAGE_CREATE_FLAGS
} GR_WSI_WIN_PRESENTABLE_IMAGE_CREATE_INFO;

typedef struct _GR_WSI_WIN_PRESENT_INFO
{
    HWND     hWndDest;
    GR_IMAGE srcImage;
    GR_ENUM  presentMode;               // GR_WSI_WIN_PRESENT_MODE
    GR_UINT  flipInterval;
    GR_FLAGS flags;                     // GR_WSI_WIN_PRESENT_FLAGS
} GR_WSI_WIN_PRESENT_INFO;

typedef struct _GR_WSI_WIN_DISPLAY_MODE
{
    GR_EXTENT2D extent;
    GR_FORMAT   format;
    GR_UINT     refreshRate;
    GR_BOOL     stereo;
    GR_BOOL     crossDisplayPresent;
} GR_WSI_WIN_DISPLAY_MODE;

GR_RESULT GR_STDCALL grWsiWinGetDisplays(
    GR_DEVICE           device,
    GR_UINT*            pDisplayCount,
    GR_WSI_WIN_DISPLAY* pDisplayList);

GR_RESULT GR_STDCALL grWsiWinGetDisplayModeList(
    GR_WSI_WIN_DISPLAY       display,
    GR_UINT*                 pDisplayModeCount,
    GR_WSI_WIN_DISPLAY_MODE* pDisplayModeList);

GR_RESULT GR_STDCALL grWsiWinTakeFullscreenOwnership(
    GR_WSI_WIN_DISPLAY display,
    GR_IMAGE           image);

GR_RESULT GR_STDCALL grWsiWinReleaseFullscreenOwnership(
    GR_WSI_WIN_DISPLAY display);

GR_RESULT GR_STDCALL grWsiWinSetGammaRamp(
    GR_WSI_WIN_DISPLAY           display,
    const GR_WSI_WIN_GAMMA_RAMP* pGammaRamp);

GR_RESULT GR_STDCALL grWsiWinWaitForVerticalBlank(
    GR_WSI_WIN_DISPLAY display);

GR_RESULT GR_STDCALL grWsiWinGetScanLine(
    GR_WSI_WIN_DISPLAY display,
    GR_INT*            pScanLine);

GR_RESULT GR_STDCALL grWsiWinCreatePresentableImage(
    GR_DEVICE                                       device,
    const GR_WSI_WIN_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    GR_IMAGE*                                       pImage,
    GR_GPU_MEMORY*                                  pMem);

GR_RESULT GR_STDCALL grWsiWinQueuePresent(
    GR_QUEUE                       queue,
    const GR_WSI_WIN_PRESENT_INFO* pPresentInfo);

GR_RESULT GR_STDCALL grWsiWinSetMaxQueuedFrames(
    GR_DEVICE device,
    GR_UINT   maxFrames);

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif // __MANTLEWSIWINEXT_H__
