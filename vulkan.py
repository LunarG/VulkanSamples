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
    headers=["vulkan.h", "vk_debug_report_lunarg.h"],
    objects=[
        "VkInstance",
        "VkPhysicalDevice",
        "VkDevice",
        "VkQueue",
        "VkCmdBuffer",
        "VkCmdPool",
        "VkFence",
        "VkDeviceMemory",
        "VkBuffer",
        "VkImage",
        "VkSemaphore",
        "VkEvent",
        "VkQueryPool",
        "VkBufferView",
        "VkImageView",
        "VkAttachmentView",
        "VkShaderModule",
        "VkShader",
        "VkPipelineCache",
        "VkPipelineLayout",
        "VkPipeline",
        "VkDescriptorSetLayout",
        "VkSampler",
        "VkDescriptorPool",
        "VkDescriptorSet",
        "VkDynamicViewportState",
        "VkDynamicRasterLineState",
        "VkDynamicRasterDepthBiasState",
        "VkDynamicColorBlendState",
        "VkDynamicDepthState",
        "VkDynamicStencilState",
        "VkRenderPass",
        "VkFramebuffer",
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

        Proto("VkResult", "GetPhysicalDeviceFeatures",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkPhysicalDeviceFeatures*", "pFeatures")]),

        Proto("VkResult", "GetPhysicalDeviceFormatProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkFormat", "format"),
             Param("VkFormatProperties*", "pFormatProperties")]),

        Proto("VkResult", "GetPhysicalDeviceImageFormatProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkFormat", "format"),
             Param("VkImageType", "type"),
             Param("VkImageTiling", "tiling"),
             Param("VkImageUsageFlags", "usage"),
             Param("VkImageFormatProperties*", "pImageFormatProperties")]),

        Proto("VkResult", "GetPhysicalDeviceLimits",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkPhysicalDeviceLimits*", "pLimits")]),

        Proto("PFN_vkVoidFunction", "GetInstanceProcAddr",
            [Param("VkInstance", "instance"),
             Param("const char*", "pName")]),

        Proto("PFN_vkVoidFunction", "GetDeviceProcAddr",
            [Param("VkDevice", "device"),
             Param("const char*", "pName")]),

        Proto("VkResult", "CreateDevice",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("const VkDeviceCreateInfo*", "pCreateInfo"),
             Param("VkDevice*", "pDevice")]),

        Proto("VkResult", "DestroyDevice",
            [Param("VkDevice", "device")]),

        Proto("VkResult", "GetPhysicalDeviceProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkPhysicalDeviceProperties*", "pProperties")]),

        Proto("VkResult", "GetPhysicalDeviceQueueFamilyProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t*", "pCount"),
             Param("VkQueueFamilyProperties*", "pQueueFamilyProperties")]),

        Proto("VkResult", "GetPhysicalDeviceMemoryProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkPhysicalDeviceMemoryProperties*", "pMemoryProperties")]),

        Proto("VkResult", "GetGlobalExtensionProperties",
            [Param("const char*", "pLayerName"),
             Param("uint32_t*", "pCount"),
             Param("VkExtensionProperties*", "pProperties")]),

        Proto("VkResult", "GetPhysicalDeviceExtensionProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("const char*", "pLayerName"),
             Param("uint32_t*", "pCount"),
             Param("VkExtensionProperties*", "pProperties")]),

        Proto("VkResult", "GetGlobalLayerProperties",
            [Param("uint32_t*", "pCount"),
             Param("VkLayerProperties*", "pProperties")]),

        Proto("VkResult", "GetPhysicalDeviceLayerProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t*", "pCount"),
             Param("VkLayerProperties*", "pProperties")]),

        Proto("VkResult", "GetDeviceQueue",
            [Param("VkDevice", "device"),
             Param("uint32_t", "queueFamilyIndex"),
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

        Proto("VkResult", "MapMemory",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "mem"),
             Param("VkDeviceSize", "offset"),
             Param("VkDeviceSize", "size"),
             Param("VkMemoryMapFlags", "flags"),
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

        Proto("VkResult", "GetDeviceMemoryCommitment",
            [Param("VkDevice", "device"),
             Param("VkDeviceMemory", "memory"),
             Param("VkDeviceSize*", "pCommittedMemoryInBytes")]),

        Proto("VkResult", "BindBufferMemory",
            [Param("VkDevice", "device"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceMemory", "mem"),
             Param("VkDeviceSize", "memOffset")]),

        Proto("VkResult", "BindImageMemory",
            [Param("VkDevice", "device"),
             Param("VkImage", "image"),
             Param("VkDeviceMemory", "mem"),
             Param("VkDeviceSize", "memOffset")]),

        Proto("VkResult", "GetBufferMemoryRequirements",
            [Param("VkDevice", "device"),
             Param("VkBuffer", "buffer"),
             Param("VkMemoryRequirements*", "pMemoryRequirements")]),

        Proto("VkResult", "GetImageMemoryRequirements",
            [Param("VkDevice", "device"),
             Param("VkImage", "image"),
             Param("VkMemoryRequirements*", "pMemoryRequirements")]),

        Proto("VkResult", "GetImageSparseMemoryRequirements",
            [Param("VkDevice", "device"),
             Param("VkImage", "image"),
             Param("uint32_t*", "pNumRequirements"),
             Param("VkSparseImageMemoryRequirements*", "pSparseMemoryRequirements")]),

        Proto("VkResult", "GetPhysicalDeviceSparseImageFormatProperties",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("VkFormat", "format"),
             Param("VkImageType", "type"),
             Param("uint32_t", "samples"),
             Param("VkImageUsageFlags", "usage"),
             Param("VkImageTiling", "tiling"),
             Param("uint32_t*", "pNumProperties"),
             Param("VkSparseImageFormatProperties*", "pProperties")]),

        Proto("VkResult", "QueueBindSparseBufferMemory",
            [Param("VkQueue", "queue"),
             Param("VkBuffer", "buffer"),
             Param("uint32_t", "numBindings"),
             Param("const VkSparseMemoryBindInfo*", "pBindInfo")]),

        Proto("VkResult", "QueueBindSparseImageOpaqueMemory",
            [Param("VkQueue", "queue"),
             Param("VkImage", "image"),
             Param("uint32_t", "numBindings"),
             Param("const VkSparseMemoryBindInfo*", "pBindInfo")]),

        Proto("VkResult", "QueueBindSparseImageMemory",
            [Param("VkQueue", "queue"),
             Param("VkImage", "image"),
             Param("uint32_t", "numBindings"),
             Param("const VkSparseImageMemoryBindInfo*", "pBindInfo")]),

        Proto("VkResult", "CreateFence",
            [Param("VkDevice", "device"),
             Param("const VkFenceCreateInfo*", "pCreateInfo"),
             Param("VkFence*", "pFence")]),

        Proto("VkResult", "DestroyFence",
            [Param("VkDevice", "device"),
             Param("VkFence", "fence")]),

        Proto("VkResult", "ResetFences",
            [Param("VkDevice", "device"),
             Param("uint32_t", "fenceCount"),
             Param("const VkFence*", "pFences")]),

        Proto("VkResult", "GetFenceStatus",
            [Param("VkDevice", "device"),
             Param("VkFence", "fence")]),

        Proto("VkResult", "WaitForFences",
            [Param("VkDevice", "device"),
             Param("uint32_t", "fenceCount"),
             Param("const VkFence*", "pFences"),
             Param("VkBool32", "waitAll"),
             Param("uint64_t", "timeout")]),

        Proto("VkResult", "CreateSemaphore",
            [Param("VkDevice", "device"),
             Param("const VkSemaphoreCreateInfo*", "pCreateInfo"),
             Param("VkSemaphore*", "pSemaphore")]),

        Proto("VkResult", "DestroySemaphore",
            [Param("VkDevice", "device"),
             Param("VkSemaphore", "semaphore")]),

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

        Proto("VkResult", "DestroyEvent",
            [Param("VkDevice", "device"),
             Param("VkEvent", "event")]),

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

        Proto("VkResult", "DestroyQueryPool",
            [Param("VkDevice", "device"),
             Param("VkQueryPool", "queryPool")]),

        Proto("VkResult", "GetQueryPoolResults",
            [Param("VkDevice", "device"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "startQuery"),
             Param("uint32_t", "queryCount"),
             Param("size_t*", "pDataSize"),
             Param("void*", "pData"),
             Param("VkQueryResultFlags", "flags")]),

        Proto("VkResult", "CreateBuffer",
            [Param("VkDevice", "device"),
             Param("const VkBufferCreateInfo*", "pCreateInfo"),
             Param("VkBuffer*", "pBuffer")]),

        Proto("VkResult", "DestroyBuffer",
            [Param("VkDevice", "device"),
             Param("VkBuffer", "buffer")]),

        Proto("VkResult", "CreateBufferView",
            [Param("VkDevice", "device"),
             Param("const VkBufferViewCreateInfo*", "pCreateInfo"),
             Param("VkBufferView*", "pView")]),

        Proto("VkResult", "DestroyBufferView",
            [Param("VkDevice", "device"),
             Param("VkBufferView", "bufferView")]),

        Proto("VkResult", "CreateImage",
            [Param("VkDevice", "device"),
             Param("const VkImageCreateInfo*", "pCreateInfo"),
             Param("VkImage*", "pImage")]),

        Proto("VkResult", "DestroyImage",
            [Param("VkDevice", "device"),
             Param("VkImage", "image")]),

        Proto("VkResult", "GetImageSubresourceLayout",
            [Param("VkDevice", "device"),
             Param("VkImage", "image"),
             Param("const VkImageSubresource*", "pSubresource"),
             Param("VkSubresourceLayout*", "pLayout")]),

        Proto("VkResult", "CreateImageView",
            [Param("VkDevice", "device"),
             Param("const VkImageViewCreateInfo*", "pCreateInfo"),
             Param("VkImageView*", "pView")]),

        Proto("VkResult", "DestroyImageView",
            [Param("VkDevice", "device"),
             Param("VkImageView", "imageView")]),

        Proto("VkResult", "CreateAttachmentView",
            [Param("VkDevice", "device"),
             Param("const VkAttachmentViewCreateInfo*", "pCreateInfo"),
             Param("VkAttachmentView*", "pView")]),

        Proto("VkResult", "DestroyAttachmentView",
            [Param("VkDevice", "device"),
             Param("VkAttachmentView", "attachmentView")]),

        Proto("VkResult", "CreateShaderModule",
            [Param("VkDevice", "device"),
             Param("const VkShaderModuleCreateInfo*", "pCreateInfo"),
             Param("VkShaderModule*", "pShaderModule")]),

        Proto("VkResult", "DestroyShaderModule",
            [Param("VkDevice", "device"),
             Param("VkShaderModule", "shaderModule")]),

        Proto("VkResult", "CreateShader",
            [Param("VkDevice", "device"),
             Param("const VkShaderCreateInfo*", "pCreateInfo"),
             Param("VkShader*", "pShader")]),

        Proto("VkResult", "DestroyShader",
            [Param("VkDevice", "device"),
             Param("VkShader", "shader")]),

        Proto("VkResult", "CreatePipelineCache",
            [Param("VkDevice", "device"),
             Param("const VkPipelineCacheCreateInfo*", "pCreateInfo"),
             Param("VkPipelineCache*", "pPipelineCache")]),

        Proto("VkResult", "DestroyPipelineCache",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "pipelineCache")]),

        Proto("size_t", "GetPipelineCacheSize",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "pipelineCache")]),

        Proto("VkResult", "GetPipelineCacheData",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "pipelineCache"),
             Param("void*", "pData")]),

        Proto("VkResult", "MergePipelineCaches",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "destCache"),
             Param("uint32_t", "srcCacheCount"),
             Param("const VkPipelineCache*", "pSrcCaches")]),

        Proto("VkResult", "CreateGraphicsPipelines",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "pipelineCache"),
             Param("uint32_t", "count"),
             Param("const VkGraphicsPipelineCreateInfo*", "pCreateInfos"),
             Param("VkPipeline*", "pPipelines")]),

        Proto("VkResult", "CreateComputePipelines",
            [Param("VkDevice", "device"),
             Param("VkPipelineCache", "pipelineCache"),
             Param("uint32_t", "count"),
             Param("const VkComputePipelineCreateInfo*", "pCreateInfos"),
             Param("VkPipeline*", "pPipelines")]),

        Proto("VkResult", "DestroyPipeline",
            [Param("VkDevice", "device"),
             Param("VkPipeline", "pipeline")]),

        Proto("VkResult", "CreatePipelineLayout",
            [Param("VkDevice", "device"),
             Param("const VkPipelineLayoutCreateInfo*", "pCreateInfo"),
             Param("VkPipelineLayout*", "pPipelineLayout")]),

        Proto("VkResult", "DestroyPipelineLayout",
            [Param("VkDevice", "device"),
             Param("VkPipelineLayout", "pipelineLayout")]),

        Proto("VkResult", "CreateSampler",
            [Param("VkDevice", "device"),
             Param("const VkSamplerCreateInfo*", "pCreateInfo"),
             Param("VkSampler*", "pSampler")]),

        Proto("VkResult", "DestroySampler",
            [Param("VkDevice", "device"),
             Param("VkSampler", "sampler")]),

        Proto("VkResult", "CreateDescriptorSetLayout",
            [Param("VkDevice", "device"),
             Param("const VkDescriptorSetLayoutCreateInfo*", "pCreateInfo"),
             Param("VkDescriptorSetLayout*", "pSetLayout")]),

        Proto("VkResult", "DestroyDescriptorSetLayout",
            [Param("VkDevice", "device"),
             Param("VkDescriptorSetLayout", "descriptorSetLayout")]),

        Proto("VkResult", "CreateDescriptorPool",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPoolUsage", "poolUsage"),
             Param("uint32_t", "maxSets"),
             Param("const VkDescriptorPoolCreateInfo*", "pCreateInfo"),
             Param("VkDescriptorPool*", "pDescriptorPool")]),

        Proto("VkResult", "DestroyDescriptorPool",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPool", "descriptorPool")]),

        Proto("VkResult", "ResetDescriptorPool",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPool", "descriptorPool")]),

        Proto("VkResult", "AllocDescriptorSets",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPool", "descriptorPool"),
             Param("VkDescriptorSetUsage", "setUsage"),
             Param("uint32_t", "count"),
             Param("const VkDescriptorSetLayout*", "pSetLayouts"),
             Param("VkDescriptorSet*", "pDescriptorSets")]),

        Proto("VkResult", "FreeDescriptorSets",
            [Param("VkDevice", "device"),
             Param("VkDescriptorPool", "descriptorPool"),
             Param("uint32_t", "count"),
             Param("const VkDescriptorSet*", "pDescriptorSets")]),

        Proto("VkResult", "UpdateDescriptorSets",
            [Param("VkDevice", "device"),
             Param("uint32_t", "writeCount"),
             Param("const VkWriteDescriptorSet*", "pDescriptorWrites"),
             Param("uint32_t", "copyCount"),
             Param("const VkCopyDescriptorSet*", "pDescriptorCopies")]),

        Proto("VkResult", "CreateDynamicViewportState",
            [Param("VkDevice", "device"),
             Param("const VkDynamicViewportStateCreateInfo*", "pCreateInfo"),
             Param("VkDynamicViewportState*", "pState")]),

        Proto("VkResult", "DestroyDynamicViewportState",
            [Param("VkDevice", "device"),
             Param("VkDynamicViewportState", "dynamicViewportState")]),

        Proto("VkResult", "CreateDynamicRasterLineState",
            [Param("VkDevice", "device"),
             Param("const VkDynamicRasterLineStateCreateInfo*", "pCreateInfo"),
             Param("VkDynamicRasterLineState*", "pState")]),

        Proto("VkResult", "DestroyDynamicRasterLineState",
            [Param("VkDevice", "device"),
             Param("VkDynamicRasterLineState", "dynamicRasterLineState")]),

        Proto("VkResult", "CreateDynamicRasterDepthBiasState",
            [Param("VkDevice", "device"),
             Param("const VkDynamicRasterDepthBiasStateCreateInfo*", "pCreateInfo"),
             Param("VkDynamicRasterDepthBiasState*", "pState")]),

        Proto("VkResult", "DestroyDynamicRasterDepthBiasState",
            [Param("VkDevice", "device"),
             Param("VkDynamicRasterDepthBiasState", "dynamicRasterDepthBiasState")]),

        Proto("VkResult", "CreateDynamicColorBlendState",
            [Param("VkDevice", "device"),
             Param("const VkDynamicColorBlendStateCreateInfo*", "pCreateInfo"),
             Param("VkDynamicColorBlendState*", "pState")]),

        Proto("VkResult", "DestroyDynamicColorBlendState",
            [Param("VkDevice", "device"),
             Param("VkDynamicColorBlendState", "dynamicColorBlendState")]),

        Proto("VkResult", "CreateDynamicDepthState",
            [Param("VkDevice", "device"),
             Param("const VkDynamicDepthStateCreateInfo*", "pCreateInfo"),
             Param("VkDynamicDepthState*", "pState")]),

        Proto("VkResult", "DestroyDynamicDepthState",
            [Param("VkDevice", "device"),
             Param("VkDynamicDepthState", "dynamicDepthState")]),

        Proto("VkResult", "CreateDynamicStencilState",
            [Param("VkDevice", "device"),
             Param("const VkDynamicStencilStateCreateInfo*", "pCreateInfoFront"),
             Param("const VkDynamicStencilStateCreateInfo*", "pCreateInfoBack"),
             Param("VkDynamicStencilState*", "pState")]),

        Proto("VkResult", "DestroyDynamicStencilState",
            [Param("VkDevice", "device"),
             Param("VkDynamicStencilState", "dynamicStencilState")]),

        Proto("VkResult", "CreateCommandPool",
            [Param("VkDevice", "device"),
             Param("const VkCmdPoolCreateInfo*", "pCreateInfo"),
             Param("VkCmdPool*", "pCmdPool")]),

        Proto("VkResult", "DestroyCommandPool",
            [Param("VkDevice", "device"),
             Param("VkCmdPool", "cmdPool")]),

        Proto("VkResult", "ResetCommandPool",
            [Param("VkDevice", "device"),
             Param("VkCmdPool", "cmdPool"),
             Param("VkCmdPoolResetFlags", "flags")]),

        Proto("VkResult", "CreateCommandBuffer",
            [Param("VkDevice", "device"),
             Param("const VkCmdBufferCreateInfo*", "pCreateInfo"),
             Param("VkCmdBuffer*", "pCmdBuffer")]),

        Proto("VkResult", "DestroyCommandBuffer",
            [Param("VkDevice", "device"),
             Param("VkCmdBuffer", "commandBuffer")]),

        Proto("VkResult", "BeginCommandBuffer",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("const VkCmdBufferBeginInfo*", "pBeginInfo")]),

        Proto("VkResult", "EndCommandBuffer",
            [Param("VkCmdBuffer", "cmdBuffer")]),

        Proto("VkResult", "ResetCommandBuffer",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkCmdBufferResetFlags", "flags")]),

        Proto("void", "CmdBindPipeline",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkPipelineBindPoint", "pipelineBindPoint"),
             Param("VkPipeline", "pipeline")]),

        Proto("void", "CmdBindDynamicViewportState",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkDynamicViewportState", "dynamicViewportState")]),

        Proto("void", "CmdBindDynamicRasterLineState",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkDynamicRasterLineState", "dynamicRasterLineState")]),

        Proto("void", "CmdBindDynamicRasterDepthBiasState",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkDynamicRasterDepthBiasState", "dynamicRasterDepthBiasState")]),

        Proto("void", "CmdBindDynamicColorBlendState",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkDynamicColorBlendState", "dynamicColorBlendState")]),

        Proto("void", "CmdBindDynamicDepthState",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkDynamicDepthState", "dynamicDepthState")]),

        Proto("void", "CmdBindDynamicStencilState",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkDynamicStencilState", "dynamicStencilState")]),

        Proto("void", "CmdBindDescriptorSets",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkPipelineBindPoint", "pipelineBindPoint"),
             Param("VkPipelineLayout", "layout"),
             Param("uint32_t", "firstSet"),
             Param("uint32_t", "setCount"),
             Param("const VkDescriptorSet*", "pDescriptorSets"),
             Param("uint32_t", "dynamicOffsetCount"),
             Param("const uint32_t*", "pDynamicOffsets")]),

        Proto("void", "CmdBindIndexBuffer",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkBuffer", "buffer"),
             Param("VkDeviceSize", "offset"),
             Param("VkIndexType", "indexType")]),

        Proto("void", "CmdBindVertexBuffers",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("uint32_t", "startBinding"),
             Param("uint32_t", "bindingCount"),
             Param("const VkBuffer*", "pBuffers"),
             Param("const VkDeviceSize*", "pOffsets")]),

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
             Param("const VkClearColorValue*", "pColor"),
             Param("uint32_t", "rangeCount"),
             Param("const VkImageSubresourceRange*", "pRanges")]),

        Proto("void", "CmdClearDepthStencilImage",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkImage", "image"),
             Param("VkImageLayout", "imageLayout"),
             Param("float", "depth"),
             Param("uint32_t", "stencil"),
             Param("uint32_t", "rangeCount"),
             Param("const VkImageSubresourceRange*", "pRanges")]),

        Proto("void", "CmdClearColorAttachment",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("uint32_t", "colorAttachment"),
             Param("VkImageLayout", "imageLayout"),
             Param("const VkClearColorValue*", "pColor"),
             Param("uint32_t", "rectCount"),
             Param("const VkRect3D*", "pRects")]),

        Proto("void", "CmdClearDepthStencilAttachment",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkImageAspectFlags", "imageAspectMask"),
             Param("VkImageLayout", "imageLayout"),
             Param("float", "depth"),
             Param("uint32_t", "stencil"),
             Param("uint32_t", "rectCount"),
             Param("const VkRect3D*", "pRects")]),

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
             Param("VkPipelineStageFlags", "stageMask")]),

        Proto("void", "CmdResetEvent",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkEvent", "event"),
             Param("VkPipelineStageFlags", "stageMask")]),

        Proto("void", "CmdWaitEvents",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("uint32_t", "eventCount"),
             Param("const VkEvent*", "pEvents"),
             Param("VkPipelineStageFlags", "srcStageMask"),
             Param("VkPipelineStageFlags", "destStageMask"),
             Param("uint32_t", "memBarrierCount"),
             Param("const void* const*", "ppMemBarriers")]),

        Proto("void", "CmdPipelineBarrier",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkPipelineStageFlags", "srcStageMask"),
             Param("VkPipelineStageFlags", "destStageMask"),
             Param("VkBool32", "byRegion"),
             Param("uint32_t", "memBarrierCount"),
             Param("const void* const*", "ppMemBarriers")]),

        Proto("void", "CmdBeginQuery",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkQueryPool", "queryPool"),
             Param("uint32_t", "slot"),
             Param("VkQueryControlFlags", "flags")]),

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
             Param("VkQueryResultFlags", "flags")]),

        Proto("VkResult", "CreateFramebuffer",
            [Param("VkDevice", "device"),
             Param("const VkFramebufferCreateInfo*", "pCreateInfo"),
             Param("VkFramebuffer*", "pFramebuffer")]),

        Proto("VkResult", "DestroyFramebuffer",
            [Param("VkDevice", "device"),
             Param("VkFramebuffer", "framebuffer")]),

        Proto("VkResult", "CreateRenderPass",
            [Param("VkDevice", "device"),
             Param("const VkRenderPassCreateInfo*", "pCreateInfo"),
             Param("VkRenderPass*", "pRenderPass")]),

        Proto("VkResult", "DestroyRenderPass",
            [Param("VkDevice", "device"),
             Param("VkRenderPass", "renderPass")]),

        Proto("VkResult", "GetRenderAreaGranularity",
            [Param("VkDevice", "device"),
             Param("VkRenderPass", "renderPass"),
             Param("VkExtent2D*", "pGranularity")]),

        Proto("void", "CmdBeginRenderPass",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("const VkRenderPassBeginInfo*", "pRenderPassBegin"),
             Param("VkRenderPassContents", "contents")]),

        Proto("void", "CmdNextSubpass",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkRenderPassContents", "contents")]),

        Proto("void", "CmdPushConstants",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("VkPipelineLayout", "layout"),
             Param("VkShaderStageFlags", "stageFlags"),
             Param("uint32_t", "start"),
             Param("uint32_t", "length"),
             Param("const void*", "values")]),

        Proto("void", "CmdEndRenderPass",
            [Param("VkCmdBuffer", "cmdBuffer")]),

        Proto("void", "CmdExecuteCommands",
            [Param("VkCmdBuffer", "cmdBuffer"),
             Param("uint32_t", "cmdBuffersCount"),
             Param("const VkCmdBuffer*", "pCmdBuffers")]),
    ],
)

