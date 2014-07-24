//
// File: mantleWsiWinExtDbg.h
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

#ifndef __MANTLEWSIWINEXTDBG_H__
#define __MANTLEWSIWINEXTDBG_H__

#include "mantle.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef enum _GR_WSI_WIN_DBG_OBJECT_TYPE
{
    GR_WSI_WIN_DBG_OBJECT_DISPLAY           = 0x00210900,
    GR_WSI_WIN_DBG_OBJECT_PRESENTABLE_IMAGE = 0x00210901,

    GR_WSI_WIN_DBG_OBJECT_TYPE_BEGIN_RANGE = GR_WSI_WIN_DBG_OBJECT_DISPLAY,
    GR_WSI_WIN_DBG_OBJECT_TYPE_END_RANGE   = GR_WSI_WIN_DBG_OBJECT_PRESENTABLE_IMAGE,
    GR_NUM_WSI_WIN_DBG_OBJECT_TYPE         = (GR_WSI_WIN_DBG_OBJECT_TYPE_END_RANGE - GR_WSI_WIN_DBG_OBJECT_TYPE_BEGIN_RANGE + 1),
} GR_WSI_WIN_DBG_OBJECT_TYPE;

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif // __MANTLEWSIWINEXTDBG_H__
