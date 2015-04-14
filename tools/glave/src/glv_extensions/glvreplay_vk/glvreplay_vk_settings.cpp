/**************************************************************************
 *
 * Copyright 2014 Valve Software, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/
#include <xglLayer.h>

#include "glvreplay_xgl_settings.h"
#include "layers_config.h"
// declared as extern in header
static glvreplay_xgl_settings s_defaultXglReplaySettings = { 1, "DrawState,ObjectTracker",
                                                            STRINGIFY(XGL_DBG_LAYER_LEVEL_ERROR), STRINGIFY(XGL_DBG_LAYER_ACTION_CALLBACK),
                                                            STRINGIFY(XGL_DBG_LAYER_LEVEL_ERROR), STRINGIFY(XGL_DBG_LAYER_ACTION_CALLBACK),
                                                            STRINGIFY(XGL_DBG_LAYER_LEVEL_ERROR), STRINGIFY(XGL_DBG_LAYER_ACTION_CALLBACK)};
glvreplay_xgl_settings g_xglReplaySettings;

glv_SettingInfo g_settings_info[] =
{
    { "dl", "DebugLevel", GLV_SETTING_UINT, &g_xglReplaySettings.debugLevel, &s_defaultXglReplaySettings.debugLevel, FALSE, "Sets the Debug Level of the Mantle validation layers."},
    { "e", "EnableLayers", GLV_SETTING_STRING, &g_xglReplaySettings.enableLayers, &s_defaultXglReplaySettings.enableLayers, TRUE, "Comma separated list of xgl layers to enable."},
    { "dsrl", "DrawStateReportLevel", GLV_SETTING_STRING, &g_xglReplaySettings.drawStateReportLevel, &s_defaultXglReplaySettings.drawStateReportLevel, TRUE, "DrawState Layer reporting level"},
    { "dsda", "DrawStateDebugAction", GLV_SETTING_STRING, &g_xglReplaySettings.drawStateDebugAction, &s_defaultXglReplaySettings.drawStateDebugAction, TRUE, "DrawState Layer debug action"},
    { "mtrl", "MemTrackerReportLevel", GLV_SETTING_STRING, &g_xglReplaySettings.memTrackerReportLevel, &s_defaultXglReplaySettings.memTrackerReportLevel, TRUE, "MemTracker Layer reporting level"},
    { "mtda", "MemTrackerDebugAction", GLV_SETTING_STRING, &g_xglReplaySettings.memTrackerDebugAction, &s_defaultXglReplaySettings.memTrackerDebugAction, TRUE, "MemTracker Layer debug action"},
    { "dsrl", "ObjectTrackerReportLevel", GLV_SETTING_STRING, &g_xglReplaySettings.objectTrackerReportLevel, &s_defaultXglReplaySettings.objectTrackerReportLevel, TRUE, "ObjectTracker Layer reporting level"},
    { "dsda", "ObjectTrackerDebugAction", GLV_SETTING_STRING, &g_xglReplaySettings.objectTrackerDebugAction, &s_defaultXglReplaySettings.objectTrackerDebugAction, TRUE, "ObjectTracker Layer debug action"},};
glv_SettingGroup g_xglReplaySettingGroup =
{
    "glvreplay_xgl",
    sizeof(g_settings_info) / sizeof(g_settings_info[0]),
    &g_settings_info[0]
};

void apply_layerSettings_overrides()
{
    setLayerOptionEnum("DrawStateReportLevel", g_xglReplaySettings.drawStateReportLevel);
    setLayerOptionEnum("DrawStateDebugAction", g_xglReplaySettings.drawStateDebugAction);
    setLayerOptionEnum("MemTrackerReportLevel", g_xglReplaySettings.memTrackerReportLevel);
    setLayerOptionEnum("MemTrackerDebugAction", g_xglReplaySettings.memTrackerDebugAction);
    setLayerOptionEnum("ObjectTrackerReportLevel", g_xglReplaySettings.objectTrackerReportLevel);
    setLayerOptionEnum("ObjectTrackerDebugAction", g_xglReplaySettings.objectTrackerDebugAction);
}

char** get_enableLayers_list(unsigned int *pNumLayers)
{
    char** pList = NULL;
    size_t len = strlen(g_xglReplaySettings.enableLayers);
    assert(pNumLayers != NULL);
    *pNumLayers = 0;

    if (g_xglReplaySettings.enableLayers != NULL && len > 0)
    {
        // The string contains 1 layer + another layer for each comma
        *pNumLayers = 1;
        size_t c;
        int i;

        // count number of commas to determine number of layers
        for (c = 0; c < len; c++)
        {
            if (g_xglReplaySettings.enableLayers[c] == ',')
            {
                (*pNumLayers)++;
            }
        }

        // allocate an array to contain pointers to the layer names
        pList = GLV_NEW_ARRAY(char*, (*pNumLayers));

        // copy the entire string to the first element in the list to keep
        // the layer names localized in memory.
        pList[0] = (char*)glv_allocate_and_copy(g_xglReplaySettings.enableLayers);

        // now walk the string and replace commas with NULL and record
        // the pointers in the pList array.
        i = 1;
        for (c = 0; c < len; c++)
        {
            if (pList[0][c] == ',')
            {
                pList[0][c] = '\0';
                pList[i] = &pList[0][c+1];
                i++;
            }
        }
    }

    return pList;
}

void release_enableLayer_list(char** pList)
{
    if (pList != NULL)
    {
        if (pList[0] != NULL)
        {
            GLV_DELETE(pList[0]);
        }

        GLV_DELETE(pList);
    }
}
