#include "engine/assets/bc_texture.h"
#include <algorithm>
#include <array>
#include <cstring>
#define BCDEC_STATIC
#define BCDEC_BC4BC5_PRECISE
#define BCDEC_IMPLEMENTATION
#include "../../../third_party/bcdec/bcdec.h"
namespace pt {
VkFormat BcFallbackFormat(VkFormat f){switch(f){
case VK_FORMAT_BC1_RGB_UNORM_BLOCK:case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:case VK_FORMAT_BC2_UNORM_BLOCK:case VK_FORMAT_BC3_UNORM_BLOCK:case VK_FORMAT_BC7_UNORM_BLOCK:return VK_FORMAT_R8G8B8A8_UNORM;
case VK_FORMAT_BC1_RGB_SRGB_BLOCK:case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:case VK_FORMAT_BC2_SRGB_BLOCK:case VK_FORMAT_BC3_SRGB_BLOCK:case VK_FORMAT_BC7_SRGB_BLOCK:return VK_FORMAT_R8G8B8A8_SRGB;
case VK_FORMAT_BC4_UNORM_BLOCK:return VK_FORMAT_R8_UNORM;case VK_FORMAT_BC4_SNORM_BLOCK:return VK_FORMAT_R8_SNORM;
case VK_FORMAT_BC5_UNORM_BLOCK:return VK_FORMAT_R8G8_UNORM;case VK_FORMAT_BC5_SNORM_BLOCK:return VK_FORMAT_R8G8_SNORM;default:return VK_FORMAT_UNDEFINED;}}
bool DecodeBcMip(VkFormat f,uint32_t width,uint32_t height,std::span<const uint8_t> data,std::vector<uint8_t>& out){
 const auto dst=BcFallbackFormat(f);if(!width||!height||width>16384||height>16384||dst==VK_FORMAT_UNDEFINED)return false;
 const uint32_t channels=(dst==VK_FORMAT_R8_UNORM||dst==VK_FORMAT_R8_SNORM)?1u:(dst==VK_FORMAT_R8G8_UNORM||dst==VK_FORMAT_R8G8_SNORM)?2u:4u;
 const bool bc1=f>=VK_FORMAT_BC1_RGB_UNORM_BLOCK && f<=VK_FORMAT_BC1_RGBA_SRGB_BLOCK,bc4=f==VK_FORMAT_BC4_UNORM_BLOCK || f==VK_FORMAT_BC4_SNORM_BLOCK;
 const size_t block_bytes=(bc1||bc4)?8:16,bw=(size_t(width)+3)/4,bh=(size_t(height)+3)/4;const uint64_t bytes=uint64_t(width)*height*channels;
 if(data.size()!=bw*bh*block_bytes || bytes>128ull*1024*1024)return false;out.resize(size_t(bytes));
 for(size_t by=0;by<bh;++by)for(size_t bx=0;bx<bw;++bx){alignas(8)std::array<uint64_t,2> input{};std::memcpy(input.data(),data.data()+(by*bw+bx)*block_bytes,block_bytes);alignas(8)uint8_t pixels[64]{};
  if(bc1)bcdec_bc1(input.data(),pixels,16);else if(f==VK_FORMAT_BC2_UNORM_BLOCK||f==VK_FORMAT_BC2_SRGB_BLOCK)bcdec_bc2(input.data(),pixels,16);else if(f==VK_FORMAT_BC3_UNORM_BLOCK||f==VK_FORMAT_BC3_SRGB_BLOCK)bcdec_bc3(input.data(),pixels,16);else if(bc4)bcdec_bc4(input.data(),pixels,4,f==VK_FORMAT_BC4_SNORM_BLOCK);else if(f==VK_FORMAT_BC5_UNORM_BLOCK||f==VK_FORMAT_BC5_SNORM_BLOCK)bcdec_bc5(input.data(),pixels,8,f==VK_FORMAT_BC5_SNORM_BLOCK);else bcdec_bc7(input.data(),pixels,16);
  if(f==VK_FORMAT_BC1_RGB_UNORM_BLOCK||f==VK_FORMAT_BC1_RGB_SRGB_BLOCK)for(size_t i=0;i<16;++i)pixels[i*4+3]=255;
  for(size_t row=0;row<4 && by*4+row<height;++row){const size_t count=std::min<size_t>(4,width-bx*4);std::memcpy(out.data()+((by*4+row)*width+bx*4)*channels,pixels+row*4*channels,count*channels);}
 }return true;
}
}
