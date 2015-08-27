[TOC]

# Validation Layer Details

## DrawState

### DrawState Overview

The DrawState layer tracks state leading into Draw cmds. This includes the Pipeline state, dynamic state, and descriptor set state. DrawState validates the consistency and correctness between and within these states.

### DrawState Details Table

| Check | Overview | ENUM DRAWSTATE_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Valid Pipeline Layouts | Validates that pipeline layout from bound descriptor set matches the current pipeline layout | PIPELINE_LAYOUT_MISMATCH | vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | TBD | None |
| Validate DbgMarker exensions | Validates that DbgMarker extensions have been enabled before use | DRAWSTATE_INVALID_EXTENSION | vkCmdDbgMarkerBegin vkCmdDbgMarkerEnd | TBD | None |
| Valid BeginCommandBuffer level-related parameters | Primary command buffers must specify VK_NULL_HANDLE for RenderPass or Framebuffer parameters, while secondary command buffers must provide non-null parameters | BEGIN_CB_INVALID_STATE | vkBeginCommandBuffer | PrimaryCmdBufferFramebufferAndRenderpass SecondaryCmdBufferFramebufferAndRenderpass | None |
| PSO Bound | Verify that a properly created and valid pipeline object is bound to the CmdBuffer specified in these calls | NO_PIPELINE_BOUND | vkCmdBindDescriptorSets vkCmdBindVertexBuffers | PipelineNotBound | This check is currently more related to DrawState data structures and less about verifying that PSO is bound at all appropriate points in API. For API purposes, need to make sure this is checked at Draw time and any other relevant calls. |
| Valid DescriptorPool | Verifies that the descriptor set pool object was properly created and is valid | INVALID_POOL | vkResetDescriptorPool, vkAllocDescriptorSets | None | This is just an internal layer data structure check. ParamChecker or ObjectTracker should really catch bad DSPool |
| Valid DescriptorSet | Validate that descriptor set was properly created and is currently valid | INVALID_SET | vkCmdBindDescriptorSets | None | Is this needed other places (like Update/Clear descriptors) |
| Valid DescriptorSetLayout | Flag DescriptorSetLayout object that was not properly created | INVALID_LAYOUT | vkAllocDescriptorSets | None | Anywhere else to check this? |
| Valid Pipeline | Flag VkPipeline object that was not properly created | INVALID_PIPELINE | vkCmdBindPipeline | InvalidPipeline | NA |
| Valid Pipeline Create Info | Tests for the following: That compute shaders are not specified for the graphics pipeline, tess evaluation and tess control shaders are included or excluded as a pair, that VK_PRIMITIVE_TOPOLOGY_PATCH is set as IA topology for tessellation pipelines, that VK_PRIMITIVE_TOPOLOGY_PATCH primitive topology is only set for tessellation pipelines, and that Vtx Shader specified | INVALID_PIPELINE_CREATE_STATE | vkCreateGraphicsPipeline | InvalidPipelineCreateState | NA |
| Valid CmdBuffer | Validates that the command buffer object was properly created and is currently valid | INVALID_CMD_BUFFER | vkQueueSubmit vkBeginCommandBuffer vkEndCommandBuffer vkCmdBindPipeline vkCmdBindDynamicViewportState vkCmdBindDynamicLineWidthState vkCmdBindDynamicDepthBiasState vkCmdBindDynamicBlendState vkCmdBindDynamicDepthBoundsState vkCmdBindDynamicStencilState vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorAttachment vkCmdClearDepthStencilAttachment vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdWaitEvents vkCmdPipelineBarrier vkCmdBeginQuery vkCmdEndQuery vkCmdResetQueryPool vkCmdWriteTimestamp vkCmdBeginRenderPass vkCmdNextSubpass vkCmdEndRenderPass vkCmdExecuteCommands vkCmdDbgMarkerBegin vkCmdDbgMarkerEnd vkCreateCommandBuffer | None | NA |
| Valid Dynamic State | Validates that each of the 4 DSOs are valid, properly constructed objects of the correct type | INVALID_DYNAMIC_STATE_OBJECT | vkCmdBindDynamicViewportState vkCmdBindDynamicLineWidthState vkCmdBindDynamicDepthBiasState vkCmdBindDynamicBlendState vkCmdBindDynamicDepthBoundsState vkCmdBindDynamicStencilState | None | NA |
| Vtx Buffer Bounds | Check if VBO index too large for PSO Vtx binding count, and that at least one vertex buffer is attached to pipeline object | VTX_INDEX_OUT_OF_BOUNDS | vkCmdBindDescriptorSets vkCmdBindVertexBuffers | VtxBufferBadIndex | NA |
| Cmd Buffer End | Verifies that EndCommandBuffer was called for this cmdBuffer at QueueSubmit time | NO_END_CMD_BUFFER | vkQueueSubmit | NoEndCmdBuffer | NA |
| Cmd Buffer Begin | Check that BeginCommandBuffer was called for this command buffer when binding commands or calling end | NO_BEGIN_CMD_BUFFER | vkEndCommandBuffer vkCmdBindPipeline vkCmdBindDynamicViewportState vkCmdBindDynamicLineWidthState vkCmdBindDynamicDepthBiasState vkCmdBindDynamicBlendState vkCmdBindDynamicDepthBoundsState vkCmdBindDynamicStencilState vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorAttachment vkCmdClearDepthStencilAttachment vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdWaitEvents vkCmdPipelineBarrier vkCmdBeginQuery vkCmdEndQuery vkCmdResetQueryPool vkCmdWriteTimestamp | NoBeginCmdBuffer | NA |
| Cmd Buffer Submit Count | Verify that ONE_TIME submit cmdbuffer is not submitted multiple times | DRAWSTATE_CMD_BUFFER_SINGLE_SUBMIT_VIOLATION | vkBeginCommandBuffer, vkQueueSubmit | CmdBufferTwoSubmits | NA |
| DS Pool End Update | Flag if DS Pool end update called w/o a begin | DS_END_WITHOUT_BEGIN | vkEndDescriptorPoolUpdate | DSEndWithoutBegin | NA |
| DS Pool Update | Flag if DS Update called w/o a begin | UPDATE_WITHOUT_BEGIN | vkEndDescriptorPoolUpdate | DSUpdateWithoutBegin | NA |
| DS Pool State at Submit | Flag if QueueSubmit includes a DS Pool under active update (begin called w/o end) | BINDING_DS_NO_END_UPDATE | vkEndDescriptorPoolUpdate, vkQueueSubmit | DSBoundWithoutEnd | NA |
| Descriptor Type | Verify Descriptor type in bound descriptor set layout matches descriptor type specified in update | DESCRIPTOR_TYPE_MISMATCH | vkUpdateDescriptors | DSTypeMismatch | With various DS API updates, need to revisit this code |
| DS Update Size | DS update out of bounds for given layout section | DESCRIPTOR_UPDATE_OUT_OF_BOUNDS | vkUpdateDescriptors | DSUpdateOutOfBounds | NA |
| DS Update Index | DS update binding too large for layout count | INVALID_UPDATE_INDEX | vkUpdateDescriptors | InvalidDSUpdateIndex | NA |
| DS Update Type | Verifies that structs in DS Update tree are properly created, currenly valid, and of the right type | INVALID_UPDATE_STRUCT | vkUpdateDescriptors | InvalidDSUpdateStruct | NA |
| MSAA Sample Count | Verifies that Pipeline, RenderPass, and Subpass sample counts are consistent | NUM_SAMPLES_MISMATCH | vkCmdBindPipeline vkCmdBeginRenderPass vkCmdNextSubpass | NumSamplesMismatch | NA |
| Dynamic State Binding | Verify that viewport and raster dynamic states are bound to Cmd Buffer at Draw time, and if color or depth are enabled, verify that their respective states are also bound | VIEWPORT_NOT_BOUND, RASTER_NOT_BOUND, COLOR_NOT_BOUND, DEPTH_NOT_BOUND |vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | RasterStateNotBound, ViewportStateNotBound, ColorBlendStateNotBound, DepthStencilStateNotBound | Write targeted test |
| RenderPass misuse | Tests for the following: that BeginRenderPass, ResolveImage, ClearColorImage, ClearDepthStencilImage are not called during an active Renderpass, and that binding compute descriptior sets or pipelines does not take place during an active  | INVALID_RENDERPASS_CMD | vkCmdBindPipeline() w/ COMPUTE vkCmdBindDescriptorSets() w/ COMPUTE vkCmdBlitImage vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage vkCmdBeginRenderPass | RenderPassWithinRenderPass | NA |
| Correct use of RenderPass | Validates that rendering commands are issued inside an active RenderPass | NO_ACTIVE_RENDERPASS | vkCmdBindPipeline vkCmdBindDynamicViewportState vkCmdBindDynamicLineWidthState vkCmdBindDynamicDepthBiasState vkCmdBindDynamicBlendState vkCmdBindDynamicDepthBoundsState vkCmdBindDynamicStencilState vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdClearColorAttachment vkCmdClearDepthStencilAttachment vkCmdNextSubpass vkCmdEndRenderPass vkCmdExecuteCommands vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | BindPipelineNoRenderPass, VtxBufferNoRenderPass | NA |
| Valid RenderPass | Flag error if attempt made to Begin/End/Continue a NULL or otherwise invalid RenderPass object | INVALID_RENDERPASS | vkCmdBeginRenderPass, vkCmdEndRenderPass, vkBeginCommandBuffer | NullRenderPass | NA |
| DescriptorSet Updated | Warn user if DescriptorSet bound that was never updated | DESCRIPTOR_SET_NOT_UPDATED | vkCmdBindDescriptorSets() | DescriptorSetNotUpdated | NA |
| Correct Clear Use | Warn user if CmdClear for Color or DepthStencil issued to Cmd Buffer prior to a Draw Cmd. RenderPass LOAD_OP_CLEAR is preferred in this case. | CLEAR_CMD_BEFORE_DRAW | vkCmdClearColorImage(), vkCmdClearDepthStencil() | ClearCmdNoDraw | NA |


