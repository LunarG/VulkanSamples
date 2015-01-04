"""XGL API description"""

# Copyright (C) 2014 LunarG, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

class Param(object):
    """A function parameter."""

    def __init__(self, ty, name):
        self.ty = ty
        self.name = name

    def c(self):
        """Return the parameter in C."""
        idx = self.ty.find("[")

        # arrays have a different syntax
        if idx >= 0:
            return "%s %s%s" % (self.ty[:idx], self.name, self.ty[idx:])
        else:
            return "%s %s" % (self.ty, self.name)

    def __repr__(self):
        return "Param(\"%s\", \"%s\")" % (self.ty, self.name)

class Proto(object):
    """A function prototype."""

    def __init__(self, ret, name, params=[]):
        # the proto has only a param
        if not isinstance(params, list):
            params = [params]

        self.ret = ret
        self.name = name
        self.params = params

    def c_params(self, need_type=True, need_name=True):
        """Return the parameter list in C."""
        if self.params and (need_type or need_name):
            if need_type and need_name:
                return ", ".join([param.c() for param in self.params])
            elif need_type:
                return ", ".join([param.ty for param in self.params])
            else:
                return ", ".join([param.name for param in self.params])
        else:
            return "void" if need_type else ""

    def c_decl(self, name, attr="", typed=False, need_param_names=True):
        """Return a named declaration in C."""
        format_vals = (self.ret,
                attr + " " if attr else "",
                name,
                self.c_params(need_name=need_param_names))

        if typed:
            return "%s (%s*%s)(%s)" % format_vals
        else:
            return "%s %s%s(%s)" % format_vals

    def c_pretty_decl(self, name, attr=""):
        """Return a named declaration in C, with xgl.h formatting."""
        plist = []
        for param in self.params:
            idx = param.ty.find("[")
            if idx < 0:
                idx = len(param.ty)

            pad = 44 - idx
            if pad <= 0:
                pad = 1

            plist.append("    %s%s%s%s" % (param.ty[:idx],
                " " * pad, param.name, param.ty[idx:]))

        return "%s %s%s(\n%s)" % (self.ret,
                attr + " " if attr else "",
                name,
                ",\n".join(plist))

    def c_typedef(self, suffix="", attr=""):
        """Return the typedef for the prototype in C."""
        return self.c_decl(self.name + suffix, attr=attr, typed=True)

    def c_func(self, prefix="", attr=""):
        """Return the prototype in C."""
        return self.c_decl(prefix + self.name, attr=attr, typed=False)

    def c_call(self):
        """Return a call to the prototype in C."""
        return "%s(%s)" % (self.name, self.c_params(need_type=False))

    def __repr__(self):
        param_strs = []
        for param in self.params:
            param_strs.append(str(param))
        param_str = "    [%s]" % (",\n     ".join(param_strs))

        return "Proto(\"%s\", \"%s\",\n%s)" % \
                (self.ret, self.name, param_str)

class Extension(object):
    def __init__(self, name, headers, objects, protos):
        self.name = name
        self.headers = headers
        self.objects = objects
        self.protos = protos

    def __repr__(self):
        lines = []
        lines.append("Extension(")
        lines.append("    name=\"%s\"," % self.name)
        lines.append("    headers=[\"%s\"]," %
                "\", \"".join(self.headers))

        lines.append("    objects=[")
        for obj in self.objects:
            lines.append("        \"%s\"," % obj)
        lines.append("    ],")

        lines.append("    protos=[")
        for proto in self.protos:
            param_lines = str(proto).splitlines()
            param_lines[-1] += ",\n" if proto != self.protos[-1] else ","
            for p in param_lines:
                lines.append("        " + p)
        lines.append("    ],")
        lines.append(")")

        return "\n".join(lines)

