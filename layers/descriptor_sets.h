/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Tobin Ehlis <tobine@google.com>
 */
#ifndef CORE_VALIDATION_DESCRIPTOR_SETS_H_
#define CORE_VALIDATION_DESCRIPTOR_SETS_H_

// Check for noexcept support
#if defined(__clang__)
#if __has_feature(cxx_noexcept)
#define HAS_NOEXCEPT
#endif
#else
#if defined(__GXX_EXPERIMENTAL_CXX0X__) && __GNUC__ * 10 + __GNUC_MINOR__ >= 46
#define HAS_NOEXCEPT
#else
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023026 && defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS
#define HAS_NOEXCEPT
#endif
#endif
#endif

#ifdef HAS_NOEXCEPT
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

#include "core_validation_error_enums.h"
#include "core_validation_types.h"
#include "vk_layer_logging.h"
#include "vk_layer_utils.h"
#include "vk_safe_struct.h"
#include "vulkan/vk_layer.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Descriptor Data structures

/*
 * DescriptorSetLayout class
 *
 * Overview - This class encapsulates the Vulkan VkDescriptorSetLayout data (layout).
 *   A layout consists of some number of bindings, each of which has a binding#, a
 *   type, descriptor count, stage flags, and pImmutableSamplers.
 *
 * Index vs Binding - A layout is created with an array of VkDescriptorSetLayoutBinding
 *  where each array index will have a corresponding binding# that is defined in that struct.
 *  This class, therefore, provides utility functions where you can retrieve data for
 *  layout bindings based on either the original index into the pBindings array, or based
 *  on the binding#.
 *  Typically if you want to cover all of the bindings in a layout, you can do that by
 *   iterating over GetBindingCount() bindings and using the Get*FromIndex() functions.
 *  Otherwise, you can use the Get*FromBinding() functions to just grab binding info
 *   for a particular binding#.
 *
 * Global Index - The "Index" referenced above is the index into the original pBindings
 *  array. So there are as many indices as there are bindings.
 *  This class also has the concept of a Global Index. For the global index functions,
 *  there are as many global indices as there are descriptors in the layout.
 *  For the global index, consider all of the bindings to be a flat array where
 *  descriptor 0 of pBinding[0] is index 0 and each descriptor in the layout increments
 *  from there. So if pBinding[0] in this example had descriptorCount of 10, then
 *  the GlobalStartIndex of pBinding[1] will be 10 where 0-9 are the global indices
 *  for pBinding[0].
 */
namespace cvdescriptorset {
class DescriptorSetLayout {
  public:
    // Constructors and destructor
    DescriptorSetLayout(debug_report_data *report_data, const VkDescriptorSetLayoutCreateInfo *p_create_info,
                        const VkDescriptorSetLayout layout);
    // Straightforward Get functions
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return layout_; };
    uint32_t GetTotalDescriptorCount() const { return descriptor_count_; };
    uint32_t GetDynamicDescriptorCount() const { return dynamic_descriptor_count_; };
    uint32_t GetBindingCount() const { return binding_count_; };
    // Fill passed-in set with bindings
    void FillBindingSet(std::unordered_set<uint32_t> *) const;
    // Return true if given binding is present in this layout
    bool HasBinding(const uint32_t binding) const { return binding_to_index_map_.count(binding) > 0; };
    // Return true if this layout is compatible with passed in layout,
    //   else return false and update error_msg with description of incompatibility
    bool IsCompatible(const DescriptorSetLayout *, std::string *) const;
    // Return true if binding 1 beyond given exists and has same type, stageFlags & immutable sampler use
    bool IsNextBindingConsistent(const uint32_t) const;
    // Various Get functions that can either be passed a binding#, which will
    //  be automatically translated into the appropriate index from the original
    //  pBindings array, or the index# can be passed in directly
    VkDescriptorSetLayoutBinding const *GetDescriptorSetLayoutBindingPtrFromBinding(const uint32_t) const;
    VkDescriptorSetLayoutBinding const *GetDescriptorSetLayoutBindingPtrFromIndex(const uint32_t) const;
    uint32_t GetDescriptorCountFromBinding(const uint32_t) const;
    uint32_t GetDescriptorCountFromIndex(const uint32_t) const;
    VkDescriptorType GetTypeFromBinding(const uint32_t) const;
    VkDescriptorType GetTypeFromIndex(const uint32_t) const;
    VkDescriptorType GetTypeFromGlobalIndex(const uint32_t) const;
    VkShaderStageFlags GetStageFlagsFromBinding(const uint32_t) const;
    VkSampler const *GetImmutableSamplerPtrFromBinding(const uint32_t) const;
    VkSampler const *GetImmutableSamplerPtrFromIndex(const uint32_t) const;
    // For a particular binding, get the global index
    uint32_t GetGlobalStartIndexFromBinding(const uint32_t) const;
    uint32_t GetGlobalEndIndexFromBinding(const uint32_t) const;
    // For a particular binding starting at offset and having update_count descriptors
    //  updated, verify that for any binding boundaries crossed, the update is consistent
    bool VerifyUpdateConsistency(uint32_t, uint32_t, uint32_t, const char *, const VkDescriptorSet, std::string *) const;

