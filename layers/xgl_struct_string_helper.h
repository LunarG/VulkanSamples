//This is the copyright
//#includes, #defines, globals and such...
#include <xgl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "xgl_string_helper.h"

char* xgl_print_xgl_raster_state_create_info(const XGL_RASTER_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sfillMode = %s\n%scullMode = %s\n%sfrontFace = %s\n%sdepthBias = %i\n%sdepthBiasClamp = %f\n%sslopeScaledDepthBias = %f\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_FILL_MODE(pStruct->fillMode), prefix, string_XGL_CULL_MODE(pStruct->cullMode), prefix, string_XGL_FACE_ORIENTATION(pStruct->frontFace), prefix, (pStruct->depthBias), prefix, (pStruct->depthBiasClamp), prefix, (pStruct->slopeScaledDepthBias));
    return str;
}
char* xgl_print_xgl_gpu_compatibility_info(const XGL_GPU_COMPATIBILITY_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%scompatibilityFlags = %u\n", prefix, (pStruct->compatibilityFlags));
    return str;
}
char* xgl_print_xgl_image_view_create_info(const XGL_IMAGE_VIEW_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%simage = %p\n%sviewType = %s\n%sformat = %p\n%schannels = %p\n%ssubresourceRange = %p\n%sminLod = %f\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->image), prefix, string_XGL_IMAGE_VIEW_TYPE(pStruct->viewType), prefix, (void*)&(pStruct->format), prefix, (void*)&(pStruct->channels), prefix, (void*)&(pStruct->subresourceRange), prefix, (pStruct->minLod));
    return str;
}
char* xgl_print_xgl_memory_open_info(const XGL_MEMORY_OPEN_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%ssharedMem = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->sharedMem));
    return str;
}
char* xgl_print_xgl_memory_heap_properties(const XGL_MEMORY_HEAP_PROPERTIES* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sstructSize = %u\n%sheapMemoryType = %s\n%sheapSize = %u\n%spageSize = %u\n%sflags = %u\n%sgpuReadPerfRating = %f\n%sgpuWritePerfRating = %f\n%scpuReadPerfRating = %f\n%scpuWritePerfRating = %f\n", prefix, (pStruct->structSize), prefix, string_XGL_HEAP_MEMORY_TYPE(pStruct->heapMemoryType), prefix, (pStruct->heapSize), prefix, (pStruct->pageSize), prefix, (pStruct->flags), prefix, (pStruct->gpuReadPerfRating), prefix, (pStruct->gpuWritePerfRating), prefix, (pStruct->cpuReadPerfRating), prefix, (pStruct->cpuWritePerfRating));
    return str;
}
char* xgl_print_xgl_image_subresource(const XGL_IMAGE_SUBRESOURCE* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%saspect = %s\n%smipLevel = %u\n%sarraySlice = %u\n", prefix, string_XGL_IMAGE_ASPECT(pStruct->aspect), prefix, (pStruct->mipLevel), prefix, (pStruct->arraySlice));
    return str;
}
char* xgl_print_xgl_physical_gpu_performance(const XGL_PHYSICAL_GPU_PERFORMANCE* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%smaxGpuClock = %f\n%saluPerClock = %f\n%stexPerClock = %f\n%sprimsPerClock = %f\n%spixelsPerClock = %f\n", prefix, (pStruct->maxGpuClock), prefix, (pStruct->aluPerClock), prefix, (pStruct->texPerClock), prefix, (pStruct->primsPerClock), prefix, (pStruct->pixelsPerClock));
    return str;
}
char* xgl_print_xgl_physical_gpu_memory_properties(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sstructSize = %u\n%ssupportsMigration = %s\n%ssupportsVirtualMemoryRemapping = %s\n%ssupportsPinning = %s\n", prefix, (pStruct->structSize), prefix, (pStruct->supportsMigration) ? "TRUE" : "FALSE", prefix, (pStruct->supportsVirtualMemoryRemapping) ? "TRUE" : "FALSE", prefix, (pStruct->supportsPinning) ? "TRUE" : "FALSE");
    return str;
}
char* xgl_print_xgl_pipeline_shader(const XGL_PIPELINE_SHADER* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sstage = %s\n%sshader = %p\n%sdescriptorSetMapping = %p\n%slinkConstBufferCount = %u\n%spLinkConstBufferInfo = %p\n%sdynamicMemoryViewMapping = %p\n", prefix, string_XGL_PIPELINE_SHADER_STAGE(pStruct->stage), prefix, (void*)(pStruct->shader), prefix, (void*)(pStruct->descriptorSetMapping), prefix, (pStruct->linkConstBufferCount), prefix, (void*)(pStruct->pLinkConstBufferInfo), prefix, (void*)&(pStruct->dynamicMemoryViewMapping));
    return str;
}
char* xgl_print_xgl_fence_create_info(const XGL_FENCE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_pipeline_cb_attachment_state(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sblendEnable = %s\n%sformat = %p\n%schannelWriteMask = %hu\n", prefix, (pStruct->blendEnable) ? "TRUE" : "FALSE", prefix, (void*)&(pStruct->format), prefix, (pStruct->channelWriteMask));
    return str;
}
char* xgl_print_xgl_alloc_callbacks(const XGL_ALLOC_CALLBACKS* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%spUserData = %p\n%spfnAlloc = %p\n%spfnFree = %p\n", prefix, (pStruct->pUserData), prefix, (void*)(pStruct->pfnAlloc), prefix, (void*)(pStruct->pfnFree));
    return str;
}
char* xgl_print_xgl_color_attachment_view_create_info(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%simage = %p\n%sformat = %p\n%smipLevel = %u\n%sbaseArraySlice = %u\n%sarraySize = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->image), prefix, (void*)&(pStruct->format), prefix, (pStruct->mipLevel), prefix, (pStruct->baseArraySlice), prefix, (pStruct->arraySize));
    return str;
}
char* xgl_print_xgl_image_copy(const XGL_IMAGE_COPY* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssrcSubresource = %p\n%ssrcOffset = %p\n%sdestSubresource = %p\n%sdestOffset = %p\n%sextent = %p\n", prefix, (void*)&(pStruct->srcSubresource), prefix, (void*)&(pStruct->srcOffset), prefix, (void*)&(pStruct->destSubresource), prefix, (void*)&(pStruct->destOffset), prefix, (void*)&(pStruct->extent));
    return str;
}
char* xgl_print_xgl_msaa_state_create_info(const XGL_MSAA_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%ssamples = %u\n%ssampleMask = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->samples), prefix, (pStruct->sampleMask));
    return str;
}
char* xgl_print_xgl_descriptor_set_create_info(const XGL_DESCRIPTOR_SET_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sslots = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->slots));
    return str;
}
char* xgl_print_xgl_color_attachment_bind_info(const XGL_COLOR_ATTACHMENT_BIND_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sview = %p\n%scolorAttachmentState = %s\n", prefix, (void*)(pStruct->view), prefix, string_XGL_IMAGE_STATE(pStruct->colorAttachmentState));
    return str;
}
char* xgl_print_xgl_event_create_info(const XGL_EVENT_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_memory_requirements(const XGL_MEMORY_REQUIREMENTS* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssize = %u\n%salignment = %u\n%sheapCount = %u\n%sheaps = %p\n", prefix, (pStruct->size), prefix, (pStruct->alignment), prefix, (pStruct->heapCount), prefix, (void*)(pStruct->heaps));
    return str;
}
char* xgl_print_xgl_queue_semaphore_open_info(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%ssharedSemaphore = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->sharedSemaphore));
    return str;
}
char* xgl_print_xgl_image_resolve(const XGL_IMAGE_RESOLVE* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssrcSubresource = %p\n%ssrcOffset = %p\n%sdestSubresource = %p\n%sdestOffset = %p\n%sextent = %p\n", prefix, (void*)&(pStruct->srcSubresource), prefix, (void*)&(pStruct->srcOffset), prefix, (void*)&(pStruct->destSubresource), prefix, (void*)&(pStruct->destOffset), prefix, (void*)&(pStruct->extent));
    return str;
}
char* xgl_print_xgl_draw_indexed_indirect_cmd(const XGL_DRAW_INDEXED_INDIRECT_CMD* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sindexCount = %u\n%sinstanceCount = %u\n%sfirstIndex = %u\n%svertexOffset = %i\n%sfirstInstance = %u\n", prefix, (pStruct->indexCount), prefix, (pStruct->instanceCount), prefix, (pStruct->firstIndex), prefix, (pStruct->vertexOffset), prefix, (pStruct->firstInstance));
    return str;
}
char* xgl_print_xgl_compute_pipeline_create_info(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%scs = %p\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)&(pStruct->cs), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_peer_image_open_info(const XGL_PEER_IMAGE_OPEN_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%soriginalImage = %p\n", prefix, (void*)(pStruct->originalImage));
    return str;
}
char* xgl_print_xgl_physical_gpu_queue_properties(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sstructSize = %u\n%squeueFlags = %u\n%squeueCount = %u\n%smaxAtomicCounters = %u\n%ssupportsTimestamps = %s\n", prefix, (pStruct->structSize), prefix, (pStruct->queueFlags), prefix, (pStruct->queueCount), prefix, (pStruct->maxAtomicCounters), prefix, (pStruct->supportsTimestamps) ? "TRUE" : "FALSE");
    return str;
}
char* xgl_print_xgl_pipeline_statistics_data(const XGL_PIPELINE_STATISTICS_DATA* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sfsInvocations = %lu\n%scPrimitives = %lu\n%scInvocations = %lu\n%svsInvocations = %lu\n%sgsInvocations = %lu\n%sgsPrimitives = %lu\n%siaPrimitives = %lu\n%siaVertices = %lu\n%stcsInvocations = %lu\n%stesInvocations = %lu\n%scsInvocations = %lu\n", prefix, (pStruct->fsInvocations), prefix, (pStruct->cPrimitives), prefix, (pStruct->cInvocations), prefix, (pStruct->vsInvocations), prefix, (pStruct->gsInvocations), prefix, (pStruct->gsPrimitives), prefix, (pStruct->iaPrimitives), prefix, (pStruct->iaVertices), prefix, (pStruct->tcsInvocations), prefix, (pStruct->tesInvocations), prefix, (pStruct->csInvocations));
    return str;
}
char* xgl_print_xgl_device_queue_create_info(const XGL_DEVICE_QUEUE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%squeueNodeIndex = %u\n%squeueCount = %u\n", prefix, (pStruct->queueNodeIndex), prefix, (pStruct->queueCount));
    return str;
}
char* xgl_print_xgl_sampler_create_info(const XGL_SAMPLER_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%smagFilter = %s\n%sminFilter = %s\n%smipMode = %s\n%saddressU = %s\n%saddressV = %s\n%saddressW = %s\n%smipLodBias = %f\n%smaxAnisotropy = %u\n%scompareFunc = %s\n%sminLod = %f\n%smaxLod = %f\n%sborderColorType = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_TEX_FILTER(pStruct->magFilter), prefix, string_XGL_TEX_FILTER(pStruct->minFilter), prefix, string_XGL_TEX_MIPMAP_MODE(pStruct->mipMode), prefix, string_XGL_TEX_ADDRESS(pStruct->addressU), prefix, string_XGL_TEX_ADDRESS(pStruct->addressV), prefix, string_XGL_TEX_ADDRESS(pStruct->addressW), prefix, (pStruct->mipLodBias), prefix, (pStruct->maxAnisotropy), prefix, string_XGL_COMPARE_FUNC(pStruct->compareFunc), prefix, (pStruct->minLod), prefix, (pStruct->maxLod), prefix, string_XGL_BORDER_COLOR_TYPE(pStruct->borderColorType));
    return str;
}
char* xgl_print_xgl_queue_semaphore_create_info(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sinitialCount = %u\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->initialCount), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_format(const XGL_FORMAT* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%schannelFormat = %s\n%snumericFormat = %s\n", prefix, string_XGL_CHANNEL_FORMAT(pStruct->channelFormat), prefix, string_XGL_NUM_FORMAT(pStruct->numericFormat));
    return str;
}
char* xgl_print_xgl_memory_state_transition(const XGL_MEMORY_STATE_TRANSITION* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%smem = %p\n%soldState = %s\n%snewState = %s\n%soffset = %u\n%sregionSize = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->mem), prefix, string_XGL_MEMORY_STATE(pStruct->oldState), prefix, string_XGL_MEMORY_STATE(pStruct->newState), prefix, (pStruct->offset), prefix, (pStruct->regionSize));
    return str;
}
char* xgl_print_xgl_extent3d(const XGL_EXTENT3D* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%swidth = %i\n%sheight = %i\n%sdepth = %i\n", prefix, (pStruct->width), prefix, (pStruct->height), prefix, (pStruct->depth));
    return str;
}
char* xgl_print_xgl_dynamic_memory_view_slot_info(const XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sslotObjectType = %s\n%sshaderEntityIndex = %u\n", prefix, string_XGL_DESCRIPTOR_SET_SLOT_TYPE(pStruct->slotObjectType), prefix, (pStruct->shaderEntityIndex));
    return str;
}
char* xgl_print_xgl_image_view_attach_info(const XGL_IMAGE_VIEW_ATTACH_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sview = %p\n%sstate = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->view), prefix, string_XGL_IMAGE_STATE(pStruct->state));
    return str;
}
char* xgl_print_xgl_image_subresource_range(const XGL_IMAGE_SUBRESOURCE_RANGE* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%saspect = %s\n%sbaseMipLevel = %u\n%smipLevels = %u\n%sbaseArraySlice = %u\n%sarraySize = %u\n", prefix, string_XGL_IMAGE_ASPECT(pStruct->aspect), prefix, (pStruct->baseMipLevel), prefix, (pStruct->mipLevels), prefix, (pStruct->baseArraySlice), prefix, (pStruct->arraySize));
    return str;
}
char* xgl_print_xgl_pipeline_db_state_create_info(const XGL_PIPELINE_DB_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sformat = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)&(pStruct->format));
    return str;
}
char* xgl_print_xgl_application_info(const XGL_APPLICATION_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%spAppName = %p\n%sappVersion = %u\n%spEngineName = %p\n%sengineVersion = %u\n%sapiVersion = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->pAppName), prefix, (pStruct->appVersion), prefix, (pStruct->pEngineName), prefix, (pStruct->engineVersion), prefix, (pStruct->apiVersion));
    return str;
}
char* xgl_print_xgl_offset2d(const XGL_OFFSET2D* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sx = %i\n%sy = %i\n", prefix, (pStruct->x), prefix, (pStruct->y));
    return str;
}
char* xgl_print_xgl_viewport_state_create_info(const XGL_VIEWPORT_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sviewportCount = %u\n%sscissorEnable = %s\n%sviewports = %p\n%sscissors = %p\n", prefix, (pStruct->viewportCount), prefix, (pStruct->scissorEnable) ? "TRUE" : "FALSE", prefix, (void*)(pStruct->viewports), prefix, (void*)(pStruct->scissors));
    return str;
}
char* xgl_print_xgl_image_state_transition(const XGL_IMAGE_STATE_TRANSITION* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%simage = %p\n%soldState = %s\n%snewState = %s\n%ssubresourceRange = %p\n", prefix, (void*)(pStruct->image), prefix, string_XGL_IMAGE_STATE(pStruct->oldState), prefix, string_XGL_IMAGE_STATE(pStruct->newState), prefix, (void*)&(pStruct->subresourceRange));
    return str;
}
char* xgl_print_xgl_device_create_info(const XGL_DEVICE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%squeueRecordCount = %u\n%spRequestedQueues = %p\n%sextensionCount = %u\n%sppEnabledExtensionNames = %p\n%smaxValidationLevel = %s\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->queueRecordCount), prefix, (void*)(pStruct->pRequestedQueues), prefix, (pStruct->extensionCount), prefix, (pStruct->ppEnabledExtensionNames), prefix, string_XGL_VALIDATION_LEVEL(pStruct->maxValidationLevel), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_image_create_info(const XGL_IMAGE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%simageType = %s\n%sformat = %p\n%sextent = %p\n%smipLevels = %u\n%sarraySize = %u\n%ssamples = %u\n%stiling = %s\n%susage = %u\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_IMAGE_TYPE(pStruct->imageType), prefix, (void*)&(pStruct->format), prefix, (void*)&(pStruct->extent), prefix, (pStruct->mipLevels), prefix, (pStruct->arraySize), prefix, (pStruct->samples), prefix, string_XGL_IMAGE_TILING(pStruct->tiling), prefix, (pStruct->usage), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_rect(const XGL_RECT* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%soffset = %p\n%sextent = %p\n", prefix, (void*)&(pStruct->offset), prefix, (void*)&(pStruct->extent));
    return str;
}
char* xgl_print_xgl_memory_copy(const XGL_MEMORY_COPY* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssrcOffset = %u\n%sdestOffset = %u\n%scopySize = %u\n", prefix, (pStruct->srcOffset), prefix, (pStruct->destOffset), prefix, (pStruct->copySize));
    return str;
}
char* xgl_print_xgl_descriptor_slot_info(const XGL_DESCRIPTOR_SLOT_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sslotObjectType = %s\n%sshaderEntityIndex = %u\n%spNextLevelSet = %p\n", prefix, string_XGL_DESCRIPTOR_SET_SLOT_TYPE(pStruct->slotObjectType), prefix, (pStruct->shaderEntityIndex), prefix, (pStruct->pNextLevelSet));
    return str;
}
char* xgl_print_xgl_link_const_buffer(const XGL_LINK_CONST_BUFFER* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sbufferId = %u\n%sbufferSize = %u\n%spBufferData = %p\n", prefix, (pStruct->bufferId), prefix, (pStruct->bufferSize), prefix, (pStruct->pBufferData));
    return str;
}
char* xgl_print_xgl_memory_image_copy(const XGL_MEMORY_IMAGE_COPY* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%smemOffset = %u\n%simageSubresource = %p\n%simageOffset = %p\n%simageExtent = %p\n", prefix, (pStruct->memOffset), prefix, (void*)&(pStruct->imageSubresource), prefix, (void*)&(pStruct->imageOffset), prefix, (void*)&(pStruct->imageExtent));
    return str;
}
char* xgl_print_xgl_depth_stencil_state_create_info(const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sdepthTestEnable = %s\n%sdepthWriteEnable = %s\n%sdepthFunc = %s\n%sdepthBoundsEnable = %s\n%sminDepth = %f\n%smaxDepth = %f\n%sstencilTestEnable = %s\n%sstencilReadMask = %u\n%sstencilWriteMask = %u\n%sfront = %p\n%sback = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->depthTestEnable) ? "TRUE" : "FALSE", prefix, (pStruct->depthWriteEnable) ? "TRUE" : "FALSE", prefix, string_XGL_COMPARE_FUNC(pStruct->depthFunc), prefix, (pStruct->depthBoundsEnable) ? "TRUE" : "FALSE", prefix, (pStruct->minDepth), prefix, (pStruct->maxDepth), prefix, (pStruct->stencilTestEnable) ? "TRUE" : "FALSE", prefix, (pStruct->stencilReadMask), prefix, (pStruct->stencilWriteMask), prefix, (void*)&(pStruct->front), prefix, (void*)&(pStruct->back));
    return str;
}
char* xgl_print_xgl_viewport(const XGL_VIEWPORT* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%soriginX = %f\n%soriginY = %f\n%swidth = %f\n%sheight = %f\n%sminDepth = %f\n%smaxDepth = %f\n", prefix, (pStruct->originX), prefix, (pStruct->originY), prefix, (pStruct->width), prefix, (pStruct->height), prefix, (pStruct->minDepth), prefix, (pStruct->maxDepth));
    return str;
}
char* xgl_print_xgl_descriptor_set_mapping(const XGL_DESCRIPTOR_SET_MAPPING* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sdescriptorCount = %u\n%spDescriptorInfo = %p\n", prefix, (pStruct->descriptorCount), prefix, (void*)(pStruct->pDescriptorInfo));
    return str;
}
char* xgl_print_xgl_peer_memory_open_info(const XGL_PEER_MEMORY_OPEN_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%soriginalMem = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->originalMem));
    return str;
}
char* xgl_print_xgl_subresource_layout(const XGL_SUBRESOURCE_LAYOUT* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%soffset = %u\n%ssize = %u\n%srowPitch = %u\n%sdepthPitch = %u\n", prefix, (pStruct->offset), prefix, (pStruct->size), prefix, (pStruct->rowPitch), prefix, (pStruct->depthPitch));
    return str;
}
char* xgl_print_xgl_descriptor_set_attach_info(const XGL_DESCRIPTOR_SET_ATTACH_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sdescriptorSet = %p\n%sslotOffset = %u\n", prefix, (void*)(pStruct->descriptorSet), prefix, (pStruct->slotOffset));
    return str;
}
char* xgl_print_xgl_pipeline_tess_state_create_info(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%spatchControlPoints = %u\n%soptimalTessFactor = %f\n%sfixedTessFactor = %f\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->patchControlPoints), prefix, (pStruct->optimalTessFactor), prefix, (pStruct->fixedTessFactor));
    return str;
}
char* xgl_print_xgl_pipeline_rs_state_create_info(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sdepthClipEnable = %s\n%srasterizerDiscardEnable = %s\n%spointSize = %f\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->depthClipEnable) ? "TRUE" : "FALSE", prefix, (pStruct->rasterizerDiscardEnable) ? "TRUE" : "FALSE", prefix, (pStruct->pointSize));
    return str;
}
char* xgl_print_xgl_stencil_op_state(const XGL_STENCIL_OP_STATE* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sstencilFailOp = %s\n%sstencilPassOp = %s\n%sstencilDepthFailOp = %s\n%sstencilFunc = %s\n%sstencilRef = %u\n", prefix, string_XGL_STENCIL_OP(pStruct->stencilFailOp), prefix, string_XGL_STENCIL_OP(pStruct->stencilPassOp), prefix, string_XGL_STENCIL_OP(pStruct->stencilDepthFailOp), prefix, string_XGL_COMPARE_FUNC(pStruct->stencilFunc), prefix, (pStruct->stencilRef));
    return str;
}
char* xgl_print_xgl_shader_create_info(const XGL_SHADER_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%scodeSize = %u\n%spCode = %p\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->codeSize), prefix, (pStruct->pCode), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_color_blend_state_create_info(const XGL_COLOR_BLEND_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sattachment = %p\n%sblendConst = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->attachment), prefix, (void*)(pStruct->blendConst));
    return str;
}
char* xgl_print_xgl_pipeline_cb_state_create_info(const XGL_PIPELINE_CB_STATE* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%salphaToCoverageEnable = %s\n%sdualSourceBlendEnable = %s\n%slogicOp = %s\n%sattachment = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->alphaToCoverageEnable) ? "TRUE" : "FALSE", prefix, (pStruct->dualSourceBlendEnable) ? "TRUE" : "FALSE", prefix, string_XGL_LOGIC_OP(pStruct->logicOp), prefix, (void*)(pStruct->attachment));
    return str;
}
char* xgl_print_xgl_channel_mapping(const XGL_CHANNEL_MAPPING* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sr = %s\n%sg = %s\n%sb = %s\n%sa = %s\n", prefix, string_XGL_CHANNEL_SWIZZLE(pStruct->r), prefix, string_XGL_CHANNEL_SWIZZLE(pStruct->g), prefix, string_XGL_CHANNEL_SWIZZLE(pStruct->b), prefix, string_XGL_CHANNEL_SWIZZLE(pStruct->a));
    return str;
}
char* xgl_print_xgl_depth_stencil_view_create_info(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%simage = %p\n%smipLevel = %u\n%sbaseArraySlice = %u\n%sarraySize = %u\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->image), prefix, (pStruct->mipLevel), prefix, (pStruct->baseArraySlice), prefix, (pStruct->arraySize), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_virtual_memory_remap_range(const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%svirtualMem = %p\n%svirtualStartPage = %u\n%srealMem = %p\n%srealStartPage = %u\n%spageCount = %u\n", prefix, (void*)(pStruct->virtualMem), prefix, (pStruct->virtualStartPage), prefix, (void*)(pStruct->realMem), prefix, (pStruct->realStartPage), prefix, (pStruct->pageCount));
    return str;
}
char* xgl_print_xgl_cmd_buffer_create_info(const XGL_CMD_BUFFER_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%squeueType = %s\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_QUEUE_TYPE(pStruct->queueType), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_format_properties(const XGL_FORMAT_PROPERTIES* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%slinearTilingFeatures = %u\n%soptimalTilingFeatures = %u\n", prefix, (pStruct->linearTilingFeatures), prefix, (pStruct->optimalTilingFeatures));
    return str;
}
char* xgl_print_xgl_physical_gpu_properties(const XGL_PHYSICAL_GPU_PROPERTIES* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sstructSize = %u\n%sapiVersion = %u\n%sdriverVersion = %u\n%svendorId = %u\n%sdeviceId = %u\n%sgpuType = %s\n%sgpuName = %s\n%smaxMemRefsPerSubmission = %u\n%svirtualMemPageSize = %u\n%smaxInlineMemoryUpdateSize = %u\n%smaxBoundDescriptorSets = %u\n%smaxThreadGroupSize = %u\n%stimestampFrequency = %lu\n%smultiColorAttachmentClears = %s\n", prefix, (pStruct->structSize), prefix, (pStruct->apiVersion), prefix, (pStruct->driverVersion), prefix, (pStruct->vendorId), prefix, (pStruct->deviceId), prefix, string_XGL_PHYSICAL_GPU_TYPE(pStruct->gpuType), prefix, (pStruct->gpuName), prefix, (pStruct->maxMemRefsPerSubmission), prefix, (pStruct->virtualMemPageSize), prefix, (pStruct->maxInlineMemoryUpdateSize), prefix, (pStruct->maxBoundDescriptorSets), prefix, (pStruct->maxThreadGroupSize), prefix, (pStruct->timestampFrequency), prefix, (pStruct->multiColorAttachmentClears) ? "TRUE" : "FALSE");
    return str;
}
char* xgl_print_xgl_depth_stencil_bind_info(const XGL_DEPTH_STENCIL_BIND_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sview = %p\n%sdepthState = %s\n%sstencilState = %s\n", prefix, (void*)(pStruct->view), prefix, string_XGL_IMAGE_STATE(pStruct->depthState), prefix, string_XGL_IMAGE_STATE(pStruct->stencilState));
    return str;
}
char* xgl_print_xgl_draw_indirect_cmd(const XGL_DRAW_INDIRECT_CMD* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%svertexCount = %u\n%sinstanceCount = %u\n%sfirstVertex = %u\n%sfirstInstance = %u\n", prefix, (pStruct->vertexCount), prefix, (pStruct->instanceCount), prefix, (pStruct->firstVertex), prefix, (pStruct->firstInstance));
    return str;
}
char* xgl_print_xgl_graphics_pipeline_create_info(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_pipeline_ia_state_create_info(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%stopology = %s\n%sdisableVertexReuse = %s\n%sprovokingVertex = %s\n%sprimitiveRestartEnable = %s\n%sprimitiveRestartIndex = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_PRIMITIVE_TOPOLOGY(pStruct->topology), prefix, (pStruct->disableVertexReuse) ? "TRUE" : "FALSE", prefix, string_XGL_PROVOKING_VERTEX_CONVENTION(pStruct->provokingVertex), prefix, (pStruct->primitiveRestartEnable) ? "TRUE" : "FALSE", prefix, (pStruct->primitiveRestartIndex));
    return str;
}
char* xgl_print_xgl_color_attachment_blend_state(const XGL_COLOR_ATTACHMENT_BLEND_STATE* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sblendEnable = %s\n%ssrcBlendColor = %s\n%sdestBlendColor = %s\n%sblendFuncColor = %s\n%ssrcBlendAlpha = %s\n%sdestBlendAlpha = %s\n%sblendFuncAlpha = %s\n", prefix, (pStruct->blendEnable) ? "TRUE" : "FALSE", prefix, string_XGL_BLEND(pStruct->srcBlendColor), prefix, string_XGL_BLEND(pStruct->destBlendColor), prefix, string_XGL_BLEND_FUNC(pStruct->blendFuncColor), prefix, string_XGL_BLEND(pStruct->srcBlendAlpha), prefix, string_XGL_BLEND(pStruct->destBlendAlpha), prefix, string_XGL_BLEND_FUNC(pStruct->blendFuncAlpha));
    return str;
}
char* xgl_print_xgl_extent2d(const XGL_EXTENT2D* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%swidth = %i\n%sheight = %i\n", prefix, (pStruct->width), prefix, (pStruct->height));
    return str;
}
char* xgl_print_xgl_memory_alloc_info(const XGL_MEMORY_ALLOC_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sallocationSize = %u\n%salignment = %u\n%sflags = %u\n%sheapCount = %u\n%sheaps = %p\n%smemPriority = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->allocationSize), prefix, (pStruct->alignment), prefix, (pStruct->flags), prefix, (pStruct->heapCount), prefix, (void*)(pStruct->heaps), prefix, string_XGL_MEMORY_PRIORITY(pStruct->memPriority));
    return str;
}
char* xgl_print_xgl_memory_ref(const XGL_MEMORY_REF* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%smem = %p\n%sflags = %u\n", prefix, (void*)(pStruct->mem), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_query_pool_create_info(const XGL_QUERY_POOL_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%squeryType = %s\n%sslots = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_QUERY_TYPE(pStruct->queryType), prefix, (pStruct->slots));
    return str;
}
char* xgl_print_xgl_offset3d(const XGL_OFFSET3D* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sx = %i\n%sy = %i\n%sz = %i\n", prefix, (pStruct->x), prefix, (pStruct->y), prefix, (pStruct->z));
    return str;
}
char* xgl_print_xgl_pipeline_shader_stage_create_info(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%sshader = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)&(pStruct->shader));
    return str;
}
char* xgl_print_xgl_memory_view_attach_info(const XGL_MEMORY_VIEW_ATTACH_INFO* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%ssType = %s\n%spNext = %p\n%smem = %p\n%soffset = %u\n%srange = %u\n%sstride = %u\n%sformat = %p\n%sstate = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->mem), prefix, (pStruct->offset), prefix, (pStruct->range), prefix, (pStruct->stride), prefix, (void*)&(pStruct->format), prefix, string_XGL_MEMORY_STATE(pStruct->state));
    return str;
}
char* xgl_print_xgl_dispatch_indirect_cmd(const XGL_DISPATCH_INDIRECT_CMD* pStruct, const char* prefix)
{
    char* str;
    str = (char*)malloc(sizeof(char)*1024);
    sprintf(str, "%sx = %u\n%sy = %u\n%sz = %u\n", prefix, (pStruct->x), prefix, (pStruct->y), prefix, (pStruct->z));
    return str;
}
