/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <stdbool.h>

static VK_LAYER_DBG_FUNCTION_NODE *g_pDbgFunctionHead = NULL;
static VK_LAYER_DBG_REPORT_LEVEL g_reportingLevel = VK_DBG_LAYER_LEVEL_INFO;
static VK_LAYER_DBG_ACTION g_debugAction = VK_DBG_LAYER_ACTION_LOG_MSG;
static bool g_actionIsDefault = true;
static FILE *g_logFile = NULL;

// Utility function to handle reporting
//  If callbacks are enabled, use them, otherwise use printf
static void layerCbMsg(VK_DBG_MSG_TYPE msgType,
    VkValidationLevel validationLevel,
    VkBaseObject      srcObject,
    size_t               location,
    int32_t              msgCode,
    const char*          pLayerPrefix,
    const char*          pMsg)
{
    if (g_logFile == NULL) {
	g_logFile = stdout;
    }

    if (g_debugAction & (VK_DBG_LAYER_ACTION_LOG_MSG | VK_DBG_LAYER_ACTION_CALLBACK)) {
        VK_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;
        switch (msgType) {
            case VK_DBG_MSG_ERROR:
                if (g_reportingLevel <= VK_DBG_LAYER_LEVEL_ERROR) {
                    if (g_debugAction & VK_DBG_LAYER_ACTION_LOG_MSG) {
                        fprintf(g_logFile, "{%s}ERROR : %s\n", pLayerPrefix, pMsg);
                        fflush(g_logFile);
                    }
                    if (g_debugAction & VK_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
				            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
            case VK_DBG_MSG_WARNING:
                if (g_reportingLevel <= VK_DBG_LAYER_LEVEL_WARN) {
                    if (g_debugAction & VK_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}WARN : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & VK_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
				            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
            case VK_DBG_MSG_PERF_WARNING:
                if (g_reportingLevel <= VK_DBG_LAYER_LEVEL_PERF_WARN) {
                    if (g_debugAction & VK_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}PERF_WARN : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & VK_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
				            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
            default:
                if (g_reportingLevel <= VK_DBG_LAYER_LEVEL_INFO) {
                    if (g_debugAction & VK_DBG_LAYER_ACTION_LOG_MSG)
                        fprintf(g_logFile, "{%s}INFO : %s\n", pLayerPrefix, pMsg);
                    if (g_debugAction & VK_DBG_LAYER_ACTION_CALLBACK)
                        while (pTrav) {
				            pTrav->pfnMsgCallback(msgType, validationLevel, srcObject, location, msgCode, pMsg, pTrav->pUserData);
                            pTrav = pTrav->pNext;
                        }
                }
                break;
        }
    }
}
