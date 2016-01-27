#ifndef MESHES_H
#define MESHES_H

#include <vulkan/vulkan.h>
#include <vector>

class MeshGenerator;

class Meshes {
public:
    Meshes(unsigned int rng_seed, const std::vector<VkMemoryPropertyFlags> &mem_flags, VkDevice dev, int count);
    ~Meshes();

    const VkPipelineVertexInputStateCreateInfo &vertex_input_state() const { return vertex_input_state_; }
    const VkPipelineInputAssemblyStateCreateInfo &input_assembly_state() const { return input_assembly_state_; }
    void cmd_bind_buffers(VkCommandBuffer cmd) const;
    void cmd_draw(VkCommandBuffer cmd, int mesh) const;

private:
    void generate(unsigned int rng_seed, const std::vector<VkMemoryPropertyFlags> &mem_flags, int count);
    void allocate_resources(const std::vector<VkMemoryPropertyFlags> &mem_flags);
    void upload_meshes(MeshGenerator &gen);

    VkDevice dev_;

    struct Mesh {
        uint32_t index_count;
        uint32_t first_index;
        int32_t vertex_offset;
    };
    std::vector<Mesh> meshes_;

    std::vector<VkVertexInputBindingDescription> vertex_input_bindings_;
    std::vector<VkVertexInputAttributeDescription> vertex_input_attrs_;
    VkPipelineVertexInputStateCreateInfo vertex_input_state_;
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_;

    VkIndexType index_type_;
    VkDeviceSize vb_size_;
    VkDeviceSize ib_size_;

    VkBuffer vb_;
    VkBuffer ib_;
    VkDeviceSize ib_mem_offset_;

    VkDeviceMemory mem_;
};

#endif // MESHES_H
