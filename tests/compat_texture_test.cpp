#include "engine/assets/compatible_texture.h"
#include "engine/assets/bc_texture.h"
#include <cassert>
#include <fstream>
#include <future>
#include <random>
#include <cstdio>

int main() {
    using namespace pt;
    const auto root=std::filesystem::temp_directory_path()/"pt13-conversion-regression";
    std::filesystem::remove_all(root);
    std::array<bool,16> native{};
    std::vector<uint8_t> red{0,248,0,248,0,0,0,0};BcSourceMip source{4,4,red};
    native.fill(true);ConfigureBcConversion(native,root,512);
    TextureReadBudget native_budget(0);
    assert(!PrepareBcMips(VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&source,1},1,&native_budget));
    assert(native_budget.Used()==0 && native_budget.Rejected()==0 && !std::filesystem::exists(root));
    native.fill(false);ConfigureBcConversion(native,root,0);
    TextureReadBudget short_budget(63);assert(!PrepareBcMips(VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&source,1},1,&short_budget));
    assert(short_budget.Used()==0 && short_budget.Rejected()==1);
    auto first=PrepareBcMips(VK_FORMAT_BC1_RGBA_SRGB_BLOCK,{&source,1},1);
    assert(first && first->format==VK_FORMAT_R8G8B8A8_SRGB && first->bytes==64 && !first->cache_file.empty());
    for(size_t i=0;i<first->mips[0].size();i+=4)assert(first->mips[0][i]==255 && first->mips[0][i+1]==0 && first->mips[0][i+2]==0 && first->mips[0][i+3]==255);
    auto cached=ReadBcCache(first->cache_file);assert(cached && cached->mips==first->mips);
    // Concurrent requests publish complete files, never a partial payload.
    std::vector<std::future<std::shared_ptr<const BcMipChain>>> workers;
    for(int i=0;i<4;++i)workers.push_back(std::async(std::launch::async,[&]{return PrepareBcMips(VK_FORMAT_BC1_RGBA_SRGB_BLOCK,{&source,1},1);}));
    for(auto& worker:workers){auto result=worker.get();assert(result && result->mips==first->mips);}
    // Odd edges, signed channels and alpha remain byte-identical to the decoder.
    std::mt19937 random(13015);
    for(int format=VK_FORMAT_BC1_RGB_UNORM_BLOCK;format<=VK_FORMAT_BC7_SRGB_BLOCK;++format){
        const VkFormat f=VkFormat(format);if(BcFallbackFormat(f)==VK_FORMAT_UNDEFINED)continue;
        const bool eight=format<=VK_FORMAT_BC1_RGBA_SRGB_BLOCK || f==VK_FORMAT_BC4_UNORM_BLOCK || f==VK_FORMAT_BC4_SNORM_BLOCK;
        for(auto [w,h]:{std::pair{1u,1u},{3u,5u},{8u,4u}}){
            std::vector<uint8_t> bytes(((w+3)/4)*((h+3)/4)*(eight?8:16));for(auto& b:bytes)b=uint8_t(random());
            BcSourceMip mip{w,h,bytes};std::vector<uint8_t> expected;assert(DecodeBcMip(f,w,h,bytes,expected));
            auto actual=PrepareBcMips(f,{&mip,1},1);assert(actual && actual->mips[0]==expected && actual->format==BcFallbackFormat(f));
        }
    }
    // Preserve face order and all tail levels, including sub-block dimensions.
    std::vector<BcSourceMip> cube;for(int face=0;face<6;++face)for(uint32_t side:{4u,2u,1u})cube.push_back({side,side,red});
    auto six=PrepareBcMips(VK_FORMAT_BC1_RGBA_UNORM_BLOCK,cube,6);assert(six && six->layers==6 && six->levels==3 && six->mips.size()==18);
    auto six_cached=ReadBcCache(six->cache_file);assert(six_cached && six_cached->mips==six->mips);
    // Explicit size limit selects existing mips, with no resizing or re-encoding.
    std::vector<uint8_t> top(256*256*8),bottom(128*128*8);std::vector<BcSourceMip> big{{1024,1024,top},{512,512,bottom}};
    ConfigureBcConversion(native,root,512);auto limited=PrepareBcMips(VK_FORMAT_BC1_RGBA_UNORM_BLOCK,big,1);
    assert(limited && limited->width==512 && limited->height==512 && limited->levels==1 && limited->bytes==512*512*4);
    ConfigureBcConversion(native,root,0);auto original=PrepareBcMips(VK_FORMAT_BC1_RGBA_UNORM_BLOCK,big,1);
    assert(original && original->width==1024 && original->levels==2 && original->cache_file!=limited->cache_file);
    // Cache corruption, stale versions, wrong length and hostile headers fail closed.
    std::ifstream in(first->cache_file,std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(in)),{});
    const auto bad=root/"bad.ptc";
    auto write=[&](const auto& data){std::ofstream f(bad,std::ios::binary|std::ios::trunc);f.write(data.data(),data.size());};
    auto corrupt=bytes;corrupt.back()^=1;write(corrupt);assert(!ReadBcCache(bad));
    corrupt=bytes;corrupt[4]=99;write(corrupt);assert(!ReadBcCache(bad));
    corrupt=bytes;corrupt.resize(39);write(corrupt);assert(!ReadBcCache(bad));
    corrupt=bytes;corrupt.push_back(0);write(corrupt);assert(!ReadBcCache(bad));
    corrupt=bytes;for(int i=12;i<16;++i)corrupt[i]=char(255);write(corrupt);assert(!ReadBcCache(bad));
    assert(!PrepareBcMips(VK_FORMAT_BC6H_UFLOAT_BLOCK,{&source,1},1));
    assert(!PrepareBcMips(VK_FORMAT_BC1_RGBA_UNORM_BLOCK,cube,2));
    std::filesystem::remove_all(root);
    puts("Compatibility conversion/cache checks passed: native bypass, byte equality, signed channels, alpha, mip/cube layout, bounds, concurrency and corruption");
}
