# Create a Render Pass

<link href="../css/lg_stylesheet.css" rel="stylesheet"></link>

Code file for this section is `10-init_render_pass.cpp`

A render pass describes the scope of a rendering operation by
specifying the collection of attachments, subpasses, and dependencies
used during the rendering operation.
The communication of this information to the driver allows the
driver to know what to expect when rendering begins and to set
up the hardware optimally for the rendering operation.

You begin by using `vkCreateRenderPass()` to define the render pass,
and then later insert a render pass instance into a command buffer
using `vkCmdBeginRenderPass()` and `vkCmdEndRenderPass()`.

In this section, you will only be creating and defining the render pass
and not using it in a command buffer until later.

Examples of render pass attachments in the context of this sample include
the color attachment, which is the image from the swapchain,
and the depth/stencil attachment, which is the depth buffer
you allocated in a previous sample.

Image attachments must be prepared for use before
being used as attachments in a render pass instance
in an executed command buffer.
This preparation involves transitioning image layouts from their
initial undefined states to states that are optimal for their
use in the render pass.
Since these layout transitions are rather involved, you
will learn about them here, before continuing on with creating the
render pass.

## Image Layout Transition

### The Need for Alternate Memory Access Patterns

The layout of an image refers to how image texels are mapped from
a grid coordinate representation to an offset in the image memory.
Typically, image data is mapped in this way linearly, which for a 2D
image might mean that a row of texels is stored in contiguous memory,
with the next row stored contiguously after that row, and so on.

Put another way:

    offset = rowCoord * pitch + colCoord

where pitch is the size of a row.  The pitch is often the same as the
width of the image, but may include some additional bytes of padding
to ensure that each row of the image begins at a memory address
that meets the alignment requirements of the GPU.

A linear layout is fine for successive texel read or writes along a
single row, by changing the `colCoord`.
But most graphics operations involve accessing texels across
multiple adjacent rows as well, by changing the `rowCoord`.
If the image width is fairly wide, then these adjacent row accesses
introduce rather large jumps across the linear memory address space.
This can cause performance problems such as slower memory address
translation due to TLB misses and cache misses in multilevel
cache memory systems.

To combat these inefficiencies, many GPU hardware implementations
support an "optimal" or tiled memory access scheme.
In an optimal layout, a rectangle of texels appearing in the
middle of an image is stored in memory such that all the
texels are in one continuous stretch of memory.
For example, the texels that make up a rectangle with the
upper-left corner at [16,32] and lower-right corner at [31,47]
might see this 16 x 16 block of texel stored contiguously
starting at one address.  There are no long gaps between rows.

If the GPU wanted to fill this block, with a solid color for example,
it is able to write to this 256 texel block with a low amount of
memory system overhead.

Here's an example of a simple 2x2 tiling scheme.  Note that the blue texels
can be far apart from each other in the linear scheme, while they are adjacent in
the tiled scheme.

![Image Memory Layout](../images/ImageMemoryLayout.png)

Most implementations use a more complex tiling pattern and tile sizes that are
larger than 2x2.

#### Vulkan Control Over the Layout

GPU hardware that has the ability to use optimal image layouts
also has controls for deciding between using linear and optimal
layouts.
Optimal layouts are often "opaque", which means that the details
of the optimal layout format are not published or made known
to other components that need to read or write image data.
So it is often necessary to convert optimal to linear in order
to allow the CPU to map the data so that the application can
read and process it in the expected and well-understood
linear layout.

Conversely, it may be desirable for the CPU to copy an image
into a buffer with linear layout and then convert it to
optimal layout so that the GPU can read it more efficiently.

Conversions from one layout to another are called layout transitions
in Vulkan.
You have control over these transitions and can invoke them
with an image memory barrier command or as a subpass dependency in a render
pass.
In this case, the sample code uses image memory barrier commands, which
are detailed later in this section.

A layout transition may or may not trigger an actual GPU layout
conversion operation.
For example, if the old layout is undefined and the new layout
is optimal, the GPU would have to do no work other than perhaps
program the GPU hardware to access memory in the optimal pattern.
This is because the contents of the image is undefined and does
not need to be preserved by converting it.
On the other hand, if the old layout was linear and there is
an indication that the data in the image needs to be preserved,
the transition probably involves some work by the GPU to
shuffle the texels around.

Even if you know or think that a layout transition won't actually do any work,
it is always best practice to do them anyway, since it gives the driver
more information.

#### Image Layout Transitions in the Samples

