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

#pragma once
#include "core_validation_error_enums.h"
#include "vk_layer_logging.h"
#include "vk_safe_struct.h"
#include "vulkan/vk_layer.h"
#include <unordered_map>
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
class DescriptorSetLayout {
  public:
    // Constructors and destructor
    DescriptorSetLayout();
    DescriptorSetLayout(debug_report_data *report_data, const VkDescriptorSetLayoutCreateInfo *p_create_info,
                        const VkDescriptorSetLayout layout);
    ~DescriptorSetLayout();
    // Straightforward Get functions
    VkDescriptorSetLayout GetDescriptorSetLayout() { return layout_; };
    uint32_t GetTotalDescriptorCount() { return descriptor_count_; };
    uint32_t GetDynamicDescriptorCount() { return dynamic_descriptor_count_; };
    uint32_t GetBindingCount() { return binding_count_; };
    // Return true if given binding is present in this layout
    bool HasBinding(const uint32_t binding) { return binding_to_index_map_.count(binding) > 0; };
    // Return true if this layout is compatible with passed in layout,
    //   else return false and update error_msg with description of incompatibility
    bool IsCompatible(DescriptorSetLayout *, string *error_msg);
    // Various Get functions that can either be passed a binding#, which will
    //  be automatically translated into the appropriate index from the original
    //  pBindings array, or the index# can be passed in directly
    VkDescriptorSetLayoutBinding const *GetDescriptorSetLayoutBindingPtrFromBinding(const uint32_t);
    VkDescriptorSetLayoutBinding const *GetDescriptorSetLayoutBindingPtrFromIndex(const uint32_t);
    uint32_t GetDescriptorCountFromBinding(const uint32_t);
    uint32_t GetDescriptorCountFromIndex(const uint32_t);
    VkDescriptorType GetTypeFromBinding(const uint32_t);
    VkDescriptorType GetTypeFromIndex(const uint32_t);
    VkDescriptorType GetTypeFromGlobalIndex(const uint32_t);
    VkShaderStageFlags GetStageFlagsFromBinding(const uint32_t);
    VkSampler const *GetImmutableSamplerPtrFromBinding(const uint32_t);
    // For a particular binding, get the global index
    uint32_t GetGlobalStartIndexFromBinding(const uint32_t);
    uint32_t GetGlobalEndIndexFromBinding(const uint32_t);

  private:
    VkDescriptorSetLayout layout_;
    unordered_map<uint32_t, uint32_t> binding_to_index_map_;
    unordered_map<uint32_t, uint32_t> binding_to_global_start_index_map_;
    unordered_map<uint32_t, uint32_t> binding_to_global_end_index_map_;
    //VkDescriptorSetLayoutCreateFlags flags_;
    uint32_t binding_count_; // # of bindings in this layout
    vector<safe_VkDescriptorSetLayoutBinding *> bindings_;
    uint32_t descriptor_count_; // total # descriptors in this layout
    uint32_t dynamic_descriptor_count_;
};
DescriptorSetLayout::DescriptorSetLayout()
    : layout_(VK_NULL_HANDLE), /*flags_(0),*/ binding_count_(0), descriptor_count_(0), dynamic_descriptor_count_(0) {}
