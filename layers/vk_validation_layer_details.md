[TOC]

# Validation Layer Details

## DrawState

### DrawState Overview

The DrawState layer tracks state leading into Draw cmds. This includes the Pipeline state, dynamic state, and descriptor set state. DrawState validates the consistency and correctness between and within these states.

### DrawState Details Table

| Check | Overview | ENUM DRAWSTATE_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Valid Pipeline Layouts | Verify that sets being bound are compatible with their PipelineLayout and that the last-bound PSO PipelineLayout at Draw time is compatible with all bound sets used by that PSO | PIPELINE_LAYOUTS_INCOMPATIBLE | vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | TBD | None |
| Validate DbgMarker exensions | Validates that DbgMarker extensions have been enabled before use | INVALID_EXTENSION | vkCmdDbgMarkerBegin vkCmdDbgMarkerEnd | TBD | None |
| Valid BeginCommandBuffer level-related parameters | Primary command buffers must specify VK_NULL_HANDLE for RenderPass or Framebuffer parameters, while secondary command buffers must provide non-null parameters | BEGIN_CB_INVALID_STATE | vkBeginCommandBuffer | PrimaryCommandBufferFramebufferAndRenderpass SecondaryCommandBufferFramebufferAndRenderpass | None |
| PSO Bound | Verify that a properly created and valid pipeline object is bound to the CommandBuffer specified in these calls | NO_PIPELINE_BOUND | vkCmdBindDescriptorSets vkCmdBindVertexBuffers | PipelineNotBound | This check is currently more related to DrawState data structures and less about verifying that PSO is bound at all appropriate points in API. For API purposes, need to make sure this is checked at Draw time and any other relevant calls. |
| Valid DescriptorPool | Verifies that the descriptor set pool object was properly created and is valid | INVALID_POOL | vkResetDescriptorPool vkAllocateDescriptorSets | None | This is just an internal layer data structure check. ParamChecker or ObjectTracker should really catch bad DSPool |
| Valid DescriptorSet | Validate that descriptor set was properly created and is currently valid | INVALID_SET | vkCmdBindDescriptorSets | None | Is this needed other places (like Update/Clear descriptors) |
| Valid DescriptorSetLayout | Flag DescriptorSetLayout object that was not properly created | INVALID_LAYOUT | vkAllocateDescriptorSets | None | Anywhere else to check this? |
| Valid Pipeline | Flag VkPipeline object that was not properly created | INVALID_PIPELINE | vkCmdBindPipeline | InvalidPipeline | NA |
| Valid PipelineLayout | Flag VkPipelineLayout object that was not properly created | INVALID_PIPELINE_LAYOUT | vkCmdBindPipeline | TODO | Write test for this case |
| Valid Pipeline Create Info | Tests for the following: That compute shaders are not specified for the graphics pipeline, tess evaluation and tess control shaders are included or excluded as a pair, that VK_PRIMITIVE_TOPOLOGY_PATCH_LIST is set as IA topology for tessellation pipelines, that VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology is only set for tessellation pipelines, and that Vtx Shader specified | INVALID_PIPELINE_CREATE_STATE | vkCreateGraphicsPipelines | InvalidPipelineCreateState | NA |
| Valid CommandBuffer | Validates that the command buffer object was properly created and is currently valid | INVALID_COMMAND_BUFFER | vkQueueSubmit vkBeginCommandBuffer vkEndCommandBuffer vkCmdBindPipeline vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearAttachments vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdWaitEvents vkCmdPipelineBarrier vkCmdBeginQuery vkCmdEndQuery vkCmdResetQueryPool vkCmdWriteTimestamp vkCmdBeginRenderPass vkCmdNextSubpass vkCmdEndRenderPass vkCmdExecuteCommands vkCmdDbgMarkerBegin vkCmdDbgMarkerEnd vkAllocateCommandBuffers | None | NA |
| Vtx Buffer Bounds | Check if VBO index too large for PSO Vtx binding count, and that at least one vertex buffer is attached to pipeline object | VTX_INDEX_OUT_OF_BOUNDS | vkCmdBindDescriptorSets vkCmdBindVertexBuffers | VtxBufferBadIndex | NA |
| Idx Buffer Alignment | Verify that offset of Index buffer falls on an alignment boundary as defined by IdxBufferAlignmentError param | VTX_INDEX_ALIGNMENT_ERROR | vkCmdBindIndexBuffer | IdxBufferAlignmentError | NA |
| Cmd Buffer End | Verifies that EndCommandBuffer was called for this commandBuffer at QueueSubmit time | NO_END_COMMAND_BUFFER | vkQueueSubmit | NoEndCommandBuffer | NA |
| Cmd Buffer Begin | Check that BeginCommandBuffer was called for this command buffer when binding commands or calling end | NO_BEGIN_COMMAND_BUFFER | vkEndCommandBuffer vkCmdBindPipeline vkCmdSetViewport vkCmdSetLineWidth vkCmdSetDepthBias vkCmdSetBlendConstants vkCmdSetDepthBounds vkCmdSetStencilCompareMask vkCmdSetStencilWriteMask vkCmdSetStencilReference vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearAttachments vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdWaitEvents vkCmdPipelineBarrier vkCmdBeginQuery vkCmdEndQuery vkCmdResetQueryPool vkCmdWriteTimestamp | NoBeginCommandBuffer | NA |
| Cmd Buffer Submit Count | Verify that ONE_TIME submit cmdbuffer is not submitted multiple times | COMMAND_BUFFER_SINGLE_SUBMIT_VIOLATION | vkBeginCommandBuffer, vkQueueSubmit | CommandBufferTwoSubmits | NA |
| Valid Secondary CommandBuffer | Validates that no primary command buffers are sent to vkCmdExecuteCommands() are | INVALID_SECONDARY_COMMAND_BUFFER | vkCmdExecuteCommands | ExecuteCommandsPrimaryCB | NA |
| Descriptor Type | Verify Descriptor type in bound descriptor set layout matches descriptor type specified in update. This also includes mismatches in the TYPES of copied descriptors. | DESCRIPTOR_TYPE_MISMATCH | vkUpdateDescriptorSets | DSTypeMismatch CopyDescriptorUpdateErrors | NA |
| Descriptor StageFlags | Verify all descriptors within a single write update have the same stageFlags | DESCRIPTOR_STAGEFLAGS_MISMATCH | vkUpdateDescriptorSets | NONE | Test this case |
| DS Update Size | DS update out of bounds for given layout section. | DESCRIPTOR_UPDATE_OUT_OF_BOUNDS | vkUpdateDescriptorSets | DSUpdateOutOfBounds CopyDescriptorUpdateErrors | NA |
| Descriptor Pool empty | Attempt to allocate descriptor type from descriptor pool when no more of that type are available to be allocated. | DESCRIPTOR_POOL_EMPTY | vkAllocateDescriptorSets | AllocDescriptorFromEmptyPool | NA |
| Free from NON_FREE Pool | It's invalid to call vkFreeDescriptorSets() on Sets that were allocated from a Pool created with NON_FREE usage. | CANT_FREE_FROM_NON_FREE_POOL | vkFreeDescriptorSets | None | NA |
| DS Update Index | DS update binding too large for layout binding count. | INVALID_UPDATE_INDEX | vkUpdateDescriptorSets | InvalidDSUpdateIndex CopyDescriptorUpdateErrors | NA |
| DS Update Type | Verifies that structs in DS Update tree are properly created, currenly valid, and of the right type | INVALID_UPDATE_STRUCT | vkUpdateDescriptorSets | InvalidDSUpdateStruct | NA |
| MSAA Sample Count | Verifies that Pipeline, RenderPass, and Subpass sample counts are consistent | NUM_SAMPLES_MISMATCH | vkCmdBindPipeline vkCmdBeginRenderPass vkCmdNextSubpass | NumSamplesMismatch | NA |
| Dynamic Viewport State Binding | Verify that viewport dynamic state bound to Cmd Buffer at Draw time | VIEWPORT_NOT_BOUND |vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | ViewportStateNotBound | NA |
| Dynamic Scissor State Binding | Verify that scissor dynamic state bound to Cmd Buffer at Draw time | SCISSOR_NOT_BOUND |vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | ScissorStateNotBound | NA |
| Dynamic Line Width State Binding | Verify that line width dynamic state bound to Cmd Buffer at when required (TODO : Verify when this is) | LINE_WIDTH_NOT_BOUND |vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | TODO | Verify this check and Write targeted test |
| Dynamic Depth Bias State Binding | Verify that depth bias dynamic state bound when depth enabled | DEPTH_BIAS_NOT_BOUND |vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | TODO | Verify this check and Write targeted test |
| Dynamic Blend State Binding | Verify that blend dynamic state bound when color blend enabled | BLEND_NOT_BOUND |vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | TODO | Verify this check and Write targeted test |
| Dynamic Depth Bounds State Binding | Verify that depth bounds dynamic state bound when depth enabled | DEPTH_BOUNDS_NOT_BOUND |vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | TODO | Verify this check and Write targeted test |
| Dynamic Stencil State Binding | Verify that stencil dynamic state bound when depth enabled | STENCIL_NOT_BOUND |vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect | TODO | Verify this check and Write targeted test |
| RenderPass misuse | Tests for the following: that vkCmdDispatch, vkCmdDispatchIndirect, vkCmdCopyBuffer, vkCmdCopyImage, vkCmdBlitImage, vkCmdCopyBufferToImage, vkCmdCopyImageToBuffer, vkCmdUpdateBuffer, vkCmdFillBuffer, vkCmdClearColorImage, vkCmdClearDepthStencilImage, vkCmdResolveImage, vkCmdSetEvent, vkCmdResetEvent, vkCmdResetQueryPool, vkCmdCopyQueryPoolResults, vkCmdBeginRenderPass are not called during an active Renderpass, and that binding compute descriptor sets or pipelines does not take place during an active Renderpass  | INVALID_RENDERPASS_CMD | vkCmdBindPipeline vkCmdBindDescriptorSets vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdResetQueryPool vkCmdCopyQueryPoolResults vkCmdBeginRenderPass | RenderPassWithinRenderPass UpdateBufferWithinRenderPass ClearColorImageWithinRenderPass ClearDepthStencilImageWithinRenderPass FillBufferWithinRenderPass | NA |
| Correct use of RenderPass | Validates that the following rendering commands are issued inside an active RenderPass: vkCmdDraw, vkCmdDrawIndexed, vkCmdDrawIndirect, vkCmdDrawIndexedIndirect, vkCmdClearAttachments, vkCmdNextSubpass, vkCmdEndRenderPass | NO_ACTIVE_RENDERPASS | vkCmdBindPipeline vkCmdBindDescriptorSets  vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdClearAttachments vkCmdNextSubpass vkCmdEndRenderPass | BindPipelineNoRenderPass ClearAttachmentsOutsideRenderPass | NA |
| Valid RenderPass | Flag error if attempt made to Begin/End/Continue a NULL or otherwise invalid RenderPass object | INVALID_RENDERPASS | vkCmdBeginRenderPass vkCmdEndRenderPass vkBeginCommandBuffer | NullRenderPass | NA |
| DescriptorSet Updated | Warn user if DescriptorSet bound that was never updated | DESCRIPTOR_SET_NOT_UPDATED | vkCmdBindDescriptorSets | DescriptorSetNotUpdated | NA |
| Dynamic Offset Count | Error if dynamicOffsetCount at CmdBindDescriptorSets time is not equal to the actual number of dynamic descriptors in all sets being bound. | INVALID_DYNAMIC_OFFSET_COUNT | vkCmdBindDescriptorSets | TODO | Write a test that hits this case |
| Correct Clear Use | Warn user if CmdClear for Color or DepthStencil issued to Cmd Buffer prior to a Draw Cmd. RenderPass LOAD_OP_CLEAR is preferred in this case. | CLEAR_CMD_BEFORE_DRAW | vkCmdClearColorImage vkCmdClearDepthStencilImage | ClearCmdNoDraw | NA |
| Index Buffer Binding | Verify that an index buffer is bound at the point when an indexed draw is attempted. | INDEX_BUFFER_NOT_BOUND | vkCmdDrawIndexed vkCmdDrawIndexedIndirect | TODO | Implement validation test |
| Viewport and Scissors match | In PSO viewportCount and scissorCount must match. Also for each count that is non-zero, there corresponding data array ptr should be non-NULL. | VIEWPORT_SCISSOR_MISMATCH | vkCreateGraphicsPipelines vkCmdSetViewport vkCmdSetScissor | TODO | Implement validation test |
| Valid Image Aspects for descriptor Updates | When updating ImageView for Descriptor Sets with layout of DEPTH_STENCIL type, the Image Aspect must not have both the DEPTH and STENCIL aspects set, but must have one of the two set. For COLOR_ATTACHMENT, aspect must have COLOR_BIT set. | INVALID_IMAGE_ASPECT | vkUpdateDescriptorSets | DepthStencilImageViewWithColorAspectBitError | This test hits Image layer error, but tough to create case that that skips that error and gets to DrawState error. |
| Valid sampler descriptor Updates | An invalid sampler is used when updating SAMPLER descriptor. | SAMPLER_DESCRIPTOR_ERROR | vkUpdateDescriptorSets | SampleDescriptorUpdateError | Currently only making sure sampler handle is known, can add further validation for sampler parameters |
| Immutable sampler update consistency | Within a single write update, all sampler updates must use either immutable samplers or non-immutable samplers, but not a combination of both. | INCONSISTENT_IMMUTABLE_SAMPLER_UPDATE | vkUpdateDescriptorSets | None | Write a test for this case |
| Valid imageView descriptor Updates | An invalid imageView is used when updating *_IMAGE or *_ATTACHMENT descriptor. | IMAGEVIEW_DESCRIPTOR_ERROR | vkUpdateDescriptorSets | ImageViewDescriptorUpdateError | Currently only making sure imageView handle is known, can add further validation for imageView and underlying image parameters |
| Valid bufferView descriptor Updates | An invalid bufferView is used when updating *_TEXEL_BUFFER descriptor. | BUFFERVIEW_DESCRIPTOR_ERROR | vkUpdateDescriptorSets | BufferViewDescriptorUpdateError | Currently only making sure bufferView handle is known, can add further validation for bufferView parameters |
| Valid bufferInfo descriptor Updates | An invalid bufferInfo is used when updating *_UNIFORM_BUFFER* or *_STORAGE_BUFFER* descriptor. | BUFFERINFO_DESCRIPTOR_ERROR | vkUpdateDescriptorSets | TODO | Implement validation test |
| Attachment References in Subpass | Attachment reference must be present in active subpass | MISSING_ATTACHMENT_REFERENCE | vkCmdClearAttachments | BufferInfoDescriptorUpdateError | Currently only making sure bufferInfo has buffer whose handle is known, can add further validation for bufferInfo parameters |
| Verify Image Layouts | Validate correct image layouts for presents, image transitions, command buffers and renderpasses | INVALID_IMAGE_LAYOUT | vkCreateRenderPass vkMapMemory vkQueuePresentKHR vkQueueSubmit vkCmdCopyImage vkCmdCopyImageToBuffer vkCmdWaitEvents VkCmdPipelineBarrier | TBD | None |
| Verify Memory Access Flags/Memory Barriers | Validate correct access flags for memory barriers | INVALID_BARRIER | vkCmdWaitEvents VkCmdPipelineBarrier | TBD | None |
| NA | Enum used for informational messages | NONE | | NA | None |
| NA | Enum used for errors in the layer itself. This does not indicate an app issue, but instead a bug in the layer. | INTERNAL_ERROR | | NA | None |
| NA | Enum used when Drawstate attempts to allocate memory for its own internal use and is unable to. | OUT_OF_MEMORY | | NA | None |
| NA | Enum used when Drawstate attempts to allocate memory for its own internal use and is unable to. | OUT_OF_MEMORY | | NA | None |

