# Create a Descriptor Set

<link href="../css/lg_stylesheet.css" rel="stylesheet"></link>

Code file for this section is `09-init_descriptor_set.cpp`

Back in the pipeline_layout sample, you defined the descriptor set
layout, but didn't actually allocate it.
Recall that the descriptor set that you defined is used to inform the
GPU how the data contained in the uniform buffer is mapped to
the shader program's uniform variables.
Now you can proceed with allocating and initializing the descriptor set.

## Descriptor Pool

Like command buffers, descriptor sets are allocated from a pool.
So you must first create the pool.
Since you know that you need only one descriptor set for a uniform
buffer, creating the pool is straightforward:

    VkDescriptorPoolSize type_count[1];
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count[0].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = NULL;
    descriptor_pool.maxSets = 1;
    descriptor_pool.poolSizeCount = 1;
    descriptor_pool.pPoolSizes = type_count;

    res = vkCreateDescriptorPool(info.device, &descriptor_pool, NULL,
                                 &info.desc_pool);

## Allocate a Descriptor Set from the Pool

Now you can allocate a descriptor set from the pool.
Note that you have to provide the descriptor set layout that you defined
in the pipeline_layout sample.
This layout describes how the descriptor set is to be allocated.

    VkDescriptorSetAllocateInfo alloc_info[1];
    alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info[0].pNext = NULL;
    alloc_info[0].descriptorPool = info.desc_pool;
    alloc_info[0].descriptorSetCount = NUM_DESCRIPTOR_SETS;
    alloc_info[0].pSetLayouts = info.desc_layout.data();
    info.desc_set.resize(NUM_DESCRIPTOR_SETS);
    res = vkAllocateDescriptorSets(info.device, alloc_info, info.desc_set.data());

## Update the Descriptor Set

Note that you have not actually used the handle for the uniform buffer anywhere yet.
You stashed away the uniform buffer's information in
a `VkDescriptorBufferInfo` structure named `info.uniform_data.buffer_info` when
you created the uniform buffer.
See the function `init_uniform_buffer()` to see how `info.uniform_data.buffer_info`
is initialized.

`info.uniform_data.buffer_info` is an instance of:

    typedef struct VkDescriptorBufferInfo {
        VkBuffer        buffer;
        VkDeviceSize    offset;
        VkDeviceSize    range;
    } VkDescriptorBufferInfo;

where the `buffer` member contains the uniform buffer handle.

    VkWriteDescriptorSet writes[1];
    writes[0] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pNext = NULL;
    writes[0].dstSet = info.desc_set[0];
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &info.uniform_data.buffer_info;
    writes[0].dstArrayElement = 0;
    writes[0].dstBinding = 0;

    vkUpdateDescriptorSets(info.device, 1, writes, 0, NULL);

The above steps essentially copy the `VkDescriptorBufferInfo` to the descriptor, which
is likely in the device memory.

This buffer info includes the handle to the uniform buffer as well as the offset and size
of the data that is accessed in the uniform buffer.
In this case, the uniform buffer contains only the MVP transform, so the offset is 0
and the size is the size of the MVP, as setup by the pipeline_layout sample
in `info.uniform_data.buffer_info`.

The exact byte-wise layout of a descriptor is probably implementation-specific
and so is opaque to you.
This is why you use Vulkan driver functions to manipulate descriptors, instead of
mapping and writing them yourself.

<table border="1" width="100%">
    <tr>
        <td align="center" width="33%"><a href="08-init_pipeline_layout.html" title="Prev">Pipeline Layouts</a></td>
        <td align="center" width="33%"><a href="index.html" title="Index">Index</a></td>
        <td align="center" width="33%"><a href="10-init_render_pass.html" title="Next">Render Pass</a></td>
    </tr>
</table>
<footer>&copy; Copyright 2016 LunarG, Inc</footer>
