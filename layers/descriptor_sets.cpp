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

#include "descriptor_sets.h"
#include "vk_enum_string_helper.h"
#include "vk_safe_struct.h"
#include <sstream>

// Construct DescriptorSetLayout instance from given create info
cvdescriptorset::DescriptorSetLayout::DescriptorSetLayout(debug_report_data *report_data,
                                                          const VkDescriptorSetLayoutCreateInfo *p_create_info,
                                                          const VkDescriptorSetLayout layout)
    : layout_(layout), binding_count_(p_create_info->bindingCount), descriptor_count_(0), dynamic_descriptor_count_(0) {
    uint32_t global_index = 0;
    for (uint32_t i = 0; i < binding_count_; ++i) {
        descriptor_count_ += p_create_info->pBindings[i].descriptorCount;
        if (!binding_to_index_map_.emplace(p_create_info->pBindings[i].binding, i).second) {
            log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT,
                    reinterpret_cast<uint64_t &>(layout_), __LINE__, DRAWSTATE_INVALID_LAYOUT, "DS",
                    "duplicated binding number in "
                    "VkDescriptorSetLayoutBinding");
        }
        binding_to_global_start_index_map_[p_create_info->pBindings[i].binding] = global_index;
        global_index += p_create_info->pBindings[i].descriptorCount ? p_create_info->pBindings[i].descriptorCount - 1 : 0;
        binding_to_global_end_index_map_[p_create_info->pBindings[i].binding] = global_index;
        global_index++;
        bindings_.push_back(safe_VkDescriptorSetLayoutBinding(&p_create_info->pBindings[i]));
        // In cases where we should ignore pImmutableSamplers make sure it's NULL
        if ((p_create_info->pBindings[i].pImmutableSamplers) &&
            ((p_create_info->pBindings[i].descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) &&
             (p_create_info->pBindings[i].descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER))) {
            bindings_.back().pImmutableSamplers = nullptr;
        }
        if (p_create_info->pBindings[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
            p_create_info->pBindings[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            dynamic_descriptor_count_++;
        }
    }
}
// put all bindings into the given set
void cvdescriptorset::DescriptorSetLayout::FillBindingSet(std::unordered_set<uint32_t> *binding_set) const {
    for (auto binding_index_pair : binding_to_index_map_)
        binding_set->insert(binding_index_pair.first);
}
VkDescriptorSetLayoutBinding const *
cvdescriptorset::DescriptorSetLayout::GetDescriptorSetLayoutBindingPtrFromBinding(const uint32_t binding) const {
    const auto &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        return bindings_[bi_itr->second].ptr();
    }
    return nullptr;
}
VkDescriptorSetLayoutBinding const *
cvdescriptorset::DescriptorSetLayout::GetDescriptorSetLayoutBindingPtrFromIndex(const uint32_t index) const {
    if (index >= bindings_.size())
        return nullptr;
    return bindings_[index].ptr();
}
// Return descriptorCount for given binding, 0 if index is unavailable
uint32_t cvdescriptorset::DescriptorSetLayout::GetDescriptorCountFromBinding(const uint32_t binding) const {
    const auto &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        return bindings_[bi_itr->second].descriptorCount;
    }
    return 0;
}
// Return descriptorCount for given index, 0 if index is unavailable
uint32_t cvdescriptorset::DescriptorSetLayout::GetDescriptorCountFromIndex(const uint32_t index) const {
    if (index >= bindings_.size())
        return 0;
    return bindings_[index].descriptorCount;
}
// For the given binding, return descriptorType
VkDescriptorType cvdescriptorset::DescriptorSetLayout::GetTypeFromBinding(const uint32_t binding) const {
    assert(binding_to_index_map_.count(binding));
    const auto &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        return bindings_[bi_itr->second].descriptorType;
    }
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}
// For the given index, return descriptorType
VkDescriptorType cvdescriptorset::DescriptorSetLayout::GetTypeFromIndex(const uint32_t index) const {
    assert(index < bindings_.size());
    return bindings_[index].descriptorType;
}
// For the given global index, return descriptorType
//  Currently just counting up through bindings_, may improve this in future
VkDescriptorType cvdescriptorset::DescriptorSetLayout::GetTypeFromGlobalIndex(const uint32_t index) const {
    uint32_t global_offset = 0;
    for (auto binding : bindings_) {
        global_offset += binding.descriptorCount;
        if (index < global_offset)
            return binding.descriptorType;
    }
    assert(0); // requested global index is out of bounds
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}
// For the given binding, return stageFlags
VkShaderStageFlags cvdescriptorset::DescriptorSetLayout::GetStageFlagsFromBinding(const uint32_t binding) const {
    assert(binding_to_index_map_.count(binding));
    const auto &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        return bindings_[bi_itr->second].stageFlags;
    }
    return VkShaderStageFlags(0);
}
// For the given binding, return start index
uint32_t cvdescriptorset::DescriptorSetLayout::GetGlobalStartIndexFromBinding(const uint32_t binding) const {
    assert(binding_to_global_start_index_map_.count(binding));
    const auto &btgsi_itr = binding_to_global_start_index_map_.find(binding);
    if (btgsi_itr != binding_to_global_start_index_map_.end()) {
        return btgsi_itr->second;
    }
    // In error case max uint32_t so index is out of bounds to break ASAP
    return 0xFFFFFFFF;
}
// For the given binding, return end index
uint32_t cvdescriptorset::DescriptorSetLayout::GetGlobalEndIndexFromBinding(const uint32_t binding) const {
    assert(binding_to_global_end_index_map_.count(binding));
    const auto &btgei_itr = binding_to_global_end_index_map_.find(binding);
    if (btgei_itr != binding_to_global_end_index_map_.end()) {
        return btgei_itr->second;
    }
    // In error case max uint32_t so index is out of bounds to break ASAP
    return 0xFFFFFFFF;
}
// For given binding, return ptr to ImmutableSampler array
VkSampler const *cvdescriptorset::DescriptorSetLayout::GetImmutableSamplerPtrFromBinding(const uint32_t binding) const {
    assert(binding_to_index_map_.count(binding));
    const auto &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        return bindings_[bi_itr->second].pImmutableSamplers;
    }
    return nullptr;
}
// For given index, return ptr to ImmutableSampler array
VkSampler const *cvdescriptorset::DescriptorSetLayout::GetImmutableSamplerPtrFromIndex(const uint32_t index) const {
    assert(index < bindings_.size());
    return bindings_[index].pImmutableSamplers;
}
// If our layout is compatible with rh_ds_layout, return true,
//  else return false and fill in error_msg will description of what causes incompatibility
bool cvdescriptorset::DescriptorSetLayout::IsCompatible(const DescriptorSetLayout *rh_ds_layout, std::string *error_msg) const {
    // Trivial case
    if (layout_ == rh_ds_layout->GetDescriptorSetLayout())
        return true;
    if (descriptor_count_ != rh_ds_layout->descriptor_count_) {
        std::stringstream error_str;
        error_str << "DescriptorSetLayout " << layout_ << " has " << descriptor_count_ << " descriptors, but DescriptorSetLayout "
                  << rh_ds_layout->GetDescriptorSetLayout() << " has " << rh_ds_layout->descriptor_count_ << " descriptors.";
        *error_msg = error_str.str();
        return false; // trivial fail case
    }
    // Descriptor counts match so need to go through bindings one-by-one
    //  and verify that type and stageFlags match
    for (auto binding : bindings_) {
        // TODO : Do we also need to check immutable samplers?
        // VkDescriptorSetLayoutBinding *rh_binding;
        if (binding.descriptorCount != rh_ds_layout->GetDescriptorCountFromBinding(binding.binding)) {
            std::stringstream error_str;
            error_str << "Binding " << binding.binding << " for DescriptorSetLayout " << layout_ << " has a descriptorCount of "
                      << binding.descriptorCount << " but binding " << binding.binding << " for DescriptorSetLayout "
                      << rh_ds_layout->GetDescriptorSetLayout() << " has a descriptorCount of "
                      << rh_ds_layout->GetDescriptorCountFromBinding(binding.binding);
            *error_msg = error_str.str();
            return false;
        } else if (binding.descriptorType != rh_ds_layout->GetTypeFromBinding(binding.binding)) {
            std::stringstream error_str;
            error_str << "Binding " << binding.binding << " for DescriptorSetLayout " << layout_ << " is type '"
                      << string_VkDescriptorType(binding.descriptorType) << "' but binding " << binding.binding
                      << " for DescriptorSetLayout " << rh_ds_layout->GetDescriptorSetLayout() << " is type '"
                      << string_VkDescriptorType(rh_ds_layout->GetTypeFromBinding(binding.binding)) << "'";
            *error_msg = error_str.str();
            return false;
        } else if (binding.stageFlags != rh_ds_layout->GetStageFlagsFromBinding(binding.binding)) {
            std::stringstream error_str;
            error_str << "Binding " << binding.binding << " for DescriptorSetLayout " << layout_ << " has stageFlags "
                      << binding.stageFlags << " but binding " << binding.binding << " for DescriptorSetLayout "
                      << rh_ds_layout->GetDescriptorSetLayout() << " has stageFlags "
                      << rh_ds_layout->GetStageFlagsFromBinding(binding.binding);
            *error_msg = error_str.str();
            return false;
        }
    }
    return true;
}

