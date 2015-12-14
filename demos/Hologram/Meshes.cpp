#include <cassert>
#include <cstring>
#include <random>

#include "Helpers.h"
#include "Meshes.h"

class MeshGenerator {
public:
    MeshGenerator(unsigned int rng_seed, int count) :
        count_(count), current_(-1), generator_(rng_seed), distribution_(0.0f, 1.0f)
    {
    }

    void head()
    {
        current_ = -1;
    }

    bool next()
    {
        if (current_ >= count_ - 1)
            return false;

        current_++;

        return true;
    }

    uint32_t vertex_count()
    {
        return 5;
    }

    uint32_t vertex_size()
    {
        // vec3 + vec4
        return sizeof(float) * 7;
    }

    VkDeviceSize write_vertices(void *data, VkDeviceSize size)
    {
        float vb_data[5][7] = {
            //        pos                      color
            {  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f, 0.5f },
            { -1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.5f },
            {  1.0f, -1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.5f },
            {  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.5f },
            { -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  0.0f, 0.5f },
        };

        for (int i = 0; i < 5; i++) {
            vb_data[i][3] = distribution_(generator_);
            vb_data[i][4] = distribution_(generator_);
            vb_data[i][5] = distribution_(generator_);
            vb_data[i][6] = distribution_(generator_);
        }

        assert(sizeof(vb_data) <= size);
        std::memcpy(data, vb_data, sizeof(vb_data));
        return sizeof(vb_data);
    }

    uint32_t index_count()
    {
        // six triangles
        return 3 * 6;
    }

    VkDeviceSize index_size()
    {
        return sizeof(uint32_t);
    }

    VkIndexType index_type()
    {
        return VK_INDEX_TYPE_UINT32;
    }

    VkDeviceSize write_indices(void *data, VkDeviceSize size)
    {
        static const uint32_t ib_data[18] = {
            0, 1, 2,
            0, 2, 3,
            0, 3, 4,
            0, 4, 1,
            1, 4, 3,
            1, 3, 2,
        };

        assert(sizeof(ib_data) <= size);
        std::memcpy(data, ib_data, sizeof(ib_data));
        return sizeof(ib_data);
    }

    std::vector<VkVertexInputBindingDescription> vertex_input_bindings()
    {
        VkVertexInputBindingDescription vi_binding = {};
        vi_binding.binding = 0;
        vi_binding.stride = vertex_size();
        vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::vector<VkVertexInputBindingDescription>(1, vi_binding);
    }

    std::vector<VkVertexInputAttributeDescription> vertex_input_attributes()
    {
        VkVertexInputAttributeDescription vi_attrs[2] = {};
        vi_attrs[0].location = 0;
        vi_attrs[0].binding = 0;
        vi_attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vi_attrs[0].offset = 0;
        vi_attrs[1].location = 1;
        vi_attrs[1].binding = 0;
        vi_attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        vi_attrs[1].offset = sizeof(float) * 3;

        return std::vector<VkVertexInputAttributeDescription>(vi_attrs, vi_attrs + 2);
    }

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state()
    {
        VkPipelineInputAssemblyStateCreateInfo ia_info = {};
        ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        ia_info.primitiveRestartEnable = false;
        return ia_info;
    }

private:
    int count_;
    int current_;

    std::mt19937 generator_;
    std::uniform_real_distribution<float> distribution_;
    float color_[4];
    float increment_;
};

Meshes::Meshes(unsigned int rng_seed, VkPhysicalDevice phy, VkDevice dev, int count)
    : physical_dev_(phy), dev_(dev), vertex_input_state_(), input_assembly_state_()
{
    generate(rng_seed, count);
}

Meshes::~Meshes()
{
    vk::FreeMemory(dev_, mem_, nullptr);
    vk::DestroyBuffer(dev_, vb_, nullptr);
    vk::DestroyBuffer(dev_, ib_, nullptr);
}

void Meshes::cmd_bind_buffers(VkCommandBuffer cmd) const
{
    const VkDeviceSize vb_offset = 0;
    vk::CmdBindVertexBuffers(cmd, 0, 1, &vb_, &vb_offset);

    vk::CmdBindIndexBuffer(cmd, ib_, 0, index_type_);
}