// Construct DescriptorSetLayout instance from given create info
DescriptorSetLayout::DescriptorSetLayout(debug_report_data *report_data, const VkDescriptorSetLayoutCreateInfo *p_create_info,
                                         const VkDescriptorSetLayout layout)
    : layout_(layout), /*flags_(p_create_info->flags),*/ binding_count_(p_create_info->bindingCount), descriptor_count_(0),
      dynamic_descriptor_count_(0) {
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
        bindings_.push_back(new safe_VkDescriptorSetLayoutBinding(&p_create_info->pBindings[i]));
        // In cases where we should ignore pImmutableSamplers make sure it's NULL
        if ((p_create_info->pBindings[i].pImmutableSamplers) &&
            ((p_create_info->pBindings[i].descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) &&
             (p_create_info->pBindings[i].descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER))) {
            bindings_.back()->pImmutableSamplers = nullptr;
        }
        if (p_create_info->pBindings[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
            p_create_info->pBindings[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            dynamic_descriptor_count_++;
        }
    }
}
DescriptorSetLayout::~DescriptorSetLayout() {
    for (auto binding : bindings_)
        delete binding;
};

VkDescriptorSetLayoutBinding const *DescriptorSetLayout::GetDescriptorSetLayoutBindingPtrFromBinding(const uint32_t binding) {
    if (!binding_to_index_map_.count(binding))
        return nullptr;
    return bindings_[binding_to_index_map_[binding]]->ptr();
}
VkDescriptorSetLayoutBinding const *DescriptorSetLayout::GetDescriptorSetLayoutBindingPtrFromIndex(const uint32_t index) {
    if (index >= bindings_.size())
        return nullptr;
    return bindings_[index]->ptr();
}
// Return descriptorCount for given binding, 0 if index is unavailable
uint32_t DescriptorSetLayout::GetDescriptorCountFromBinding(const uint32_t binding) {
    if (!binding_to_index_map_.count(binding))
        return 0;
    return bindings_[binding_to_index_map_[binding]]->descriptorCount;
}
// Return descriptorCount for given index, 0 if index is unavailable
uint32_t DescriptorSetLayout::GetDescriptorCountFromIndex(const uint32_t index) {
    if (index >= bindings_.size())
        return 0;
    return bindings_[index]->descriptorCount;
}
// For the given binding, return descriptorType
VkDescriptorType DescriptorSetLayout::GetTypeFromBinding(const uint32_t binding) {
    assert(binding_to_index_map_.count(binding));
    return bindings_[binding_to_index_map_[binding]]->descriptorType;
}
// For the given index, return descriptorType
VkDescriptorType DescriptorSetLayout::GetTypeFromIndex(const uint32_t index) {
    assert(index < bindings_.size());
    return bindings_[index]->descriptorType;
}
// For the given global index, return descriptorType
//  Currently just counting up through bindings_, may improve this in future
VkDescriptorType DescriptorSetLayout::GetTypeFromGlobalIndex(const uint32_t index) {
    uint32_t global_offset = 0;
    for (auto binding : bindings_) {
        global_offset += binding->descriptorCount;
        if (index < global_offset)
            return binding->descriptorType;
    }
    assert(0); // requested global index is out of bounds
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}
// For the given binding, return stageFlags
VkShaderStageFlags DescriptorSetLayout::GetStageFlagsFromBinding(const uint32_t binding) {
    assert(binding_to_index_map_.count(binding));
    return bindings_[binding_to_index_map_[binding]]->stageFlags;
}
// For the given binding, return start index
uint32_t DescriptorSetLayout::GetGlobalStartIndexFromBinding(const uint32_t binding) {
    assert(binding_to_global_start_index_map_.count(binding));
    return binding_to_global_start_index_map_[binding];
}
// For the given binding, return end index
uint32_t DescriptorSetLayout::GetGlobalEndIndexFromBinding(const uint32_t binding) {
    assert(binding_to_global_end_index_map_.count(binding));
    return binding_to_global_end_index_map_[binding];
}
//
VkSampler const *DescriptorSetLayout::GetImmutableSamplerPtrFromBinding(const uint32_t binding) {
    assert(binding_to_index_map_.count(binding));
    return bindings_[binding_to_index_map_[binding]]->pImmutableSamplers;
}
// If our layout is compatible with rh_ds_layout, return true,
//  else return false and fill in error_msg will description of what causes incompatibility
bool DescriptorSetLayout::IsCompatible(DescriptorSetLayout *rh_ds_layout, string *error_msg) {
    // Trivial case
    if (layout_ == rh_ds_layout->GetDescriptorSetLayout())
        return true;
    if (descriptor_count_ != rh_ds_layout->descriptor_count_) {
        stringstream error_str;
        error_str << "DescriptorSetLayout " << layout_ << " has " << descriptor_count_ << " descriptors, but DescriptorSetLayout "
                  << rh_ds_layout->GetDescriptorSetLayout() << " has " << rh_ds_layout->descriptor_count_ << " descriptors.";
        *error_msg = error_str.str();
        return false; // trivial fail case
    }
    // Descriptor counts match so need to go through bindings one-by-one
    //  and verify that type and stageFlags match
    for (auto binding : bindings_) {
        // TODO : Do we also need to check immutable samplers?
        if (binding->descriptorCount != rh_ds_layout->GetDescriptorCountFromBinding(binding->binding)) {
            stringstream error_str;
            error_str << "Binding " << binding->binding << " for DescriptorSetLayout " << layout_ << " has a descriptorCount of "
                      << binding->descriptorCount << " but binding " << binding->binding << " for DescriptorSetLayout "
                      << rh_ds_layout->GetDescriptorSetLayout() << " has a descriptorCount of "
                      << rh_ds_layout->GetDescriptorCountFromBinding(binding->binding);
            *error_msg = error_str.str();
            return false;
        } else if (binding->descriptorType != rh_ds_layout->GetTypeFromBinding(binding->binding)) {
            stringstream error_str;
            error_str << "Binding " << binding->binding << " for DescriptorSetLayout " << layout_ << " is type '"
                      << string_VkDescriptorType(binding->descriptorType) << "' but binding " << binding->binding
                      << " for DescriptorSetLayout " << rh_ds_layout->GetDescriptorSetLayout() << " is type '"
                      << string_VkDescriptorType(rh_ds_layout->GetTypeFromBinding(binding->binding)) << "'";
            *error_msg = error_str.str();
            return false;
        } else if (binding->stageFlags != rh_ds_layout->GetStageFlagsFromBinding(binding->binding)) {
            stringstream error_str;
            error_str << "Binding " << binding->binding << " for DescriptorSetLayout " << layout_ << " has stageFlags "
                      << binding->stageFlags << " but binding " << binding->binding << " for DescriptorSetLayout "
                      << rh_ds_layout->GetDescriptorSetLayout() << " has stageFlags "
                      << rh_ds_layout->GetStageFlagsFromBinding(binding->binding);
            *error_msg = error_str.str();
            return false;
        }
    }
    return true;
}
#endif // CORE_VALIDATION_DESCRIPTOR_SETS_H_