bool cvdescriptorset::DescriptorSetLayout::IsNextBindingConsistent(const uint32_t binding) const {
    if (!binding_to_index_map_.count(binding + 1))
        return false;
    auto const &bi_itr = binding_to_index_map_.find(binding);
    if (bi_itr != binding_to_index_map_.end()) {
        const auto &next_bi_itr = binding_to_index_map_.find(binding + 1);
        if (next_bi_itr != binding_to_index_map_.end()) {
            auto type = bindings_[bi_itr->second].descriptorType;
            auto stage_flags = bindings_[bi_itr->second].stageFlags;
            auto immut_samp = bindings_[bi_itr->second].pImmutableSamplers ? true : false;
            if ((type != bindings_[next_bi_itr->second].descriptorType) ||
                (stage_flags != bindings_[next_bi_itr->second].stageFlags) ||
                (immut_samp != (bindings_[next_bi_itr->second].pImmutableSamplers ? true : false))) {
                return false;
            }
            return true;
        }
    }
    return false;
}
// Starting at offset descriptor of given binding, parse over update_count
//  descriptor updates and verify that for any binding boundaries that are crossed, the next binding(s) are all consistent
//  Consistency means that their type, stage flags, and whether or not they use immutable samplers matches
//  If so, return true. If not, fill in error_msg and return false
bool cvdescriptorset::DescriptorSetLayout::VerifyUpdateConsistency(uint32_t current_binding, uint32_t offset, uint32_t update_count,
                                                                   const char *type, const VkDescriptorSet set,
                                                                   std::string *error_msg) const {
    // Verify consecutive bindings match (if needed)
    auto orig_binding = current_binding;
    // Track count of descriptors in the current_bindings that are remaining to be updated
    auto binding_remaining = GetDescriptorCountFromBinding(current_binding);
    // First, it's legal to offset beyond your own binding so handle that case
    //  Really this is just searching for the binding in which the update begins and adjusting offset accordingly
    while (offset >= binding_remaining) {
        // Advance to next binding, decrement offset by binding size
        offset -= binding_remaining;
        binding_remaining = GetDescriptorCountFromBinding(++current_binding);
    }
    binding_remaining -= offset;
    while (update_count > binding_remaining) { // While our updates overstep current binding
        // Verify next consecutive binding matches type, stage flags & immutable sampler use
        if (!IsNextBindingConsistent(current_binding++)) {
            std::stringstream error_str;
            error_str << "Attempting " << type << " descriptor set " << set << " binding #" << orig_binding << " with #"
                      << update_count << " descriptors being updated but this update oversteps the bounds of this binding and the "
                                         "next binding is not consistent with current binding so this update is invalid.";
            *error_msg = error_str.str();
            return false;
        }
        // For sake of this check consider the bindings updated and grab count for next binding
        update_count -= binding_remaining;
        binding_remaining = GetDescriptorCountFromBinding(current_binding);
    }
    return true;
}