void Meshes::cmd_draw(VkCommandBuffer cmd, int mesh) const
{
    const Mesh &m = meshes_[mesh];
    vk::CmdDrawIndexed(cmd, m.index_count, 1, m.first_index, m.vertex_offset, 0);
}

void Meshes::generate(unsigned int rng_seed, int count)
{
    MeshGenerator gen(rng_seed, count);

    meshes_.reserve(count);

    uint32_t vertex_total = 0;
    uint32_t index_total = 0;
    gen.head();
    while (gen.next()) {
        Mesh m;
        m.index_count = gen.index_count();
        m.first_index = index_total;
        m.vertex_offset = vertex_total;
        meshes_.push_back(m);

        vertex_total += gen.vertex_count();
        index_total += m.index_count;
    }

    vertex_input_bindings_ = gen.vertex_input_bindings();
    vertex_input_attrs_ = gen.vertex_input_attributes();
    vertex_input_state_.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_.vertexBindingDescriptionCount = vertex_input_bindings_.size();
    vertex_input_state_.pVertexBindingDescriptions = vertex_input_bindings_.data();
    vertex_input_state_.vertexAttributeDescriptionCount = vertex_input_attrs_.size();
    vertex_input_state_.pVertexAttributeDescriptions = vertex_input_attrs_.data();

    input_assembly_state_ = gen.input_assembly_state();

    index_type_ = gen.index_type();
    vb_size_ = gen.vertex_size() * vertex_total;
    ib_size_ = gen.index_size() * index_total;

    allocate_resources();
    upload_meshes(gen);
}

void Meshes::allocate_resources()
{
    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = vb_size_;
    buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vk::CreateBuffer(dev_, &buf_info, nullptr, &vb_);

    buf_info.size = ib_size_;
    buf_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    vk::CreateBuffer(dev_, &buf_info, nullptr, &ib_);

    VkMemoryRequirements vb_mem_reqs, ib_mem_reqs;
    vk::GetBufferMemoryRequirements(dev_, vb_, &vb_mem_reqs);
    vk::GetBufferMemoryRequirements(dev_, ib_, &ib_mem_reqs);

    // indices follow vertices
    ib_mem_offset_ = vb_mem_reqs.size +
        (ib_mem_reqs.alignment - (vb_mem_reqs.size % ib_mem_reqs.alignment));

    VkMemoryAllocateInfo mem_info = {};
    mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_info.allocationSize = ib_mem_offset_ + ib_mem_reqs.size;

    // find any supported and mappable memory type
    uint32_t mem_types = (vb_mem_reqs.memoryTypeBits & ib_mem_reqs.memoryTypeBits);
    VkPhysicalDeviceMemoryProperties mem_props;
    vk::GetPhysicalDeviceMemoryProperties(physical_dev_, &mem_props);
    for (uint32_t idx = 0; idx < mem_props.memoryTypeCount; idx++) {
        if (!(mem_types & (1 << idx)))
            continue;

        if (!(mem_props.memoryTypes[idx].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
            continue;

        // TODO this may not be reachable
        mem_info.memoryTypeIndex = idx;
        break;
    }

    vk::AllocateMemory(dev_, &mem_info, nullptr, &mem_);

    vk::BindBufferMemory(dev_, vb_, mem_, 0);
    vk::BindBufferMemory(dev_, ib_, mem_, ib_mem_offset_);
}

void Meshes::upload_meshes(MeshGenerator &gen)
{
    uint8_t *vb_data, *ib_data;
    vk::MapMemory(dev_, mem_, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void **>(&vb_data));
    ib_data = vb_data + ib_mem_offset_;

    VkDeviceSize vb_avail = vb_size_;
    VkDeviceSize ib_avail = ib_size_;
    gen.head();
    while (gen.next()) {
        VkDeviceSize written = gen.write_vertices(vb_data, vb_avail);
        vb_data += written;
        vb_avail -= written;

        written = gen.write_indices(ib_data, ib_avail);
        ib_data += written;
        ib_avail -= written;
    }

    vk::UnmapMemory(dev_, mem_);
}
