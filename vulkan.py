"""VK API description"""

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
        """Return a named declaration in C, with vulkan.h formatting."""
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
        """Return the params that are simple VK objects and are inputs."""
        return [param for param in self.params if param.ty in objects]

    def object_out_params(self):
        """Return the params that are simple VK objects and are outputs."""
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

# VK core API
core = Extension(
    name="VK_CORE",
    headers=["vulkan.h", "vkDbg.h"],
    objects=[
        "VK_INSTANCE",
        "VK_PHYSICAL_GPU",
        "VK_BASE_OBJECT",
        "VK_DEVICE",
        "VK_QUEUE",
        "VK_GPU_MEMORY",
        "VK_OBJECT",
        "VK_BUFFER",
        "VK_BUFFER_VIEW",
        "VK_IMAGE",
        "VK_IMAGE_VIEW",
        "VK_COLOR_ATTACHMENT_VIEW",
        "VK_DEPTH_STENCIL_VIEW",
        "VK_SHADER",
        "VK_PIPELINE",
        "VK_SAMPLER",
        "VK_DESCRIPTOR_SET",
        "VK_DESCRIPTOR_SET_LAYOUT",
        "VK_DESCRIPTOR_SET_LAYOUT_CHAIN",
        "VK_DESCRIPTOR_POOL",
        "VK_DYNAMIC_STATE_OBJECT",
        "VK_DYNAMIC_VP_STATE_OBJECT",
        "VK_DYNAMIC_RS_STATE_OBJECT",
        "VK_DYNAMIC_CB_STATE_OBJECT",
        "VK_DYNAMIC_DS_STATE_OBJECT",
        "VK_CMD_BUFFER",
        "VK_FENCE",
        "VK_SEMAPHORE",
        "VK_EVENT",
        "VK_QUERY_POOL",
        "VK_FRAMEBUFFER",
        "VK_RENDER_PASS",
    ],
    protos=[
        Proto("VK_RESULT", "CreateInstance",
            [Param("const VkInstanceCreateInfo*", "pCreateInfo"),
             Param("VK_INSTANCE*", "pInstance")]),

        Proto("VK_RESULT", "DestroyInstance",
            [Param("VK_INSTANCE", "instance")]),

        Proto("VK_RESULT", "EnumerateGpus",
            [Param("VK_INSTANCE", "instance"),
             Param("uint32_t", "maxGpus"),
             Param("uint32_t*", "pGpuCount"),
             Param("VK_PHYSICAL_GPU*", "pGpus")]),

        Proto("VK_RESULT", "GetGpuInfo",
            [Param("VK_PHYSICAL_GPU", "gpu"),
             Param("VK_PHYSICAL_GPU_INFO_TYPE", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("void*", "GetProcAddr",
            [Param("VK_PHYSICAL_GPU", "gpu"),
             Param("const char*", "pName")]),

        Proto("VK_RESULT", "CreateDevice",
            [Param("VK_PHYSICAL_GPU", "gpu"),
             Param("const VkDeviceCreateInfo*", "pCreateInfo"),
             Param("VK_DEVICE*", "pDevice")]),

        Proto("VK_RESULT", "DestroyDevice",
            [Param("VK_DEVICE", "device")]),

        Proto("VK_RESULT", "GetExtensionSupport",
            [Param("VK_PHYSICAL_GPU", "gpu"),
             Param("const char*", "pExtName")]),

        Proto("VK_RESULT", "EnumerateLayers",
            [Param("VK_PHYSICAL_GPU", "gpu"),
             Param("size_t", "maxLayerCount"),
             Param("size_t", "maxStringSize"),
             Param("size_t*", "pOutLayerCount"),
             Param("char* const*", "pOutLayers"),
             Param("void*", "pReserved")]),

        Proto("VK_RESULT", "GetDeviceQueue",
            [Param("VK_DEVICE", "device"),
             Param("uint32_t", "queueNodeIndex"),
             Param("uint32_t", "queueIndex"),
             Param("VK_QUEUE*", "pQueue")]),

        Proto("VK_RESULT", "QueueSubmit",
            [Param("VK_QUEUE", "queue"),
             Param("uint32_t", "cmdBufferCount"),
             Param("const VK_CMD_BUFFER*", "pCmdBuffers"),
             Param("VK_FENCE", "fence")]),

        Proto("VK_RESULT", "QueueAddMemReference",
            [Param("VK_QUEUE", "queue"),
             Param("VK_GPU_MEMORY", "mem")]),

        Proto("VK_RESULT", "QueueRemoveMemReference",
            [Param("VK_QUEUE", "queue"),
             Param("VK_GPU_MEMORY", "mem")]),

        Proto("VK_RESULT", "QueueWaitIdle",
            [Param("VK_QUEUE", "queue")]),

        Proto("VK_RESULT", "DeviceWaitIdle",
            [Param("VK_DEVICE", "device")]),

        Proto("VK_RESULT", "AllocMemory",
            [Param("VK_DEVICE", "device"),
             Param("const VkMemoryAllocInfo*", "pAllocInfo"),
             Param("VK_GPU_MEMORY*", "pMem")]),

        Proto("VK_RESULT", "FreeMemory",
            [Param("VK_GPU_MEMORY", "mem")]),

        Proto("VK_RESULT", "SetMemoryPriority",
            [Param("VK_GPU_MEMORY", "mem"),
             Param("VK_MEMORY_PRIORITY", "priority")]),

        Proto("VK_RESULT", "MapMemory",
            [Param("VK_GPU_MEMORY", "mem"),
             Param("VK_FLAGS", "flags"),
             Param("void**", "ppData")]),

        Proto("VK_RESULT", "UnmapMemory",
            [Param("VK_GPU_MEMORY", "mem")]),

        Proto("VK_RESULT", "PinSystemMemory",
            [Param("VK_DEVICE", "device"),
             Param("const void*", "pSysMem"),
             Param("size_t", "memSize"),
             Param("VK_GPU_MEMORY*", "pMem")]),

        Proto("VK_RESULT", "GetMultiGpuCompatibility",
            [Param("VK_PHYSICAL_GPU", "gpu0"),
             Param("VK_PHYSICAL_GPU", "gpu1"),
             Param("VK_GPU_COMPATIBILITY_INFO*", "pInfo")]),

        Proto("VK_RESULT", "OpenSharedMemory",
            [Param("VK_DEVICE", "device"),
             Param("const VK_MEMORY_OPEN_INFO*", "pOpenInfo"),
             Param("VK_GPU_MEMORY*", "pMem")]),

        Proto("VK_RESULT", "OpenSharedSemaphore",
            [Param("VK_DEVICE", "device"),
             Param("const VK_SEMAPHORE_OPEN_INFO*", "pOpenInfo"),
             Param("VK_SEMAPHORE*", "pSemaphore")]),

        Proto("VK_RESULT", "OpenPeerMemory",
            [Param("VK_DEVICE", "device"),
             Param("const VK_PEER_MEMORY_OPEN_INFO*", "pOpenInfo"),
             Param("VK_GPU_MEMORY*", "pMem")]),

        Proto("VK_RESULT", "OpenPeerImage",
            [Param("VK_DEVICE", "device"),
             Param("const VK_PEER_IMAGE_OPEN_INFO*", "pOpenInfo"),
             Param("VK_IMAGE*", "pImage"),
             Param("VK_GPU_MEMORY*", "pMem")]),

        Proto("VK_RESULT", "DestroyObject",
            [Param("VK_OBJECT", "object")]),

        Proto("VK_RESULT", "GetObjectInfo",
            [Param("VK_BASE_OBJECT", "object"),
             Param("VK_OBJECT_INFO_TYPE", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VK_RESULT", "BindObjectMemory",
            [Param("VK_OBJECT", "object"),
             Param("uint32_t", "allocationIdx"),
             Param("VK_GPU_MEMORY", "mem"),
             Param("VK_GPU_SIZE", "offset")]),

        Proto("VK_RESULT", "BindObjectMemoryRange",
            [Param("VK_OBJECT", "object"),
             Param("uint32_t", "allocationIdx"),
             Param("VK_GPU_SIZE", "rangeOffset"),
             Param("VK_GPU_SIZE", "rangeSize"),
             Param("VK_GPU_MEMORY", "mem"),
             Param("VK_GPU_SIZE", "memOffset")]),

        Proto("VK_RESULT", "BindImageMemoryRange",
            [Param("VK_IMAGE", "image"),
             Param("uint32_t", "allocationIdx"),
             Param("const VK_IMAGE_MEMORY_BIND_INFO*", "bindInfo"),
             Param("VK_GPU_MEMORY", "mem"),
             Param("VK_GPU_SIZE", "memOffset")]),

        Proto("VK_RESULT", "CreateFence",
            [Param("VK_DEVICE", "device"),
             Param("const VK_FENCE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_FENCE*", "pFence")]),

        Proto("VK_RESULT", "ResetFences",
            [Param("VK_DEVICE", "device"),
             Param("uint32_t", "fenceCount"),
             Param("VK_FENCE*", "pFences")]),

        Proto("VK_RESULT", "GetFenceStatus",
            [Param("VK_FENCE", "fence")]),

        Proto("VK_RESULT", "WaitForFences",
            [Param("VK_DEVICE", "device"),
             Param("uint32_t", "fenceCount"),
             Param("const VK_FENCE*", "pFences"),
             Param("bool32_t", "waitAll"),
             Param("uint64_t", "timeout")]),

        Proto("VK_RESULT", "CreateSemaphore",
            [Param("VK_DEVICE", "device"),
             Param("const VK_SEMAPHORE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_SEMAPHORE*", "pSemaphore")]),

        Proto("VK_RESULT", "QueueSignalSemaphore",
            [Param("VK_QUEUE", "queue"),
             Param("VK_SEMAPHORE", "semaphore")]),

        Proto("VK_RESULT", "QueueWaitSemaphore",
            [Param("VK_QUEUE", "queue"),
             Param("VK_SEMAPHORE", "semaphore")]),

        Proto("VK_RESULT", "CreateEvent",
            [Param("VK_DEVICE", "device"),
             Param("const VK_EVENT_CREATE_INFO*", "pCreateInfo"),
             Param("VK_EVENT*", "pEvent")]),

        Proto("VK_RESULT", "GetEventStatus",
            [Param("VK_EVENT", "event")]),

        Proto("VK_RESULT", "SetEvent",
            [Param("VK_EVENT", "event")]),

        Proto("VK_RESULT", "ResetEvent",
            [Param("VK_EVENT", "event")]),

        Proto("VK_RESULT", "CreateQueryPool",
            [Param("VK_DEVICE", "device"),
             Param("const VK_QUERY_POOL_CREATE_INFO*", "pCreateInfo"),
             Param("VK_QUERY_POOL*", "pQueryPool")]),

        Proto("VK_RESULT", "GetQueryPoolResults",
            [Param("VK_QUERY_POOL", "queryPool"),
             Param("uint32_t", "startQuery"),
             Param("uint32_t", "queryCount"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VK_RESULT", "GetFormatInfo",
            [Param("VK_DEVICE", "device"),
             Param("VK_FORMAT", "format"),
             Param("VK_FORMAT_INFO_TYPE", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VK_RESULT", "CreateBuffer",
            [Param("VK_DEVICE", "device"),
             Param("const VkBufferCreateInfo*", "pCreateInfo"),
             Param("VK_BUFFER*", "pBuffer")]),

        Proto("VK_RESULT", "CreateBufferView",
            [Param("VK_DEVICE", "device"),
             Param("const VkBufferViewCreateInfo*", "pCreateInfo"),
             Param("VK_BUFFER_VIEW*", "pView")]),

        Proto("VK_RESULT", "CreateImage",
            [Param("VK_DEVICE", "device"),
             Param("const VK_IMAGE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_IMAGE*", "pImage")]),

        Proto("VK_RESULT", "GetImageSubresourceInfo",
            [Param("VK_IMAGE", "image"),
             Param("const VK_IMAGE_SUBRESOURCE*", "pSubresource"),
             Param("VK_SUBRESOURCE_INFO_TYPE", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VK_RESULT", "CreateImageView",
            [Param("VK_DEVICE", "device"),
             Param("const VK_IMAGE_VIEW_CREATE_INFO*", "pCreateInfo"),
             Param("VK_IMAGE_VIEW*", "pView")]),

        Proto("VK_RESULT", "CreateColorAttachmentView",
            [Param("VK_DEVICE", "device"),
             Param("const VK_COLOR_ATTACHMENT_VIEW_CREATE_INFO*", "pCreateInfo"),
             Param("VK_COLOR_ATTACHMENT_VIEW*", "pView")]),

        Proto("VK_RESULT", "CreateDepthStencilView",
            [Param("VK_DEVICE", "device"),
             Param("const VK_DEPTH_STENCIL_VIEW_CREATE_INFO*", "pCreateInfo"),
             Param("VK_DEPTH_STENCIL_VIEW*", "pView")]),

        Proto("VK_RESULT", "CreateShader",
            [Param("VK_DEVICE", "device"),
             Param("const VK_SHADER_CREATE_INFO*", "pCreateInfo"),
             Param("VK_SHADER*", "pShader")]),

        Proto("VK_RESULT", "CreateGraphicsPipeline",
            [Param("VK_DEVICE", "device"),
             Param("const VK_GRAPHICS_PIPELINE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_PIPELINE*", "pPipeline")]),

        Proto("VK_RESULT", "CreateGraphicsPipelineDerivative",
            [Param("VK_DEVICE", "device"),
             Param("const VK_GRAPHICS_PIPELINE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_PIPELINE", "basePipeline"),
             Param("VK_PIPELINE*", "pPipeline")]),

        Proto("VK_RESULT", "CreateComputePipeline",
            [Param("VK_DEVICE", "device"),
             Param("const VK_COMPUTE_PIPELINE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_PIPELINE*", "pPipeline")]),

        Proto("VK_RESULT", "StorePipeline",
            [Param("VK_PIPELINE", "pipeline"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VK_RESULT", "LoadPipeline",
            [Param("VK_DEVICE", "device"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData"),
             Param("VK_PIPELINE*", "pPipeline")]),

        Proto("VK_RESULT", "LoadPipelineDerivative",
            [Param("VK_DEVICE", "device"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData"),
             Param("VK_PIPELINE", "basePipeline"),
             Param("VK_PIPELINE*", "pPipeline")]),

        Proto("VK_RESULT", "CreateSampler",
            [Param("VK_DEVICE", "device"),
             Param("const VK_SAMPLER_CREATE_INFO*", "pCreateInfo"),
             Param("VK_SAMPLER*", "pSampler")]),

        Proto("VK_RESULT", "CreateDescriptorSetLayout",
            [Param("VK_DEVICE", "device"),
             Param("const VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*", "pCreateInfo"),
             Param("VK_DESCRIPTOR_SET_LAYOUT*", "pSetLayout")]),

        Proto("VK_RESULT", "CreateDescriptorSetLayoutChain",
            [Param("VK_DEVICE", "device"),
             Param("uint32_t", "setLayoutArrayCount"),
             Param("const VK_DESCRIPTOR_SET_LAYOUT*", "pSetLayoutArray"),
             Param("VK_DESCRIPTOR_SET_LAYOUT_CHAIN*", "pLayoutChain")]),

        Proto("VK_RESULT", "BeginDescriptorPoolUpdate",
            [Param("VK_DEVICE", "device"),
             Param("VK_DESCRIPTOR_UPDATE_MODE", "updateMode")]),

        Proto("VK_RESULT", "EndDescriptorPoolUpdate",
            [Param("VK_DEVICE", "device"),
             Param("VK_CMD_BUFFER", "cmd")]),

        Proto("VK_RESULT", "CreateDescriptorPool",
            [Param("VK_DEVICE", "device"),
             Param("VK_DESCRIPTOR_POOL_USAGE", "poolUsage"),
             Param("uint32_t", "maxSets"),
             Param("const VK_DESCRIPTOR_POOL_CREATE_INFO*", "pCreateInfo"),
             Param("VK_DESCRIPTOR_POOL*", "pDescriptorPool")]),

        Proto("VK_RESULT", "ResetDescriptorPool",
            [Param("VK_DESCRIPTOR_POOL", "descriptorPool")]),

        Proto("VK_RESULT", "AllocDescriptorSets",
            [Param("VK_DESCRIPTOR_POOL", "descriptorPool"),
             Param("VK_DESCRIPTOR_SET_USAGE", "setUsage"),
             Param("uint32_t", "count"),
             Param("const VK_DESCRIPTOR_SET_LAYOUT*", "pSetLayouts"),
             Param("VK_DESCRIPTOR_SET*", "pDescriptorSets"),
             Param("uint32_t*", "pCount")]),

        Proto("void", "ClearDescriptorSets",
            [Param("VK_DESCRIPTOR_POOL", "descriptorPool"),
             Param("uint32_t", "count"),
             Param("const VK_DESCRIPTOR_SET*", "pDescriptorSets")]),

        Proto("void", "UpdateDescriptors",
            [Param("VK_DESCRIPTOR_SET", "descriptorSet"),
             Param("uint32_t", "updateCount"),
             Param("const void**", "ppUpdateArray")]),

        Proto("VK_RESULT", "CreateDynamicViewportState",
            [Param("VK_DEVICE", "device"),
             Param("const VK_DYNAMIC_VP_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_DYNAMIC_VP_STATE_OBJECT*", "pState")]),

        Proto("VK_RESULT", "CreateDynamicRasterState",
            [Param("VK_DEVICE", "device"),
             Param("const VK_DYNAMIC_RS_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_DYNAMIC_RS_STATE_OBJECT*", "pState")]),

        Proto("VK_RESULT", "CreateDynamicColorBlendState",
            [Param("VK_DEVICE", "device"),
             Param("const VK_DYNAMIC_CB_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_DYNAMIC_CB_STATE_OBJECT*", "pState")]),

        Proto("VK_RESULT", "CreateDynamicDepthStencilState",
            [Param("VK_DEVICE", "device"),
             Param("const VK_DYNAMIC_DS_STATE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_DYNAMIC_DS_STATE_OBJECT*", "pState")]),

        Proto("VK_RESULT", "CreateCommandBuffer",
            [Param("VK_DEVICE", "device"),
             Param("const VK_CMD_BUFFER_CREATE_INFO*", "pCreateInfo"),
             Param("VK_CMD_BUFFER*", "pCmdBuffer")]),

        Proto("VK_RESULT", "BeginCommandBuffer",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("const VK_CMD_BUFFER_BEGIN_INFO*", "pBeginInfo")]),

        Proto("VK_RESULT", "EndCommandBuffer",
            [Param("VK_CMD_BUFFER", "cmdBuffer")]),

        Proto("VK_RESULT", "ResetCommandBuffer",
            [Param("VK_CMD_BUFFER", "cmdBuffer")]),

        Proto("void", "CmdBindPipeline",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("VK_PIPELINE", "pipeline")]),

        Proto("void", "CmdBindDynamicStateObject",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_STATE_BIND_POINT", "stateBindPoint"),
             Param("VK_DYNAMIC_STATE_OBJECT", "state")]),

        Proto("void", "CmdBindDescriptorSets",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("VK_DESCRIPTOR_SET_LAYOUT_CHAIN", "layoutChain"),
             Param("uint32_t", "layoutChainSlot"),
             Param("uint32_t", "count"),
             Param("const VK_DESCRIPTOR_SET*", "pDescriptorSets"),
             Param("const uint32_t*", "pUserData")]),

        Proto("void", "CmdBindVertexBuffer",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_BUFFER", "buffer"),
             Param("VK_GPU_SIZE", "offset"),
             Param("uint32_t", "binding")]),

        Proto("void", "CmdBindIndexBuffer",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_BUFFER", "buffer"),
             Param("VK_GPU_SIZE", "offset"),
             Param("VK_INDEX_TYPE", "indexType")]),

        Proto("void", "CmdDraw",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("uint32_t", "firstVertex"),
             Param("uint32_t", "vertexCount"),
             Param("uint32_t", "firstInstance"),
             Param("uint32_t", "instanceCount")]),

        Proto("void", "CmdDrawIndexed",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("uint32_t", "firstIndex"),
             Param("uint32_t", "indexCount"),
             Param("int32_t", "vertexOffset"),
             Param("uint32_t", "firstInstance"),
             Param("uint32_t", "instanceCount")]),

        Proto("void", "CmdDrawIndirect",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_BUFFER", "buffer"),
             Param("VK_GPU_SIZE", "offset"),
             Param("uint32_t", "count"),
             Param("uint32_t", "stride")]),

        Proto("void", "CmdDrawIndexedIndirect",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_BUFFER", "buffer"),
             Param("VK_GPU_SIZE", "offset"),
             Param("uint32_t", "count"),
             Param("uint32_t", "stride")]),

        Proto("void", "CmdDispatch",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("uint32_t", "x"),
             Param("uint32_t", "y"),
             Param("uint32_t", "z")]),

        Proto("void", "CmdDispatchIndirect",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_BUFFER", "buffer"),
             Param("VK_GPU_SIZE", "offset")]),

        Proto("void", "CmdCopyBuffer",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_BUFFER", "srcBuffer"),
             Param("VK_BUFFER", "destBuffer"),
             Param("uint32_t", "regionCount"),
             Param("const VK_BUFFER_COPY*", "pRegions")]),

        Proto("void", "CmdCopyImage",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_IMAGE", "srcImage"),
             Param("VK_IMAGE_LAYOUT", "srcImageLayout"),
             Param("VK_IMAGE", "destImage"),
             Param("VK_IMAGE_LAYOUT", "destImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VK_IMAGE_COPY*", "pRegions")]),

        Proto("void", "CmdBlitImage",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_IMAGE", "srcImage"),
             Param("VK_IMAGE_LAYOUT", "srcImageLayout"),
             Param("VK_IMAGE", "destImage"),
             Param("VK_IMAGE_LAYOUT", "destImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VK_IMAGE_BLIT*", "pRegions")]),

        Proto("void", "CmdCopyBufferToImage",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_BUFFER", "srcBuffer"),
             Param("VK_IMAGE", "destImage"),
             Param("VK_IMAGE_LAYOUT", "destImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VK_BUFFER_IMAGE_COPY*", "pRegions")]),

        Proto("void", "CmdCopyImageToBuffer",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_IMAGE", "srcImage"),
             Param("VK_IMAGE_LAYOUT", "srcImageLayout"),
             Param("VK_BUFFER", "destBuffer"),
             Param("uint32_t", "regionCount"),
             Param("const VK_BUFFER_IMAGE_COPY*", "pRegions")]),

        Proto("void", "CmdCloneImageData",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_IMAGE", "srcImage"),
             Param("VK_IMAGE_LAYOUT", "srcImageLayout"),
             Param("VK_IMAGE", "destImage"),
             Param("VK_IMAGE_LAYOUT", "destImageLayout")]),

        Proto("void", "CmdUpdateBuffer",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_BUFFER", "destBuffer"),
             Param("VK_GPU_SIZE", "destOffset"),
             Param("VK_GPU_SIZE", "dataSize"),
             Param("const uint32_t*", "pData")]),

        Proto("void", "CmdFillBuffer",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_BUFFER", "destBuffer"),
             Param("VK_GPU_SIZE", "destOffset"),
             Param("VK_GPU_SIZE", "fillSize"),
             Param("uint32_t", "data")]),

        Proto("void", "CmdClearColorImage",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_IMAGE", "image"),
             Param("VK_IMAGE_LAYOUT", "imageLayout"),
	     Param("VK_CLEAR_COLOR", "color"),
             Param("uint32_t", "rangeCount"),
             Param("const VK_IMAGE_SUBRESOURCE_RANGE*", "pRanges")]),

        Proto("void", "CmdClearDepthStencil",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_IMAGE", "image"),
             Param("VK_IMAGE_LAYOUT", "imageLayout"),
             Param("float", "depth"),
             Param("uint32_t", "stencil"),
             Param("uint32_t", "rangeCount"),
             Param("const VK_IMAGE_SUBRESOURCE_RANGE*", "pRanges")]),

        Proto("void", "CmdResolveImage",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_IMAGE", "srcImage"),
             Param("VK_IMAGE_LAYOUT", "srcImageLayout"),
             Param("VK_IMAGE", "destImage"),
             Param("VK_IMAGE_LAYOUT", "destImageLayout"),
             Param("uint32_t", "rectCount"),
             Param("const VK_IMAGE_RESOLVE*", "pRects")]),

        Proto("void", "CmdSetEvent",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_EVENT", "event"),
             Param("VK_PIPE_EVENT", "pipeEvent")]),

        Proto("void", "CmdResetEvent",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_EVENT", "event"),
             Param("VK_PIPE_EVENT", "pipeEvent")]),

        Proto("void", "CmdWaitEvents",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("const VK_EVENT_WAIT_INFO*", "pWaitInfo")]),

        Proto("void", "CmdPipelineBarrier",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("const VK_PIPELINE_BARRIER*", "pBarrier")]),

        Proto("void", "CmdBeginQuery",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_QUERY_POOL", "queryPool"),
             Param("uint32_t", "slot"),
             Param("VK_FLAGS", "flags")]),

        Proto("void", "CmdEndQuery",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_QUERY_POOL", "queryPool"),
             Param("uint32_t", "slot")]),

        Proto("void", "CmdResetQueryPool",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_QUERY_POOL", "queryPool"),
             Param("uint32_t", "startQuery"),
             Param("uint32_t", "queryCount")]),

        Proto("void", "CmdWriteTimestamp",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_TIMESTAMP_TYPE", "timestampType"),
             Param("VK_BUFFER", "destBuffer"),
             Param("VK_GPU_SIZE", "destOffset")]),

        Proto("void", "CmdInitAtomicCounters",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("uint32_t", "startCounter"),
             Param("uint32_t", "counterCount"),
             Param("const uint32_t*", "pData")]),

        Proto("void", "CmdLoadAtomicCounters",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("uint32_t", "startCounter"),
             Param("uint32_t", "counterCount"),
             Param("VK_BUFFER", "srcBuffer"),
             Param("VK_GPU_SIZE", "srcOffset")]),

        Proto("void", "CmdSaveAtomicCounters",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_PIPELINE_BIND_POINT", "pipelineBindPoint"),
             Param("uint32_t", "startCounter"),
             Param("uint32_t", "counterCount"),
             Param("VK_BUFFER", "destBuffer"),
             Param("VK_GPU_SIZE", "destOffset")]),

        Proto("VK_RESULT", "CreateFramebuffer",
            [Param("VK_DEVICE", "device"),
             Param("const VK_FRAMEBUFFER_CREATE_INFO*", "pCreateInfo"),
             Param("VK_FRAMEBUFFER*", "pFramebuffer")]),

        Proto("VK_RESULT", "CreateRenderPass",
            [Param("VK_DEVICE", "device"),
             Param("const VK_RENDER_PASS_CREATE_INFO*", "pCreateInfo"),
             Param("VK_RENDER_PASS*", "pRenderPass")]),

        Proto("void", "CmdBeginRenderPass",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("const VK_RENDER_PASS_BEGIN*", "pRenderPassBegin")]),

        Proto("void", "CmdEndRenderPass",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("VK_RENDER_PASS", "renderPass")]),

        Proto("VK_RESULT", "DbgSetValidationLevel",
            [Param("VK_DEVICE", "device"),
             Param("VK_VALIDATION_LEVEL", "validationLevel")]),

        Proto("VK_RESULT", "DbgRegisterMsgCallback",
            [Param("VK_INSTANCE", "instance"),
             Param("VK_DBG_MSG_CALLBACK_FUNCTION", "pfnMsgCallback"),
             Param("void*", "pUserData")]),

        Proto("VK_RESULT", "DbgUnregisterMsgCallback",
            [Param("VK_INSTANCE", "instance"),
             Param("VK_DBG_MSG_CALLBACK_FUNCTION", "pfnMsgCallback")]),

        Proto("VK_RESULT", "DbgSetMessageFilter",
            [Param("VK_DEVICE", "device"),
             Param("int32_t", "msgCode"),
             Param("VK_DBG_MSG_FILTER", "filter")]),

        Proto("VK_RESULT", "DbgSetObjectTag",
            [Param("VK_BASE_OBJECT", "object"),
             Param("size_t", "tagSize"),
             Param("const void*", "pTag")]),

        Proto("VK_RESULT", "DbgSetGlobalOption",
            [Param("VK_INSTANCE", "instance"),
             Param("VK_DBG_GLOBAL_OPTION", "dbgOption"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData")]),

        Proto("VK_RESULT", "DbgSetDeviceOption",
            [Param("VK_DEVICE", "device"),
             Param("VK_DBG_DEVICE_OPTION", "dbgOption"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData")]),

        Proto("void", "CmdDbgMarkerBegin",
            [Param("VK_CMD_BUFFER", "cmdBuffer"),
             Param("const char*", "pMarker")]),

        Proto("void", "CmdDbgMarkerEnd",
            [Param("VK_CMD_BUFFER", "cmdBuffer")]),
    ],
)

wsi_x11 = Extension(
    name="VK_WSI_X11",
    headers=["vkWsiX11Ext.h"],
    objects=[],
    protos=[
        Proto("VK_RESULT", "WsiX11AssociateConnection",
            [Param("VK_PHYSICAL_GPU", "gpu"),
             Param("const VK_WSI_X11_CONNECTION_INFO*", "pConnectionInfo")]),

        Proto("VK_RESULT", "WsiX11GetMSC",
            [Param("VK_DEVICE", "device"),
             Param("xcb_window_t", "window"),
             Param("xcb_randr_crtc_t", "crtc"),
             Param("uint64_t*", "pMsc")]),

        Proto("VK_RESULT", "WsiX11CreatePresentableImage",
            [Param("VK_DEVICE", "device"),
             Param("const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO*", "pCreateInfo"),
             Param("VK_IMAGE*", "pImage"),
             Param("VK_GPU_MEMORY*", "pMem")]),

        Proto("VK_RESULT", "WsiX11QueuePresent",
            [Param("VK_QUEUE", "queue"),
             Param("const VK_WSI_X11_PRESENT_INFO*", "pPresentInfo"),
             Param("VK_FENCE", "fence")]),
    ],
)

extensions = [core, wsi_x11]

object_root_list = [
    "VK_INSTANCE",
    "VK_PHYSICAL_GPU",
    "VK_BASE_OBJECT"
]

object_base_list = [
    "VK_DEVICE",
    "VK_QUEUE",
    "VK_GPU_MEMORY",
    "VK_OBJECT"
]

object_list = [
    "VK_BUFFER",
    "VK_BUFFER_VIEW",
    "VK_IMAGE",
    "VK_IMAGE_VIEW",
    "VK_COLOR_ATTACHMENT_VIEW",
    "VK_DEPTH_STENCIL_VIEW",
    "VK_SHADER",
    "VK_PIPELINE",
    "VK_SAMPLER",
    "VK_DESCRIPTOR_SET",
    "VK_DESCRIPTOR_SET_LAYOUT",
    "VK_DESCRIPTOR_SET_LAYOUT_CHAIN",
    "VK_DESCRIPTOR_POOL",
    "VK_DYNAMIC_STATE_OBJECT",
    "VK_CMD_BUFFER",
    "VK_FENCE",
    "VK_SEMAPHORE",
    "VK_EVENT",
    "VK_QUERY_POOL",
    "VK_FRAMEBUFFER",
    "VK_RENDER_PASS"
]

object_dynamic_state_list = [
    "VK_DYNAMIC_VP_STATE_OBJECT",
    "VK_DYNAMIC_RS_STATE_OBJECT",
    "VK_DYNAMIC_CB_STATE_OBJECT",
    "VK_DYNAMIC_DS_STATE_OBJECT"
]

object_type_list = object_root_list + object_base_list + object_list + object_dynamic_state_list

object_parent_list = ["VK_BASE_OBJECT", "VK_OBJECT", "VK_DYNAMIC_STATE_OBJECT"]

headers = []
objects = []
protos = []
for ext in extensions:
    headers.extend(ext.headers)
    objects.extend(ext.objects)
    protos.extend(ext.protos)

proto_names = [proto.name for proto in protos]

def parse_vk_h(filename):
    # read object and protoype typedefs
    object_lines = []
    proto_lines = []
    with open(filename, "r") as fp:
        for line in fp:
            line = line.strip()
            if line.startswith("VK_DEFINE"):
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
        first, rest = line.split(" (VKAPI *vk")
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
    ext = Extension("VK_CORE",
            headers=["vulkan.h", "vkDbg.h"],
            objects=object_lines,
            protos=protos)
    print("core =", str(ext))

    print("")
    print("typedef struct _VK_LAYER_DISPATCH_TABLE")
    print("{")
    for proto in ext.protos:
        print("    vk%sType %s;" % (proto.name, proto.name))
    print("} VK_LAYER_DISPATCH_TABLE;")

if __name__ == "__main__":
    parse_vk_h("include/vulkan.h")