Look for a call to a function named `set_image_layout()` in the sample
near the beginning of the program.
This is a helper function that generates a `vkCmdPipelineBarrier`
command and puts it into a command buffer.
This helper function is designed to make it easy to transition
image layouts in the samples.

    set_image_layout(info, info.buffers[info.current_buffer].image,
                     VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

In this case, it is changing the image layouts from
`VK_IMAGE_LAYOUT_UNDEFINED`
to
`VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`.

The old layouts are clearly undefined because the images were
recently created by `vkCreateSwapchainKHR()`
back in the swapchain sample and so are in an undefined state.

The new layout of `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`
is the layout needed for color buffers
that are defined in the render pass as color attachments.

The depth buffer layout also needs to be transitioned from
`VK_IMAGE_LAYOUT_UNDEFINED`
to
`VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL`

Transitioning the depth buffer is accomplished in basically the same way as for the color buffer
except that it is handled in the `init_depth_buffer()` helper function.

Later in the sample, you can find where we close the command buffer
by calling `execute_end_command_buffer()`.
You don't actually send these commands to the GPU in this sample, but
you will use these commands in a later sample.

As mentioned previously, it is unlikely that these commands will cause
the GPU to do any work.
But at least the GPU is now programmed to use the color and depth
buffers in the optimum way.

### Create the Render Pass

Now that you know how to get the image layouts into the correct state,
you can proceed with defining the render pass.

#### Attachments

As mentioned at the top of this section, there are two attachments:

    VkAttachmentDescription attachments[2];
    attachments[0].format = info.format;
    attachments[0].samples = NUM_SAMPLES;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].flags = 0;

    attachments[1].format = info.depth.format;
    attachments[1].samples = NUM_SAMPLES;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].flags = 0;

Setting the `loadOp` member to CLEAR in both attachments indicates that you want the buffer to be
cleared at the start of the render pass instance.

Setting the `storeOp` member to STORE for the color attachment means that you
want to leave the rendering result in this buffer, so it can be presented to
the display.

Setting the `storeOp` member to DONT_CARE for the depth attachment means that
you don't need the contents of the buffer when the render pass instance is complete.
Telling the driver that you don't care about the contents of a buffer after it is used
can be useful because it allows the driver to
discard or page out that memory if it needed to without saving the contents.

For image layouts, you created a command buffer earlier in this section that
transitions the image layouts to their optimum layouts.
Therefore, you need to set the initial layouts in both attachments to optimal
in order to match the result of the layout transitions.

For the color attachment, you specify the final layout to be the
`VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` layout, which is the one that is
appropriate for the present operation that occurs after the render pass is complete.
This avoids needing to build up and submit a command in the command buffer to
explicitly perform the layout transition.
It also tells the GPU that it needs to perform an image layout transition at the end
of the render pass.

The depth buffer image final layout can remain the same as the initial layout,
since it is not used after the render pass is complete.

#### Subpass

The subpass definition is straightforward and would be more interesting
if you were doing multiple subpasses.
And you might be interested in doing subpasses if you were doing
some pre-processing or post-processing of your graphics data, perhaps
for ambient occlusion or some other effect.
But here, the subpass definition is useful for indicating which attachments
are active during the subpass.

    VkAttachmentReference color_reference = {};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_reference = {};
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

The `attachment` member is the index of the attachment in the
attachments array you just defined above.

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pResolveAttachments = NULL;
    subpass.pDepthStencilAttachment = &depth_reference;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

The `pipelineBindPoint` member is meant to indicate if this is a graphics
or a compute subpass.
Currently, only the graphics subpass is valid.

#### Render Pass

Now you have all you need to define the render pass:

    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = NULL;
    rp_info.attachmentCount = 2;
    rp_info.pAttachments = attachments;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 0;
    rp_info.pDependencies = NULL;
    res = vkCreateRenderPass(info.device, &rp_info, NULL, &info.render_pass);

You'll be using the render pass in several of the upcoming samples.

Note that you have done two things here:

1. Created a command buffer with layout transition commands in it and sent it to the GPU
1. Created (defined) a render pass

Sending the command buffer with the layout transition commands in this particular sample
really does not accomplish anything, since nothing is sent to the GPU afterwards.
And the render pass you defined is not used for anything either.
These are all just pieces and parts that you will put together shortly to draw the cube
in the final sample.

Looking ahead a bit, these pieces might fit together more like this:

1. Create a render pass as you just did here
1. Create a command buffer and "open" it to start receiving commands
1. Insert image layout transition commands in the command buffer
1. Insert a begin render pass command (vkCmdBeginRenderPass) in the command buffer
1. Insert other commands to finish the drawing - coming in the next sections

The important point is the the vkCmdPipelineBarrier commands used to perform the image layout transitions
need to be in the command buffer before the vkCmdBeginRenderPass, since the render pass is expecting
the images to be in the specific image layouts before the render pass begins.

<table border="1" width="100%">
    <tr>
        <td align="center" width="33%"><a href="09-init_descriptor_set.html" title="Prev">Descriptor Set</a></td>
        <td align="center" width="33%"><a href="index.html" title="Index">Index</a></td>
        <td align="center" width="33%"><a href="11-init_shaders.html" title="Next">Shaders</a></td>
    </tr>
</table>
<footer>&copy; Copyright 2016 LunarG, Inc</footer>
