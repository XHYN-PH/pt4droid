#pragma once
#include "engine/render/vk.h"

namespace pt {
// Scope is one render pass. Reset before another recorder/pipeline owns the state.
class MeshDrawState {
public:
    void Reset(){valid_=false;}
    void Bind(VkCommandBuffer cmd,VkBuffer vertices,VkBuffer indices,VkCullModeFlags cull,VkFrontFace front) {
        if(!valid_ || cull_!=cull)vkCmdSetCullMode(cmd,cull);
        if(!valid_ || front_!=front)vkCmdSetFrontFace(cmd,front);
        if(!valid_ || vertices_!=vertices){VkDeviceSize offset=0;vkCmdBindVertexBuffers(cmd,0,1,&vertices,&offset);}
        if(!valid_ || indices_!=indices)vkCmdBindIndexBuffer(cmd,indices,0,VK_INDEX_TYPE_UINT32);
        vertices_=vertices;indices_=indices;cull_=cull;front_=front;valid_=true;
    }
private:
    bool valid_=false;
    VkBuffer vertices_=VK_NULL_HANDLE,indices_=VK_NULL_HANDLE;
    VkCullModeFlags cull_=VK_CULL_MODE_NONE;
    VkFrontFace front_=VK_FRONT_FACE_COUNTER_CLOCKWISE;
};
}