# XGL core API
core = Extension(
    name="XGL_CORE",
    headers=["xgl.h", "xglDbg.h"],
    objects=[
        "XGL_PHYSICAL_GPU",
        "XGL_BASE_OBJECT",
        "XGL_DEVICE",
        "XGL_QUEUE",
        "XGL_GPU_MEMORY",
        "XGL_OBJECT",
        "XGL_IMAGE",
        "XGL_IMAGE_VIEW",
        "XGL_COLOR_ATTACHMENT_VIEW",
        "XGL_DEPTH_STENCIL_VIEW",
        "XGL_SHADER",
        "XGL_PIPELINE",
        "XGL_PIPELINE_DELTA",
        "XGL_SAMPLER",
        "XGL_DESCRIPTOR_SET",
        "XGL_STATE_OBJECT",
        "XGL_VIEWPORT_STATE_OBJECT",
        "XGL_RASTER_STATE_OBJECT",
        "XGL_MSAA_STATE_OBJECT",
        "XGL_COLOR_BLEND_STATE_OBJECT",
        "XGL_DEPTH_STENCIL_STATE_OBJECT",
        "XGL_CMD_BUFFER",
        "XGL_FENCE",
        "XGL_QUEUE_SEMAPHORE",
        "XGL_EVENT",
        "XGL_QUERY_POOL",
    ],
    protos=[
        Proto("XGL_VOID *", "GetProcAddr",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("const XGL_CHAR*", "pName")]),

        Proto("XGL_RESULT", "InitAndEnumerateGpus",
            [Param("const XGL_APPLICATION_INFO*", "pAppInfo"),
             Param("const XGL_ALLOC_CALLBACKS*", "pAllocCb"),
             Param("XGL_UINT", "maxGpus"),
             Param("XGL_UINT*", "pGpuCount"),
             Param("XGL_PHYSICAL_GPU*", "pGpus")]),

        Proto("XGL_RESULT", "GetGpuInfo",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("XGL_PHYSICAL_GPU_INFO_TYPE", "infoType"),
             Param("XGL_SIZE*", "pDataSize"),
             Param("XGL_VOID*", "pData")]),

        Proto("XGL_RESULT", "CreateDevice",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("const XGL_DEVICE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DEVICE*", "pDevice")]),

        Proto("XGL_RESULT", "DestroyDevice",
            [Param("XGL_DEVICE", "device")]),

        Proto("XGL_RESULT", "GetExtensionSupport",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("const XGL_CHAR*", "pExtName")]),

        Proto("XGL_RESULT", "EnumerateLayers",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("XGL_SIZE", "maxLayerCount"),
             Param("XGL_SIZE", "maxStringSize"),
             Param("XGL_CHAR* const*", "pOutLayers"),
             Param("XGL_SIZE *", "pOutLayerCount"),
             Param("XGL_VOID *", "pReserved")]),

        Proto("XGL_RESULT", "GetDeviceQueue",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_QUEUE_TYPE", "queueType"),
             Param("XGL_UINT", "queueIndex"),
             Param("XGL_QUEUE*", "pQueue")]),

        Proto("XGL_RESULT", "QueueSubmit",
            [Param("XGL_QUEUE", "queue"),
             Param("XGL_UINT", "cmdBufferCount"),
             Param("const XGL_CMD_BUFFER*", "pCmdBuffers"),
             Param("XGL_UINT", "memRefCount"),
             Param("const XGL_MEMORY_REF*", "pMemRefs"),
             Param("XGL_FENCE", "fence")]),

        Proto("XGL_RESULT", "QueueSetGlobalMemReferences",
            [Param("XGL_QUEUE", "queue"),
             Param("XGL_UINT", "memRefCount"),
             Param("const XGL_MEMORY_REF*", "pMemRefs")]),

        Proto("XGL_RESULT", "QueueWaitIdle",
            [Param("XGL_QUEUE", "queue")]),

        Proto("XGL_RESULT", "DeviceWaitIdle",
            [Param("XGL_DEVICE", "device")]),

        Proto("XGL_RESULT", "GetMemoryHeapCount",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_UINT*", "pCount")]),

        Proto("XGL_RESULT", "GetMemoryHeapInfo",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_UINT", "heapId"),
             Param("XGL_MEMORY_HEAP_INFO_TYPE", "infoType"),
             Param("XGL_SIZE*", "pDataSize"),
             Param("XGL_VOID*", "pData")]),

        Proto("XGL_RESULT", "AllocMemory",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_MEMORY_ALLOC_INFO*", "pAllocInfo"),
             Param("XGL_GPU_MEMORY*", "pMem")]),

        Proto("XGL_RESULT", "FreeMemory",
            [Param("XGL_GPU_MEMORY", "mem")]),

        Proto("XGL_RESULT", "SetMemoryPriority",
            [Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_MEMORY_PRIORITY", "priority")]),

        Proto("XGL_RESULT", "MapMemory",
            [Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_FLAGS", "flags"),
             Param("XGL_VOID**", "ppData")]),

        Proto("XGL_RESULT", "UnmapMemory",
            [Param("XGL_GPU_MEMORY", "mem")]),

        Proto("XGL_RESULT", "PinSystemMemory",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_VOID*", "pSysMem"),
             Param("XGL_SIZE", "memSize"),
             Param("XGL_GPU_MEMORY*", "pMem")]),

        Proto("XGL_RESULT", "RemapVirtualMemoryPages",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_UINT", "rangeCount"),
             Param("const XGL_VIRTUAL_MEMORY_REMAP_RANGE*", "pRanges"),
             Param("XGL_UINT", "preWaitSemaphoreCount"),
             Param("const XGL_QUEUE_SEMAPHORE*", "pPreWaitSemaphores"),
             Param("XGL_UINT", "postSignalSemaphoreCount"),
             Param("const XGL_QUEUE_SEMAPHORE*", "pPostSignalSemaphores")]),

        Proto("XGL_RESULT", "GetMultiGpuCompatibility",
            [Param("XGL_PHYSICAL_GPU", "gpu0"),
             Param("XGL_PHYSICAL_GPU", "gpu1"),
             Param("XGL_GPU_COMPATIBILITY_INFO*", "pInfo")]),

        Proto("XGL_RESULT", "OpenSharedMemory",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_MEMORY_OPEN_INFO*", "pOpenInfo"),
             Param("XGL_GPU_MEMORY*", "pMem")]),

        Proto("XGL_RESULT", "OpenSharedQueueSemaphore",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_QUEUE_SEMAPHORE_OPEN_INFO*", "pOpenInfo"),
             Param("XGL_QUEUE_SEMAPHORE*", "pSemaphore")]),

        Proto("XGL_RESULT", "OpenPeerMemory",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_PEER_MEMORY_OPEN_INFO*", "pOpenInfo"),
             Param("XGL_GPU_MEMORY*", "pMem")]),

        Proto("XGL_RESULT", "OpenPeerImage",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_PEER_IMAGE_OPEN_INFO*", "pOpenInfo"),
             Param("XGL_IMAGE*", "pImage"),
             Param("XGL_GPU_MEMORY*", "pMem")]),

        Proto("XGL_RESULT", "DestroyObject",
            [Param("XGL_OBJECT", "object")]),

        Proto("XGL_RESULT", "GetObjectInfo",
            [Param("XGL_BASE_OBJECT", "object"),
             Param("XGL_OBJECT_INFO_TYPE", "infoType"),
             Param("XGL_SIZE*", "pDataSize"),
             Param("XGL_VOID*", "pData")]),

        Proto("XGL_RESULT", "BindObjectMemory",
            [Param("XGL_OBJECT", "object"),
             Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_GPU_SIZE", "offset")]),

        Proto("XGL_RESULT", "CreateFence",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_FENCE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_FENCE*", "pFence")]),

        Proto("XGL_RESULT", "GetFenceStatus",
            [Param("XGL_FENCE", "fence")]),

        Proto("XGL_RESULT", "WaitForFences",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_UINT", "fenceCount"),
             Param("const XGL_FENCE*", "pFences"),
             Param("XGL_BOOL", "waitAll"),
             Param("XGL_UINT64", "timeout")]),

        Proto("XGL_RESULT", "CreateQueueSemaphore",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_QUEUE_SEMAPHORE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_QUEUE_SEMAPHORE*", "pSemaphore")]),

        Proto("XGL_RESULT", "SignalQueueSemaphore",
            [Param("XGL_QUEUE", "queue"),
             Param("XGL_QUEUE_SEMAPHORE", "semaphore")]),

        Proto("XGL_RESULT", "WaitQueueSemaphore",
            [Param("XGL_QUEUE", "queue"),
             Param("XGL_QUEUE_SEMAPHORE", "semaphore")]),

        Proto("XGL_RESULT", "CreateEvent",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_EVENT_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_EVENT*", "pEvent")]),

        Proto("XGL_RESULT", "GetEventStatus",
            [Param("XGL_EVENT", "event")]),

        Proto("XGL_RESULT", "SetEvent",
            [Param("XGL_EVENT", "event")]),

        Proto("XGL_RESULT", "ResetEvent",
            [Param("XGL_EVENT", "event")]),

        Proto("XGL_RESULT", "CreateQueryPool",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_QUERY_POOL_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_QUERY_POOL*", "pQueryPool")]),

        Proto("XGL_RESULT", "GetQueryPoolResults",
            [Param("XGL_QUERY_POOL", "queryPool"),
             Param("XGL_UINT", "startQuery"),
             Param("XGL_UINT", "queryCount"),
             Param("XGL_SIZE*", "pDataSize"),
             Param("XGL_VOID*", "pData")]),

        Proto("XGL_RESULT", "GetFormatInfo",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_FORMAT", "format"),
             Param("XGL_FORMAT_INFO_TYPE", "infoType"),
             Param("XGL_SIZE*", "pDataSize"),
             Param("XGL_VOID*", "pData")]),

        Proto("XGL_RESULT", "CreateImage",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_IMAGE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_IMAGE*", "pImage")]),

        Proto("XGL_RESULT", "GetImageSubresourceInfo",
            [Param("XGL_IMAGE", "image"),
             Param("const XGL_IMAGE_SUBRESOURCE*", "pSubresource"),
             Param("XGL_SUBRESOURCE_INFO_TYPE", "infoType"),
             Param("XGL_SIZE*", "pDataSize"),
             Param("XGL_VOID*", "pData")]),

        Proto("XGL_RESULT", "CreateImageView",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_IMAGE_VIEW_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_IMAGE_VIEW*", "pView")]),

        Proto("XGL_RESULT", "CreateColorAttachmentView",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_COLOR_ATTACHMENT_VIEW*", "pView")]),

        Proto("XGL_RESULT", "CreateDepthStencilView",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DEPTH_STENCIL_VIEW*", "pView")]),

        Proto("XGL_RESULT", "CreateShader",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_SHADER_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_SHADER*", "pShader")]),

        Proto("XGL_RESULT", "CreateGraphicsPipeline",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_GRAPHICS_PIPELINE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_PIPELINE*", "pPipeline")]),

        Proto("XGL_RESULT", "CreateComputePipeline",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_COMPUTE_PIPELINE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_PIPELINE*", "pPipeline")]),

        Proto("XGL_RESULT", "StorePipeline",
            [Param("XGL_PIPELINE", "pipeline"),
             Param("XGL_SIZE*", "pDataSize"),
             Param("XGL_VOID*", "pData")]),

        Proto("XGL_RESULT", "LoadPipeline",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_SIZE", "dataSize"),
             Param("const XGL_VOID*", "pData"),
             Param("XGL_PIPELINE*", "pPipeline")]),

        Proto("XGL_RESULT", "CreatePipelineDelta",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_PIPELINE", "p1"),
             Param("XGL_PIPELINE", "p2"),
             Param("XGL_PIPELINE_DELTA*", "delta")]),

        Proto("XGL_RESULT", "CreateSampler",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_SAMPLER_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_SAMPLER*", "pSampler")]),

        Proto("XGL_RESULT", "CreateDescriptorSet",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_DESCRIPTOR_SET_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DESCRIPTOR_SET*", "pDescriptorSet")]),

        Proto("XGL_VOID", "BeginDescriptorSetUpdate",
            [Param("XGL_DESCRIPTOR_SET", "descriptorSet")]),

        Proto("XGL_VOID", "EndDescriptorSetUpdate",
            [Param("XGL_DESCRIPTOR_SET", "descriptorSet")]),

        Proto("XGL_VOID", "AttachSamplerDescriptors",
            [Param("XGL_DESCRIPTOR_SET", "descriptorSet"),
             Param("XGL_UINT", "startSlot"),
             Param("XGL_UINT", "slotCount"),
             Param("const XGL_SAMPLER*", "pSamplers")]),

        Proto("XGL_VOID", "AttachImageViewDescriptors",
            [Param("XGL_DESCRIPTOR_SET", "descriptorSet"),
             Param("XGL_UINT", "startSlot"),
             Param("XGL_UINT", "slotCount"),
             Param("const XGL_IMAGE_VIEW_ATTACH_INFO*", "pImageViews")]),

        Proto("XGL_VOID", "AttachMemoryViewDescriptors",
            [Param("XGL_DESCRIPTOR_SET", "descriptorSet"),
             Param("XGL_UINT", "startSlot"),
             Param("XGL_UINT", "slotCount"),
             Param("const XGL_MEMORY_VIEW_ATTACH_INFO*", "pMemViews")]),

        Proto("XGL_VOID", "AttachNestedDescriptors",
            [Param("XGL_DESCRIPTOR_SET", "descriptorSet"),
             Param("XGL_UINT", "startSlot"),
             Param("XGL_UINT", "slotCount"),
             Param("const XGL_DESCRIPTOR_SET_ATTACH_INFO*", "pNestedDescriptorSets")]),

        Proto("XGL_VOID", "ClearDescriptorSetSlots",
            [Param("XGL_DESCRIPTOR_SET", "descriptorSet"),
             Param("XGL_UINT", "startSlot"),
             Param("XGL_UINT", "slotCount")]),

        Proto("XGL_RESULT", "CreateViewportState",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_VIEWPORT_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_VIEWPORT_STATE_OBJECT*", "pState")]),

        Proto("XGL_RESULT", "CreateRasterState",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_RASTER_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_RASTER_STATE_OBJECT*", "pState")]),

        Proto("XGL_RESULT", "CreateMsaaState",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_MSAA_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_MSAA_STATE_OBJECT*", "pState")]),

        Proto("XGL_RESULT", "CreateColorBlendState",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_COLOR_BLEND_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_COLOR_BLEND_STATE_OBJECT*", "pState")]),

        Proto("XGL_RESULT", "CreateDepthStencilState",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_DEPTH_STENCIL_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DEPTH_STENCIL_STATE_OBJECT*", "pState")]),

        Proto("XGL_RESULT", "CreateCommandBuffer",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_CMD_BUFFER_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_CMD_BUFFER*", "pCmdBuffer")]),

        Proto("XGL_RESULT", "BeginCommandBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_FLAGS", "flags")]),

        Proto("XGL_RESULT", "EndCommandBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer")]),

        Proto("XGL_RESULT", "ResetCommandBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer")]),

        Proto("XGL_VOID", "CmdBindPipeline",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("XGL_PIPELINE", "pipeline")]),

        Proto("XGL_VOID", "CmdBindPipelineDelta",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("XGL_PIPELINE_DELTA", "delta")]),

        Proto("XGL_VOID", "CmdBindStateObject",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_STATE_BIND_POINT", "stateBindPoint"),
             Param("XGL_STATE_OBJECT", "state")]),

        Proto("XGL_VOID", "CmdBindDescriptorSet",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("XGL_UINT", "index"),
             Param("XGL_DESCRIPTOR_SET", "descriptorSet"),
             Param("XGL_UINT", "slotOffset")]),

        Proto("XGL_VOID", "CmdBindDynamicMemoryView",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("const XGL_MEMORY_VIEW_ATTACH_INFO*", "pMemView")]),

        Proto("XGL_VOID", "CmdBindVertexData",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_GPU_SIZE", "offset"),
             Param("XGL_UINT", "binding")]),

        Proto("XGL_VOID", "CmdBindIndexData",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_GPU_SIZE", "offset"),
             Param("XGL_INDEX_TYPE", "indexType")]),

        Proto("XGL_VOID", "CmdBindAttachments",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_UINT", "colorAttachmentCount"),
             Param("const XGL_COLOR_ATTACHMENT_BIND_INFO*", "pColorAttachments"),
             Param("const XGL_DEPTH_STENCIL_BIND_INFO*", "pDepthStencilAttachment")]),

        Proto("XGL_VOID", "CmdPrepareMemoryRegions",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_UINT", "transitionCount"),
             Param("const XGL_MEMORY_STATE_TRANSITION*", "pStateTransitions")]),

        Proto("XGL_VOID", "CmdPrepareImages",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_UINT", "transitionCount"),
             Param("const XGL_IMAGE_STATE_TRANSITION*", "pStateTransitions")]),

        Proto("XGL_VOID", "CmdDraw",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_UINT", "firstVertex"),
             Param("XGL_UINT", "vertexCount"),
             Param("XGL_UINT", "firstInstance"),
             Param("XGL_UINT", "instanceCount")]),

        Proto("XGL_VOID", "CmdDrawIndexed",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_UINT", "firstIndex"),
             Param("XGL_UINT", "indexCount"),
             Param("XGL_INT", "vertexOffset"),
             Param("XGL_UINT", "firstInstance"),
             Param("XGL_UINT", "instanceCount")]),

        Proto("XGL_VOID", "CmdDrawIndirect",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_GPU_SIZE", "offset"),
             Param("XGL_UINT32", "count"),
             Param("XGL_UINT32", "stride")]),

        Proto("XGL_VOID", "CmdDrawIndexedIndirect",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_GPU_SIZE", "offset"),
             Param("XGL_UINT32", "count"),
             Param("XGL_UINT32", "stride")]),

        Proto("XGL_VOID", "CmdDispatch",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_UINT", "x"),
             Param("XGL_UINT", "y"),
             Param("XGL_UINT", "z")]),

        Proto("XGL_VOID", "CmdDispatchIndirect",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_GPU_SIZE", "offset")]),

        Proto("XGL_VOID", "CmdCopyMemory",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_GPU_MEMORY", "srcMem"),
             Param("XGL_GPU_MEMORY", "destMem"),
             Param("XGL_UINT", "regionCount"),
             Param("const XGL_MEMORY_COPY*", "pRegions")]),

        Proto("XGL_VOID", "CmdCopyImage",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "srcImage"),
             Param("XGL_IMAGE", "destImage"),
             Param("XGL_UINT", "regionCount"),
             Param("const XGL_IMAGE_COPY*", "pRegions")]),

        Proto("XGL_VOID", "CmdCopyMemoryToImage",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_GPU_MEMORY", "srcMem"),
             Param("XGL_IMAGE", "destImage"),
             Param("XGL_UINT", "regionCount"),
             Param("const XGL_MEMORY_IMAGE_COPY*", "pRegions")]),

        Proto("XGL_VOID", "CmdCopyImageToMemory",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "srcImage"),
             Param("XGL_GPU_MEMORY", "destMem"),
             Param("XGL_UINT", "regionCount"),
             Param("const XGL_MEMORY_IMAGE_COPY*", "pRegions")]),

        Proto("XGL_VOID", "CmdCloneImageData",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "srcImage"),
             Param("XGL_IMAGE_STATE", "srcImageState"),
             Param("XGL_IMAGE", "destImage"),
             Param("XGL_IMAGE_STATE", "destImageState")]),

        Proto("XGL_VOID", "CmdUpdateMemory",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_GPU_MEMORY", "destMem"),
             Param("XGL_GPU_SIZE", "destOffset"),
             Param("XGL_GPU_SIZE", "dataSize"),
             Param("const XGL_UINT32*", "pData")]),

        Proto("XGL_VOID", "CmdFillMemory",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_GPU_MEMORY", "destMem"),
             Param("XGL_GPU_SIZE", "destOffset"),
             Param("XGL_GPU_SIZE", "fillSize"),
             Param("XGL_UINT32", "data")]),

        Proto("XGL_VOID", "CmdClearColorImage",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "image"),
             Param("const XGL_FLOAT[4]", "color"),
             Param("XGL_UINT", "rangeCount"),
             Param("const XGL_IMAGE_SUBRESOURCE_RANGE*", "pRanges")]),

        Proto("XGL_VOID", "CmdClearColorImageRaw",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "image"),
             Param("const XGL_UINT32[4]", "color"),
             Param("XGL_UINT", "rangeCount"),
             Param("const XGL_IMAGE_SUBRESOURCE_RANGE*", "pRanges")]),

        Proto("XGL_VOID", "CmdClearDepthStencil",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "image"),
             Param("XGL_FLOAT", "depth"),
             Param("XGL_UINT32", "stencil"),
             Param("XGL_UINT", "rangeCount"),
             Param("const XGL_IMAGE_SUBRESOURCE_RANGE*", "pRanges")]),

        Proto("XGL_VOID", "CmdResolveImage",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "srcImage"),
             Param("XGL_IMAGE", "destImage"),
             Param("XGL_UINT", "rectCount"),
             Param("const XGL_IMAGE_RESOLVE*", "pRects")]),

        Proto("XGL_VOID", "CmdSetEvent",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_EVENT", "event")]),

        Proto("XGL_VOID", "CmdResetEvent",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_EVENT", "event")]),

        Proto("XGL_VOID", "CmdMemoryAtomic",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_GPU_MEMORY", "destMem"),
             Param("XGL_GPU_SIZE", "destOffset"),
             Param("XGL_UINT64", "srcData"),
             Param("XGL_ATOMIC_OP", "atomicOp")]),

        Proto("XGL_VOID", "CmdBeginQuery",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_QUERY_POOL", "queryPool"),
             Param("XGL_UINT", "slot"),
             Param("XGL_FLAGS", "flags")]),

        Proto("XGL_VOID", "CmdEndQuery",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_QUERY_POOL", "queryPool"),
             Param("XGL_UINT", "slot")]),

        Proto("XGL_VOID", "CmdResetQueryPool",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_QUERY_POOL", "queryPool"),
             Param("XGL_UINT", "startQuery"),
             Param("XGL_UINT", "queryCount")]),

        Proto("XGL_VOID", "CmdWriteTimestamp",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_TIMESTAMP_TYPE", "timestampType"),
             Param("XGL_GPU_MEMORY", "destMem"),
             Param("XGL_GPU_SIZE", "destOffset")]),

        Proto("XGL_VOID", "CmdInitAtomicCounters",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("XGL_UINT", "startCounter"),
             Param("XGL_UINT", "counterCount"),
             Param("const XGL_UINT32*", "pData")]),

        Proto("XGL_VOID", "CmdLoadAtomicCounters",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("XGL_UINT", "startCounter"),
             Param("XGL_UINT", "counterCount"),
             Param("XGL_GPU_MEMORY", "srcMem"),
             Param("XGL_GPU_SIZE", "srcOffset")]),

        Proto("XGL_VOID", "CmdSaveAtomicCounters",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("XGL_UINT", "startCounter"),
             Param("XGL_UINT", "counterCount"),
             Param("XGL_GPU_MEMORY", "destMem"),
             Param("XGL_GPU_SIZE", "destOffset")]),

        Proto("XGL_RESULT", "DbgSetValidationLevel",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_VALIDATION_LEVEL", "validationLevel")]),

        Proto("XGL_RESULT", "DbgRegisterMsgCallback",
            [Param("XGL_DBG_MSG_CALLBACK_FUNCTION", "pfnMsgCallback"),
             Param("XGL_VOID*", "pUserData")]),

        Proto("XGL_RESULT", "DbgUnregisterMsgCallback",
            [Param("XGL_DBG_MSG_CALLBACK_FUNCTION", "pfnMsgCallback")]),

        Proto("XGL_RESULT", "DbgSetMessageFilter",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_INT", "msgCode"),
             Param("XGL_DBG_MSG_FILTER", "filter")]),

        Proto("XGL_RESULT", "DbgSetObjectTag",
            [Param("XGL_BASE_OBJECT", "object"),
             Param("XGL_SIZE", "tagSize"),
             Param("const XGL_VOID*", "pTag")]),

        Proto("XGL_RESULT", "DbgSetGlobalOption",
            [Param("XGL_DBG_GLOBAL_OPTION", "dbgOption"),
             Param("XGL_SIZE", "dataSize"),
             Param("const XGL_VOID*", "pData")]),

        Proto("XGL_RESULT", "DbgSetDeviceOption",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_DBG_DEVICE_OPTION", "dbgOption"),
             Param("XGL_SIZE", "dataSize"),
             Param("const XGL_VOID*", "pData")]),

        Proto("XGL_VOID", "CmdDbgMarkerBegin",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("const XGL_CHAR*", "pMarker")]),

        Proto("XGL_VOID", "CmdDbgMarkerEnd",
            [Param("XGL_CMD_BUFFER", "cmdBuffer")]),
    ],
)