  private:
    VkDescriptorSetLayout layout_;
    std::unordered_map<uint32_t, uint32_t> binding_to_index_map_;
    std::unordered_map<uint32_t, uint32_t> binding_to_global_start_index_map_;
    std::unordered_map<uint32_t, uint32_t> binding_to_global_end_index_map_;
    // VkDescriptorSetLayoutCreateFlags flags_;
    uint32_t binding_count_; // # of bindings in this layout
    std::vector<safe_VkDescriptorSetLayoutBinding> bindings_;
    uint32_t descriptor_count_; // total # descriptors in this layout
    uint32_t dynamic_descriptor_count_;
};

/*
 * Descriptor classes
 *  Descriptor is an abstract base class from which 5 separate descriptor types are derived.
 *   This allows the WriteUpdate() and CopyUpdate() operations to be specialized per
 *   descriptor type, but all descriptors in a set can be accessed via the common Descriptor*.
 */

// Slightly broader than type, each c++ "class" will has a corresponding "DescriptorClass"
typedef enum _DescriptorClass { PlainSampler, ImageSampler, Image, TexelBuffer, GeneralBuffer } DescriptorClass;

class Descriptor {
  public:
    virtual ~Descriptor(){};
    virtual bool WriteUpdate(const VkWriteDescriptorSet *, const uint32_t, std::string *) = 0;
    virtual bool CopyUpdate(const Descriptor *, std::string *) = 0;
    virtual DescriptorClass GetClass() const { return descriptor_class; };
    // Special fast-path check for SamplerDescriptors that are immutable
    virtual bool IsImmutableSampler() const { return false; };
    // Check for dynamic descriptor type
    virtual bool IsDynamic() const { return false; };
    // Check for storage descriptor type
    virtual bool IsStorage() const { return false; };
    bool updated; // Has descriptor been updated?
    DescriptorClass descriptor_class;
};
// Shared helper functions - These are useful because the shared sampler image descriptor type
//  performs common functions with both sampler and image descriptors so they can share their common functions
bool ValidateSampler(const VkSampler, const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *);
bool ValidateImageUpdate(const VkImageView, const VkImageLayout, const std::unordered_map<VkImageView, VkImageViewCreateInfo> *,
                         const std::unordered_map<VkImage, IMAGE_NODE> *, const std::unordered_map<VkImage, VkSwapchainKHR> *,
                         const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *, std::string *);

class SamplerDescriptor : public Descriptor {
  public:
    SamplerDescriptor(const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *);
    SamplerDescriptor(const VkSampler *, const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *);
    bool WriteUpdate(const VkWriteDescriptorSet *, const uint32_t, std::string *) override;
    bool CopyUpdate(const Descriptor *, std::string *) override;
    virtual bool IsImmutableSampler() const override { return immutable_; };

  private:
    // bool ValidateSampler(const VkSampler) const;
    VkSampler sampler_;
    bool immutable_;
    const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *sampler_map_;
};

class ImageSamplerDescriptor : public Descriptor {
  public:
    ImageSamplerDescriptor(const std::unordered_map<VkImageView, VkImageViewCreateInfo> *,
                           const std::unordered_map<VkImage, IMAGE_NODE> *, const std::unordered_map<VkImage, VkSwapchainKHR> *,
                           const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *,
                           const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *);
    ImageSamplerDescriptor(const VkSampler *, const std::unordered_map<VkImageView, VkImageViewCreateInfo> *,
                           const std::unordered_map<VkImage, IMAGE_NODE> *, const std::unordered_map<VkImage, VkSwapchainKHR> *,
                           const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *,
                           const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *);
    bool WriteUpdate(const VkWriteDescriptorSet *, const uint32_t, std::string *) override;
    bool CopyUpdate(const Descriptor *, std::string *) override;

  private:
    VkSampler sampler_;
    bool immutable_;
    VkImageView image_view_;
    VkImageLayout image_layout_;
    const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *sampler_map_;
    const std::unordered_map<VkImageView, VkImageViewCreateInfo> *image_view_map_;
    const std::unordered_map<VkImage, IMAGE_NODE> *image_map_;
    const std::unordered_map<VkImage, VkSwapchainKHR> *image_to_swapchain_map_;
    const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *swapchain_map_;
};