### DrawState Pending Work
Additional checks to be added to DrawState

 7. Lifetime validation (See [bug 13383](https://cvs.khronos.org/bugzilla/show_bug.cgi?id=13383))
	 8. XGL_DESCRIPTOR_SET
		 9. Cannot be deleted until no longer in use on GPU, or referenced in any pending command.
                 10. Sets in XGL_DESCRIPTOR_REGION_USAGE_NON_FREE regions can never be deleted. Instead the xglClearDescriptorRegion() deletes all sets.
		 11. Sets in XGL_DESCRIPTOR_REGION_USAGE_DYNAMIC regions can be deleted. 
	 12. XGL_DESCRIPTOR_SET_LAYOUT
		 13. What do IHVs want here?
		 14. Option 1 (assuming this one): Must not be deleted until all sets and layout chains referencing the set layout are deleted. 
		 15. Option 2: Can be freely deleted after usage. 
	 19. XGL_DESCRIPTOR_REGION
		 20. Cannot be deleted until no longer in use on the GPU, or referenced in any pending command. 
	 21. XGL_BUFFER_VIEW, XGL_IMAGE_VIEW, etc
		 22. Cannot be deleted until the descriptors referencing the objects are deleted.
	 23. For ClearAttachments function, verify that the index of referenced attachment actually exists
 24. GetRenderAreaGranularity - The pname:renderPass parameter must be the same as the one given in the sname:VkRenderPassBeginInfo structure for which the render area is relevant.
 28. Verify that all relevent dynamic state objects are bound (See https://cvs.khronos.org/bugzilla/show_bug.cgi?id=14323)
 29. Flag an error if CommandBuffer has Begin called while it's being constructed - this is not a reset, this is a violation
 30. At PSO creation time, there is no case when NOT including a FS should flag an error since there exist dynamic state configurations that can be set to cause a FS to not be required. Instead, in the case when no FS is in the PSO, validation should detect at runtime if dynamic state will require a FS, and in those case issue a runtime warning about undefined behavior. (see bug https://cvs.khronos.org/bugzilla/show_bug.cgi?id=14429)
 31. Error if a cmdbuffer is submitted on a queue whose family doesn't match the family of the pool from which it was created.
 32. Update Gfx Pipe Create Info shadowing to remove new/delete and instead use unique_ptrs for auto clean-up
 33. Add validation for Pipeline Derivatives (see Pipeline Derivatives) section of the spec

## ParamChecker

### ParamChecker Overview

The ParamChecker layer validates parameter values and flags errors for any values that are outside of acceptable values for the given parameter.

### ParamChecker Details Table

| Check | Overview | ENUM | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Input Parameters | Pointers in structures are recursively validated to be non-null. Enumerated types are validated against min and max enum values. Structure Types are verified to be correct. | NA | vkQueueSubmit vkAllocateMemory vkFlushMappedMemoryRanges vkInvalidateMappedMemoryRanges vkQueueBindSparse vkCreateFence vkResetFences vkWaitForFences vkCreateSemaphore vkCreateEvent vkCreateQueryPool vkCreateBuffer vkCreateBufferView vkCreateImage vkGetImageSubresourceLayout vkCreateImageView vkCreatePipelineCache vkMergePipelineCaches vkCreateGraphicsPipelines vkCreateComputePipelines vkCreatePipelineLayout vkCreateSampler vkCreateDescriptorSetLayout( vkCreateDescriptorPool vkAllocateDescriptorSets vkFreeDescriptorSets vkUpdateDescriptorSets vkCreateFramebuffer vkCreateRenderPass vkCreateCommandPool vkAllocateCommandBuffers vkBeginCommandBuffer vkCmdBindDescriptorSets vkCmdBindVertexBuffers vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdClearAttachments vkCmdResolveImage vkCmdWaitEvents vkCmdPipelineBarrier vkCmdPushConstants vkCmdBeginRenderPass vkCmdExecuteCommands | TBD | NA |
| Call results, Output Parameters | Return values are checked for VK_SUCCESS, returned pointers are checked to be NON-NULL, enumerated types of return values are checked to be within the defined range. | NA | vkEnumeratePhysicalDevices vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceLimits vkGetPhysicalDeviceProperties vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceMemoryProperties vkGetDeviceQueue vkQueueSubmit vkQueueWaitIdle vkDeviceWaitIdle vkAllocateMemory vkFreeMemory vkMapMemory vkUnmapMemory vkFlushMappedMemoryRanges vkInvalidateMappedMemoryRanges vkGetDeviceMemoryCommitment vkBindBufferMemory vkBindImageMemory vkGetBufferMemoryRequirements vkGetImageMemoryRequirements vkGetImageSparseMemoryRequirements vkGetPhysicalDeviceSparseImageFormatProperties vkQueueBindSparse vkCreateFence vkDestroyFence vkResetFences vkGetFenceStatus vkWaitForFences vkCreateSemaphore vkDestroySemaphore vkCreateEvent vkDestroyEvent vkGetEventStatus vkSetEvent vkResetEvent vkCreateQueryPool vkDestroyQueryPool vkGetQueryPoolResults vkCreateBuffer vkDestroyBuffer vkCreateBufferView vkDestroyBufferView vkCreateImage vkDestroyImage vkGetImageSubresourceLayout vkCreateImageView vkDestroyImageView vkDestroyShaderModule vkCreatePipelineCache vkDestroyPipelineCache vkGetPipelineCacheData vkMergePipelineCaches vkCreateGraphicsPipelines vkCreateComputePipelines vkDestroyPipeline vkCreatePipelineLayout vkDestroyPipelineLayout vkCreateSampler vkDestroySampler vkCreateDescriptorSetLayout vkDestroyDescriptorSetLayout vkCreateDescriptorPool vkDestroyDescriptorPool vkResetDescriptorPool vkAllocateDescriptorSets vkFreeDescriptorSets vkUpdateDescriptorSets vkCreateFramebuffer vkDestroyFramebuffer vkCreateRenderPass vkDestroyRenderPass vkGetRenderAreaGranularity vkCreateCommandPool vkDestroyCommandPool vkResetCommandPool vkAllocateCommandBuffers vkFreeCommandBuffers vkBeginCommandBuffer vkEndCommandBuffer vkResetCommandBuffer vkCmdBindPipeline vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdClearAttachments vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdWaitEvents vkCmdPipelineBarrier vkCmdBeginQuery vkCmdEndQuery vkCmdResetQueryPool vkCmdWriteTimestamp vkCmdCopyQueryPoolResults vkCmdPushConstants vkCmdBeginRenderPass vkCmdNextSubpass vkCmdEndRenderPass vkCmdExecuteCommands | TBD | NA |
| NA | Enum used for informational messages | NONE | | NA | None |

### ParamChecker Pending Work
Additional work to be done

 1. Source2 was creating a VK_FORMAT_R8_SRGB texture (and image view) which was not supported by the underlying implementation (rendersystemtest imageformat test).  Checking that formats are supported by the implementation is something the validation layer could do using the VK_FORMAT_INFO_TYPE_PROPERTIES query.   There are probably a bunch of checks here you could be doing around vkCreateImage formats along with whether image/color/depth attachment views are valid.  I’m not sure how much of this is already there.
 2.  From AMD: we were using an image view with a swizzle of VK_COLOR_COMPONENT_FORMAT_A with a BC1_RGB texture, which is not valid because the texture does not have an alpha channel.  In general, should validate that the swizzles do not reference components not in the texture format.
 3. When querying VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES must provide enough memory for a all the queues on the device (not just 1 when device has multiple queues).
 4. INT & FLOAT bordercolors. Border color int/float selection must match associated texture format.
 5. Flag error on VkBufferCreateInfo if buffer size is 0
 6. VkImageViewCreateInfo.format must be set
 7. For vkCreateGraphicsPipelines, correctly handle array of pCreateInfos and array of pStages within each element of pCreatInfos
 8. Check for valid VkIndexType in vkCmdBindIndexBuffer() should be in PreCmdBindIndexBuffer() call
 9. Check for valid VkPipelineBindPoint in vkCmdBindPipeline() & vkCmdBindDescriptorSets() should be in PreCmdBindPipeline() & PreCmdBindDescriptorSets() calls respectively.

## Image

### Image Layer Overview

The Image layer is responsible for validating format-related information and enforcing format restrictions.

### Image Layer Details Table

DETAILS TABLE PENDING

| Check | Overview | ENUM IMAGE_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Image Format | Verifies that requested format is a supported Vulkan format on this device | FORMAT_UNSUPPORTED | vkCreateImage vkCreateRenderPass | TBD | NA |
| RenderPass Attachments | Validates that attachment image layouts, loadOps, and storeOps are valid Vulkan values | RENDERPASS_INVALID_ATTACHMENT | vkCreateRenderPass | TBD | NA |
| Subpass DS Settings | Verifies that if there is no depth attachment then the subpass attachment is set to VK_ATTACHMENT_UNUSED | RENDERPASS_INVALID_DS_ATTACHMENT | vkCreateRenderPass | TBD | NA |
| View Creation | Verify that requested Image View Creation parameters are reasonable for the image that the view is being created for | VIEW_CREATE_ERROR | vkCreateImageView | TBD | NA |
| Image Aspects | Verify that Image commands are using valid Image Aspect flags | INVALID_IMAGE_ASPECT | vkCreateImageView vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdClearAttachments vkCmdCopyImage vkCmdCopyImageToBuffer vkCmdCopyBufferToImage vkCmdResolveImage vkCmdBlitImage | InvalidImageViewAspect | NA |
| Image Aspect Mismatch | Verify that Image commands with source and dest images use matching aspect flags | MISMATCHED_IMAGE_ASPECT | vkCmdCopyImage | TBD | NA |
| Image Type Mismatch | Verify that Image commands with source and dest images use matching types | MISMATCHED_IMAGE_TYPE | vkCmdCopyImage vkCmdResolveImage | CopyImageTypeMismatch ResolveImageTypeMismatch | NA |
| Image Format Mismatch | Verify that Image commands with source and dest images use matching formats | MISMATCHED_IMAGE_FORMAT | vkCmdCopyImage vkCmdResolveImage | CopyImageDepthStencilFormatMismatch ResolveImageFormatMismatch | NA |
| Resolve Sample Count | Verifies that source and dest images sample counts are valid | INVALID_RESOLVE_SAMPLES | vkCmdResolveImage | ResolveImageHighSampleCount ResolveImageLowSampleCount | NA |
| Verify Format | Verifies the formats are valid for this image operation | INVALID_FORMAT | vkCreateImageView vkCmdBlitImage | TBD | NA |
| Verify Correct Image Filter| Verifies that specified filter is valid | INVALID_FILTER | vkCmdBlitImage | TBD | NA |
| Verify Image Format Limits | Verifies that image creation parameters are with the device format limits | INVALID_FORMAT_LIMITS_VIOLATION | vkCreateImage | TBD | NA |
| NA | Enum used for informational messages | NONE | | NA | None |

### Image Pending Work
Additional work to be done

## MemTracker

### MemTracker Overview

The MemTracker layer tracks memory objects and references and validates that they are managed correctly by the application.  This includes tracking object bindings, memory hazards, and memory object lifetimes. MemTracker validates several other hazard-related issues related to command buffers, fences, and memory mapping.

### MemTracker Details Table

| Check | Overview | ENUM MEMTRACK_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Valid Command Buffer | Verifies that the command buffer was properly created and is currently valid | INVALID_CB | vkCmdBindPipeline vkCmdSetViewport vkCmdSetLineWidth vkCmdSetDepthBias vkCmdSetBlendConstants vkCmdSetDepthBounds vkCmdSetStencilCompareMask vkCmdSetStencilWriteMask vkCmdSetStencilReference vkBeginCommandBuffer vkResetCommandBuffer vkDestroyDevice vkFreeMemory | NA | NA |
| Valid Memory Object | Verifies that the memory object was properly created and is currently valid | INVALID_MEM_OBJ | vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage vkFreeMemory vkBindBufferMemory vkBindImageMemory vkQueueBindSparse | NA | NA |
| Free Referenced Memory | Checks to see if memory being freed still has current references | FREED_MEM_REF | vmFreeMemory | FreeBoundMemory | NA |
| Memory Properly Bound | Validate that the memory object referenced in the call was properly created, is currently valid, and is properly bound to the object | MISSING_MEM_BINDINGS | vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyQueryPoolResults vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage | NA | NA |
| Valid Object | Verifies that the specified Vulkan object was created properly and is currently valid | INVALID_OBJECT | vkCmdBindPipeline vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage | NA | NA |
| Bind Invalid Memory | Validate that memory object was correctly created, that the command buffer object was correctly created, and that both are currently valid objects. | MEMORY_BINDING_ERROR | vkQueueBindSparse vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdResolveImage | NA | The valid Object checks are primarily the responsibilty of ObjectTracker layer, so these checks are more of a backup in case ObjectTracker is not enabled |
| Objects Not Destroyed | Verify all objects destroyed at DestroyDevice time | MEMORY_LEAK | vkDestroyDevice | NA | NA |
| Memory Mapping State | Verifies that mapped memory is CPU-visible | INVALID_STATE | vkMapMemory | MapMemWithoutHostVisibleBit | NA |
| Command Buffer Synchronization | Command Buffer must be complete before BeginCommandBuffer or ResetCommandBuffer can be called | RESET_CB_WHILE_IN_FLIGHT | vkBeginCommandBuffer vkResetCommandBuffer | CallBeginCommandBufferBeforeCompletion CallBeginCommandBufferBeforeCompletion | NA |
| Submitted Fence Status | Verifies that: The fence is not submitted in an already signaled state, that ResetFences is not called with a fence in an unsignaled state, and that fences being checked have been submitted | INVALID_FENCE_STATE | vkResetFences vkWaitForFences vkQueueSubmit vkGetFenceStatus | SubmitSignaledFence ResetUnsignaledFence | Create test(s) for case where an unsubmitted fence is having its status checked |
| Immutable Memory Binding | Validates that non-sparse memory bindings are immutable, so objects are not re-boundt | REBIND_OBJECT | vkBindBufferMemory, vkBindImageMemory | RebindMemory | NA |
| Image/Buffer Usage bits | Verify correct USAGE bits set based on how Images and Buffers are used | INVALID_USAGE_FLAG | vkCreateImage, vkCreateBuffer, vkCreateBufferView, vkCmdCopyBuffer, vkCmdCopyQueryPoolResults, vkCmdCopyImage, vkCmdBlitImage, vkCmdCopyBufferToImage, vkCmdCopyImageToBuffer, vkCmdUpdateBuffer, vkCmdFillBuffer  | InvalidUsageBits | NA |
| Objects Not Destroyed Warning | Warns if any memory objects have not been freed before their objects are destroyed | MEM_OBJ_CLEAR_EMPTY_BINDINGS | vkDestroyDevice | TBD | NA |
| Memory Map Range Checks | Validates that Memory Mapping Requests are valid for the Memory Object (in-range, not currently mapped on Map, currently mapped on UnMap, size is non-zero) | INVALID_MAP | vkMapMemory | TBD | NA |
| NA | Enum used for informational messages | NONE | | NA | None |
| NA | Enum used for errors in the layer itself. This does not indicate an app issue, but instead a bug in the layer. | INTERNAL_ERROR | | NA | None |

### MemTracker Pending Work

#### MemTracker Enhancements

1.  Flag any memory hazards: Validate that the pipeline barriers for buffers are sufficient to avoid hazards
2.  Make sure that the XGL_IMAGE_VIEW_ATTACH_INFO.layout matches the layout of the image as determined by the last IMAGE_MEMORY_BARRIER
3.  Verify that the XGL_IMAGE_MEMORY_BARRIER.oldLayout matches the actual previous layout (this one was super important for previous work in dealing with out-of-order command buffer generation). Note that these need to be tracked for each subresource.
4.  Update for new Memory Binding Model
5.  Consolidate error messages and make them consistent
7.  Add validation for having mapped objects in a command list - GPU writing to mapped object is warning
8.  Add validation for maximum memory references, maximum object counts, and object leaks
9. When performing clears on surfaces that have both Depth and Stencil, WARN user if subresource range for depth and stencil are not both set (see blit_tests.cpp VkCmdClearDepthStencilTest test).
10.  Re-enable INFO messages that were disabled during v138 integration
11. Warn on image/buffer deletion if USAGE bits were set that were not needed
12. Modify INVALID_FENCE_STATE to be WARNINGs instead of ERROR
13. Report destroy or modify of resources in use on queues and not cleared by fence or WaitIdle. Could be fence, semaphore, or objects used by submitted CommandBuffers.


## ShaderChecker

### ShaderChecker Overview

The ShaderChecker layer inspects the SPIR-V shader images and fixed function pipeline stages at PSO creation time.
It flags errors when inconsistencies are found across interfaces between shader stages. The exact behavior of the checks
depends on the pair of pipeline stages involved.

### ShaderChecker Details Table

| Check | Overview | ENUM SHADER_CHECKER_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Not consumed | Flag warning if a location is not consumed (useless work) | OUTPUT_NOT_CONSUMED | vkCreateGraphicsPipelines | CreatePipeline*NotConsumed | NA |
| Not produced | Flag error if a location is not produced (consumer reads garbage) | INPUT_NOT_PRODUCED | vkCreateGraphicsPipelines | CreatePipeline*NotProvided | NA |
| Type mismatch | Flag error if a location has inconsistent types | INTERFACE_TYPE_MISMATCH | vkCreateGraphicsPipelines | CreatePipeline*TypeMismatch | Between shader stages, an exact structural type match is required. Between VI and VS, or between FS and CB, only the basic component type must match (float for UNORM/SNORM/FLOAT, int for SINT, uint for UINT) as the VI and CB stages perform conversions to the exact format. |
| Inconsistent shader | Flag error if an inconsistent SPIR-V image is detected. Possible cases include broken type definitions which the layer fails to walk. | INCONSISTENT_SPIRV | vkCreateGraphicsPipelines | TODO | All current tests use the reference compiler to produce valid SPIRV images from GLSL. |
| Non-SPIRV shader | Flag warning if a non-SPIR-V shader image is detected. This can occur if early drivers are ingesting GLSL. ShaderChecker cannot analyze non-SPIRV shaders, so this suppresses most other checks. | NON_SPIRV_SHADER | vkCreateGraphicsPipelines | TODO | NA |
| FS mixed broadcast | Flag error if the fragment shader writes both the legacy gl_FragCoord (which broadcasts to all CBs) and custom FS outputs. | FS_MIXED_BROADCAST | vkCreateGraphicsPipelines | TODO | Reference compiler refuses to compile shaders which do this |
| VI Binding Descriptions | Validate that there is a single vertex input binding description for each binding | INCONSISTENT_VI | vkCreateGraphicsPipelines | CreatePipelineAttribBindingConflict | NA |
| Shader Stage Check | Warns if shader stage is unsupported | UNKNOWN_STAGE | vkCreateGraphicsPipelines | TBD | NA |
| Missing Descriptor | Flags error if shader attempts to use a descriptor binding not declared in the layout | MISSING_DESCRIPTOR | vkCreateGraphicsPipelines | CreatePipelineUniformBlockNotProvided | NA |
| NA | Enum used for informational messages | NONE | | NA | None |

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
| Valid Object | Validates that referenced object was properly created and is currently valid. | INVALID_OBJECT | vkAcquireNextImageKHR vkAllocateDescriptorSets vkAllocateMemory vkBeginCommandBuffer vkBindBufferMemory vkBindImageMemory vkCmdBeginQuery vkCmdBeginRenderPass vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindPipeline vkCmdBindVertexBuffers vkCmdBlitImage vkCmdClearAttachments vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdCopyBuffer vkCmdCopyBufferToImage vkCmdCopyImage vkCmdCopyImageToBuffer vkCmdCopyQueryPoolResults vkCmdDispatch vkCmdDispatchIndirect vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndexedIndirect vkCmdDrawIndirect vkCmdEndQuery vkCmdEndRenderPass vkCmdExecuteCommands vkCmdFillBuffer vkCmdNextSubpass vkCmdPipelineBarrier vkCmdPushConstants vkCmdResetEvent vkCmdResetQueryPool vkCmdResolveImage vkCmdSetEvent vkCmdUpdateBuffer vkCmdWaitEvents vkCmdWriteTimestamp vkCreateBuffer vkCreateBufferView vkAllocateCommandBuffers vkCreateCommandPool vkCreateComputePipelines vkCreateDescriptorPool vkCreateDescriptorSetLayout vkCreateEvent vkCreateFence vkCreateFramebuffer vkCreateGraphicsPipelines vkCreateImage vkCreateImageView vkCreatePipelineCache vkCreatePipelineLayout vkCreateQueryPool vkCreateRenderPass vkCreateSampler vkCreateSemaphore vkCreateShaderModule vkCreateSwapchainKHR vkDestroyBuffer vkDestroyBufferView vkFreeCommandBuffers vkDestroyCommandPool vkDestroyDescriptorPool vkDestroyDescriptorSetLayout vkDestroyEvent vkDestroyFence vkDestroyFramebuffer vkDestroyImage vkDestroyImageView vkDestroyPipeline vkDestroyPipelineCache vkDestroyPipelineLayout vkDestroyQueryPool vkDestroyRenderPass vkDestroySampler vkDestroySemaphore vkDestroyShaderModule vkDestroySwapchainKHR vkDeviceWaitIdle vkEndCommandBuffer vkEnumeratePhysicalDevices vkFreeDescriptorSets vkFreeMemory vkFreeMemory vkGetBufferMemoryRequirements vkGetDeviceMemoryCommitment vkGetDeviceQueue vkGetEventStatus vkGetFenceStatus vkGetImageMemoryRequirements vkGetImageSparseMemoryRequirements vkGetImageSubresourceLayout vkGetPhysicalDeviceSurfaceSupportKHR vkGetPipelineCacheData vkGetQueryPoolResults vkGetRenderAreaGranularity vkInvalidateMappedMemoryRanges vkMapMemory vkMergePipelineCaches vkQueueBindSparse vkResetCommandBuffer vkResetCommandPool vkResetDescriptorPool vkResetEvent vkResetFences vkSetEvent vkUnmapMemory vkUpdateDescriptorSets vkWaitForFences | BindInvalidMemory BindMemoryToDestroyedObject | Every VkObject class of parameter will be run through this check. This check may ultimately supersede UNKNOWN_OBJECT |
| Object Cleanup | Verify that object properly destroyed | DESTROY_OBJECT_FAILED | vkDestroyInstance, vkDestroyDevice, vkFreeMemory | ? | NA |
| Objects Leak | When an Instance or Device object is destroyed, validates that all objects belonging to that device/instance have previously been destroyed | OBJECT_LEAK | vkDestroyDevice vkDestroyInstance | ? | NA |
| Object Count | Flag error if number of objects requested from extenstion functions exceeds max number of actual objects | OBJCOUNT_MAX_EXCEEDED | objTrackGetObjects objTrackGetObjectsOfType | ? | NA |
| Valid Destroy Object | Validates that an object pass into a destroy function was properly created and is currently valid | NONE | vkDestroyInstance vkDestroyDevice vkDestroyFence vkDestroySemaphore vkDestroyEvent vkDestroyQueryPool vkDestroyBuffer vkDestroyBufferView vkDestroyImage vkDestroyImageView vkDestroyShaderModule vkDestroyPipelineCache vkDestroyPipeline vkDestroyPipelineLayout vkDestroySampler vkDestroyDescriptorSetLayout vkDestroyDescriptorPool vkDestroyCommandPool vkFreeCommandBuffers vkDestroyFramebuffer vkDestroyRenderPass vkDestroySwapchainKHR | TBD | These cases need to be moved to a more appropriate error enum |
| Unknown object  | Internal layer errors when it attempts to update use count for an object that's not in its internal tracking datastructures. | UNKNOWN_OBJECT | | NA | This may be irrelevant due to INVALID_OBJECT error, need to look closely and merge this with that error as appropriate. |
| Correct Command Pool | Validates that command buffers in a FreeCommandBuffers call were all created in the specified commandPool | COMMAND_POOL_MISMATCH | vkFreeCommandBuffers | TBD | NA |
| Correct Descriptor Pool | Validates that descriptor sets in a FreeDescriptorSets call were all created in the specified descriptorPool | DESCRIPTOR_POOL_MISMATCH | vkFreeDescriptorSets | TBD | NA |
| NA | Enum used for informational messages | NONE | | NA | None |
| NA | Enum used for errors in the layer itself. This does not indicate an app issue, but instead a bug in the layer. | INTERNAL_ERROR | | NA | None |

### ObjectTracker Pending Work

 4. Verify images have CmdPipelineBarrier layouts matching new layout parameters to Cmd*Image* functions
 6. For specific object instances that are allowed to be NULL, update object validation to verify that such objects are either NULL or valid
 7. Verify cube array VkImageView objects use subresourceRange.arraySize (or effective arraySize when VK_REMAINING_ARRAY_SLICES is specified) that is a multiple of 6. 
 8. Make object maps specific to instance and device.  Objects may only be used with matching instance or device.
 9. Use reference counting for non-dispatchable objects.  Multiple object creation calls may return identical handles.
 10. Update codegen for destroy_obj & validate_obj to generate all of the correct signatures and use the generated code

## Threading

### Threading Overview

The Threading layer checks for simultaneous use of objects by calls from multiple threads.
Application code is responsible for preventing simultaneous use of the same objects by certain calls that modify objects.
See [bug 13433](https://cvs.khronos.org/bugzilla/show_bug.cgi?id=13433) and
<https://cvs.khronos.org/svn/repos/oglc/trunk/nextgen/vulkan/function_properties.csv>
for threading rules.
Objects that may need a mutex include VkQueue, VkDeviceMemory, VkObject, VkBuffer, VkImage, VkDescriptorSet, VkDescriptorPool, VkCommandBuffer, and VkSemaphore.
The most common case is that a VkCommandBuffer passed to VkCmd* calls must be used by only one thread at a time.

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

| Check | Overview | ENUM THREADING_CHECKER_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ---------------- | -------- | ---------- |
| Thread Collision | Detects and notifies user if multiple threads are modifying thes same object | MULTIPLE_THREADS | vkQueueSubmit vkFreeMemory vkMapMemory vkUnmapMemory vkFlushMappedMemoryRanges vkInvalidateMappedMemoryRanges vkBindBufferMemory vkBindImageMemory vkQueueBindSparse vkDestroySemaphore vkDestroyBuffer vkDestroyImage vkDestroyDescriptorPool vkResetDescriptorPool vkAllocateDescriptorSets vkFreeDescriptorSets vkFreeCommandBuffers vkBeginCommandBuffer vkEndCommandBuffer vkResetCommandBuffer vkCmdBindPipeline vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdClearAttachments vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdWaitEvents vkCmdPipelineBarrier vkCmdBeginQuery vkCmdEndQuery vkCmdResetQueryPool vkCmdWriteTimestamp vkCmdCopyQueryPoolResults vkCmdBeginRenderPass vkCmdNextSubpass vkCmdPushConstants vkCmdEndRenderPass vkCmdExecuteCommands | ??? | NA |
| Thread Reentrancy | Detects cases of a single thread calling Vulkan reentrantly | SINGLE_THREAD_REUSE | vkQueueSubmit vkFreeMemory vkMapMemory vkUnmapMemory vkFlushMappedMemoryRanges vkInvalidateMappedMemoryRanges vkBindBufferMemory vkBindImageMemory vkQueueBindSparse vkDestroySemaphore vkDestroyBuffer vkDestroyImage vkDestroyDescriptorPool vkResetDescriptorPool vkAllocateDescriptorSets vkFreeDescriptorSets vkFreeCommandBuffers vkBeginCommandBuffer vkEndCommandBuffer vkResetCommandBuffer vkCmdBindPipeline vkCmdSetViewport vkCmdSetBlendConstants vkCmdSetLineWidth vkCmdSetDepthBias vkCmdSetDepthBounds vkCmdSetStencilCompareMask vkCmdSetStencilWriteMask vkCmdSetStencilReference vkCmdBindDescriptorSets vkCmdBindIndexBuffer vkCmdBindVertexBuffers vkCmdDraw vkCmdDrawIndexed vkCmdDrawIndirect vkCmdDrawIndexedIndirect vkCmdDispatch vkCmdDispatchIndirect vkCmdCopyBuffer vkCmdCopyImage vkCmdBlitImage vkCmdCopyBufferToImage vkCmdCopyImageToBuffer vkCmdUpdateBuffer vkCmdFillBuffer vkCmdClearColorImage vkCmdClearDepthStencilImage vkCmdClearAttachments vkCmdResolveImage vkCmdSetEvent vkCmdResetEvent vkCmdWaitEvents vkCmdPipelineBarrier vkCmdBeginQuery vkCmdEndQuery vkCmdResetQueryPool vkCmdWriteTimestamp vkCmdCopyQueryPoolResults vkCmdBeginRenderPass vkCmdNextSubpass vkCmdPushConstants vkCmdEndRenderPass vkCmdExecuteCommands | ??? | NA |
| NA | Enum used for informational messages | NONE | | NA | None |

### Threading Pending Work
Additional work to be done

## Device Limitations

### Device Limitations Overview

This layer is a work in progress. DeviceLimits layer is intended to capture two broad categories of errors:
 1. Incorrect use of APIs to query device capabilities
 2. Attempt to use API functionality beyond the capability of the underlying device

For the first category, the layer tracks which calls are made and flags errors if calls are excluded that should not be, or if call sequencing is incorrect. An example is an app that assumes attempts to Query and use queues without ever having called vkGetPhysicalDeviceQueueFamilyProperties(). Also, if an app is calling vkGetPhysicalDeviceQueueFamilyProperties() to retrieve properties with some assumed count for array size instead of first calling vkGetPhysicalDeviceQueueFamilyProperties() w/ a NULL pQueueFamilyProperties parameter in order to query the actual count.
For the second category of errors, DeviceLimits stores its own internal record of underlying device capabilities and flags errors if requests are made beyond those limits. Most (all?) of the limits are queried via vkGetPhysicalDevice* calls.

### Device Limitations Details Table

| Check | Overview | ENUM DEVLIMITS_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ---------------- | -------- | ---------- |
| Valid instance | If an invalid instance is used, this error will be flagged | INVALID_INSTANCE | vkEnumeratePhysicalDevices | NA | ObjectTracker should also catch this so if we made sure ObjectTracker was always on top, we could avoid this check |
| Valid physical device | Enum used for informational messages | INVALID_PHYSICAL_DEVICE | vkEnumeratePhysicalDevices | NA | ObjectTracker should also catch this so if we made sure ObjectTracker was always on top, we could avoid this check |
| Querying array counts | For API calls where an array count should be queried with an initial call and a NULL array pointer, verify that such a call was made before making a call with non-null array pointer. | MUST_QUERY_COUNT | vkEnumeratePhysicalDevices vkGetPhysicalDeviceQueueFamilyProperties | NA | Create focused test |
| Array count value | For API calls where an array of details is queried, verify that the size of the requested array matches the size of the array supported by the device. | COUNT_MISMATCH | vkEnumeratePhysicalDevices vkGetPhysicalDeviceQueueFamilyProperties | NA | Create focused test |
| Queue Creation | When creating/requesting queues, make sure that QueueFamilyPropertiesIndex and index/count within that queue family are valid. | INVALID_QUEUE_CREATE_REQUEST | vkGetDeviceQueue vkCreateDevice | NA | Create focused test |
| Query Properties | Before creating an Image, warn if physical device properties have not been queried | MUST_QUERY_PROPERTIES | vkCreateImage | NA | Add validation test |
| API Call Sequencing | This is a general error indicating that an app did not use vkGetPhysicalDevice* and other such query calls, but rather made an assumption about device capabilities. | INVALID_CALL_SEQUENCE | vkCreateDevice | NA | Add validation test |
| Feature Request | Attempting to vkCreateDevice with a feature that is not supported by the underlying physical device. | INVALID_FEATURE_REQUESTED | vkCreateDevice | NA | Add validation test |
| Valid Image Extents | When creating an Image, ensure that image extents are within device limits for the specified format | LIMITS_VIOLATION | vkCreateImage | CreateImageLimitsViolationWidth | NA |
| Valid Image Resource Size | When creating an image, ensure the the total image resource size is less than the queried device maximum resource size  | LIMITS_VIOLATION | vkCreateImage | CreateImageResourceSizeViolation | NA |
| Alignment | When updating a buffer, data should be aligned on 4 byte boundaries  | LIMITS_VIOLATION | vkCmdUpdateBuffer | UpdateBufferAlignment | NA |
| Alignment | When filling a buffer, data should be aligned on 4 byte boundaries  | LIMITS_VIOLATION | vkCmdFillBuffer | UpdateBufferAlignment | NA |
| NA | Enum used for informational messages | NONE | | NA | None |

### Device Limitations Pending Work

 1. For all Formats, call vkGetPhysicalDeviceFormatProperties to pull their properties for the underlying device. After that point, if the app attempts to use any formats in violation of those properties, flag errors (this is done for Images).

## Swapchain

### Swapchain Overview

This layer is a work in progress. DeviceLimits layer is intended to capture two broad categories of errors:
 1. Incorrect use of APIs to query device capabilities
 2. Attempt to use API functionality beyond the capability of the underlying device

For the first category, the layer tracks which calls are made and flags errors if calls are excluded that should not be, or if call sequencing is incorrect. An example is an app that assumes attempts to Query and use queues without ever having called vkGetPhysicalDeviceQueueFamilyProperties(). Also, if an app is calling vkGetPhysicalDeviceQueueFamilyProperties() to retrieve properties with some assumed count for array size instead of first calling vkGetPhysicalDeviceQueueFamilyProperties() w/ a NULL pQueueFamilyProperties parameter in order to query the actual count.
For the second category of errors, DeviceLimits stores its own internal record of underlying device capabilities and flags errors if requests are made beyond those limits. Most (all?) of the limits are queried via vkGetPhysicalDevice* calls.

### Swapchain Details Table

| Check | Overview | ENUM SWAPCHAIN_* | Relevant API | Testname | Notes/TODO |
| ----- | -------- | ---------------- | ------------ | -------- | ---------- |
| Valid handle | If an invalid handle is used, this error will be flagged | INVALID_HANDLE | vkDestroyInstance vkEnumeratePhysicalDevices vkCreateDevice vkDestroyDevice vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfacePresentModesKHR vkCreateSwapchainKHR vkDestroySwapchainKHR vkGetSwapchainImagesKHR vkAcquireNextImageKHR vkQueuePresentKHR | NA | None |
| Extension enabled before use | Validates that a WSI extension is enabled before its functions are used | EXT_NOT_ENABLED_BUT_USED |  vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfacePresentModesKHR vkCreateSwapchainKHR vkDestroySwapchainKHR vkGetSwapchainImagesKHR vkAcquireNextImageKHR vkQueuePresentKHR | NA | None |
| Swapchains destroyed before devices | Validates that  vkDestroySwapchainKHR() is called for all swapchains associated with a device before vkDestroyDevice() is called | DEL_DEVICE_BEFORE_SWAPCHAINS | vkDestroyDevice | NA | None |
| Queries occur before swapchain creation | Validates that vkGetPhysicalDeviceSurfaceCapabilitiesKHR(), vkGetPhysicalDeviceSurfaceFormatsKHR() and vkGetPhysicalDeviceSurfacePresentModesKHR() are called before vkCreateSwapchainKHR() | CREATE_SWAP_WITHOUT_QUERY | vkCreateSwapchainKHR | NA | None |
| vkCreateSwapchainKHR(pCreateInfo->minImageCount) | Validates vkCreateSwapchainKHR(pCreateInfo->minImageCount) | CREATE_SWAP_BAD_MIN_IMG_COUNT | vkCreateSwapchainKHR | NA | None |
| vkCreateSwapchainKHR(pCreateInfo->imageExtent) | Validates vkCreateSwapchainKHR(pCreateInfo->imageExtent) when window has no fixed size | CREATE_SWAP_OUT_OF_BOUNDS_EXTENTS | vkCreateSwapchainKHR | NA | None |
| vkCreateSwapchainKHR(pCreateInfo->imageExtent) | Validates vkCreateSwapchainKHR(pCreateInfo->imageExtent) when window has a fixed size | CREATE_SWAP_EXTENTS_NO_MATCH_WIN | vkCreateSwapchainKHR | NA | None |
| vkCreateSwapchainKHR(pCreateInfo->preTransform) | Validates vkCreateSwapchainKHR(pCreateInfo->preTransform) | CREATE_SWAP_BAD_PRE_TRANSFORM | vkCreateSwapchainKHR | NA | None |
| vkCreateSwapchainKHR(pCreateInfo->imageArraySize) | Validates vkCreateSwapchainKHR(pCreateInfo->imageArraySize) | CREATE_SWAP_BAD_IMG_ARRAY_SIZE | vkCreateSwapchainKHR | NA | None |
| vkCreateSwapchainKHR(pCreateInfo->imageUsageFlags) | Validates vkCreateSwapchainKHR(pCreateInfo->imageUsageFlags) | CREATE_SWAP_BAD_IMG_USAGE_FLAGS | vkCreateSwapchainKHR | NA | None |
| vkCreateSwapchainKHR(pCreateInfo->imageColorSpace) | Validates vkCreateSwapchainKHR(pCreateInfo->imageColorSpace) | CREATE_SWAP_BAD_IMG_COLOR_SPACE | vkCreateSwapchainKHR | NA | None |
| vkCreateSwapchainKHR(pCreateInfo->imageFormat) | Validates vkCreateSwapchainKHR(pCreateInfo->imageFormat) | CREATE_SWAP_BAD_IMG_FORMAT | vkCreateSwapchainKHR | NA | None |
| vkCreateSwapchainKHR(pCreateInfo->imageFormat and pCreateInfo->imageColorSpace) | Validates vkCreateSwapchainKHR(pCreateInfo->imageFormat and pCreateInfo->imageColorSpace) | CREATE_SWAP_BAD_IMG_FMT_CLR_SP | vkCreateSwapchainKHR | NA | None |
| vkCreateSwapchainKHR(pCreateInfo->presentMode) | Validates vkCreateSwapchainKHR(pCreateInfo->presentMode) | CREATE_SWAP_BAD_PRESENT_MODE | vkCreateSwapchainKHR | NA | None |
| Use same device for swapchain | Validates that vkDestroySwapchainKHR() called with the same VkDevice as vkCreateSwapchainKHR() | DESTROY_SWAP_DIFF_DEVICE | vkDestroySwapchainKHR | NA | None |
| Don't use too many images | Validates that app never tries to own too many swapchain images at a time | APP_OWNS_TOO_MANY_IMAGES | vkAcquireNextImageKHR | NA | None |
| Index too large | Validates that an image index is within the number of images in a swapchain | INDEX_TOO_LARGE | vkQueuePresentKHR | NA | None |
| Can't present a non-owned image | Validates that application only presents images that it owns | INDEX_NOT_IN_USE | vkQueuePresentKHR | NA | None |

### Swapchain Pending Work
Additional checks to be added to Swapchain

 1. Check that the queue used for presenting was checked/valid during vkGetPhysicalDeviceSurfaceSupportKHR.
 2. One issue that has already come up is correct UsageFlags for WSI SwapChains and SurfaceProperties.
 3. Tons of other stuff including semaphore and synchronization validation.

# Non-validation Layer Details

## APIDump

APIDump layer is used for dumping a stream of all the Vulkan API calls that are made, along with details of the parameters to those calls.

### APIDump Pending Work

 1. vkAllocateDescriptorSets does not correctly print out all of the created DescriptorSets (no array printing following main API txt)


## General Pending Work
A place to capture general validation work to be done. This includes new checks that don't clearly fit into the above layers.

 1. For Upcoming Dynamic State overhaul (if approved): If dynamic state value that is consumed is never set prior to consumption, flag an error
 2. For Upcoming Dynamic State overhaul (if approved): If dynamic state that was bound as "static" in current PSO is attempted to be set with vkCmdSet* flag an error
 3. Need to check VkShaderCreateInfo.stage is being set properly (Issue reported by Dan Ginsberg)