wsi_x11 = Extension(
    name="XGL_WSI_X11",
    headers=["xglWsiX11Ext.h"],
    objects=[],
    protos=[
        Proto("XGL_RESULT", "WsiX11AssociateConnection",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("const XGL_WSI_X11_CONNECTION_INFO*", "pConnectionInfo")]),

        Proto("XGL_RESULT", "WsiX11GetMSC",
            [Param("XGL_DEVICE", "device"),
             Param("xcb_window_t", "window"),
             Param("xcb_randr_crtc_t", "crtc"),
             Param("XGL_UINT64*", "pMsc")]),

        Proto("XGL_RESULT", "WsiX11CreatePresentableImage",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_IMAGE*", "pImage"),
             Param("XGL_GPU_MEMORY*", "pMem")]),

        Proto("XGL_RESULT", "WsiX11QueuePresent",
            [Param("XGL_QUEUE", "queue"),
             Param("const XGL_WSI_X11_PRESENT_INFO*", "pPresentInfo"),
             Param("XGL_FENCE", "fence")]),
    ],
)

extensions = [core, wsi_x11]

headers = []
objects = []
protos = []
for ext in extensions:
    headers.extend(ext.headers)
    objects.extend(ext.objects)
    protos.extend(ext.protos)

proto_names = [proto.name for proto in protos]