### DrawState Pending Work
Additional checks to be added to DrawState

 5. dynamicOffsetCount lists how many entries are present in pDynamicOffsets - account for this (not certain of what needs to be done here but we don't have any test cases for dynamicOffsets, so keeping this task around until we do)
 7. Lifetime validation (See [bug 13383](https://cvs.khronos.org/bugzilla/show_bug.cgi?id=13383))
	 8. XGL_DESCRIPTOR_SET
		 9. Cannot be deleted until no longer in use on GPU, or referenced in any pending command.
		 10. Sets in XGL_DESCRIPTOR_REGION_USAGE_ONE_SHOT regions can never be deleted. Instead the xglClearDescriptorRegion() deletes all sets.
		 11. Sets in XGL_DESCRIPTOR_REGION_USAGE_DYNAMIC regions can be deleted. 
	 12. XGL_DESCRIPTOR_SET_LAYOUT
		 13. What do IHVs want here?
		 14. Option 1 (assuming this one): Must not be deleted until all sets and layout chains referencing the set layout are deleted. 
		 15. Option 2: Can be freely deleted after usage. 
	 19. XGL_DESCRIPTOR_REGION
		 20. Cannot be deleted until no longer in use on the GPU, or referenced in any pending command. 
	 21. XGL_BUFFER_VIEW, XGL_IMAGE_VIEW, etc
		 22. Cannot be deleted until the descriptors referencing the objects are deleted.
	 23. For ClearColorAttachment function, verify that the index of referenced attachment actually exists
 24. GetRenderAreaGranularity - The pname:renderPass parameter must be the same as the one given in the sname:VkRenderPassBeginInfo structure for which the render area is relevant.
 26. vkFreeDescriptorSets must not be called for sets created on top of one-shot pool
 27. If Cmd Buffer one-time submit flag is set, then verify that cmd buffer is only submitted once
 28. Verify that all relevent dynamic state objects are bound (See https://cvs.khronos.org/bugzilla/show_bug.cgi?id=14323)
 29. Flag an error if CmdBuffer has Begin called while it's being constructed - this is not a reset, this is a violation
 30. At PSO creation time, there is no case when NOT including a FS should flag an error since there exist dynamic state configurations that can be set to cause a FS to not be required. Instead, in the case when no FS is in the PSO, validation should detect at runtime if dynamic state will require a FS, and in those case issue a runtime warning about undefined behavior. (see bug https://cvs.khronos.org/bugzilla/show_bug.cgi?id=14429)
 31. Error if a cmdbuffer is submitted on a queue whose family doesn't match the family of the pool from which it was created.

## ParamChecker

### ParamChecker Overview

The ParamChecker layer validates parameter values and flags errors for any values that are outside of acceptable values for the given parameter.

### ParamChecker Details Table

| Check | Overview | ENUM | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Input Parameters | Pointers in structures are recursively validated to be non-null. Enumerated types are validated against min and max enum values. Structure Types are verified to be correct. | NA | vkQueueSubmit vkAllocMemory vkFlushMappedMemoryRanges vkInvalidateMappedMemoryRanges vkQueueBindSparseBufferMemory vkQueueBindSparseImageOpaqueMemory vkQueueBindSparseImageMemory vkCreateFence vkResetFences vkWaitForFences vkCreateSemaphore vkCreateEvent vkCreateQueryPool vkCreateBuffer vkCreateBufferView vkCreateImage vkGetImageSubresourceLayout vkCreateImageView vkCreateAttachmentView vkCreateShader vkCreatePipelineCache vkMergePipelineCaches vkCreateGraphicsPipelines vkCreateComputePipelines vkCreatePipelineLayout vkCreateSampler vkCreateDescriptorSetLayout( vkCreateDescriptorPool vkAllocDescriptorSets vkFreeDescriptorSets vkUpdateDescriptorSets vkCreateDynamicViewportState vkCreateDynamicLineWidthState vkCreateDynamicDepthBiasState vkCreateDynamicBlendState vkCreateDynamicDepthBoundsState vkCreateDynamicStencilState vkCreateFramebuffer vkCreateRenderPass vkCreateCommandPool vkCreateCommandBuffer vkBeginCommandBuffer vkCmdBindDescriptorSets vkCmdBindVertexBuffers vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdClearColorAttachment vkCmdClearDepthStencilAttachment vkCmdResolveImage vkCmdWaitEvents vkCmdPipelineBarrier vkCmdPushConstants vkCmdBeginRenderPass vkCmdExecuteCommands | TBD | NA |
| Call results, Output Parameters | Return values are checked for VK_SUCCESS, returned pointers are checked to be NON-NULL, enumerated types of return values are checked to be within the defined range. | NA | vkEnumeratePhysicalDevices vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceLimits vkGetPhysicalDeviceProperties vkGetPhysicalDeviceQueueCount vkGetPhysicalDeviceQueueProperties vkGetPhysicalDeviceMemoryProperties vkGetDeviceQueue vkQueueSubmit vkQueueWaitIdle vkDeviceWaitIdle vkAllocMemory vkFreeMemory vkMapMemory vkUnmapMemory vkFlushMappedMemoryRanges vkInvalidateMappedMemoryRanges vkGetDeviceMemoryCommitment vkBindBufferMemory vkBindImageMemory vkGetBufferMemoryRequirements vkGetImageMemoryRequirements vkGetImageSparseMemoryRequirements vkGetPhysicalDeviceSparseImageFormatProperties vkQueueBindSparseBufferMemory vkQueueBindSparseImageOpaqueMemory vkQueueBindSparseImageMemory vkCreateFence vkDestroyFence vkResetFences vkGetFenceStatus vkWaitForFences vkCreateSemaphore vkDestroySemaphore vkQueueSignalSemaphore vkQueueWaitSemaphore vkCreateEvent vkDestroyEvent vkGetEventStatus vkSetEvent vkResetEvent vkCreateQueryPool vkDestroyQueryPool vkGetQueryPoolResults vkCreateBuffer vkDestroyBuffer vkCreateBufferView vkDestroyBufferView vkCreateImage vkDestroyImage vkGetImageSubresourceLayout vkCreateImageView vkDestroyImageView vkCreateAttachmentView vkDestroyAttachmentView vkDestroyShaderModule vkCreateShader vkDestroyShader vkCreatePipelineCache vkDestroyPipelineCache vkGetPipelineCacheSize vkGetPipelineCacheData vkMergePipelineCaches vkCreateGraphicsPipelines vkCreateComputePipelines vkDestroyPipeline vkCreatePipelineLayout vkDestroyPipelineLayout vkCreateSampler vkDestroySampler vkCreateDescriptorSetLayout vkDestroyDescriptorSetLayout vkCreateDescriptorPool vkDestroyDescriptorPool vkResetDescriptorPool vkAllocDescriptorSets vkFreeDescriptorSets vkUpdateDescriptorSets vkCreateDynamicViewportState vkDestroyDynamicViewportState vkCreateDynamicLineWidthState vkDestroyDynamicLineWidthState vkCreateDynamicDepthBiasState vkDestroyDynamicDepthBiasState vkCreateDynamicBlendState vkDestroyDynamicBlendState vkCreateDynamicDepthBiasState vkDestroyDynamicDepthBiasState vkCreateDynamicStencilState vkDestroyDynamicStencilState  vkCreateFramebuffer vkDestroyFramebuffer vkCreateRenderPass vkDestroyRenderPass vkGetRenderAreaGranularity vkCreateCommandPool vkDestroyCommandPool vkResetCommandPool vkCreateCommandBuffer vkDestroyCommandBuffer vkBeginCommandBuffer vkEndCommandBuffer vkResetCommandBuffer vkCmdBindPipeline vkCmdBindDynamicViewportState vkCmdBindDynamicRasterState vkCmdBindDynamicBlendState vkCmdBindDynamicDepthStencilState vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdClearColorAttachment vkCmdClearDepthStencilAttachment vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdWaitEvents vkCmdPipelineBarrier vkCmdBeginQuery vkCmdEndQuery vkCmdResetQueryPool vkCmdWriteTimestamp vkCmdCopyQueryPoolResults vkCmdPushConstants vkCmdBeginRenderPass vkCmdNextSubpass vkCmdEndRenderPass vkCmdExecuteCommands | TBD | NA |

### ParamChecker Pending Work
Additional work to be done

 1. Source2 was creating a VK_FORMAT_R8_SRGB texture (and image view) which was not supported by the underlying implementation (rendersystemtest imageformat test).  Checking that formats are supported by the implementation is something the validation layer could do using the VK_FORMAT_INFO_TYPE_PROPERTIES query.   There are probably a bunch of checks here you could be doing around vkCreateImage formats along with whether image/color/depth attachment views are valid.  I’m not sure how much of this is already there.
 2.  From AMD: we were using an image view with a swizzle of VK_CHANNEL_FORMAT_A with a BC1_RGB texture, which is not valid because the texture does not have an alpha channel.  In general, should validate that the swizzles do not reference components not in the texture format.
 3. When querying VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES must provide enough memory for a all the queues on the device (not just 1 when device has multiple queues).
 4. INT & FLOAT bordercolors. Border color int/float selection must match associated texture format.
 5. Flag error on VkBufferCreateInfo if buffer size is 0
 6. VkAttachmentViewCreateInfo.format must be set

## Image

### Image Overview

The Image layer is responsible for validating format-related information and enforcing format restrictions.

### Image Details Table

DETAILS TABLE PENDING

| Check | Overview | ENUM | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Image Format | Verifies returned format to ensure that it is a supported Vulkan format | NA | vkCreateImage vkCreateRenderPass | TBD | NA |
| Image Format | Validates that attachment image layouts, loadOps, and storeOps are valid Vulkan values; Verifies that if there is no depth attachment then the subpass attachment is set to VK_ATTACHMENT_UNUSED | NA | vkCreateRenderPass | TBD | NA |

### Image Pending Work
Additional work to be done

## MemTracker

### MemTracker Overview

The MemTracker layer tracks memory objects and references and validates that they are managed correctly by the application.  This includes tracking object bindings, memory hazards, and memory object lifetimes.. MemTracker validates several other hazard-related issues related to command buffers, fences, and memory mapping.

### MemTracker Details Table

| Check | Overview | ENUM MEMTRACK_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Valid Command Buffer | Verifies that the command buffer was properly created and is currently valid | INVALID_CB | vkCmdBindPipeline vkCmdBindDynamicViewportState vkCmdBindDynamicLineWidthState vkCmdBindDynamicDepthBiasState vkCmdBindDynamicBlendState vkCmdBindDynamicDepthBoundsState vkCmdBindDynamicStencilState vkBeginCommandBuffer vkResetCommandBuffer vkDestroyDevice vkFreeMemory | NA | NA |
| Valid Memory Object | Verifies that the memory object was properly created and is currently valid | INVALID_MEM_OBJ | vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage vkFreeMemory vkBindBufferMemory vkBindImageMemory vkQueueBindSparseImageOpaqueMemory vkQueueBindSparseImageMemory vkQueueBindSparseBufferMemory | NA | NA |                                 
| Free Referenced Memory | Checks to see if memory being freed still has current references | FREED_MEM_REF | vmFreeMemory | FreeBoundMemory | NA |               
| Memory Properly Bound | Validate that the memory object referenced in the call was properly created, is currently valid, and is properly bound to the object | MISSING_MEM_BINDINGS | vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage | NA | NA |        
| Valid Object | Verifies that the specified Vulkan object was created properly and is currently valid | INVALID_OBJECT | vkCmdBindPipeline vkCmdBindDynamicViewportState vkCmdBindDynamicLineWidthState vkCmdBindDynamicDepthBiasState vkCmdBindDynamicBlendState vkCmdBindDynamicDepthBoundsState vkCmdBindDynamicStencilState vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage | NA | NA |
| Bind Invalid Memory | Validate that memory object was correctly created, that the command buffer object was correctly created, and that both are currently valid objects. | MEMORY_BINDING_ERROR | vkQueueBindSparseImageOpaqueMemory vkQueueBindSparseImageMemory vkQueueBindSparseBufferMemory vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage | BindInvalidMemory BindMemoryToDestroyedObject | This is probably redundant, as these checks are performed at a lower level. | 
| Objects Not Destroyed | Verify all objects destroyed at DestroyDevice time | MEMORY_LEAK | vkDestroyDevice | NA | NA |                 
| Memory Mapping State | Verifies that mapped memory is CPU-visible | INVALID_STATE | vkMapMemory | MapMemWithoutHostVisibleBit | NA |               
| Command Buffer Synchronization | Command Buffer must be complete before BeginCommandBuffer or ResetCommandBuffer can be called | RESET_CB_WHILE_IN_FLIGHT | vkBeginCommandBuffer vkResetCommandBuffer | CallBeginCmdBufferBeforeCompletion CallBeginCmdBufferBeforeCompletion | NA |
| Submitted Fence Status | Verifies that: The fence is not submitted in an already signaled state, and that ResetFences is not called with a fence in an unsignaled state | INVALID_FENCE_STATE | vkResetFences vkWaitForFences vkQueueSubmit | SubmitSignaledFence ResetUnsignaledFence | NA |
| Immutable Memory Binding | Validates that non-sparse memory bindings are immutable, so objects are not re-boundt | REBIND_OBJECT | vkBindBufferMemory, vkBindImageMemory | RebindMemory | NA |
| Image/Buffer Usage bits | Verify correct USAGE bits set based on how Images and Buffers are used | INVALID_USAGE_FLAG | vkCreateColorImage, vkCreateColorAttachmentView, vkCreateDepthStencilView, vkCmdCopyBuffer, vkCmdCopyImage, vkCmdBlitImage, vkCmdCopyBufferToImage, vkCmdCopyImageToBuffer, vkCmdUpdateBuffer, vkCmdFillBuffer  | InvalidUsageBits | NA |
| Objects Not Destroyed Warning | Warns if any memory objects have not been freed before their objects are destroyed | MEM_OBJ_CLEAR_EMPTY_BINDINGS | vkDestroyDevice | TBD | NA |


### MemTracker Pending Work

#### MemTracker Enhancements

1.  Flag any memory hazards: Validate that the pipeline barriers for buffers are sufficient to avoid hazards
2.  Make sure that the XGL_IMAGE_VIEW_ATTACH_INFO.layout matches the layout of the image as determined by the last IMAGE_MEMORY_BARRIER
3.  Verify that the XGL_IMAGE_MEMORY_BARRIER.oldLayout matches the actual previous layout (this one was super important for previous work in dealing with out-of-order command buffer generation). Note that these   
need to be tracked for each subresource.  
4.  Update for new Memory Binding Model
5.  Consolidate error messages and make them consistent
7.  Add validation for having mapped objects in a command list - GPU writing to mapped object is warning
8.  Add validation for maximum memory references, maximum object counts, and object leaks
9. When performing clears on surfaces that have both Depth and Stencil, WARN user if subresource range for depth and stencil are not both set (see blit_tests.cpp VkCmdClearDepthStencilTest test).
10.  Re-enable INFO messages that were disabled during v138 integration
11. Warn on image/buffer deletion if USAGE bits were set that were not needed
12. Modify INVALID_FENCE_STATE to be WARNINGs instead of ERROR


## ShaderChecker

### ShaderChecker Overview

The ShaderChecker layer inspects the SPIR-V shader images and fixed function pipeline stages at PSO creation time.
It flags errors when inconsistencies are found across interfaces between shader stages. The exact behavior of the checks
depends on the pair of pipeline stages involved.

### ShaderChecker Details Table

| Check | Overview | ENUM SHADER_CHECKER_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Not consumed | Flag warning if a location is not consumed (useless work) | OUTPUT_NOT_CONSUMED | vkCreateGraphicsPipeline | CreatePipeline*NotConsumed | NA |
| Not produced | Flag error if a location is not produced (consumer reads garbage) | INPUT_NOT_PRODUCED | vkCreateGraphicsPipeline | CreatePipeline*NotProvided | NA |
| Type mismatch | Flag error if a location has inconsistent types | INTERFACE_TYPE_MISMATCH | vkCreateGraphicsPipeline | CreatePipeline*TypeMismatch | Between shader stages, an exact structural type match is required. Between VI and VS, or between FS and CB, only the basic component type must match (float for UNORM/SNORM/FLOAT, int for SINT, uint for UINT) as the VI and CB stages perform conversions to the exact format. |
| Inconsistent shader | Flag error if an inconsistent SPIR-V image is detected. Possible cases include broken type definitions which the layer fails to walk. | INCONSISTENT_SPIRV | vkCreateGraphicsPipeline | TODO | All current tests use the reference compiler to produce valid SPIRV images from GLSL. |
| Non-SPIRV shader | Flag warning if a non-SPIR-V shader image is detected. This can occur if early drivers are ingesting GLSL. ShaderChecker cannot analyze non-SPIRV shaders, so this suppresses most other checks. | NON_SPIRV_SHADER | vkCreateGraphicsPipeline | TODO | NA |
| FS mixed broadcast | Flag error if the fragment shader writes both the legacy gl_FragCoord (which broadcasts to all CBs) and custom FS outputs. | FS_MIXED_BROADCAST | vkCreateGraphicsPipeline | TODO | Reference compiler refuses to compile shaders which do this |
| VI Binding Descriptions | Validate that there is a single vertex input binding description for each binding | INCONSISTENT_VI | vkCreateGraphicsPipeline | CreatePipelineAttribBindingConflict | NA |
| Shader Stage Check | Warns if shader stage is unsupported | UNKNOWN_STAGE | vkCreateGraphicsPipeline | TBD | NA |

### ShaderChecker Pending Work
- Additional test cases for variously broken SPIRV images
- Validation of a single SPIRV image in isolation (the spec describes many constraints)
- Validation of SPIRV use of descriptors against the declared descriptor set layout

## ObjectTracker

### ObjectTracker Overview

The ObjectTracker layer maintains a record of all Vulkan objects. It flags errors when invalid objects are used and at DestroyInstance time it flags any objects that were not properly destroyed.

### ObjectTracker Details Table

| Check | Overview | ENUM OBJTRACK_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Valid Object | Validates that referenced object was properly created and is currently valid. | UNKNOWN_OBJECT | vkAcquireNextImageWSI vkAllocDescriptorSets vkAllocMemory vkBeginCommandBuffer vkBindBufferMemory vkBindImageMemory vkCmdBeginQuery vkCmdBeginRenderPass vkCmdBindDescriptorSets vkCmdBindDynamicViewportState vkCmdBindDynamicLineWidthState vkCmdBindDynamicDepthBiasState vkCmdBindDynamicBlendState vkCmdBindDynamicDepthBoundsState vkCmdBindDynamicStencilState vkCmdBindIndexBuffer vkCmdBindPipeline vkCmdBindVertexBuffers vkCmdBlitImage vkCmdClearColorAttachment vkCmdClearColorImage vkCmdClearDepthStencilAttachment vkCmdClearDepthStencilImage vkCmdCopyBuffer vkCmdCopyBufferToImage vkCmdCopyImage vkCmdCopyImageToBuffer vkCmdCopyQueryPoolResults vkCmdDispatch vkCmdDispatchIndirect vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndexedIndirect vkCmdDrawIndirect vkCmdEndQuery vkCmdEndRenderPass vkCmdExecuteCommands vkCmdFillBuffer vkCmdNextSubpass vkCmdPipelineBarrier vkCmdPushConstants vkCmdResetEvent vkCmdResetQueryPool vkCmdResolveImage vkCmdSetEvent vkCmdUpdateBuffer vkCmdWaitEvents vkCmdWriteTimestamp vkCreateAttachmentView vkCreateBuffer vkCreateBufferView vkCreateCommandBuffer vkCreateCommandPool vkCreateComputePipelines vkCreateDescriptorPool vkCreateDescriptorSetLayout vkCreateDynamicViewportState vkCreateDynamicLineWidthState vkCreateDynamicDepthBiasState vkCreateDynamicBlendState vkCreateDynamicDepthBoundsState vkCreateDynamicStencilState vkCreateEvent vkCreateFence vkCreateFramebuffer vkCreateGraphicsPipelines vkCreateImage vkCreateImageView vkCreatePipelineCache vkCreatePipelineLayout vkCreateQueryPool vkCreateRenderPass vkCreateSampler vkCreateSemaphore vkCreateShader vkCreateShaderModule vkCreateSwapChainWSI vkDestroyAttachmentView vkDestroyBuffer vkDestroyBufferView vkDestroyCommandBuffer vkDestroyCommandPool vkDestroyDescriptorPool vkDestroyDescriptorSetLayout vkDestroyDynamicBlendState vkDestroyDynamicDepthStencilState vkDestroyDynamicRasterState vkDestroyDynamicViewportState vkDestroyEvent vkDestroyFence vkDestroyFramebuffer vkDestroyImage vkDestroyImageView vkDestroyPipeline vkDestroyPipelineCache vkDestroyPipelineLayout vkDestroyQueryPool vkDestroyRenderPass vkDestroySampler vkDestroySemaphore vkDestroyShader vkDestroyShaderModule vkDestroySwapChainWSI vkDeviceWaitIdle vkEndCommandBuffer vkEnumeratePhysicalDevices vkFreeDescriptorSets vkFreeMemory vkFreeMemory vkGetBufferMemoryRequirements vkGetDeviceMemoryCommitment vkGetDeviceQueue vkGetEventStatus vkGetFenceStatus vkGetImageMemoryRequirements vkGetImageSparseMemoryRequirements vkGetImageSubresourceLayout vkGetPhysicalDeviceSurfaceSupportWSI vkGetPipelineCacheData vkGetPipelineCacheSize vkGetQueryPoolResults vkGetRenderAreaGranularity vkGetSurfaceInfoWSI vkGetSwapChainInfoWSI vkInvalidateMappedMemoryRanges vkMapMemory vkMergePipelineCaches vkQueueBindSparseBufferMemory vkQueueSignalSemaphore vkQueueWaitSemaphore vkResetCommandBuffer vkResetCommandPool vkResetDescriptorPool vkResetEvent vkResetFences vkSetEvent vkUnmapMemory vkUpdateDescriptorSets vkWaitForFences | ? | Every VkObject class of parameter will be run through this check. This check may ultimately supersede UNKNOWN_OBJECT |
| Object Cleanup | Verify that object properly destroyed | DESTROY_OBJECT_FAILED | vkDestroyInstance, vkDestroyDevice, vkFreeMemory, vkDestroyObject | ? | NA |
| Object Type | Verify generic VkObject if of expected VkObjectType | OBJECT_TYPE_MISMATCH | vkDestroyObject, vkGetObjectInfo | GetObjectInfoMismatchedType | NA |
| Objects Leak | When an Instance or Device object is destroyed, validates that all objects belonging to that device/instance have previously been destroyed | OBJECT_LEAK | vkDestroyDevice vk DestroyInstance | ? | NA |
| Object Count | Flag error if number of objects requested from extenstion functions exceeds max number of actual objects | OBJCOUNT_MAX_EXCEEDED | objTrackGetObjects, objTrackGetObjectsOfType | ? | NA |
| Valid Fence for Wait | Flag error if waiting on unsubmitted fence object | INVALID_FENCE | vkGetFenceStatus | WaitForUnsubmittedFence | NA |
| Valid Destroy Object | Validates that an object pass into a destroy function was properly created and is currently valid | NONE | vkDestroyInstance vkDestroyDevice vkDestroyFence vkDestroySemaphore vkDestroyEvent vkDestroyQueryPool vkDestroyBuffer vkDestroyBufferView vkDestroyImage vkDestroyImageView vkDestroyAttachmentView vkDestroyShaderModule vkDestroyShader vkDestroyPipelineCache vkDestroyPipeline vkDestroyPipelineLayout vkDestroySampler vkDestroyDescriptorSetLayout vkDestroyDescriptorPool vkDestroyDynamicViewportState vkDestroyDynamicRasterState vkDestroyDynamicBlendState vkDestroyDynamicDepthStencilState vkDestroyCommandPool vkDestroyCommandBuffer vkDestroyFramebuffer vkDestroyRenderPass vkDestroySwapChainWSI | TBD | These cases need to be moved to a more appropriate error enum |

### ObjectTracker Pending Work

 4. Verify images have CmdPipelineBarrier layouts matching new layout parameters to Cmd*Image* functions
 6. For specific object instances that are allowed to be NULL, update object validation to verify that such objects are either NULL or valid
 7. Verify cube array VkImageView objects use subresourceRange.arraySize (or effective arraySize when VK_REMAINING_ARRAY_SLICES is specified) that is a multiple of 6. 

## Threading

### Threading Overview

The Threading layer checks for simultaneous use of objects by calls from multiple threads.
Application code is responsible for preventing simultaneous use of the same objects by certain calls that modify objects.
See [bug 13433](https://cvs.khronos.org/bugzilla/show_bug.cgi?id=13433) and
<https://cvs.khronos.org/svn/repos/oglc/trunk/nextgen/vulkan/function_properties.csv>
for threading rules.
Objects that may need a mutex include VkQueue, VkDeviceMemory, VkObject, VkBuffer, VkImage, VkDescriptorSet, VkDescriptorPool, VkCmdBuffer, and VkSemaphore.
The most common case is that a VkCmdBuffer passed to VkCmd* calls must be used by only one thread at a time.

In addition to reporting threading rule violations, the layer will enforce a mutex for those calls.
That can allow an application to continue running without actually crashing due to the reported threading problem.

The layer can only observe when a mutual exclusion rule is actually violated.
It cannot insure that there is no latent race condition needing mutual exclusion.

The layer can also catch reentrant use of the same object by calls from a single thread.
That might happen if Vulkan calls are made from a callback function or a signal handler.
But the layer cannot prevent such a reentrant use of an object.

The layer can only observe when a mutual exclusion rule is actually violated.
It cannot insure that there is no latent race condition.

### Threading Details Table

| Check | Overview | ENUM THREADING_CHECKER_* | Relevant API |
| ----- | -------- | ---------------- | ---------------- |
| Thread Collision | Detects and notifies user if multiple threads are modifying thes same object | MULTIPLE_THREADS | vkQueueSubmit vkFreeMemory vkMapMemory vkUnmapMemory vkFlushMappedMemoryRanges vkInvalidateMappedMemoryRanges vkBindBufferMemory vkBindImageMemory vkQueueBindSparseBufferMemory vkQueueBindSparseImageOpaqueMemory vkQueueBindSparseImageMemory vkDestroySemaphore vkQueueSignalSemaphore vkDestroyBuffer vkDestroyImage vkDestroyDescriptorPool vkResetDescriptorPool vkAllocDescriptorSets vkFreeDescriptorSets vkDestroyCommandBuffer vkBeginCommandBuffer vkEndCommandBuffer vkResetCommandBuffer vkCmdBindPipeline vkCmdBindDynamicViewportState vkCmdBindDynamicRasterState vkCmdBindDynamicBlendState vkCmdBindDynamicDepthStencilState vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdClearColorAttachment vkCmdClearDepthStencilAttachment vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdWaitEvents vkCmdPipelineBarrier vkCmdBeginQuery vkCmdEndQuery vkCmdResetQueryPool vkCmdWriteTimestamp vkCmdCopyQueryPoolResults vkCmdBeginRenderPass vkCmdNextSubpass vkCmdPushConstants vkCmdEndRenderPass vkCmdExecuteCommands |
| Thread Reentrancy | Detects cases of a single thread calling Vulkan reentrantly | SINGLE_THREAD_REUSE | vkQueueSubmit vkFreeMemory vkMapMemory vkUnmapMemory vkFlushMappedMemoryRanges vkInvalidateMappedMemoryRanges vkBindBufferMemory vkBindImageMemory vkQueueBindSparseBufferMemory vkQueueBindSparseImageOpaqueMemory vkQueueBindSparseImageMemory vkDestroySemaphore vkQueueSignalSemaphore vkDestroyBuffer vkDestroyImage vkDestroyDescriptorPool vkResetDescriptorPool vkAllocDescriptorSets vkFreeDescriptorSets vkDestroyCommandBuffer vkBeginCommandBuffer vkEndCommandBuffer vkResetCommandBuffer vkCmdBindPipeline vkCmdBindDynamicViewportState vkCmdBindDynamicRasterState vkCmdBindDynamicBlendState vkCmdBindDynamicDepthStencilState vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdClearColorAttachment vkCmdClearDepthStencilAttachment vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdWaitEvents vkCmdPipelineBarrier vkCmdBeginQuery vkCmdEndQuery vkCmdResetQueryPool vkCmdWriteTimestamp vkCmdCopyQueryPoolResults vkCmdBeginRenderPass vkCmdNextSubpass vkCmdPushConstants vkCmdEndRenderPass vkCmdExecuteCommands |

### Threading Pending Work
Additional work to be done

## General Pending Work
A place to capture general validation work to be done. This includes new checks that don't clearly fit into the above layers.

## Device Limitations

### Device Limitations Overview

This layer does not yet exist. The general idea is that at the beginning of time this layer would query device limitations in terms of memory size, format and feature support, and so on. This entails making a complete set of vkGetPhysicalDevice* calls and storing the results. If, later on, the app violates these limitations, then this layer would flag those violations.

### Device Limitations Pending Work

 1. For all Formats, call vkGetPhysicalDeviceFormatProperties to pull their properties for the underlying device. After that point, if the app attempts to use any formats in violation of those properties, flag errors.

# Non-validation Layer Details

## APIDump

APIDump layer is used for dumping a stream of all the Vulkan API calls that are made, along with details of the parameters to those calls.

### APIDump Pending Work

 1. vkAllocDescriptorSets does not correctly print out all of the created DescriptorSets (no array printing following main API txt)
