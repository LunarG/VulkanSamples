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
        "VkInstance",
        "VkPhysicalDevice",
        "VkDevice",
        "VkQueue",
        "VkDeviceMemory",
        "VkObject",
        "VkBuffer",
        "VkBufferView",
        "VkImage",
        "VkImageView",
        "VkColorAttachmentView",
        "VkDepthStencilView",
        "VkShader",
        "VkPipeline",
        "VkSampler",
        "VkDescriptorSet",
        "VkDescriptorSetLayout",
        "VkPipelineLayout",
        "VkDescriptorPool",
        "VkDynamicStateObject",
        "VkDynamicVpState",
        "VkDynamicRsState",
        "VkDynamicCbState",
        "VkDynamicDsState",
        "VkCmdBuffer",
        "VkFence",
        "VkSemaphore",
        "VkEvent",
        "VkQueryPool",
        "VkFramebuffer",
        "VkRenderPass",
    ],
    protos=[
        Proto("VkResult", "CreateInstance",
            [Param("const VkInstanceCreateInfo*", "pCreateInfo"),
             Param("VkInstance*", "pInstance")]),

        Proto("VkResult", "DestroyInstance",
            [Param("VkInstance", "instance")]),

        Proto("VkResult", "EnumeratePhysicalDevices",
            [Param("VkInstance", "instance"),
             Param("uint32_t*", "pPhysicalDeviceCount"),
             Param("VkPhysicalDevice*", "pPhysicalDevices")]),

        Proto("VkResult", "GetPhysicalDeviceInfo",
            [Param("VkPhysicalDevice", "gpu"),
             Param("VkPhysicalDeviceInfoType", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("void*", "GetInstanceProcAddr",
            [Param("VkInstance", "instance"),
             Param("const char*", "pName")]),

        Proto("void*", "GetProcAddr",
            [Param("VkPhysicalDevice", "gpu"),
             Param("const char*", "pName")]),

        Proto("VkResult", "CreateDevice",
            [Param("VkPhysicalDevice", "gpu"),
             Param("const VkDeviceCreateInfo*", "pCreateInfo"),
             Param("VkDevice*", "pDevice")]),

        Proto("VkResult", "DestroyDevice",
            [Param("VkDevice", "device")]),

        Proto("VkResult", "GetGlobalExtensionInfo",
            [Param("VkExtensionInfoType", "infoType"),
             Param("uint32_t", "extensionIndex"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VkResult", "GetPhysicalDeviceExtensionInfo",
            [Param("VkPhysicalDevice", "gpu"),
             Param("VkExtensionInfoType", "infoType"),
             Param("uint32_t", "extensionIndex"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VkResult", "EnumerateLayers",
            [Param("VkPhysicalDevice", "gpu"),
             Param("size_t", "maxStringSize"),
             Param("size_t*", "pLayerCount"),
             Param("char* const*", "pOutLayers"),
             Param("void*", "pReserved")]),

        Proto("VkResult", "GetDeviceQueue",
            [Param("VkDevice", "device"),
             Param("uint32_t", "queueNodeIndex"),
             Param("uint32_t", "queueIndex"),
             Param("VkQueue*", "pQueue")]),

        Proto("VkResult", "QueueSubmit",
            [Param("VkQueue", "queue"),
             Param("uint32_t", "cmdBufferCount"),
             Param("const VkCmdBuffer*", "pCmdBuffers"),
             Param("VkFence", "fence")]),

        Proto("VkResult", "QueueWaitIdle",
            [Param("VkQueue", "queue")]),

        Proto("VkResult", "DeviceWaitIdle",
            [Param("VkDevice", "device")]),

        Proto("VkResult", "AllocMemory",
            [Param("VkDevice", "device"),
             Param("const VkMemoryAllocInfo*", "pAllocInfo"),
             Param("VkDeviceMemory*", "pMem")]),

        Proto("VkResult", "FreeMemory",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "mem")]),

        Proto("VkResult", "SetMemoryPriority",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "mem"),
             Param("VkMemoryPriority", "priority")]),

        Proto("VkResult", "MapMemory",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "mem"),
             Param("VkDeviceSize", "offset"),
             Param("VkDeviceSize", "size"),
             Param("VkFlags", "flags"),
             Param("void**", "ppData")]),

        Proto("VkResult", "UnmapMemory",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "mem")]),

        Proto("VkResult", "FlushMappedMemoryRanges",
            [Param("VkDevice", "device"),
             Param("uint32_t", "memRangeCount"),
             Param("const VkMappedMemoryRange*", "pMemRanges")]),

        Proto("VkResult", "InvalidateMappedMemoryRanges",
            [Param("VkDevice", "device"),
             Param("uint32_t", "memRangeCount"),
             Param("const VkMappedMemoryRange*", "pMemRanges")]),

        Proto("VkResult", "PinSystemMemory",
            [Param("VkDevice", "device"),
             Param("const void*", "pSysMem"),
             Param("size_t", "memSize"),
             Param("VkDeviceMemory*", "pMem")]),

        Proto("VkResult", "GetMultiDeviceCompatibility",
            [Param("VkPhysicalDevice", "gpu0"),
             Param("VkPhysicalDevice", "gpu1"),
             Param("VkPhysicalDeviceCompatibilityInfo*", "pInfo")]),

        Proto("VkResult", "OpenSharedMemory",
            [Param("VkDevice", "device"),
             Param("const VkMemoryOpenInfo*", "pOpenInfo"),
             Param("VkDeviceMemory*", "pMem")]),

        Proto("VkResult", "OpenSharedSemaphore",
            [Param("VkDevice", "device"),
             Param("const VkSemaphoreOpenInfo*", "pOpenInfo"),
             Param("VkSemaphore*", "pSemaphore")]),

        Proto("VkResult", "OpenPeerMemory",
            [Param("VkDevice", "device"),
             Param("const VkPeerMemoryOpenInfo*", "pOpenInfo"),
             Param("VkDeviceMemory*", "pMem")]),

        Proto("VkResult", "OpenPeerImage",
            [Param("VkDevice", "device"),
             Param("const VkPeerImageOpenInfo*", "pOpenInfo"),
             Param("VkImage*", "pImage"),
             Param("VkDeviceMemory*", "pMem")]),

        Proto("VkResult", "DestroyObject",
            [Param("VkDevice", "device"),
             Param("VkObjectType", "objType"), 
             Param("VkObject", "object")]),

        Proto("VkResult", "GetObjectInfo",
            [Param("VkDevice", "device"),
             Param("VkObjectType", "objType"), 
             Param("VkObject", "object"),
             Param("VkObjectInfoType", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VkResult", "BindObjectMemory",
            [Param("VkDevice", "device"),
             Param("VkObjectType", "objType"), 
             Param("VkObject", "object"),
             Param("uint32_t", "allocationIdx"),
             Param("VkDeviceMemory", "mem"),
             Param("VkDeviceSize", "offset")]),

        Proto("VkResult", "QueueBindSparseBufferMemory",
            [Param("VkQueue", "queue"),
             Param("VkBuffer", "buffer"),
             Param("uint32_t", "allocationIdx"),
             Param("VkDeviceSize", "rangeOffset"),
             Param("VkDeviceSize", "rangeSize"),
             Param("VkDeviceMemory", "mem"),
             Param("VkDeviceSize", "memOffset")]),

        Proto("VkResult", "QueueBindSparseImageMemory",
            [Param("VkQueue", "queue"),
             Param("VkImage", "image"),
             Param("uint32_t", "allocationIdx"),
             Param("const VkImageMemoryBindInfo*", "pBindInfo"),
             Param("VkDeviceMemory", "mem"),
             Param("VkDeviceSize", "memOffset")]),

        Proto("VkResult", "CreateFence",
            [Param("VkDevice", "device"),
             Param("const VkFenceCreateInfo*", "pCreateInfo"),
             Param("VkFence*", "pFence")]),

        Proto("VkResult", "ResetFences",
            [Param("VkDevice", "device"),
             Param("uint32_t", "fenceCount"),
             Param("VkFence*", "pFences")]),

        Proto("VkResult", "GetFenceStatus",
            [Param("VkDevice", "device"),
             Param("VkFence", "fence")]),

        Proto("VkResult", "WaitForFences",
            [Param("VkDevice", "device"),
             Param("uint32_t", "fenceCount"),
             Param("const VkFence*", "pFences"),
             Param("bool32_t", "waitAll"),
             Param("uint64_t", "timeout")]),

        Proto("VkResult", "CreateSemaphore",
            [Param("VkDevice", "device"),
             Param("const VkSemaphoreCreateInfo*", "pCreateInfo"),
             Param("VkSemaphore*", "pSemaphore")]),

        Proto("VkResult", "QueueSignalSemaphore",
            [Param("VkQueue", "queue"),
             Param("VkSemaphore", "semaphore")]),

        Proto("VkResult", "QueueWaitSemaphore",
            [Param("VkQueue", "queue"),
             Param("VkSemaphore", "semaphore")]),

        Proto("VkResult", "CreateEvent",
            [Param("VkDevice", "device"),
             Param("const VkEventCreateInfo*", "pCreateInfo"),
             Param("VkEvent*", "pEvent")]),

        Proto("VkResult", "GetEventStatus",
            [Param("VkDevice", "device"),
             Param("VkEvent", "event")]),

        Proto("VkResult", "SetEvent",
            [Param("VkDevice", "device"),
             Param("VkEvent", "event")]),

        Proto("VkResult", "ResetEvent",
            [Param("VkDevice", "device"),
             Param("VkEvent", "event")]),

        Proto("VkResult", "CreateQueryPool",
            [Param("VkDevice", "device"),
             Param("const VkQueryPoolCreateInfo*", "pCreateInfo"),
             Param("VkQueryPool*", "pQueryPool")]),

        Proto("VkResult", "GetQueryPoolResults",
            [Param("VkDevice", "device"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "startQuery"),
             Param("uint32_t", "queryCount"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData"),
             Param("VkQueryResultFlags", "flags")]),

        Proto("VkResult", "GetFormatInfo",
            [Param("VkDevice", "device"),
             Param("VkFormat", "format"),
             Param("VkFormatInfoType", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VkResult", "CreateBuffer",
            [Param("VkDevice", "device"),
             Param("const VkBufferCreateInfo*", "pCreateInfo"),
             Param("VkBuffer*", "pBuffer")]),

        Proto("VkResult", "CreateBufferView",
            [Param("VkDevice", "device"),
             Param("const VkBufferViewCreateInfo*", "pCreateInfo"),
             Param("VkBufferView*", "pView")]),

        Proto("VkResult", "CreateImage",
            [Param("VkDevice", "device"),
             Param("const VkImageCreateInfo*", "pCreateInfo"),
             Param("VkImage*", "pImage")]),

        Proto("VkResult", "GetImageSubresourceInfo",
            [Param("VkDevice", "device"),
             Param("VkImage", "image"),
             Param("const VkImageSubresource*", "pSubresource"),
             Param("VkSubresourceInfoType", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VkResult", "CreateImageView",
            [Param("VkDevice", "device"),
             Param("const VkImageViewCreateInfo*", "pCreateInfo"),
             Param("VkImageView*", "pView")]),

        Proto("VkResult", "CreateColorAttachmentView",
            [Param("VkDevice", "device"),
             Param("const VkColorAttachmentViewCreateInfo*", "pCreateInfo"),
             Param("VkColorAttachmentView*", "pView")]),

        Proto("VkResult", "CreateDepthStencilView",
            [Param("VkDevice", "device"),
             Param("const VkDepthStencilViewCreateInfo*", "pCreateInfo"),
             Param("VkDepthStencilView*", "pView")]),

        Proto("VkResult", "CreateShader",
            [Param("VkDevice", "device"),
             Param("const VkShaderCreateInfo*", "pCreateInfo"),
             Param("VkShader*", "pShader")]),

        Proto("VkResult", "CreateGraphicsPipeline",
            [Param("VkDevice", "device"),
             Param("const VkGraphicsPipelineCreateInfo*", "pCreateInfo"),
             Param("VkPipeline*", "pPipeline")]),

        Proto("VkResult", "CreateGraphicsPipelineDerivative",
            [Param("VkDevice", "device"),
             Param("const VkGraphicsPipelineCreateInfo*", "pCreateInfo"),
             Param("VkPipeline", "basePipeline"),
             Param("VkPipeline*", "pPipeline")]),

        Proto("VkResult", "CreateComputePipeline",
            [Param("VkDevice", "device"),
             Param("const VkComputePipelineCreateInfo*", "pCreateInfo"),
             Param("VkPipeline*", "pPipeline")]),

        Proto("VkResult", "StorePipeline",
            [Param("VkDevice", "device"),
             Param("VkPipeline", "pipeline"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VkResult", "LoadPipeline",
            [Param("VkDevice", "device"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData"),
             Param("VkPipeline*", "pPipeline")]),

        Proto("VkResult", "LoadPipelineDerivative",
            [Param("VkDevice", "device"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData"),
             Param("VkPipeline", "basePipeline"),
             Param("VkPipeline*", "pPipeline")]),

        Proto("VkResult", "CreatePipelineLayout",
            [Param("VkDevice", "device"),
             Param("const VkPipelineLayoutCreateInfo*", "pCreateInfo"),
             Param("VkPipelineLayout*", "pPipelineLayout")]),

        Proto("VkResult", "CreateSampler",
            [Param("VkDevice", "device"),
             Param("const VkSamplerCreateInfo*", "pCreateInfo"),
             Param("VkSampler*", "pSampler")]),

        Proto("VkResult", "CreateDescriptorSetLayout",
            [Param("VkDevice", "device"),
             Param("const VkDescriptorSetLayoutCreateInfo*", "pCreateInfo"),
             Param("VkDescriptorSetLayout*", "pSetLayout")]),

        Proto("VkResult", "BeginDescriptorPoolUpdate",
            [Param("VkDevice", "device"),
             Param("VkDescriptorUpdateMode", "updateMode")]),

        Proto("VkResult", "EndDescriptorPoolUpdate",
            [Param("VkDevice", "device"),
             Param("VkCmdBuffer", "cmd")]),

        Proto("VkResult", "CreateDescriptorPool",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPoolUsage", "poolUsage"),
             Param("uint32_t", "maxSets"),
             Param("const VkDescriptorPoolCreateInfo*", "pCreateInfo"),
             Param("VkDescriptorPool*", "pDescriptorPool")]),

        Proto("VkResult", "ResetDescriptorPool",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPool", "descriptorPool")]),

        Proto("VkResult", "AllocDescriptorSets",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPool", "descriptorPool"),
             Param("VkDescriptorSetUsage", "setUsage"),
             Param("uint32_t", "count"),
             Param("const VkDescriptorSetLayout*", "pSetLayouts"),
             Param("VkDescriptorSet*", "pDescriptorSets"),
             Param("uint32_t*", "pCount")]),

        Proto("void", "ClearDescriptorSets",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPool", "descriptorPool"),
             Param("uint32_t", "count"),
             Param("const VkDescriptorSet*", "pDescriptorSets")]),

        Proto("void", "UpdateDescriptors",
            [Param("VkDevice", "device"),
             Param("VkDescriptorSet", "descriptorSet"),
             Param("uint32_t", "updateCount"),
             Param("const void**", "ppUpdateArray")]),

        Proto("VkResult", "CreateDynamicViewportState",
            [Param("VkDevice", "device"),
             Param("const VkDynamicVpStateCreateInfo*", "pCreateInfo"),
             Param("VkDynamicVpState*", "pState")]),

        Proto("VkResult", "CreateDynamicRasterState",
            [Param("VkDevice", "device"),
             Param("const VkDynamicRsStateCreateInfo*", "pCreateInfo"),
             Param("VkDynamicRsState*", "pState")]),

        Proto("VkResult", "CreateDynamicColorBlendState",
            [Param("VkDevice", "device"),
             Param("const VkDynamicCbStateCreateInfo*", "pCreateInfo"),
             Param("VkDynamicCbState*", "pState")]),

        Proto("VkResult", "CreateDynamicDepthStencilState",
            [Param("VkDevice", "device"),
             Param("const VkDynamicDsStateCreateInfo*", "pCreateInfo"),
             Param("VkDynamicDsState*", "pState")]),

        Proto("VkResult", "CreateCommandBuffer",
            [Param("VkDevice", "device"),
             Param("const VkCmdBufferCreateInfo*", "pCreateInfo"),
             Param("VkCmdBuffer*", "pCmdBuffer")]),

        Proto("VkResult", "BeginCommandBuffer",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("const VkCmdBufferBeginInfo*", "pBeginInfo")]),

        Proto("VkResult", "EndCommandBuffer",
            [Param("VkCmdBuffer", "cmdBuffer")]),

        Proto("VkResult", "ResetCommandBuffer",
            [Param("VkCmdBuffer", "cmdBuffer")]),

        Proto("void", "CmdBindPipeline",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkPipelineBindPoint", "pipelineBindPoint"),
             Param("VkPipeline", "pipeline")]),

        Proto("void", "CmdBindDynamicStateObject",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkStateBindPoint", "stateBindPoint"),
             Param("VkDynamicStateObject", "state")]),

        Proto("void", "CmdBindDescriptorSets",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkPipelineBindPoint", "pipelineBindPoint"),
             Param("uint32_t", "firstSet"),
             Param("uint32_t", "setCount"),
             Param("const VkDescriptorSet*", "pDescriptorSets"),
             Param("uint32_t", "dynamicOffsetCount"),
             Param("const uint32_t*", "pDynamicOffsets")]),

        Proto("void", "CmdBindVertexBuffers",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("uint32_t", "startBinding"),
             Param("uint32_t", "bindingCount"),
             Param("const VkBuffer*", "pBuffers"),
             Param("const VkDeviceSize*", "pOffsets")]),


        Proto("void", "CmdBindIndexBuffer",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset"),
             Param("VkIndexType", "indexType")]),

        Proto("void", "CmdDraw",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("uint32_t", "firstVertex"),
             Param("uint32_t", "vertexCount"),
             Param("uint32_t", "firstInstance"),
             Param("uint32_t", "instanceCount")]),

        Proto("void", "CmdDrawIndexed",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("uint32_t", "firstIndex"),
             Param("uint32_t", "indexCount"),
             Param("int32_t", "vertexOffset"),
             Param("uint32_t", "firstInstance"),
             Param("uint32_t", "instanceCount")]),

        Proto("void", "CmdDrawIndirect",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset"),
             Param("uint32_t", "count"),
             Param("uint32_t", "stride")]),

        Proto("void", "CmdDrawIndexedIndirect",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset"),
             Param("uint32_t", "count"),
             Param("uint32_t", "stride")]),

        Proto("void", "CmdDispatch",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("uint32_t", "x"),
             Param("uint32_t", "y"),
             Param("uint32_t", "z")]),

        Proto("void", "CmdDispatchIndirect",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset")]),

        Proto("void", "CmdCopyBuffer",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkBuffer", "srcBuffer"),
             Param("VkBuffer", "destBuffer"),
             Param("uint32_t", "regionCount"),
             Param("const VkBufferCopy*", "pRegions")]),

        Proto("void", "CmdCopyImage",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkImage", "srcImage"),
             Param("VkImageLayout", "srcImageLayout"),
             Param("VkImage", "destImage"),
             Param("VkImageLayout", "destImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VkImageCopy*", "pRegions")]),

        Proto("void", "CmdBlitImage",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkImage", "srcImage"),
             Param("VkImageLayout", "srcImageLayout"),
             Param("VkImage", "destImage"),
             Param("VkImageLayout", "destImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VkImageBlit*", "pRegions"),
             Param("VkTexFilter", "filter")]),

        Proto("void", "CmdCopyBufferToImage",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkBuffer", "srcBuffer"),
             Param("VkImage", "destImage"),
             Param("VkImageLayout", "destImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VkBufferImageCopy*", "pRegions")]),

        Proto("void", "CmdCopyImageToBuffer",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkImage", "srcImage"),
             Param("VkImageLayout", "srcImageLayout"),
             Param("VkBuffer", "destBuffer"),
             Param("uint32_t", "regionCount"),
             Param("const VkBufferImageCopy*", "pRegions")]),

        Proto("void", "CmdUpdateBuffer",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkBuffer", "destBuffer"),
             Param("VkDeviceSize", "destOffset"),
             Param("VkDeviceSize", "dataSize"),
             Param("const uint32_t*", "pData")]),

        Proto("void", "CmdFillBuffer",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkBuffer", "destBuffer"),
             Param("VkDeviceSize", "destOffset"),
             Param("VkDeviceSize", "fillSize"),
             Param("uint32_t", "data")]),

        Proto("void", "CmdClearColorImage",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkImage", "image"),
             Param("VkImageLayout", "imageLayout"),
             Param("const VkClearColor*", "pColor"),
             Param("uint32_t", "rangeCount"),
             Param("const VkImageSubresourceRange*", "pRanges")]),

        Proto("void", "CmdClearDepthStencil",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkImage", "image"),
             Param("VkImageLayout", "imageLayout"),
             Param("float", "depth"),
             Param("uint32_t", "stencil"),
             Param("uint32_t", "rangeCount"),
             Param("const VkImageSubresourceRange*", "pRanges")]),

        Proto("void", "CmdResolveImage",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkImage", "srcImage"),
             Param("VkImageLayout", "srcImageLayout"),
             Param("VkImage", "destImage"),
             Param("VkImageLayout", "destImageLayout"),
             Param("uint32_t", "regionCount"),
             Param("const VkImageResolve*", "pRegions")]),

        Proto("void", "CmdSetEvent",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkEvent", "event"),
             Param("VkPipeEvent", "pipeEvent")]),

        Proto("void", "CmdResetEvent",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkEvent", "event"),
             Param("VkPipeEvent", "pipeEvent")]),

        Proto("void", "CmdWaitEvents",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkWaitEvent", "waitEvent"),
             Param("uint32_t", "eventCount"),
             Param("const VkEvent*", "pEvents"),
             Param("uint32_t", "memBarrierCount"),
             Param("const void**", "ppMemBarriers")]),

        Proto("void", "CmdPipelineBarrier",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkWaitEvent", "waitEvent"),
             Param("uint32_t", "pipeEventCount"),
             Param("const VkPipeEvent*", "pPipeEvents"),
             Param("uint32_t", "memBarrierCount"),
             Param("const void**", "ppMemBarriers")]),

        Proto("void", "CmdBeginQuery",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "slot"),
             Param("VkFlags", "flags")]),

        Proto("void", "CmdEndQuery",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "slot")]),

        Proto("void", "CmdResetQueryPool",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "startQuery"),
             Param("uint32_t", "queryCount")]),

        Proto("void", "CmdWriteTimestamp",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkTimestampType", "timestampType"),
             Param("VkBuffer", "destBuffer"),
             Param("VkDeviceSize", "destOffset")]),

        Proto("void", "CmdCopyQueryPoolResults",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "startQuery"),
             Param("uint32_t", "queryCount"),
             Param("VkBuffer", "destBuffer"),
             Param("VkDeviceSize", "destOffset"),
             Param("VkDeviceSize", "destStride"),
             Param("VkFlags", "flags")]),

        Proto("void", "CmdInitAtomicCounters",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkPipelineBindPoint", "pipelineBindPoint"),
             Param("uint32_t", "startCounter"),
             Param("uint32_t", "counterCount"),
             Param("const uint32_t*", "pData")]),

        Proto("void", "CmdLoadAtomicCounters",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkPipelineBindPoint", "pipelineBindPoint"),
             Param("uint32_t", "startCounter"),
             Param("uint32_t", "counterCount"),
             Param("VkBuffer", "srcBuffer"),
             Param("VkDeviceSize", "srcOffset")]),

        Proto("void", "CmdSaveAtomicCounters",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkPipelineBindPoint", "pipelineBindPoint"),
             Param("uint32_t", "startCounter"),
             Param("uint32_t", "counterCount"),
             Param("VkBuffer", "destBuffer"),
             Param("VkDeviceSize", "destOffset")]),

        Proto("VkResult", "CreateFramebuffer",
            [Param("VkDevice", "device"),
             Param("const VkFramebufferCreateInfo*", "pCreateInfo"),
             Param("VkFramebuffer*", "pFramebuffer")]),

        Proto("VkResult", "CreateRenderPass",
            [Param("VkDevice", "device"),
             Param("const VkRenderPassCreateInfo*", "pCreateInfo"),
             Param("VkRenderPass*", "pRenderPass")]),

        Proto("void", "CmdBeginRenderPass",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("const VkRenderPassBegin*", "pRenderPassBegin")]),

        Proto("void", "CmdEndRenderPass",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkRenderPass", "renderPass")]),

        Proto("VkResult", "DbgSetValidationLevel",
            [Param("VkDevice", "device"),
             Param("VkValidationLevel", "validationLevel")]),

        Proto("VkResult", "DbgRegisterMsgCallback",
            [Param("VkInstance", "instance"),
             Param("VK_DBG_MSG_CALLBACK_FUNCTION", "pfnMsgCallback"),
             Param("void*", "pUserData")]),

        Proto("VkResult", "DbgUnregisterMsgCallback",
            [Param("VkInstance", "instance"),
             Param("VK_DBG_MSG_CALLBACK_FUNCTION", "pfnMsgCallback")]),

        Proto("VkResult", "DbgSetMessageFilter",
            [Param("VkDevice", "device"),
             Param("int32_t", "msgCode"),
             Param("VK_DBG_MSG_FILTER", "filter")]),

        Proto("VkResult", "DbgSetObjectTag",
            [Param("VkDevice", "device"),
             Param("VkObject", "object"),
             Param("size_t", "tagSize"),
             Param("const void*", "pTag")]),

        Proto("VkResult", "DbgSetGlobalOption",
            [Param("VkInstance", "instance"),
             Param("VK_DBG_GLOBAL_OPTION", "dbgOption"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData")]),

        Proto("VkResult", "DbgSetDeviceOption",
            [Param("VkDevice", "device"),
             Param("VK_DBG_DEVICE_OPTION", "dbgOption"),
             Param("size_t", "dataSize"),
             Param("const void*", "pData")]),

        Proto("void", "CmdDbgMarkerBegin",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("const char*", "pMarker")]),

        Proto("void", "CmdDbgMarkerEnd",
            [Param("VkCmdBuffer", "cmdBuffer")]),
    ],
)

wsi_lunarg = Extension(
    name="VK_WSI_LunarG",
    headers=["vk_wsi_lunarg.h"],
    objects=[
        "VkDisplayWSI",
        "VkSwapChainWSI",
    ],
    protos=[
        Proto("VkResult", "GetDisplayInfoWSI",
            [Param("VkDisplayWSI", "display"),
             Param("VkDisplayInfoTypeWSI", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VkResult", "CreateSwapChainWSI",
            [Param("VkDevice", "device"),
             Param("const VkSwapChainCreateInfoWSI*", "pCreateInfo"),
             Param("VkSwapChainWSI*", "pSwapChain")]),

        Proto("VkResult", "DestroySwapChainWSI",
            [Param("VkSwapChainWSI", "swapChain")]),

        Proto("VkResult", "GetSwapChainInfoWSI",
            [Param("VkSwapChainWSI", "swapChain"),
             Param("VkSwapChainInfoTypeWSI", "infoType"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData")]),

        Proto("VkResult", "QueuePresentWSI",
            [Param("VkQueue", "queue"),
             Param("const VkPresentInfoWSI*", "pPresentInfo")]),
    ],
)

extensions = [core, wsi_lunarg]

object_root_list = [
    "VkInstance",
    "VkPhysicalDevice",
    "VkDisplayWSI",
    "VkSwapChainWSI",
]

object_base_list = [
    "VkDevice",
    "VkQueue",
    "VkDeviceMemory",
    "VkObject"
]

object_list = [
    "VkBuffer",
    "VkBufferView",
    "VkImage",
    "VkImageView",
    "VkColorAttachmentView",
    "VkDepthStencilView",
    "VkShader",
    "VkPipeline",
    "VkPipelineLayout",
    "VkSampler",
    "VkDescriptorSet",
    "VkDescriptorSetLayout",
    "VkDescriptorPool",
    "VkDynamicStateObject",
    "VkCmdBuffer",
    "VkFence",
    "VkSemaphore",
    "VkEvent",
    "VkQueryPool",
    "VkFramebuffer",
    "VkRenderPass"
]

object_dynamic_state_list = [
    "VkDynamicVpState",
    "VkDynamicRsState",
    "VkDynamicCbState",
    "VkDynamicDsState"
]

object_type_list = object_root_list + object_base_list + object_list + object_dynamic_state_list

object_parent_list = ["VkObject", "VkDynamicStateObject"]

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
        first, rest = line.split(" (VKAPI *PFN_vk")
        second, third = rest.split(")(")

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
    print("typedef struct VkLayerDispatchTable_")
    print("{")
    for proto in ext.protos:
        print("    vk%sType %s;" % (proto.name, proto.name))
    print("} VkLayerDispatchTable;")

if __name__ == "__main__":
    parse_vk_h("include/vulkan.h")
