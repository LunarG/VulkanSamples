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

    def indirection_level(self):
        """Return the level of indirection."""
        return self.ty.count("*") + self.ty.count("[")

    def dereferenced_type(self, level=0):
        """Return the type after dereferencing."""
        if not level:
            level = self.indirection_level()

        deref = self.ty if level else ""
        while level > 0:
            idx = deref.rfind("[")
            if idx < 0:
                idx = deref.rfind("*")
            if idx < 0:
                deref = ""
                break
            deref = deref[:idx]
            level -= 1;

        return deref.rstrip()

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

    def object_in_params(self):
        """Return the params that are simple XGL objects and are inputs."""
        return [param for param in self.params if param.ty in objects]

    def object_out_params(self):
        """Return the params that are simple XGL objects and are outputs."""
        return [param for param in self.params
                if param.dereferenced_type() in objects]

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
        "XGL_INSTANCE",
        "XGL_PHYSICAL_GPU",
        "XGL_BASE_OBJECT",
        "XGL_DEVICE",
        "XGL_QUEUE",
        "XGL_GPU_MEMORY",
        "XGL_OBJECT",
        "XGL_BUFFER",
        "XGL_BUFFER_VIEW",
        "XGL_IMAGE",
        "XGL_IMAGE_VIEW",
        "XGL_COLOR_ATTACHMENT_VIEW",
        "XGL_DEPTH_STENCIL_VIEW",
        "XGL_SHADER",
        "XGL_PIPELINE",
        "XGL_SAMPLER",
        "XGL_DESCRIPTOR_SET",
        "XGL_DESCRIPTOR_SET_LAYOUT",
        "XGL_DESCRIPTOR_SET_LAYOUT_CHAIN",
        "XGL_DESCRIPTOR_POOL",
        "XGL_DYNAMIC_STATE_OBJECT",
        "XGL_DYNAMIC_VP_STATE_OBJECT",
        "XGL_DYNAMIC_RS_STATE_OBJECT",
        "XGL_DYNAMIC_CB_STATE_OBJECT",
        "XGL_DYNAMIC_DS_STATE_OBJECT",
        "XGL_CMD_BUFFER",
        "XGL_FENCE",
        "XGL_SEMAPHORE",
        "XGL_EVENT",
        "XGL_QUERY_POOL",
        "XGL_FRAMEBUFFER",
        "XGL_RENDER_PASS",
    ],
    protos=[
        Proto("XGL_RESULT", "CreateInstance",
            [Param("const XGL_APPLICATION_INFO*", "pAppInfo"),
             Param("const XGL_ALLOC_CALLBACKS*", "pAllocCb"),
             Param("XGL_INSTANCE*", "pInstance")]),

        Proto("XGL_RESULT", "DestroyInstance",
            [Param("XGL_INSTANCE", "instance")]),

        Proto("XGL_RESULT", "EnumerateGpus",
            [Param("XGL_INSTANCE", "instance"),
             Param("uint32_t", "maxGpus"),
             Param("uint32_t*", "pGpuCount"),
             Param("XGL_PHYSICAL_GPU*", "pGpus")]),

        Proto("XGL_RESULT", "GetGpuInfo",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("XGL_PHYSICAL_GPU_INFO_TYPE", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("void*", "GetProcAddr",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("const char*", "pName")]),

        Proto("XGL_RESULT", "CreateDevice",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("const XGL_DEVICE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DEVICE*", "pDevice")]),

        Proto("XGL_RESULT", "DestroyDevice",
            [Param("XGL_DEVICE", "device")]),

        Proto("XGL_RESULT", "GetExtensionSupport",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("const char*", "pExtName")]),

        Proto("XGL_RESULT", "EnumerateLayers",
            [Param("XGL_PHYSICAL_GPU", "gpu"),
             Param("size_t", "maxLayerCount"),
             Param("size_t", "maxStringSize"),
             Param("size_t*", "pOutLayerCount"),
             Param("char* const*", "pOutLayers"),
             Param("void*", "pReserved")]),

        Proto("XGL_RESULT", "GetDeviceQueue",
            [Param("XGL_DEVICE", "device"),
             Param("uint32_t", "queueNodeIndex"),
             Param("uint32_t", "queueIndex"),
             Param("XGL_QUEUE*", "pQueue")]),

        Proto("XGL_RESULT", "QueueSubmit",
            [Param("XGL_QUEUE", "queue"),
             Param("uint32_t", "cmdBufferCount"),
             Param("const XGL_CMD_BUFFER*", "pCmdBuffers"),
             Param("uint32_t", "memRefCount"),
             Param("const XGL_MEMORY_REF*", "pMemRefs"),
             Param("XGL_FENCE", "fence")]),

        Proto("XGL_RESULT", "QueueSetGlobalMemReferences",
            [Param("XGL_QUEUE", "queue"),
             Param("uint32_t", "memRefCount"),
             Param("const XGL_MEMORY_REF*", "pMemRefs")]),

        Proto("XGL_RESULT", "QueueWaitIdle",
            [Param("XGL_QUEUE", "queue")]),

        Proto("XGL_RESULT", "DeviceWaitIdle",
            [Param("XGL_DEVICE", "device")]),

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
             Param("void**", "ppData")]),

        Proto("XGL_RESULT", "UnmapMemory",
            [Param("XGL_GPU_MEMORY", "mem")]),

        Proto("XGL_RESULT", "PinSystemMemory",
            [Param("XGL_DEVICE", "device"),
             Param("const void*", "pSysMem"),
             Param("size_t", "memSize"),
             Param("XGL_GPU_MEMORY*", "pMem")]),

        Proto("XGL_RESULT", "GetMultiGpuCompatibility",
            [Param("XGL_PHYSICAL_GPU", "gpu0"),
             Param("XGL_PHYSICAL_GPU", "gpu1"),
             Param("XGL_GPU_COMPATIBILITY_INFO*", "pInfo")]),

        Proto("XGL_RESULT", "OpenSharedMemory",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_MEMORY_OPEN_INFO*", "pOpenInfo"),
             Param("XGL_GPU_MEMORY*", "pMem")]),

        Proto("XGL_RESULT", "OpenSharedSemaphore",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_SEMAPHORE_OPEN_INFO*", "pOpenInfo"),
             Param("XGL_SEMAPHORE*", "pSemaphore")]),

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
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("XGL_RESULT", "BindObjectMemory",
            [Param("XGL_OBJECT", "object"),
             Param("uint32_t", "allocationIdx"),
             Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_GPU_SIZE", "offset")]),

        Proto("XGL_RESULT", "BindObjectMemoryRange",
            [Param("XGL_OBJECT", "object"),
             Param("uint32_t", "allocationIdx"),
             Param("XGL_GPU_SIZE", "rangeOffset"),
             Param("XGL_GPU_SIZE", "rangeSize"),
             Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_GPU_SIZE", "memOffset")]),

        Proto("XGL_RESULT", "BindImageMemoryRange",
            [Param("XGL_IMAGE", "image"),
             Param("uint32_t", "allocationIdx"),
             Param("const XGL_IMAGE_MEMORY_BIND_INFO*", "bindInfo"),
             Param("XGL_GPU_MEMORY", "mem"),
             Param("XGL_GPU_SIZE", "memOffset")]),

        Proto("XGL_RESULT", "CreateFence",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_FENCE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_FENCE*", "pFence")]),

        Proto("XGL_RESULT", "GetFenceStatus",
            [Param("XGL_FENCE", "fence")]),

        Proto("XGL_RESULT", "WaitForFences",
            [Param("XGL_DEVICE", "device"),
             Param("uint32_t", "fenceCount"),
             Param("const XGL_FENCE*", "pFences"),
             Param("bool32_t", "waitAll"),
             Param("uint64_t", "timeout")]),

        Proto("XGL_RESULT", "CreateSemaphore",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_SEMAPHORE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_SEMAPHORE*", "pSemaphore")]),

        Proto("XGL_RESULT", "QueueSignalSemaphore",
            [Param("XGL_QUEUE", "queue"),
             Param("XGL_SEMAPHORE", "semaphore")]),

        Proto("XGL_RESULT", "QueueWaitSemaphore",
            [Param("XGL_QUEUE", "queue"),
             Param("XGL_SEMAPHORE", "semaphore")]),

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
             Param("uint32_t", "startQuery"),
             Param("uint32_t", "queryCount"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("XGL_RESULT", "GetFormatInfo",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_FORMAT", "format"),
             Param("XGL_FORMAT_INFO_TYPE", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("XGL_RESULT", "CreateBuffer",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_BUFFER_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_BUFFER*", "pBuffer")]),

        Proto("XGL_RESULT", "CreateBufferView",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_BUFFER_VIEW_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_BUFFER_VIEW*", "pView")]),

        Proto("XGL_RESULT", "CreateImage",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_IMAGE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_IMAGE*", "pImage")]),

        Proto("XGL_RESULT", "GetImageSubresourceInfo",
            [Param("XGL_IMAGE", "image"),
             Param("const XGL_IMAGE_SUBRESOURCE*", "pSubresource"),
             Param("XGL_SUBRESOURCE_INFO_TYPE", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

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

        Proto("XGL_RESULT", "CreateGraphicsPipelineDerivative",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_GRAPHICS_PIPELINE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_PIPELINE", "basePipeline"),
             Param("XGL_PIPELINE*", "pPipeline")]),

        Proto("XGL_RESULT", "CreateComputePipeline",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_COMPUTE_PIPELINE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_PIPELINE*", "pPipeline")]),

        Proto("XGL_RESULT", "StorePipeline",
            [Param("XGL_PIPELINE", "pipeline"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("XGL_RESULT", "LoadPipeline",
            [Param("XGL_DEVICE", "device"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData"),
             Param("XGL_PIPELINE*", "pPipeline")]),

        Proto("XGL_RESULT", "LoadPipelineDerivative",
            [Param("XGL_DEVICE", "device"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData"),
             Param("XGL_PIPELINE", "basePipeline"),
             Param("XGL_PIPELINE*", "pPipeline")]),

        Proto("XGL_RESULT", "CreateSampler",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_SAMPLER_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_SAMPLER*", "pSampler")]),

        Proto("XGL_RESULT", "CreateDescriptorSetLayout",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DESCRIPTOR_SET_LAYOUT*", "pSetLayout")]),

        Proto("XGL_RESULT", "CreateDescriptorSetLayoutChain",
            [Param("XGL_DEVICE", "device"),
             Param("uint32_t", "setLayoutArrayCount"),
             Param("const XGL_DESCRIPTOR_SET_LAYOUT*", "pSetLayoutArray"),
             Param("XGL_DESCRIPTOR_SET_LAYOUT_CHAIN*", "pLayoutChain")]),

        Proto("XGL_RESULT", "BeginDescriptorPoolUpdate",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_DESCRIPTOR_UPDATE_MODE", "updateMode")]),

        Proto("XGL_RESULT", "EndDescriptorPoolUpdate",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_CMD_BUFFER", "cmd")]),

        Proto("XGL_RESULT", "CreateDescriptorPool",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_DESCRIPTOR_POOL_USAGE", "poolUsage"),
             Param("uint32_t", "maxSets"),
             Param("const XGL_DESCRIPTOR_POOL_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DESCRIPTOR_POOL*", "pDescriptorPool")]),

        Proto("XGL_RESULT", "ResetDescriptorPool",
            [Param("XGL_DESCRIPTOR_POOL", "descriptorPool")]),

        Proto("XGL_RESULT", "AllocDescriptorSets",
            [Param("XGL_DESCRIPTOR_POOL", "descriptorPool"),
             Param("XGL_DESCRIPTOR_SET_USAGE", "setUsage"),
             Param("uint32_t", "count"),
             Param("const XGL_DESCRIPTOR_SET_LAYOUT*", "pSetLayouts"),
             Param("XGL_DESCRIPTOR_SET*", "pDescriptorSets"),
             Param("uint32_t*", "pCount")]),

        Proto("void", "ClearDescriptorSets",
            [Param("XGL_DESCRIPTOR_POOL", "descriptorPool"),
             Param("uint32_t", "count"),
             Param("const XGL_DESCRIPTOR_SET*", "pDescriptorSets")]),

        Proto("void", "UpdateDescriptors",
            [Param("XGL_DESCRIPTOR_SET", "descriptorSet"),
             Param("uint32_t", "updateCount"),
             Param("const void**", "ppUpdateArray")]),

        Proto("XGL_RESULT", "CreateDynamicViewportState",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_DYNAMIC_VP_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DYNAMIC_VP_STATE_OBJECT*", "pState")]),

        Proto("XGL_RESULT", "CreateDynamicRasterState",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_DYNAMIC_RS_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DYNAMIC_RS_STATE_OBJECT*", "pState")]),

        Proto("XGL_RESULT", "CreateDynamicColorBlendState",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_DYNAMIC_CB_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DYNAMIC_CB_STATE_OBJECT*", "pState")]),

        Proto("XGL_RESULT", "CreateDynamicDepthStencilState",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_DYNAMIC_DS_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_DYNAMIC_DS_STATE_OBJECT*", "pState")]),

        Proto("XGL_RESULT", "CreateCommandBuffer",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_CMD_BUFFER_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_CMD_BUFFER*", "pCmdBuffer")]),

        Proto("XGL_RESULT", "BeginCommandBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("const XGL_CMD_BUFFER_BEGIN_INFO*", "pBeginInfo")]),

        Proto("XGL_RESULT", "EndCommandBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer")]),

        Proto("XGL_RESULT", "ResetCommandBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer")]),

        Proto("void", "CmdBindPipeline",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("XGL_PIPELINE", "pipeline")]),

        Proto("void", "CmdBindDynamicStateObject",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_STATE_BIND_POINT", "stateBindPoint"),
             Param("XGL_DYNAMIC_STATE_OBJECT", "state")]),

        Proto("void", "CmdBindDescriptorSets",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("XGL_DESCRIPTOR_SET_LAYOUT_CHAIN", "layoutChain"),
             Param("uint32_t", "layoutChainSlot"),
             Param("uint32_t", "count"),
             Param("const XGL_DESCRIPTOR_SET*", "pDescriptorSets"),
             Param("const uint32_t*", "pUserData")]),

        Proto("void", "CmdBindVertexBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_BUFFER", "buffer"),
             Param("XGL_GPU_SIZE", "offset"),
             Param("uint32_t", "binding")]),

        Proto("void", "CmdBindIndexBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_BUFFER", "buffer"),
             Param("XGL_GPU_SIZE", "offset"),
             Param("XGL_INDEX_TYPE", "indexType")]),

        Proto("void", "CmdDraw",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("uint32_t", "firstVertex"),
             Param("uint32_t", "vertexCount"),
             Param("uint32_t", "firstInstance"),
             Param("uint32_t", "instanceCount")]),

        Proto("void", "CmdDrawIndexed",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("uint32_t", "firstIndex"),
             Param("uint32_t", "indexCount"),
             Param("int32_t", "vertexOffset"),
             Param("uint32_t", "firstInstance"),
             Param("uint32_t", "instanceCount")]),

        Proto("void", "CmdDrawIndirect",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_BUFFER", "buffer"),
             Param("XGL_GPU_SIZE", "offset"),
             Param("uint32_t", "count"),
             Param("uint32_t", "stride")]),

        Proto("void", "CmdDrawIndexedIndirect",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_BUFFER", "buffer"),
             Param("XGL_GPU_SIZE", "offset"),
             Param("uint32_t", "count"),
             Param("uint32_t", "stride")]),

        Proto("void", "CmdDispatch",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("uint32_t", "x"),
             Param("uint32_t", "y"),
             Param("uint32_t", "z")]),

        Proto("void", "CmdDispatchIndirect",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_BUFFER", "buffer"),
             Param("XGL_GPU_SIZE", "offset")]),

        Proto("void", "CmdCopyBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_BUFFER", "srcBuffer"),
             Param("XGL_BUFFER", "destBuffer"),
             Param("uint32_t", "regionCount"),
             Param("const XGL_BUFFER_COPY*", "pRegions")]),

        Proto("void", "CmdCopyImage",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "srcImage"),
             Param("XGL_IMAGE_LAYOUT", "srcImageLayout"),
             Param("XGL_IMAGE", "destImage"),
             Param("XGL_IMAGE_LAYOUT", "destImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const XGL_IMAGE_COPY*", "pRegions")]),

        Proto("void", "CmdBlitImage",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "srcImage"),
             Param("XGL_IMAGE_LAYOUT", "srcImageLayout"),
             Param("XGL_IMAGE", "destImage"),
             Param("XGL_IMAGE_LAYOUT", "destImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const XGL_IMAGE_BLIT*", "pRegions")]),

        Proto("void", "CmdCopyBufferToImage",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_BUFFER", "srcBuffer"),
             Param("XGL_IMAGE", "destImage"),
             Param("XGL_IMAGE_LAYOUT", "destImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const XGL_BUFFER_IMAGE_COPY*", "pRegions")]),

        Proto("void", "CmdCopyImageToBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "srcImage"),
             Param("XGL_IMAGE_LAYOUT", "srcImageLayout"),
             Param("XGL_BUFFER", "destBuffer"),
             Param("uint32_t", "regionCount"),
             Param("const XGL_BUFFER_IMAGE_COPY*", "pRegions")]),

        Proto("void", "CmdCloneImageData",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "srcImage"),
             Param("XGL_IMAGE_LAYOUT", "srcImageLayout"),
             Param("XGL_IMAGE", "destImage"),
             Param("XGL_IMAGE_LAYOUT", "destImageLayout")]),

        Proto("void", "CmdUpdateBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_BUFFER", "destBuffer"),
             Param("XGL_GPU_SIZE", "destOffset"),
             Param("XGL_GPU_SIZE", "dataSize"),
             Param("const uint32_t*", "pData")]),

        Proto("void", "CmdFillBuffer",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_BUFFER", "destBuffer"),
             Param("XGL_GPU_SIZE", "destOffset"),
             Param("XGL_GPU_SIZE", "fillSize"),
             Param("uint32_t", "data")]),

        Proto("void", "CmdClearColorImage",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "image"),
             Param("XGL_IMAGE_LAYOUT", "imageLayout"),
	     Param("XGL_CLEAR_COLOR", "color"),
             Param("uint32_t", "rangeCount"),
             Param("const XGL_IMAGE_SUBRESOURCE_RANGE*", "pRanges")]),

        Proto("void", "CmdClearDepthStencil",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "image"),
             Param("XGL_IMAGE_LAYOUT", "imageLayout"),
             Param("float", "depth"),
             Param("uint32_t", "stencil"),
             Param("uint32_t", "rangeCount"),
             Param("const XGL_IMAGE_SUBRESOURCE_RANGE*", "pRanges")]),

        Proto("void", "CmdResolveImage",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_IMAGE", "srcImage"),
             Param("XGL_IMAGE_LAYOUT", "srcImageLayout"),
             Param("XGL_IMAGE", "destImage"),
             Param("XGL_IMAGE_LAYOUT", "destImageLayout"),
             Param("uint32_t", "rectCount"),
             Param("const XGL_IMAGE_RESOLVE*", "pRects")]),

        Proto("void", "CmdSetEvent",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_EVENT", "event"),
             Param("XGL_PIPE_EVENT", "pipeEvent")]),

        Proto("void", "CmdResetEvent",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_EVENT", "event"),
             Param("XGL_PIPE_EVENT", "pipeEvent")]),

        Proto("void", "CmdWaitEvents",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("const XGL_EVENT_WAIT_INFO*", "pWaitInfo")]),

        Proto("void", "CmdPipelineBarrier",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("const XGL_PIPELINE_BARRIER*", "pBarrier")]),

        Proto("void", "CmdBeginQuery",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_QUERY_POOL", "queryPool"),
             Param("uint32_t", "slot"),
             Param("XGL_FLAGS", "flags")]),

        Proto("void", "CmdEndQuery",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_QUERY_POOL", "queryPool"),
             Param("uint32_t", "slot")]),

        Proto("void", "CmdResetQueryPool",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_QUERY_POOL", "queryPool"),
             Param("uint32_t", "startQuery"),
             Param("uint32_t", "queryCount")]),

        Proto("void", "CmdWriteTimestamp",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_TIMESTAMP_TYPE", "timestampType"),
             Param("XGL_BUFFER", "destBuffer"),
             Param("XGL_GPU_SIZE", "destOffset")]),

        Proto("void", "CmdInitAtomicCounters",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("uint32_t", "startCounter"),
             Param("uint32_t", "counterCount"),
             Param("const uint32_t*", "pData")]),

        Proto("void", "CmdLoadAtomicCounters",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("uint32_t", "startCounter"),
             Param("uint32_t", "counterCount"),
             Param("XGL_BUFFER", "srcBuffer"),
             Param("XGL_GPU_SIZE", "srcOffset")]),

        Proto("void", "CmdSaveAtomicCounters",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("uint32_t", "startCounter"),
             Param("uint32_t", "counterCount"),
             Param("XGL_BUFFER", "destBuffer"),
             Param("XGL_GPU_SIZE", "destOffset")]),

        Proto("XGL_RESULT", "CreateFramebuffer",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_FRAMEBUFFER_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_FRAMEBUFFER*", "pFramebuffer")]),

        Proto("XGL_RESULT", "CreateRenderPass",
            [Param("XGL_DEVICE", "device"),
             Param("const XGL_RENDER_PASS_CREATE_INFO*", "pCreateInfo"),
             Param("XGL_RENDER_PASS*", "pRenderPass")]),

        Proto("void", "CmdBeginRenderPass",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("const XGL_RENDER_PASS_BEGIN*", "pRenderPassBegin")]),

        Proto("void", "CmdEndRenderPass",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("XGL_RENDER_PASS", "renderPass")]),

        Proto("XGL_RESULT", "DbgSetValidationLevel",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_VALIDATION_LEVEL", "validationLevel")]),

        Proto("XGL_RESULT", "DbgRegisterMsgCallback",
            [Param("XGL_INSTANCE", "instance"),
             Param("XGL_DBG_MSG_CALLBACK_FUNCTION", "pfnMsgCallback"),
             Param("void*", "pUserData")]),

        Proto("XGL_RESULT", "DbgUnregisterMsgCallback",
            [Param("XGL_INSTANCE", "instance"),
             Param("XGL_DBG_MSG_CALLBACK_FUNCTION", "pfnMsgCallback")]),

        Proto("XGL_RESULT", "DbgSetMessageFilter",
            [Param("XGL_DEVICE", "device"),
             Param("int32_t", "msgCode"),
             Param("XGL_DBG_MSG_FILTER", "filter")]),

        Proto("XGL_RESULT", "DbgSetObjectTag",
            [Param("XGL_BASE_OBJECT", "object"),
             Param("size_t", "tagSize"),
             Param("const void*", "pTag")]),

        Proto("XGL_RESULT", "DbgSetGlobalOption",
            [Param("XGL_INSTANCE", "instance"),
             Param("XGL_DBG_GLOBAL_OPTION", "dbgOption"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData")]),

        Proto("XGL_RESULT", "DbgSetDeviceOption",
            [Param("XGL_DEVICE", "device"),
             Param("XGL_DBG_DEVICE_OPTION", "dbgOption"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData")]),

        Proto("void", "CmdDbgMarkerBegin",
            [Param("XGL_CMD_BUFFER", "cmdBuffer"),
             Param("const char*", "pMarker")]),

        Proto("void", "CmdDbgMarkerEnd",
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
             Param("uint64_t*", "pMsc")]),

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

object_root_list = [
    "XGL_INSTANCE",
    "XGL_PHYSICAL_GPU",
    "XGL_BASE_OBJECT"
]

object_base_list = [
    "XGL_DEVICE",
    "XGL_QUEUE",
    "XGL_GPU_MEMORY",
    "XGL_OBJECT"
]

object_list = [
    "XGL_BUFFER",
    "XGL_BUFFER_VIEW",
    "XGL_IMAGE",
    "XGL_IMAGE_VIEW",
    "XGL_COLOR_ATTACHMENT_VIEW",
    "XGL_DEPTH_STENCIL_VIEW",
    "XGL_SHADER",
    "XGL_PIPELINE",
    "XGL_SAMPLER",
    "XGL_DESCRIPTOR_SET",
    "XGL_DESCRIPTOR_SET_LAYOUT",
    "XGL_DESCRIPTOR_SET_LAYOUT_CHAIN",
    "XGL_DESCRIPTOR_POOL",
    "XGL_DYNAMIC_STATE_OBJECT",
    "XGL_CMD_BUFFER",
    "XGL_FENCE",
    "XGL_SEMAPHORE",
    "XGL_EVENT",
    "XGL_QUERY_POOL",
    "XGL_FRAMEBUFFER",
    "XGL_RENDER_PASS"
]

object_dynamic_state_list = [
    "XGL_DYNAMIC_VP_STATE_OBJECT",
    "XGL_DYNAMIC_RS_STATE_OBJECT",
    "XGL_DYNAMIC_CB_STATE_OBJECT",
    "XGL_DYNAMIC_DS_STATE_OBJECT"
]

object_type_list = object_root_list + object_base_list + object_list + object_dynamic_state_list

object_parent_list = ["XGL_BASE_OBJECT", "XGL_OBJECT", "XGL_DYNAMIC_STATE_OBJECT"]

headers = []
objects = []
protos = []
for ext in extensions:
    headers.extend(ext.headers)
    objects.extend(ext.objects)
    protos.extend(ext.protos)

proto_names = [proto.name for proto in protos]

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
        first, rest = line.split(" (XGLAPI *xgl")
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
        print("    xgl%sType %s;" % (proto.name, proto.name))
    print("} XGL_LAYER_DISPATCH_TABLE;")

if __name__ == "__main__":
    parse_xgl_h("include/xgl.h")