wsi_swapchain = Extension(
    name="VK_WSI_swapchain",
    headers=["vk_wsi_swapchain.h"],
    objects=[],
    protos=[
        Proto("VkResult", "GetPhysicalDeviceSurfaceSupportWSI",
            [Param("VkPhysicalDevice", "physicalDevice"),
             Param("uint32_t", "queueFamilyIndex"),
             Param("const VkSurfaceDescriptionWSI*", "pSurfaceDescription"),
             Param("VkBool32*", "pSupported")]),
    ],
)

wsi_device_swapchain = Extension(
    name="VK_WSI_device_swapchain",
    headers=["vk_wsi_device_swapchain.h"],
    objects=["VkSwapChainWSI"],
    protos=[
        Proto("VkResult", "GetSurfacePropertiesWSI",
            [Param("VkDevice", "device"),
	     Param("const VkSurfaceDescriptionWSI*", "pSurfaceDescription"),
             Param("VkSurfacePropertiesWSI*", "pSurfaceProperties")]),

        Proto("VkResult", "GetSurfaceFormatsWSI",
            [Param("VkDevice", "device"),
	     Param("const VkSurfaceDescriptionWSI*", "pSurfaceDescription"),
	     Param("uint32_t*", "pCount"),
             Param("VkSurfaceFormatWSI*", "pSurfaceFormats")]),

        Proto("VkResult", "GetSurfacePresentModesWSI",
            [Param("VkDevice", "device"),
	     Param("const VkSurfaceDescriptionWSI*", "pSurfaceDescription"),
	     Param("uint32_t*", "pCount"),
             Param("VkPresentModeWSI*", "pPresentModes")]),

        Proto("VkResult", "CreateSwapChainWSI",
            [Param("VkDevice", "device"),
             Param("const VkSwapChainCreateInfoWSI*", "pCreateInfo"),
             Param("VkSwapChainWSI*", "pSwapChain")]),

        Proto("VkResult", "DestroySwapChainWSI",
            [Param("VkDevice", "device"),
             Param("VkSwapChainWSI", "swapChain")]),

        Proto("VkResult", "GetSwapChainImagesWSI",
            [Param("VkDevice", "device"),
	     Param("VkSwapChainWSI", "swapChain"),
	     Param("uint32_t*", "pCount"),
             Param("VkImage*", "pSwapChainImages")]),

        Proto("VkResult", "AcquireNextImageWSI",
            [Param("VkDevice", "device"),
             Param("VkSwapChainWSI", "swapChain"),
             Param("uint64_t", "timeout"),
             Param("VkSemaphore", "semaphore"),
             Param("uint32_t*", "pImageIndex")]),

        Proto("VkResult", "QueuePresentWSI",
            [Param("VkQueue", "queue"),
             Param("VkPresentInfoWSI*", "pPresentInfo")]),
    ],
)
debug_report_lunarg = Extension(
    name="VK_DEBUG_REPORT_LunarG",
    headers=["vk_debug_report_lunarg.h"],
    objects=[
        "VkDbgMsgCallback",
    ],
    protos=[
        Proto("VkResult", "DbgCreateMsgCallback",
            [Param("VkInstance", "instance"),
             Param("VkFlags", "msgFlags"),
             Param("const PFN_vkDbgMsgCallback", "pfnMsgCallback"),
             Param("void*", "pUserData"),
             Param("VkDbgMsgCallback*", "pMsgCallback")]),

        Proto("VkResult", "DbgDestroyMsgCallback",
            [Param("VkInstance", "instance"),
             Param("VkDbgMsgCallback", "msgCallback")]),
    ],
)