class ImageDescriptor : public Descriptor {
  public:
    ImageDescriptor(const VkDescriptorType, const std::unordered_map<VkImageView, VkImageViewCreateInfo> *,
                    const std::unordered_map<VkImage, IMAGE_NODE> *, const std::unordered_map<VkImage, VkSwapchainKHR> *,
                    const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *);
    bool WriteUpdate(const VkWriteDescriptorSet *, const uint32_t, std::string *) override;
    bool CopyUpdate(const Descriptor *, std::string *) override;
    virtual bool IsStorage() const override { return storage_; }
    VkImageView GetImageView() const { return image_view_; }
    VkImageLayout GetImageLayout() const { return image_layout_; }

  private:
    bool storage_;
    VkImageView image_view_;
    VkImageLayout image_layout_;
    const std::unordered_map<VkImageView, VkImageViewCreateInfo> *image_view_map_;
    const std::unordered_map<VkImage, IMAGE_NODE> *image_map_;
    const std::unordered_map<VkImage, VkSwapchainKHR> *image_to_swapchain_map_;
    const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *swapchain_map_;
};

class TexelDescriptor : public Descriptor {
  public:
    TexelDescriptor(const VkDescriptorType, const std::unordered_map<VkBufferView, VkBufferViewCreateInfo> *);
    bool WriteUpdate(const VkWriteDescriptorSet *, const uint32_t, std::string *) override;
    bool CopyUpdate(const Descriptor *, std::string *) override;
    virtual bool IsStorage() const override { return storage_; }
    VkBufferView GetBufferView() const { return buffer_view_; }

  private:
    VkBufferView buffer_view_;
    bool storage_;
    const std::unordered_map<VkBufferView, VkBufferViewCreateInfo> *buffer_view_map_;
};

class BufferDescriptor : public Descriptor {
  public:
    BufferDescriptor(const VkDescriptorType, const std::unordered_map<VkBuffer, BUFFER_NODE> *);
    bool WriteUpdate(const VkWriteDescriptorSet *, const uint32_t, std::string *) override;
    bool CopyUpdate(const Descriptor *, std::string *) override;
    virtual bool IsDynamic() const override { return dynamic_; }
    virtual bool IsStorage() const override { return storage_; }
    VkBuffer GetBuffer() const { return buffer_; }
    VkDeviceSize GetOffset() const { return offset_; }
    VkDeviceSize GetRange() const { return range_; }

  private:
    bool storage_;
    bool dynamic_;
    VkBuffer buffer_;
    VkDeviceSize offset_;
    VkDeviceSize range_;
    const std::unordered_map<VkBuffer, BUFFER_NODE> *buffer_map_;
};
/*
 * DescriptorSet class
 *
 * Overview - This class encapsulates the Vulkan VkDescriptorSet data (set).
 *   A set has an underlying layout which defines the bindings in the set and the
 *   types and numbers of descriptors in each descriptor slot. Most of the layout
 *   interfaces are exposed through identically-named functions in the set class.
 *   Please refer to the DescriptorSetLayout comment above for a description of
 *   index, binding, and global index.
 *
 * At construction a vector of Descriptor* is created with types corresponding to the
 *   layout. The primary operation performed on the descriptors is to update them
 *   via write or copy updates, and validate that the update contents are correct.
 *   In order to validate update contents, the DescriptorSet stores a bunch of ptrs
 *   to data maps where various Vulkan objects can be looked up. The management of
 *   those maps is performed externally. The set class relies on their contents to
 *   be correct at the time of update.
 */
