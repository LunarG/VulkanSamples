# Create the Framebuffers

<link href="../css/lg_stylesheet.css" rel="stylesheet"></link>

Code file for this section is `12-init_frame_buffers.cpp`

## The Vulkan Framebuffer

Framebuffers represent a collection of memory attachments that are used
by a render pass instance.
Examples of these memory attachments include the color image buffers and
depth buffer that we created in previous samples.
A framebuffer provides the attachments that a render pass needs while rendering.

In the previous swapchain sample, you created a swapchain, which provided
a color image attachment for each "frame" in the swapchain.
You also created a depth image attachment in the depthbuffer sample that is
to be reused with each color image attachment for each frame.

In the render_pass example, you created a render pass that
described the nature of the attachments, but didn't actually connect
the actual resources (images) to the render pass.
The framebuffer essentially associates the actual attachments to the renderpass.

For example, if the swapchain has two frames in it, you would need to create
two framebuffers.
The first framebuffer consists of the first color image buffer,
which you obtained in the swapchain example using `vkGetSwapchainImagesKHR()`.
It also contains the single depth buffer you created in the depthbuffer example.
Then the second framebuffer contains the second color image buffer and the
same depth buffer used by the first framebuffer.

![Framebuffers](../images/FrameBuffers.png)

Looking at the code, you can start with an array of two attachments and
pre-initialize the second one to the depth buffer, since it will be used
for all framebuffers.
You will fill in the first attachment later.
Note the use of the "view" object to reference this image, since the view
contains the additional metadata needed to describe the format and usage
of the buffer.

    VkImageView attachments[2];
    attachments[1] = info.depth.view;

Next, fill in the create info structure:

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = info.render_pass;
    fb_info.attachmentCount = 2;
    fb_info.pAttachments = attachments;
    fb_info.width = info.width;
    fb_info.height = info.height;
    fb_info.layers = 1;

The sample is written to support the number of swapchain buffers determined at
run time, so dynamically allocate an array of handles for framebuffers.

    info.framebuffers = (VkFramebuffer *)malloc(info.swapchainImageCount *
                                                sizeof(VkFramebuffer));

For each framebuffer, set the first attachment to each respective color image buffer in the swapchain,
but leave the second attachment set to the same depth buffer, so that it is shared by all framebuffers.

    for (int i = 0; i < info.swapchainImageCount; i++) {
        attachments[0] = info.buffers[i].view;
        res = vkCreateFramebuffer(info.device, &fb_info, NULL, &info.framebuffers[i]);
    }

Note that the same render pass you defined in the renderpass sample is associated
with each framebuffer as well.

<table border="1" width="100%">
    <tr>
        <td align="center" width="33%"><a href="11-init_shaders.html" title="Prev">Shaders</a></td>
        <td align="center" width="33%"><a href="index.html" title="Index">Index</a></td>
        <td align="center" width="33%"><a href="13-init_vertex_buffer.html" title="Next">Vertex Buffer</a></td>
    </tr>
</table>
<footer>&copy; Copyright 2016 LunarG, Inc</footer>