cvdescriptorset::DescriptorSet::DescriptorSet(const VkDescriptorSet set, const DescriptorSetLayout *layout,
                                              const std::unordered_map<VkBuffer, BUFFER_NODE> *buffer_map,
                                              const std::unordered_map<VkDeviceMemory, DEVICE_MEM_INFO> *memory_map,
                                              const std::unordered_map<VkBufferView, VkBufferViewCreateInfo> *buffer_view_map,
                                              const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *sampler_map,
                                              const std::unordered_map<VkImageView, VkImageViewCreateInfo> *image_view_map,
                                              const std::unordered_map<VkImage, IMAGE_NODE> *image_map,
                                              const std::unordered_map<VkImage, VkSwapchainKHR> *image_to_swapchain_map,
                                              const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *swapchain_map)
    : some_update_(false), set_(set), p_layout_(layout), buffer_map_(buffer_map), memory_map_(memory_map),
      buffer_view_map_(buffer_view_map), sampler_map_(sampler_map), image_view_map_(image_view_map), image_map_(image_map),
      image_to_swapchain_map_(image_to_swapchain_map), swapchain_map_(swapchain_map) {
    // Foreach binding, create default descriptors of given type
    for (uint32_t i = 0; i < p_layout_->GetBindingCount(); ++i) {
        auto type = p_layout_->GetTypeFromIndex(i);
        switch (type) {
        case VK_DESCRIPTOR_TYPE_SAMPLER: {
            auto immut_sampler = p_layout_->GetImmutableSamplerPtrFromIndex(i);
            for (uint32_t di = 0; di < p_layout_->GetDescriptorCountFromIndex(i); ++di) {
                if (immut_sampler)
                    descriptors_.emplace_back(std::unique_ptr<Descriptor>(new SamplerDescriptor(immut_sampler + di, sampler_map_)));
                else
                    descriptors_.emplace_back(std::unique_ptr<Descriptor>(new SamplerDescriptor(sampler_map_)));
            }
            break;
        }
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
            auto immut = p_layout_->GetImmutableSamplerPtrFromIndex(i);
            for (uint32_t di = 0; di < p_layout_->GetDescriptorCountFromIndex(i); ++di) {
                if (immut)
                    descriptors_.emplace_back(std::unique_ptr<Descriptor>(new ImageSamplerDescriptor(
                        immut + di, image_view_map_, image_map_, image_to_swapchain_map_, swapchain_map_, sampler_map_)));
                else
                    descriptors_.emplace_back(std::unique_ptr<Descriptor>(new ImageSamplerDescriptor(
                        image_view_map_, image_map_, image_to_swapchain_map_, swapchain_map_, sampler_map_)));
            }
            break;
        }
        // ImageDescriptors
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            for (uint32_t di = 0; di < p_layout_->GetDescriptorCountFromIndex(i); ++di)
                descriptors_.emplace_back(std::unique_ptr<Descriptor>(
                    new ImageDescriptor(type, image_view_map_, image_map_, image_to_swapchain_map_, swapchain_map_)));
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            for (uint32_t di = 0; di < p_layout_->GetDescriptorCountFromIndex(i); ++di)
                descriptors_.emplace_back(std::unique_ptr<Descriptor>(new TexelDescriptor(type, buffer_view_map_)));
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            for (uint32_t di = 0; di < p_layout_->GetDescriptorCountFromIndex(i); ++di)
                descriptors_.emplace_back(std::unique_ptr<Descriptor>(new BufferDescriptor(type, buffer_map_)));
            break;
        default:
            assert(0); // Bad descriptor type specified
            break;
        }
    }
}
// Is this sets underlying layout compatible with passed in layout according to "Pipeline Layout Compatibility" in spec?
bool cvdescriptorset::DescriptorSet::IsCompatible(const DescriptorSetLayout *layout, std::string *error) const {
    return layout->IsCompatible(p_layout_, error);
}
// Validate that the state of this set is appropriate for the given bindings and dynami_offsets at Draw time
//  This includes validating that all descriptors in the given bindings are updated,
//  that any update buffers are valid, and that any dynamic offsets are within the bounds of their buffers.
// Return true if state is acceptable, or false and write an error message into error string
bool cvdescriptorset::DescriptorSet::ValidateDrawState(const std::unordered_set<uint32_t> &bindings,
                                                       const std::vector<uint32_t> &dynamic_offsets, std::string *error) const {
    for (auto binding : bindings) {
        auto start_idx = p_layout_->GetGlobalStartIndexFromBinding(binding);
        if (descriptors_[start_idx]->IsImmutableSampler()) {
            // Nothing to do for strictly immutable sampler
        } else {
            auto end_idx = p_layout_->GetGlobalEndIndexFromBinding(binding);
            auto dyn_offset_index = 0;
            for (uint32_t i = start_idx; i <= end_idx; ++i) {
                if (!descriptors_[i]->updated) {
                    std::stringstream error_str;
                    error_str << "Descriptor in binding #" << binding << " at global descriptor index " << i
                              << " is being used in draw but has not been updated.";
                    *error = error_str.str();
                    return false;
                } else {
                    if (GeneralBuffer == descriptors_[i]->GetClass()) {
                        // Verify that buffers are valid
                        auto buffer = static_cast<BufferDescriptor *>(descriptors_[i].get())->GetBuffer();
                        auto buffer_node = buffer_map_->find(buffer);
                        if (buffer_node == buffer_map_->end()) {
                            std::stringstream error_str;
                            error_str << "Descriptor in binding #" << binding << " at global descriptor index " << i
                                      << " references invalid buffer " << buffer << ".";
                            *error = error_str.str();
                            return false;
                        } else {
                            auto mem_entry = memory_map_->find(buffer_node->second.mem);
                            if (mem_entry == memory_map_->end()) {
                                std::stringstream error_str;
                                error_str << "Descriptor in binding #" << binding << " at global descriptor index " << i
                                          << " uses buffer " << buffer << " that references invalid memory "
                                          << buffer_node->second.mem << ".";
                                *error = error_str.str();
                                return false;
                            }
                        }
                        if (descriptors_[i]->IsDynamic()) {
                            // Validate that dynamic offsets are within the buffer
                            auto buffer_size = buffer_node->second.createInfo.size;
                            auto range = static_cast<BufferDescriptor *>(descriptors_[i].get())->GetRange();
                            auto desc_offset = static_cast<BufferDescriptor *>(descriptors_[i].get())->GetOffset();
                            auto dyn_offset = dynamic_offsets[dyn_offset_index++];
                            if (VK_WHOLE_SIZE == range) {
                                if ((dyn_offset + desc_offset) > buffer_size) {
                                    std::stringstream error_str;
                                    error_str << "Dynamic descriptor in binding #" << binding << " at global descriptor index " << i
                                              << " uses buffer " << buffer
                                              << " with update range of VK_WHOLE_SIZE has dynamic offset " << dyn_offset
                                              << " combined with offset " << desc_offset << " that oversteps the buffer size of "
                                              << buffer_size << ".";
                                    *error = error_str.str();
                                    return false;
                                }
                            } else {
                                if ((dyn_offset + desc_offset + range) > buffer_size) {
                                    std::stringstream error_str;
                                    error_str << "Dynamic descriptor in binding #" << binding << " at global descriptor index " << i
                                              << " uses buffer " << buffer << " with dynamic offset " << dyn_offset
                                              << " combined with offset " << desc_offset << " and range " << range
                                              << " that oversteps the buffer size of " << buffer_size << ".";
                                    *error = error_str.str();
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}
// For given bindings, place any update buffers or images into the passed-in unordered_sets
uint32_t cvdescriptorset::DescriptorSet::GetStorageUpdates(const std::unordered_set<uint32_t> &bindings,
                                                           std::unordered_set<VkBuffer> *buffer_set,
                                                           std::unordered_set<VkImageView> *image_set) const {
    auto num_updates = 0;
    for (auto binding : bindings) {
        auto start_idx = p_layout_->GetGlobalStartIndexFromBinding(binding);
        if (descriptors_[start_idx]->IsStorage()) {
            if (Image == descriptors_[start_idx]->descriptor_class) {
                for (uint32_t i = 0; i < p_layout_->GetDescriptorCountFromBinding(binding); ++i) {
                    if (descriptors_[start_idx + i]->updated) {
                        image_set->insert(static_cast<ImageDescriptor *>(descriptors_[start_idx + i].get())->GetImageView());
                        num_updates++;
                    }
                }
            } else if (TexelBuffer == descriptors_[start_idx]->descriptor_class) {
                for (uint32_t i = 0; i < p_layout_->GetDescriptorCountFromBinding(binding); ++i) {
                    if (descriptors_[start_idx + i]->updated) {
                        auto bufferview = static_cast<TexelDescriptor *>(descriptors_[start_idx + i].get())->GetBufferView();
                        const auto &buff_pair = buffer_view_map_->find(bufferview);
                        if (buff_pair != buffer_view_map_->end()) {
                            buffer_set->insert(buff_pair->second.buffer);
                            num_updates++;
                        }
                    }
                }
            } else if (GeneralBuffer == descriptors_[start_idx]->descriptor_class) {
                for (uint32_t i = 0; i < p_layout_->GetDescriptorCountFromBinding(binding); ++i) {
                    if (descriptors_[start_idx + i]->updated) {
                        buffer_set->insert(static_cast<BufferDescriptor *>(descriptors_[start_idx + i].get())->GetBuffer());
                        num_updates++;
                    }
                }
            }
        }
    }
    return num_updates;
}
// This is a special case for compute shaders that should eventually be removed once we have proper valid binding info for compute
// case
uint32_t cvdescriptorset::DescriptorSet::GetAllStorageUpdates(std::unordered_set<VkBuffer> *buffer_set,
                                                              std::unordered_set<VkImageView> *image_set) const {
    std::unordered_set<uint32_t> binding_set;
    p_layout_->FillBindingSet(&binding_set);
    return GetStorageUpdates(binding_set, buffer_set, image_set);
}
// Perform write update in given update struct
//  If an error occurs, return false and fill in details in error_msg string
bool cvdescriptorset::DescriptorSet::WriteUpdate(debug_report_data *report_data, const VkWriteDescriptorSet *update,
                                                 std::string *error_msg) {
    auto num_updates = 0;
    // Verify dst binding exists
    if (!p_layout_->HasBinding(update->dstBinding)) {
        std::stringstream error_str;
        error_str << "DescriptorSet " << set_ << " does not have binding " << update->dstBinding << ".";
        *error_msg = error_str.str();
        return false;
    } else {
        // We know that binding is valid, verify update and do update on each descriptor
        auto start_idx = p_layout_->GetGlobalStartIndexFromBinding(update->dstBinding) + update->dstArrayElement;
        auto type = p_layout_->GetTypeFromBinding(update->dstBinding);
        if (type != update->descriptorType) {
            std::stringstream error_str;
            error_str << "Attempting write update to descriptor set " << set_ << " binding #" << update->dstBinding << " with type "
                      << string_VkDescriptorType(type) << " but update type is " << string_VkDescriptorType(update->descriptorType);
            *error_msg = error_str.str();
            return false;
        }
        if ((start_idx + update->descriptorCount) > p_layout_->GetTotalDescriptorCount()) {
            std::stringstream error_str;
            error_str << "Attempting write update to descriptor set " << set_ << " binding #" << update->dstBinding << " with "
                      << p_layout_->GetTotalDescriptorCount() << " total descriptors but update of " << update->descriptorCount
                      << " descriptors starting at binding offset of "
                      << p_layout_->GetGlobalStartIndexFromBinding(update->dstBinding)
                      << " combined with update array element offset of " << update->dstArrayElement
                      << " oversteps the size of this descriptor set.";
            *error_msg = error_str.str();
            return false;
        }
        // Verify consecutive bindings match (if needed)
        if (!p_layout_->VerifyUpdateConsistency(update->dstBinding, update->dstArrayElement, update->descriptorCount,
                                                "write update to", set_, error_msg))
            return false;
        // Update is within bounds and consistent so perform update
        for (uint32_t di = 0; di < update->descriptorCount; ++di) {
            // TODO : Can we break this into a set-level "Validate" followed by Descriptor updating itself
            //  if the validate passes? That saves us all the map ptrs in each descriptor instance
            if (!descriptors_[start_idx + di]->WriteUpdate(update, di, error_msg)) {
                std::stringstream error_str;
                error_str << "Write update to descriptor at global index " << start_idx + di << " within set " << set_
                          << " binding #" << update->dstBinding << " failed with error message: " << error_msg->c_str();
                *error_msg = error_str.str();
                return false;
            }
            ++num_updates;
        }
    }
    if (num_updates != 0) {
        some_update_ = true;
    }
    return true;
}
// Copy update
bool cvdescriptorset::DescriptorSet::CopyUpdate(debug_report_data *report_data, const VkCopyDescriptorSet *update,
                                                const DescriptorSet *src_set, std::string *error) {
    auto num_updates = 0;
    if (!p_layout_->HasBinding(update->dstBinding)) {
        std::stringstream error_str;
        error_str << "DescriptorSet " << set_ << " does not have copy update dest binding of " << update->dstBinding << ".";
        *error = error_str.str();
        return false;
    }
    if (!src_set->HasBinding(update->srcBinding)) {
        std::stringstream error_str;
        error_str << "DescriptorSet " << set_ << " does not have copy update src binding of " << update->srcBinding << ".";
        *error = error_str.str();
        return false;
    }
    // src & dst set bindings are valid
    // Check bounds of src & dst
    auto src_start_idx = src_set->GetGlobalStartIndexFromBinding(update->srcBinding) + update->srcArrayElement;
    if ((src_start_idx + update->descriptorCount) > src_set->GetTotalDescriptorCount()) {
        // SRC update out of bounds
        std::stringstream error_str;
        error_str << "Attempting copy update from descriptorSet " << update->srcSet << " binding#" << update->srcBinding
                  << " with offset index of " << src_set->GetGlobalStartIndexFromBinding(update->srcBinding)
                  << " plus update array offset of " << update->srcArrayElement << " and update of " << update->descriptorCount
                  << " descriptors oversteps total number of descriptors in set: " << src_set->GetTotalDescriptorCount() << ".";
        *error = error_str.str();
        return false;
    }
    auto dst_start_idx = p_layout_->GetGlobalStartIndexFromBinding(update->dstBinding) + update->dstArrayElement;
    if ((dst_start_idx + update->descriptorCount) > p_layout_->GetTotalDescriptorCount()) {
        // DST update out of bounds
        std::stringstream error_str;
        error_str << "Attempting copy update to descriptorSet " << set_ << " binding#" << update->dstBinding
                  << " with offset index of " << p_layout_->GetGlobalStartIndexFromBinding(update->dstBinding)
                  << " plus update array offset of " << update->dstArrayElement << " and update of " << update->descriptorCount
                  << " descriptors oversteps total number of descriptors in set: " << p_layout_->GetTotalDescriptorCount() << ".";
        *error = error_str.str();
        return false;
    }
    // Check that types match
    auto src_type = src_set->GetTypeFromBinding(update->srcBinding);
    auto dst_type = p_layout_->GetTypeFromBinding(update->dstBinding);
    if (src_type != dst_type) {
        std::stringstream error_str;
        error_str << "Attempting copy update to descriptorSet " << set_ << " binding #" << update->dstBinding << " with type "
                  << string_VkDescriptorType(dst_type) << " from descriptorSet " << src_set->GetSet() << " binding #"
                  << update->srcBinding << " with type " << string_VkDescriptorType(src_type) << ". Types do not match.";
        *error = error_str.str();
        return false;
    }
    // Verify consistency of src & dst bindings if update crosses binding boundaries
    if ((!src_set->GetLayout()->VerifyUpdateConsistency(update->srcBinding, update->srcArrayElement, update->descriptorCount,
                                                        "copy update from", src_set->GetSet(), error)) ||
        (!p_layout_->VerifyUpdateConsistency(update->dstBinding, update->dstArrayElement, update->descriptorCount, "copy update to",
                                             set_, error))) {
        return false;
    }
    // Update parameters all look good so perform update
    for (uint32_t di = 0; di < update->descriptorCount; ++di) {
        if (!descriptors_[dst_start_idx]->CopyUpdate(src_set->descriptors_[src_start_idx].get(), error))
            return false;
        ++num_updates;
    }
    if (num_updates != 0) {
        some_update_ = true;
    }
    return true;
}
cvdescriptorset::SamplerDescriptor::SamplerDescriptor(
    const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *sampler_map)
    : sampler_(VK_NULL_HANDLE), immutable_(false), sampler_map_(sampler_map) {
    updated = false;
    descriptor_class = PlainSampler;
};

cvdescriptorset::SamplerDescriptor::SamplerDescriptor(
    const VkSampler *immut, const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *sampler_map)
    : sampler_(VK_NULL_HANDLE), immutable_(false), sampler_map_(sampler_map) {
    updated = false;
    descriptor_class = PlainSampler;
    if (immut) {
        sampler_ = *immut;
        immutable_ = true;
        updated = true;
    }
}
bool cvdescriptorset::ValidateSampler(const VkSampler sampler,
                                      const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *sampler_map) {
    return (sampler_map->count(sampler) != 0);
}
bool cvdescriptorset::ValidateImageUpdate(const VkImageView image_view, const VkImageLayout image_layout,
                                          const std::unordered_map<VkImageView, VkImageViewCreateInfo> *image_view_map,
                                          const std::unordered_map<VkImage, IMAGE_NODE> *image_map,
                                          const std::unordered_map<VkImage, VkSwapchainKHR> *image_to_swapchain_map,
                                          const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *swapchain_map,
                                          std::string *error) {
    auto image_pair = image_view_map->find(image_view);
    if (image_pair == image_view_map->end()) {
        std::stringstream error_str;
        error_str << "Invalid VkImageView: " << image_view;
        *error = error_str.str();
        return false;
    } else {
        // Validate that imageLayout is compatible with aspect_mask and image format
        VkImageAspectFlags aspect_mask = image_pair->second.subresourceRange.aspectMask;
        VkImage image = image_pair->second.image;
        VkFormat format = VK_FORMAT_MAX_ENUM;
        auto img_pair = image_map->find(image);
        if (img_pair != image_map->end()) {
            format = img_pair->second.createInfo.format;
        } else {
            // Also need to check the swapchains.
            auto swapchain_pair = image_to_swapchain_map->find(image);
            if (swapchain_pair != image_to_swapchain_map->end()) {
                VkSwapchainKHR swapchain = swapchain_pair->second;
                auto swapchain_pair = swapchain_map->find(swapchain);
                if (swapchain_pair != swapchain_map->end()) {
                    format = swapchain_pair->second->createInfo.imageFormat;
                }
            }
        }
        if (format == VK_FORMAT_MAX_ENUM) {
            std::stringstream error_str;
            error_str << "Invalid image (" << image << ") in imageView (" << image_view << ").";
            *error = error_str.str();
            return false;
        } else {
            bool ds = vk_format_is_depth_or_stencil(format);
            switch (image_layout) {
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                // Only Color bit must be set
                if ((aspect_mask & VK_IMAGE_ASPECT_COLOR_BIT) != VK_IMAGE_ASPECT_COLOR_BIT) {
                    std::stringstream error_str;
                    error_str << "ImageView (" << image_view << ") uses layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL but does "
                                                                "not have VK_IMAGE_ASPECT_COLOR_BIT set.";
                    *error = error_str.str();
                    return false;
                }
                // format must NOT be DS
                if (ds) {
                    std::stringstream error_str;
                    error_str << "ImageView (" << image_view
                              << ") uses layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL but the image format is "
                              << string_VkFormat(format) << " which is not a color format.";
                    *error = error_str.str();
                    return false;
                }
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                // Depth or stencil bit must be set, but both must NOT be set
                if (aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT) {
                    if (aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT) {
                        // both  must NOT be set
                        std::stringstream error_str;
                        error_str << "ImageView (" << image_view << ") has both STENCIL and DEPTH aspects set";
                        *error = error_str.str();
                        return false;
                    }
                } else if (!(aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT)) {
                    // Neither were set
                    std::stringstream error_str;
                    error_str << "ImageView (" << image_view << ") has layout " << string_VkImageLayout(image_layout)
                              << " but does not have STENCIL or DEPTH aspects set";
                    *error = error_str.str();
                    return false;
                }
                // format must be DS
                if (!ds) {
                    std::stringstream error_str;
                    error_str << "ImageView (" << image_view << ") has layout " << string_VkImageLayout(image_layout)
                              << " but the image format is " << string_VkFormat(format) << " which is not a depth/stencil format.";
                    *error = error_str.str();
                    return false;
                }
                break;
            default:
                // anything to check for other layouts?
                break;
            }
        }
    }
    return true;
}
bool cvdescriptorset::SamplerDescriptor::WriteUpdate(const VkWriteDescriptorSet *update, const uint32_t index, std::string *error) {
    if (!immutable_) {
        if (!ValidateSampler(update->pImageInfo[index].sampler, sampler_map_)) {
            std::stringstream error_str;
            error_str << "Attempted write update to sampler descriptor with invalid sampler: " << update->pImageInfo[index].sampler
                      << ".";
            *error = error_str.str();
            return false;
        }
        sampler_ = update->pImageInfo[index].sampler;
    } else {
        if (!ValidateSampler(sampler_, sampler_map_)) {
            std::stringstream error_str;
            error_str << "Attempted write update to immutable sampler descriptor which has invalid immutable sampler: " << sampler_
                      << ".";
            *error = error_str.str();
            return false;
        }
        // TODO : Do we want a way to warn here in case of updating immutable sampler (which can't be updated)?
    }
    updated = true;
    return true;
}

bool cvdescriptorset::SamplerDescriptor::CopyUpdate(const Descriptor *src, std::string *error) {
    if (!immutable_) {
        auto update_sampler = static_cast<const SamplerDescriptor *>(src)->sampler_;
        if (!ValidateSampler(update_sampler, sampler_map_)) {
            std::stringstream error_str;
            error_str << "Attempted copy update to sampler descriptor with invalid sampler: " << update_sampler << ".";
            *error = error_str.str();
            return false;
        }
        sampler_ = update_sampler;
    } else {
        if (!ValidateSampler(sampler_, sampler_map_)) {
            std::stringstream error_str;
            error_str << "Attempted copy update to immutable sampler descriptor which has invalid immutable sampler: " << sampler_
                      << ".";
            *error = error_str.str();
            return false;
        }
        // TODO : Do we want a way to warn here in case of updating immutable sampler (which can't be updated)?
    }
    updated = true;
    return true;
}
cvdescriptorset::ImageSamplerDescriptor::ImageSamplerDescriptor(
    const std::unordered_map<VkImageView, VkImageViewCreateInfo> *image_view_map,
    const std::unordered_map<VkImage, IMAGE_NODE> *image_map,
    const std::unordered_map<VkImage, VkSwapchainKHR> *image_to_swapchain_map,
    const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *swapchain_map,
    const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *sampler_map)
    : sampler_(VK_NULL_HANDLE), immutable_(false), image_view_(VK_NULL_HANDLE), image_layout_(VK_IMAGE_LAYOUT_UNDEFINED),
      sampler_map_(sampler_map), image_view_map_(image_view_map), image_map_(image_map),
      image_to_swapchain_map_(image_to_swapchain_map), swapchain_map_(swapchain_map) {
    updated = false;
    descriptor_class = ImageSampler;
}

cvdescriptorset::ImageSamplerDescriptor::ImageSamplerDescriptor(
    const VkSampler *immut, const std::unordered_map<VkImageView, VkImageViewCreateInfo> *image_view_map,
    const std::unordered_map<VkImage, IMAGE_NODE> *image_map,
    const std::unordered_map<VkImage, VkSwapchainKHR> *image_to_swapchain_map,
    const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *swapchain_map,
    const std::unordered_map<VkSampler, std::unique_ptr<SAMPLER_NODE>> *sampler_map)
    : sampler_(VK_NULL_HANDLE), immutable_(true), image_view_(VK_NULL_HANDLE), image_layout_(VK_IMAGE_LAYOUT_UNDEFINED),
      sampler_map_(sampler_map), image_view_map_(image_view_map), image_map_(image_map),
      image_to_swapchain_map_(image_to_swapchain_map), swapchain_map_(swapchain_map) {
    updated = false;
    descriptor_class = ImageSampler;
    if (immut) {
        sampler_ = *immut;
        immutable_ = true;
        updated = true;
    }
}
bool cvdescriptorset::ImageSamplerDescriptor::WriteUpdate(const VkWriteDescriptorSet *update, const uint32_t index,
                                                          std::string *error) {
    // First check sampler
    if (!immutable_) {
        if (!ValidateSampler(update->pImageInfo[index].sampler, sampler_map_)) {
            std::stringstream error_str;
            error_str << "Attempted write update to combined image sampler descriptor with invalid sampler: "
                      << update->pImageInfo[index].sampler << ".";
            *error = error_str.str();
            return false;
        }
        sampler_ = update->pImageInfo[index].sampler;
    } else {
        if (!ValidateSampler(sampler_, sampler_map_)) {
            std::stringstream error_str;
            error_str << "Attempted write update to combined image sampler descriptor with immutable sampler which has invalid "
                         "immutable sampler: "
                      << sampler_ << ".";
            *error = error_str.str();
            return false;
        }
        // TODO : Do we want a way to warn here in case of updating immutable sampler (which can't be updated)?
    }
    // Now validate images
    auto image_view = update->pImageInfo[index].imageView;
    auto image_layout = update->pImageInfo[index].imageLayout;
    if (!ValidateImageUpdate(image_view, image_layout, image_view_map_, image_map_, image_to_swapchain_map_, swapchain_map_,
                             error)) {
        std::stringstream error_str;
        error_str << "Attempted write update to combined image sampler descriptor failed due to: " << error->c_str();
        *error = error_str.str();
        return false;
    }
    updated = true;
    image_view_ = image_view;
    image_layout_ = image_layout;
    return true;
}

bool cvdescriptorset::ImageSamplerDescriptor::CopyUpdate(const Descriptor *src, std::string *error) {
    if (!immutable_) {
        auto update_sampler = static_cast<const ImageSamplerDescriptor *>(src)->sampler_;
        if (!ValidateSampler(update_sampler, sampler_map_)) {
            std::stringstream error_str;
            error_str << "Attempted copy update to combined image sampler descriptor with invalid sampler: " << update_sampler
                      << ".";
            *error = error_str.str();
            return false;
        }
        sampler_ = update_sampler;
    } else {
        if (!ValidateSampler(sampler_, sampler_map_)) {
            std::stringstream error_str;
            error_str << "Attempted copy update to combined image sampler descriptor with immutable sampler that has invalid "
                         "immutable sampler: "
                      << sampler_ << ".";
            *error = error_str.str();
            return false;
        }
        // TODO : Do we want a way to warn here in case of updating immutable sampler (which can't be updated)?
    }
    // Now validate images
    auto image_view = static_cast<const ImageSamplerDescriptor *>(src)->image_view_;
    auto image_layout = static_cast<const ImageSamplerDescriptor *>(src)->image_layout_;
    if (!ValidateImageUpdate(image_view, image_layout, image_view_map_, image_map_, image_to_swapchain_map_, swapchain_map_,
                             error)) {
        std::stringstream error_str;
        error_str << "Attempted copy update to combined image sampler descriptor failed due to: " << error->c_str();
        *error = error_str.str();
        return false;
    }
    updated = true;
    image_view_ = image_view;
    image_layout_ = image_layout;
    return true;
}

cvdescriptorset::ImageDescriptor::ImageDescriptor(const VkDescriptorType type,
                                                  const std::unordered_map<VkImageView, VkImageViewCreateInfo> *image_view_map,
                                                  const std::unordered_map<VkImage, IMAGE_NODE> *image_map,
                                                  const std::unordered_map<VkImage, VkSwapchainKHR> *image_to_swapchain_map,
                                                  const std::unordered_map<VkSwapchainKHR, SWAPCHAIN_NODE *> *swapchain_map)
    : storage_(false), image_view_(VK_NULL_HANDLE), image_layout_(VK_IMAGE_LAYOUT_UNDEFINED), image_view_map_(image_view_map),
      image_map_(image_map), image_to_swapchain_map_(image_to_swapchain_map), swapchain_map_(swapchain_map) {
    updated = false;
    descriptor_class = Image;
    if (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE == type)
        storage_ = true;
};

bool cvdescriptorset::ImageDescriptor::WriteUpdate(const VkWriteDescriptorSet *update, const uint32_t index, std::string *error) {
    // First validate that the write update is valid
    auto image_view = update->pImageInfo[index].imageView;
    auto image_layout = update->pImageInfo[index].imageLayout;
    if (!ValidateImageUpdate(image_view, image_layout, image_view_map_, image_map_, image_to_swapchain_map_, swapchain_map_,
                             error)) {
        std::stringstream error_str;
        error_str << "Attempted write update to image descriptor failed due to: " << error->c_str();
        *error = error_str.str();
        return false;
    }
    // Update is clean so process it
    updated = true;
    image_view_ = image_view;
    image_layout_ = image_layout;
    return true;
}

bool cvdescriptorset::ImageDescriptor::CopyUpdate(const Descriptor *src, std::string *error) {
    // First validate update
    auto image_view = static_cast<const ImageDescriptor *>(src)->image_view_;
    auto image_layout = static_cast<const ImageDescriptor *>(src)->image_layout_;
    if (!ValidateImageUpdate(image_view, image_layout, image_view_map_, image_map_, image_to_swapchain_map_, swapchain_map_,
                             error)) {
        std::stringstream error_str;
        error_str << "Attempted copy update to image descriptor failed due to: " << error->c_str();
        *error = error_str.str();
        return false;
    }
    updated = true;
    image_view_ = image_view;
    image_layout_ = image_layout;
    return true;
}

cvdescriptorset::BufferDescriptor::BufferDescriptor(const VkDescriptorType type,
                                                    const std::unordered_map<VkBuffer, BUFFER_NODE> *buffer_map)
    : storage_(false), dynamic_(false), buffer_(VK_NULL_HANDLE), offset_(0), range_(0), buffer_map_(buffer_map) {
    updated = false;
    descriptor_class = GeneralBuffer;
    if (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC == type) {
        dynamic_ = true;
    } else if (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER == type) {
        storage_ = true;
    } else if (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC == type) {
        dynamic_ = true;
        storage_ = true;
    }
}
bool cvdescriptorset::BufferDescriptor::WriteUpdate(const VkWriteDescriptorSet *update, const uint32_t index, std::string *error) {
    // First validate bufferinfo
    auto buffer = update->pBufferInfo[index].buffer;
    if (!buffer_map_->count(buffer)) {
        std::stringstream error_str;
        error_str << "Attempted write update to buffer descriptor with invalid buffer: " << buffer;
        *error = error_str.str();
        return false;
    }
    updated = true;
    buffer_ = buffer;
    offset_ = update->pBufferInfo[index].offset;
    range_ = update->pBufferInfo[index].range;
    return true;
}

bool cvdescriptorset::BufferDescriptor::CopyUpdate(const Descriptor *src, std::string *error) {
    // First validate bufferinfo
    auto buffer = static_cast<const BufferDescriptor *>(src)->buffer_;
    if (!buffer_map_->count(buffer)) {
        std::stringstream error_str;
        error_str << "Attempted copy update to buffer descriptor with invalid buffer: " << buffer;
        *error = error_str.str();
        return false;
    }
    updated = true;
    buffer_ = buffer;
    offset_ = static_cast<const BufferDescriptor *>(src)->offset_;
    range_ = static_cast<const BufferDescriptor *>(src)->range_;
    return true;
}

cvdescriptorset::TexelDescriptor::TexelDescriptor(const VkDescriptorType type,
                                                  const std::unordered_map<VkBufferView, VkBufferViewCreateInfo> *buffer_view_map)
    : buffer_view_(VK_NULL_HANDLE), storage_(false), buffer_view_map_(buffer_view_map) {
    updated = false;
    descriptor_class = TexelBuffer;
    if (VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER == type)
        storage_ = true;
};
bool cvdescriptorset::TexelDescriptor::WriteUpdate(const VkWriteDescriptorSet *update, const uint32_t index, std::string *error) {
    // First validate buffer view
    auto buffer_view = update->pTexelBufferView[index];
    if (!buffer_view_map_->count(buffer_view)) {
        std::stringstream error_str;
        error_str << "Attempted write update to texel buffer descriptor with invalid buffer view: " << buffer_view;
        *error = error_str.str();
        return false;
    }
    updated = true;
    buffer_view_ = buffer_view;
    return true;
}

bool cvdescriptorset::TexelDescriptor::CopyUpdate(const Descriptor *src, std::string *error) {
    // First validate buffer view
    auto buffer_view = static_cast<const TexelDescriptor *>(src)->buffer_view_;
    if (!buffer_view_map_->count(buffer_view)) {
        std::stringstream error_str;
        error_str << "Attempted write update to texel buffer descriptor with invalid buffer view: " << buffer_view;
        *error = error_str.str();
        return false;
    }
    updated = true;
    buffer_view_ = buffer_view;
    return true;
}