class DescriptorSet : public BASE_NODE {
  public:
    DescriptorSet(const VkDescriptorSet, const DescriptorSetLayout *, const std::unordered_map<VkBuffer, BUFFER_NODE> *,
                  const std::unordered_map<VkDeviceMemory, DEVICE_MEM_INFO> *,
                  const std::unordered_map<VkBufferView, VkBufferViewCreateInfo> *,
                  const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *,
                  const std::unordered_map<VkImageView, VkImageViewCreateInfo> *, const std::unordered_map<VkImage, IMAGE_NODE> *,
                  const std::unordered_map<VkImage, VkSwapchainKHR> *,
                  const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *);
    ~DescriptorSet(){};
    // A number of common Get* functions that return data based on layout from which this set was created
    uint32_t GetTotalDescriptorCount() const { return p_layout_ ? p_layout_->GetTotalDescriptorCount() : 0; };
    uint32_t GetDynamicDescriptorCount() const { return p_layout_ ? p_layout_->GetDynamicDescriptorCount() : 0; };
    uint32_t GetBindingCount() const { return p_layout_ ? p_layout_->GetBindingCount() : 0; };
    VkDescriptorType GetTypeFromIndex(const uint32_t index) const {
        return p_layout_ ? p_layout_->GetTypeFromIndex(index) : VK_DESCRIPTOR_TYPE_MAX_ENUM;
    };
    VkDescriptorType GetTypeFromGlobalIndex(const uint32_t index) const {
        return p_layout_ ? p_layout_->GetTypeFromGlobalIndex(index) : VK_DESCRIPTOR_TYPE_MAX_ENUM;
    };
    VkDescriptorType GetTypeFromBinding(const uint32_t binding) const {
        return p_layout_ ? p_layout_->GetTypeFromBinding(binding) : VK_DESCRIPTOR_TYPE_MAX_ENUM;
    };
    uint32_t GetDescriptorCountFromIndex(const uint32_t index) const {
        return p_layout_ ? p_layout_->GetDescriptorCountFromIndex(index) : 0;
    };
    uint32_t GetDescriptorCountFromBinding(const uint32_t binding) const {
        return p_layout_ ? p_layout_->GetDescriptorCountFromBinding(binding) : 0;
    };
    // Return true if given binding is present in this set
    bool HasBinding(const uint32_t binding) const { return p_layout_->HasBinding(binding); };
    // Is this set compatible with the given layout?
    bool IsCompatible(const DescriptorSetLayout *, std::string *) const;
    // For given bindings validate state at time of draw is correct, returning false on error and writing error details into string*
    bool ValidateDrawState(const std::unordered_set<uint32_t> &, const std::vector<uint32_t> &, std::string *) const;
    // For given set of bindings, add any buffers and images that will be updated to their respective unordered_sets & return number
    // of objects inserted
    uint32_t GetStorageUpdates(const std::unordered_set<uint32_t> &, std::unordered_set<VkBuffer> *,
                               std::unordered_set<VkImageView> *) const;
    // For all descriptors in a set, add any buffers and images that may be updated to their respective unordered_sets & return
    // number of objects inserted
    uint32_t GetAllStorageUpdates(std::unordered_set<VkBuffer> *, std::unordered_set<VkImageView> *) const;
    // Perform write update based on update struct
    bool WriteUpdate(debug_report_data *, const VkWriteDescriptorSet *, std::string *);
    // Perform copy update, using 'this' set as the dest and the passed-in DescriptorSet as the src
    bool CopyUpdate(debug_report_data *, const VkCopyDescriptorSet *, const DescriptorSet *, std::string *);

    const DescriptorSetLayout *GetLayout() const { return p_layout_; };
    VkDescriptorSet GetSet() const { return set_; };
    // Return unordered_set of all command buffers that this set is bound to
    std::unordered_set<VkCommandBuffer> GetBoundCmdBuffers() const { return bound_cmd_buffers_; }
    // Bind given cmd_buffer to this descriptor set
    void BindCommandBuffer(const VkCommandBuffer cmd_buffer) { bound_cmd_buffers_.insert(cmd_buffer); }
    // If given cmd_buffer is in the bound_cmd_buffers_ set, remove it
    void RemoveBoundCommandBuffer(const VkCommandBuffer cmd_buffer) { bound_cmd_buffers_.erase(cmd_buffer); }
    VkSampler const *GetImmutableSamplerPtrFromBinding(const uint32_t index) const {
        return p_layout_->GetImmutableSamplerPtrFromBinding(index);
    };
    // For a particular binding, get the global index
    uint32_t GetGlobalStartIndexFromBinding(const uint32_t binding) const {
        return p_layout_->GetGlobalStartIndexFromBinding(binding);
    };
    uint32_t GetGlobalEndIndexFromBinding(const uint32_t binding) const {
        return p_layout_->GetGlobalEndIndexFromBinding(binding);
    };
    // Return true if any part of set has ever been updated
    bool IsUpdated() const { return some_update_; };

  private:
    bool ValidateUpdate(const VkWriteDescriptorSet *, const uint32_t, std::string *) const;
    bool some_update_; // has any part of the set ever been updated?
    VkDescriptorSet set_;
    uint32_t descriptor_count_; // Count of all descriptors in this set
    const DescriptorSetLayout *p_layout_;
    std::unordered_set<VkCommandBuffer> bound_cmd_buffers_;
    std::vector<std::unique_ptr<Descriptor>> descriptors_;
    // Ptrs to object containers to verify bound data
    const std::unordered_map<VkBuffer, BUFFER_NODE> *buffer_map_;
    const std::unordered_map<VkDeviceMemory, DEVICE_MEM_INFO> *memory_map_;
    const std::unordered_map<VkBufferView, VkBufferViewCreateInfo> *buffer_view_map_;
    const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *sampler_map_;
    const std::unordered_map<VkImageView, VkImageViewCreateInfo> *image_view_map_;
    // TODO : For next 3 maps all we really need (currently) is an image to format mapping
    const std::unordered_map<VkImage, IMAGE_NODE> *image_map_;
    const std::unordered_map<VkImage, VkSwapchainKHR> *image_to_swapchain_map_;
    const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *swapchain_map_;
};
}
#endif // CORE_VALIDATION_DESCRIPTOR_SETS_H_
