/* IN DEVELOPMENT.  DO NOT SHIP. */

#ifndef __XGLINTELEXT_H__
#define __XGLINTELEXT_H__

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include "xgl.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef enum _XGL_INTEL_STRUCTURE_TYPE
{
    XGL_INTEL_STRUCTURE_TYPE_SHADER_CREATE_INFO             = 1000,
} XGL_INTEL_STRUCTURE_TYPE;

typedef struct _XGL_INTEL_COMPILE_GLSL
{
    XGL_PIPELINE_SHADER_STAGE       stage;
    const char                     *pCode;
} XGL_INTEL_COMPILE_GLSL;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __XGLINTELEXT_H__
