#pragma once
#include <algorithm>
#include <array>
#include <vector>
#include <vulkan/vulkan.h>
namespace pt::android {
// Swappy writes into caller-owned strings; allocating just char* entries is insufficient.
template<class Determine>
auto FramePacingExtensions(VkPhysicalDevice physical,uint32_t available_count,VkExtensionProperties* available,Determine determine){
    uint32_t count=0;determine(physical,available_count,available,&count,nullptr);
    std::vector<std::array<char,VK_MAX_EXTENSION_NAME_SIZE+1>> names;
    if(count==0||count>32)return names;
    names.resize(count);std::vector<char*> pointers;
    for(auto& name:names)pointers.push_back(name.data());
    determine(physical,available_count,available,&count,pointers.data());
    names.resize(std::min(count,static_cast<uint32_t>(names.size())));
    return names;
}
}