def does_function_create_object(name):
    return name in (
        "CreateDevice",
        "GetDeviceQueue",
        "AllocMemory",
        "PinSystemMemory",
        "OpenSharedMemory",
        "OpenSharedQueueSemaphore",
        "OpenPeerMemory",
        "OpenPeerImage",
        "CreateFence",
        "CreateQueueSemaphore",
        "CreateEvent",
        "CreateQueryPool",
        "CreateImage",
        "CreateImageView",
        "CreateColorAttachmentView",
        "CreateDepthStencilView",
        "CreateShader",
        "CreateGraphicsPipeline",
        "CreateComputePipeline",
        "LoadPipeline",
        "CreatePipelineDelta",
        "CreateSampler",
        "CreateDescriptorSet",
        "CreateViewportState",
        "CreateRasterState",
        "CreateMsaaState",
        "CreateColorBlendState",
        "CreateDepthStencilState",
        "CreateCommandBuffer",
        "WsiX11CreatePresentableImage")

def is_name_dispatchable(name):
    return name not in (
        "GetProcAddr",
        "InitAndEnumerateGpus",
        "EnumerateLayers",
        "DbgRegisterMsgCallback",
        "DbgUnregisterMsgCallback",
        "DbgSetGlobalOption")

def is_dispatchable(proto):
    """Return true if the prototype is dispatchable.

    That is, return true when the prototype takes a XGL_PHYSICAL_GPU or
    XGL_BASE_OBJECT.
    """
    return is_name_dispatchable(proto.name)

