# Create a Uniform Buffer

<link href="../css/lg_stylesheet.css" rel="stylesheet"></link>

Code file for this section is `07-init_uniform_buffer.cpp`

Since you created buffers in the recent samples, you might
as well work on another one now.

A uniform buffer is a buffer that is made accessible
in a read-only fashion to shaders so that the shaders
can read constant parameter data.

This is another example of a step that you have to perform in
a Vulkan program that you wouldn't have to do in another graphics API.
In GLES, you would simply make an API call to set the contents of
uniform variables that are sent to the shader.
Here, you have to allocate the memory and fill it in.

## Setting up the Uniform Data

The sample uses the uniform buffer to pass the MVP (Model-View-Projection)
matrix to the shader so that the shader can use it to transform each
vertex.

The sample sets it up like this:

    info.Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    info.View = glm::lookAt(
        glm::vec3(-5, 3, -10), // Camera is at (-5,3,-10), in World Space
        glm::vec3(0, 0, 0),    // and looks at the origin
        glm::vec3(0, -1, 0)    // Head is up (set to 0,-1,0 to look upside-down)
        );
    info.Model = glm::mat4(1.0f);
    // Vulkan clip space has inverted Y and half Z.
    info.Clip = glm::mat4(1.0f,  0.0f, 0.0f, 0.0f,
                          0.0f, -1.0f, 0.0f, 0.0f,
                          0.0f,  0.0f, 0.5f, 0.0f,
                          0.0f,  0.0f, 0.5f, 1.0f);

    info.MVP = info.Clip * info.Projection * info.View * info.Model;

Note that the `glm` library is used here to simplify the code.
`info.MVP` is a 4x4 matrix.

## Creating the Uniform Buffer Object

Creating this buffer is fairly similar to how you created the
depth buffer in a previous sample, with just a change to the usage:

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = sizeof(info.MVP);
    buf_info.queueFamilyIndexCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    res = vkCreateBuffer(info.device, &buf_info, NULL, &info.uniform_data.buf);
    assert(res == VK_SUCCESS);

## Allocating the Uniform Buffer Memory

Like the depth buffer, you need to explicitly allocate the memory for the
uniform buffer.

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(info.device, info.uniform_data.buf,
                                  &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    pass = memory_type_from_properties(info, mem_reqs.memoryTypeBits,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &alloc_info.memoryTypeIndex);

    res = vkAllocateMemory(info.device, &alloc_info, NULL,
                           &(info.uniform_data.mem));

The VK\_MEMORY\_PROPERTY\_HOST\_VISIBLE_BIT communicates that the memory
should be mapped so that the CPU (host) can access it.

The VK\_MEMORY\_PROPERTY\_HOST\_COHERENT_BIT requests that the writes
to the memory by the host are visible to the device (and vice-versa)
without the need to flush memory caches.
This just makes it a bit simpler to program, since it isn't
necessary to call `vkFlushMappedMemoryRanges` and `vkInvalidateMappedMemoryRanges`
to make sure that the data is visible to the GPU.

## Mapping and Setting the Uniform Buffer Memory

You didn't need to initialize the contents of the depth buffer memory when
you allocated it in a previous sample.
This is because the GPU takes care of reading and writing it.
But with the uniform buffer, you need to populate it with the data
that you want the shader to read.
In this case, the data is the MVP matrix.
In order to get CPU access to the memory, you need to map it:

    res = vkMapMemory(info.device, info.uniform_data.mem, 0, mem_reqs.size, 0,
                      (void **)&pData);

It is then a pretty simple matter to copy the MVP into the uniform buffer
and then unmap it:

    memcpy(pData, &info.MVP, sizeof(info.MVP));

    vkUnmapMemory(info.device, info.uniform_data.mem);

You want to unmap it fairly immediately because the memory mapping
mechanisms such as page tables have limited size, especially for memory
that is visible to both the CPU and GPU.

Finally, you associate the memory you just allocated with the buffer object:

    res = vkBindBufferMemory(info.device, info.uniform_data.buf,
                             info.uniform_data.mem, 0);

And you are done.

<table border="1" width="100%">
    <tr>
        <td align="center" width="33%">Previous: <a href="06-init_depth_buffer.html" title="Prev">Depth Buffer</a></td>
        <td align="center" width="33%">Back to: <a href="index.html" title="Index">Index</a></td>
        <td align="center" width="33%">Next: <a href="08-init_pipeline_layout.html" title="Next">Pipeline Layouts</a></td>
    </tr>
</table>
<footer>&copy; Copyright 2016 LunarG, Inc</footer>
