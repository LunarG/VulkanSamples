/*
 * Copyright (C) 2016 Google, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <cassert>
#include <cstring>
#include <array>

#include "Helpers.h"
#include "Meshes.h"

namespace {

class Mesh {
public:
    struct Position {
        float x;
        float y;
        float z;
    };

    struct Normal {
        float x;
        float y;
        float z;
    };

    struct Face {
        int v0;
        int v1;
        int v2;
    };

    static uint32_t vertex_stride()
    {
        // Position + Normal
        const int comp_count = 6;

        return sizeof(float) * comp_count;
    }

    static VkVertexInputBindingDescription vertex_input_binding()
    {
        VkVertexInputBindingDescription vi_binding = {};
        vi_binding.binding = 0;
        vi_binding.stride = vertex_stride();
        vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return vi_binding;
    }

    static std::vector<VkVertexInputAttributeDescription> vertex_input_attributes()
    {
        std::vector<VkVertexInputAttributeDescription> vi_attrs(2);
        // Position
        vi_attrs[0].location = 0;
        vi_attrs[0].binding = 0;
        vi_attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vi_attrs[0].offset = 0;
        // Normal
        vi_attrs[1].location = 1;
        vi_attrs[1].binding = 0;
        vi_attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        vi_attrs[1].offset = sizeof(float) * 3;

        return vi_attrs;
    }

    static VkIndexType index_type()
    {
        return VK_INDEX_TYPE_UINT32;
    }

    static VkPipelineInputAssemblyStateCreateInfo input_assembly_state()
    {
        VkPipelineInputAssemblyStateCreateInfo ia_info = {};
        ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        ia_info.primitiveRestartEnable = false;
        return ia_info;
    }

    void build(const std::vector<std::array<float, 6>> &vertices, const std::vector<std::array<int, 3>> &faces)
    {
        positions_.reserve(vertices.size());
        normals_.reserve(vertices.size());
        for (const auto &v : vertices) {
            positions_.emplace_back(Position{ v[0], v[1], v[2] });
            normals_.emplace_back(Normal{ v[3], v[4], v[5] });
        }

        faces_.reserve(faces.size());
        for (const auto &f : faces)
            faces_.emplace_back(Face{ f[0], f[1], f[2] });
    }

    uint32_t vertex_count() const
    {
        return positions_.size();
    }

    VkDeviceSize vertex_buffer_size() const
    {
        return vertex_stride() * vertex_count();
    }

    void vertex_buffer_write(void *data) const
    {
        float *dst = reinterpret_cast<float *>(data);
        for (size_t i = 0; i < positions_.size(); i++) {
            const Position &pos = positions_[i];
            const Normal &normal = normals_[i];
            dst[0] = pos.x;
            dst[1] = pos.y;
            dst[2] = pos.z;
            dst[3] = normal.x;
            dst[4] = normal.y;
            dst[5] = normal.z;
            dst += 6;
        }
    }

    uint32_t index_count() const
    {
        return faces_.size() * 3;
    }

    VkDeviceSize index_buffer_size() const
    {
        return sizeof(uint32_t) * index_count();
    }

    void index_buffer_write(void *data) const
    {
        uint32_t *dst = reinterpret_cast<uint32_t *>(data);
        for (const auto &face : faces_) {
            dst[0] = face.v0;
            dst[1] = face.v1;
            dst[2] = face.v2;
            dst += 3;
        }
    }

    std::vector<Position> positions_;
    std::vector<Normal> normals_;
    std::vector<Face> faces_;
};

class BuildPyramid {
public:
    BuildPyramid(Mesh &mesh)
    {
        const std::vector<std::array<float, 6>> vertices = {
            //      position                normal
            {  0.0f,  0.0f,  1.0f,    0.0f,  0.0f,  1.0f },
            { -1.0f, -1.0f, -1.0f,   -1.0f, -1.0f, -1.0f },
            {  1.0f, -1.0f, -1.0f,    1.0f, -1.0f, -1.0f },
            {  1.0f,  1.0f, -1.0f,    1.0f,  1.0f, -1.0f },
            { -1.0f,  1.0f, -1.0f,   -1.0f,  1.0f, -1.0f },
        };

        const std::vector<std::array<int, 3>> faces = {
            { 0, 1, 2 },
            { 0, 2, 3 },
            { 0, 3, 4 },
            { 0, 4, 1 },
            { 1, 4, 3 },
            { 1, 3, 2 },
        };

        mesh.build(vertices, faces);
    }
};

void build_meshes(std::array<Mesh, Meshes::MESH_COUNT> &meshes)
{
    BuildPyramid build_pyramid(meshes[Meshes::MESH_PYRAMID]);
}

} // namespace

Meshes::Meshes(VkDevice dev, const std::vector<VkMemoryPropertyFlags> &mem_flags)
    : dev_(dev),
      vertex_input_binding_(Mesh::vertex_input_binding()),
      vertex_input_attrs_(Mesh::vertex_input_attributes()),
      vertex_input_state_(),
      input_assembly_state_(Mesh::input_assembly_state()),
      index_type_(Mesh::index_type())
{
    vertex_input_state_.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_.vertexBindingDescriptionCount = 1;
    vertex_input_state_.pVertexBindingDescriptions = &vertex_input_binding_;
    vertex_input_state_.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attrs_.size());
    vertex_input_state_.pVertexAttributeDescriptions = vertex_input_attrs_.data();

    std::array<Mesh, MESH_COUNT> meshes;
    build_meshes(meshes);

    draw_commands_.reserve(meshes.size());
    uint32_t first_index = 0;
    int32_t vertex_offset = 0;
    VkDeviceSize vb_size = 0;
    VkDeviceSize ib_size = 0;
    for (const auto &mesh : meshes) {
        VkDrawIndexedIndirectCommand draw = {};
        draw.indexCount = mesh.index_count();
        draw.instanceCount = 1;
        draw.firstIndex = first_index;
        draw.vertexOffset = vertex_offset;
        draw.firstInstance = 0;

        draw_commands_.push_back(draw);

        first_index += mesh.index_count();
        vertex_offset += mesh.vertex_count();
        vb_size += mesh.vertex_buffer_size();
        ib_size += mesh.index_buffer_size();
    }

    allocate_resources(vb_size, ib_size, mem_flags);

    uint8_t *vb_data, *ib_data;
    vk::assert_success(vk::MapMemory(dev_, mem_, 0, VK_WHOLE_SIZE,
                0, reinterpret_cast<void **>(&vb_data)));
    ib_data = vb_data + ib_mem_offset_;

    for (const auto &mesh : meshes) {
        mesh.vertex_buffer_write(vb_data);
        mesh.index_buffer_write(ib_data);
        vb_data += mesh.vertex_buffer_size();
        ib_data += mesh.index_buffer_size();
    }

    vk::UnmapMemory(dev_, mem_);
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

void Meshes::cmd_draw(VkCommandBuffer cmd, Type type) const
{
    const auto &draw = draw_commands_[type];
    vk::CmdDrawIndexed(cmd, draw.indexCount, draw.instanceCount,
            draw.firstIndex, draw.vertexOffset, draw.firstInstance);
}

void Meshes::allocate_resources(VkDeviceSize vb_size, VkDeviceSize ib_size, const std::vector<VkMemoryPropertyFlags> &mem_flags)
{
    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = vb_size;
    buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vk::CreateBuffer(dev_, &buf_info, nullptr, &vb_);

    buf_info.size = ib_size;
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
    for (uint32_t idx = 0; idx < mem_flags.size(); idx++) {
        if ((mem_types & (1 << idx)) &&
            (mem_flags[idx] & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (mem_flags[idx] & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            // TODO this may not be reachable
            mem_info.memoryTypeIndex = idx;
            break;
        }
    }

    vk::AllocateMemory(dev_, &mem_info, nullptr, &mem_);

    vk::BindBufferMemory(dev_, vb_, mem_, 0);
    vk::BindBufferMemory(dev_, ib_, mem_, ib_mem_offset_);
}