extensions = [core, wsi_swapchain, wsi_device_swapchain]
extensions_all = [core, wsi_swapchain, wsi_device_swapchain, debug_report_lunarg]
object_dispatch_list = [
    "VkInstance",
    "VkPhysicalDevice",
    "VkDevice",
    "VkQueue",
    "VkCmdBuffer",
]

object_non_dispatch_list = [
    "VkCmdPool",
    "VkFence",
    "VkDeviceMemory",
    "VkBuffer",
    "VkImage",
    "VkSemaphore",
    "VkEvent",
    "VkQueryPool",
    "VkBufferView",
    "VkImageView",
    "VkAttachmentView",
    "VkShaderModule",
    "VkShader",
    "VkPipelineCache",
    "VkPipelineLayout",
    "VkPipeline",
    "VkDescriptorSetLayout",
    "VkSampler",
    "VkDescriptorPool",
    "VkDescriptorSet",
    "VkDynamicViewportState",
    "VkDynamicRasterLineState",
    "VkDynamicRasterDepthBiasState",
    "VkDynamicColorBlendState",
    "VkDynamicDepthState",
    "VkDynamicStencilState",
    "VkRenderPass",
    "VkFramebuffer",
    "VkSwapChainWSI",
    "VkDbgMsgCallback",
]

object_type_list = object_dispatch_list + object_non_dispatch_list

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
            headers=["vulkan.h", "vk_debug_report_lunarg.h"],
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