def parse_xgl_h(filename):
    # read object and protoype typedefs
    object_lines = []
    proto_lines = []
    with open(filename, "r") as fp:
        for line in fp:
            line = line.strip()
            if line.startswith("XGL_DEFINE"):
                begin = line.find("(") + 1
                end = line.find(",")
                # extract the object type
                object_lines.append(line[begin:end])
            if line.startswith("typedef") and line.endswith(");"):
                # drop leading "typedef " and trailing ");"
                proto_lines.append(line[8:-2])

    # parse proto_lines to protos
    protos = []
    for line in proto_lines:
        first, rest = line.split(" (XGLAPI *")
        second, third = rest.split("Type)(")

        # get the return type, no space before "*"
        proto_ret = "*".join([t.rstrip() for t in first.split("*")])

        # get the name
        proto_name = second.strip()

        # get the list of params
        param_strs = third.split(", ")
        params = []
        for s in param_strs:
            ty, name = s.rsplit(" ", 1)

            # no space before "*"
            ty = "*".join([t.rstrip() for t in ty.split("*")])
            # attach [] to ty
            idx = name.rfind("[")
            if idx >= 0:
                ty += name[idx:]
                name = name[:idx]

            params.append(Param(ty, name))

        protos.append(Proto(proto_ret, proto_name, params))

    # make them an extension and print
    ext = Extension("XGL_CORE",
            headers=["xgl.h", "xglDbg.h"],
            objects=object_lines,
            protos=protos)
    print("core =", str(ext))

    print("")
    print("typedef struct _XGL_LAYER_DISPATCH_TABLE")
    print("{")
    for proto in ext.protos:
        print("    %sType %s;" % (proto.name, proto.name))
    print("} XGL_LAYER_DISPATCH_TABLE;")

if __name__ == "__main__":
    parse_xgl_h("include/xgl.h")
