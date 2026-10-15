#include "engine/assets/compatible_texture.h"
#include "engine/assets/bc_texture.h"
#include <zlib.h>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace pt {
namespace {
constexpr uint64_t kMaxBytes = 128ull * 1024 * 1024;
constexpr uint32_t kMagic = 0x31435450; // PTC1, independent of enhanced-texture caches
struct Policy { std::array<bool,16> native{}; std::filesystem::path cache; uint32_t max_side = 0; };
std::shared_ptr<const Policy> policy;
std::mutex cache_mutex;
uint32_t Channels(VkFormat f) {
    return f == VK_FORMAT_R8_UNORM || f == VK_FORMAT_R8_SNORM ? 1 :
        f == VK_FORMAT_R8G8_UNORM || f == VK_FORMAT_R8G8_SNORM ? 2 : 4;
}
bool IsBc(VkFormat f) { return f >= VK_FORMAT_BC1_RGB_UNORM_BLOCK && f <= VK_FORMAT_BC7_SRGB_BLOCK; }
void Hash(uint64_t& h, std::span<const uint8_t> bytes) { for (uint8_t b : bytes) { h ^= b; h *= 1099511628211ull; } }
bool WriteCache(const std::filesystem::path& path, const BcMipChain& chain) {
    if (path.empty()) return false;
    std::lock_guard lock(cache_mutex);
    std::error_code ec; std::filesystem::create_directories(path.parent_path(), ec); if (ec) return false;
    uint32_t crc=0;for(const auto& mip:chain.mips)crc=crc32(crc,mip.data(),mip.size());
    uint32_t header[] = {kMagic,1,uint32_t(chain.format),chain.width,chain.height,chain.levels,chain.layers,uint32_t(chain.bytes),0,crc};
    auto temp=path;temp+=".partial";
    std::ofstream out(temp,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(header),sizeof(header));
    z_stream stream{};if(deflateInit(&stream,Z_BEST_SPEED)!=Z_OK){out.close();std::filesystem::remove(temp,ec);return false;}
    std::array<uint8_t,64*1024> buffer{};bool ok=true;
    for(size_t i=0;i<chain.mips.size() && ok;++i){
        const auto& mip=chain.mips[i];stream.next_in=const_cast<Bytef*>(mip.data());stream.avail_in=uInt(mip.size());
        const int flush=i+1==chain.mips.size()?Z_FINISH:Z_NO_FLUSH;
        int result=Z_OK;do{
            stream.next_out=buffer.data();stream.avail_out=buffer.size();result=deflate(&stream,flush);
            if(result!=Z_OK && result!=Z_STREAM_END){ok=false;break;}
            out.write(reinterpret_cast<const char*>(buffer.data()),buffer.size()-stream.avail_out);
        }while(stream.avail_in || stream.avail_out==0 || (flush==Z_FINISH && result!=Z_STREAM_END));
    }
    header[8]=uint32_t(stream.total_out);deflateEnd(&stream);
    out.seekp(0);out.write(reinterpret_cast<const char*>(header),sizeof(header));out.flush();ok=ok&&out.good();out.close();
    if(!ok){std::filesystem::remove(temp,ec);return false;}
    std::filesystem::rename(temp, path, ec);
    if (ec) { std::filesystem::remove(path, ec); ec.clear(); std::filesystem::rename(temp, path, ec); }
    if (ec) { std::filesystem::remove(temp, ec); return false; }
    return true;
}
}
void ConfigureBcConversion(std::array<bool,16> native, std::filesystem::path cache, uint32_t max_side) {
    if(std::all_of(native.begin(),native.end(),[](bool supported){return supported;})){
        auto next=std::make_shared<Policy>();next->native=native;
        std::atomic_store(&policy,std::shared_ptr<const Policy>(std::move(next)));return;
    }
    // Reclaim old files only before the manager starts. Live residency backing
    // files are never removed under a texture slot during this session.
    std::error_code ec; uint64_t bytes = 0;
    std::vector<std::filesystem::directory_entry> files;
    for (std::filesystem::directory_iterator it(cache,ec), end; !ec && it!=end; it.increment(ec)) {
        if (!it->is_regular_file(ec) || it->path().extension() != ".ptc") continue;
        auto size = it->file_size(ec); if (!ec) { bytes += size; files.push_back(*it); }
    }
    if (bytes > 1024ull*1024*1024) {
        std::sort(files.begin(),files.end(),[](const auto& a,const auto& b){return a.path()<b.path();});
        for (const auto& file:files) { if(bytes<=768ull*1024*1024) break; auto size=file.file_size(ec);
            if(!ec && std::filesystem::remove(file.path(),ec))bytes-=size; }
    }
    auto next=std::make_shared<Policy>();next->native=native;next->cache=std::move(cache);
    next->max_side=max_side==512||max_side==1024||max_side==2048?max_side:0;
    std::atomic_store(&policy,std::shared_ptr<const Policy>(std::move(next)));
}
std::shared_ptr<const BcMipChain> ReadBcCache(const std::filesystem::path& file) {
    std::ifstream in(file,std::ios::binary);uint32_t h[10]{};in.read(reinterpret_cast<char*>(h),sizeof(h));
    if(!in || h[0]!=kMagic || h[1]!=1 || !h[3] || !h[4] || h[3]>16384 || h[4]>16384 ||
       !h[5] || h[5]>std::bit_width(std::max(h[3],h[4])) || (h[6]!=1 && h[6]!=6) || !h[7] || h[7]>kMaxBytes || !h[8] || h[8]>compressBound(kMaxBytes))return {};
    const VkFormat f=VkFormat(h[2]);
    if(f!=VK_FORMAT_R8_UNORM && f!=VK_FORMAT_R8_SNORM && f!=VK_FORMAT_R8G8_UNORM && f!=VK_FORMAT_R8G8_SNORM && f!=VK_FORMAT_R8G8B8A8_UNORM && f!=VK_FORMAT_R8G8B8A8_SRGB)return {};
    uint64_t expected=0;for(uint32_t l=0;l<h[5];++l)expected+=uint64_t(std::max(1u,h[3]>>l))*std::max(1u,h[4]>>l)*Channels(f)*h[6];
    if(expected!=h[7])return {};
    // Reject truncation before allocating a payload described by a corrupt header.
    std::error_code ec;if(std::filesystem::file_size(file,ec)!=sizeof(h)+uint64_t(h[8]) || ec)return {};
    auto out=std::make_shared<BcMipChain>();out->format=f;out->width=h[3];out->height=h[4];out->levels=h[5];out->layers=h[6];out->bytes=expected;out->cache_file=file;
    z_stream stream{};if(inflateInit(&stream)!=Z_OK)return {};
    std::array<uint8_t,64*1024> packed{};uint32_t remaining=h[8],crc=0;bool ok=true;int result=Z_OK;
    for(uint32_t face=0;face<h[6] && ok;++face)for(uint32_t l=0;l<h[5] && ok;++l){
        size_t n=size_t(std::max(1u,h[3]>>l))*std::max(1u,h[4]>>l)*Channels(f);auto& mip=out->mips.emplace_back(n);
        stream.next_out=mip.data();stream.avail_out=uInt(n);
        while(stream.avail_out){
            if(stream.avail_in==0 && remaining){uint32_t take=std::min<uint32_t>(remaining,packed.size());in.read(reinterpret_cast<char*>(packed.data()),take);if(!in){ok=false;break;}remaining-=take;stream.next_in=packed.data();stream.avail_in=take;}
            const auto before=stream.total_out;result=inflate(&stream,Z_NO_FLUSH);
            if((result!=Z_OK && result!=Z_STREAM_END) || (result==Z_STREAM_END && stream.avail_out) || stream.total_out==before){ok=false;break;}
        }
        crc=crc32(crc,mip.data(),mip.size());
    }
    // A zlib stream may consume its trailer only after the last output byte.
    if(ok && result!=Z_STREAM_END){
        uint8_t extra{};stream.next_out=&extra;stream.avail_out=1;
        while(result==Z_OK){
            if(stream.avail_in==0 && remaining){uint32_t take=std::min<uint32_t>(remaining,packed.size());in.read(reinterpret_cast<char*>(packed.data()),take);remaining-=take;stream.next_in=packed.data();stream.avail_in=take;}
            result=inflate(&stream,Z_FINISH);if(stream.avail_out==0){ok=false;break;}
        }
    }
    ok=ok && result==Z_STREAM_END && stream.total_out==h[7] && stream.total_in==h[8] && crc==h[9];inflateEnd(&stream);
    if(!ok)return {};
    return out;
}
std::shared_ptr<const BcMipChain> PrepareBcMips(VkFormat format,std::span<const BcSourceMip> mips,uint32_t layers,TextureReadBudget* budget) {
    auto p=std::atomic_load(&policy);
    if(!p || !IsBc(format) || p->native[size_t(format-VK_FORMAT_BC1_RGB_UNORM_BLOCK)] || mips.empty() || (layers!=1&&layers!=6) || mips.size()%layers)return {};
    VkFormat fallback=BcFallbackFormat(format);if(fallback==VK_FORMAT_UNDEFINED)return {};
    const uint32_t levels=uint32_t(mips.size())/layers;
    if(!mips[0].width || !mips[0].height || mips[0].width>16384 || mips[0].height>16384 || levels>std::bit_width(std::max(mips[0].width,mips[0].height)))return {};
    uint32_t first=0;while(p->max_side && first+1<levels && std::max(mips[first].width,mips[first].height)>p->max_side)++first;
    uint64_t bytes=0,hash=14695981039346656037ull;
    const uint32_t info[]={1,uint32_t(format),mips[first].width,mips[first].height,levels-first,layers};
    Hash(hash,{reinterpret_cast<const uint8_t*>(info),sizeof(info)});
    for(uint32_t face=0;face<layers;++face)for(uint32_t l=first;l<levels;++l){const auto& mip=mips[face*levels+l];
        if(mip.width!=std::max(1u,mips[0].width>>l) || mip.height!=std::max(1u,mips[0].height>>l))return {};
        bytes+=uint64_t(mip.width)*mip.height*Channels(fallback);Hash(hash,mip.bytes);}
    if(bytes>kMaxBytes || (budget && !budget->Reserve(bytes)))return {};
    std::ostringstream name;name<<std::hex<<std::setw(16)<<std::setfill('0')<<hash<<".ptc";
    const auto file=p->cache/name.str();
    if(auto cached=ReadBcCache(file);cached && cached->format==fallback && cached->width==mips[first].width && cached->height==mips[first].height && cached->layers==layers && cached->levels==levels-first)return cached;
    auto out=std::make_shared<BcMipChain>();out->format=fallback;out->width=mips[first].width;out->height=mips[first].height;out->levels=levels-first;out->layers=layers;out->bytes=bytes;
    for(uint32_t face=0;face<layers;++face)for(uint32_t l=first;l<levels;++l){const auto& mip=mips[face*levels+l];auto& data=out->mips.emplace_back();if(!DecodeBcMip(format,mip.width,mip.height,mip.bytes,data))return {};}
    if(WriteCache(file,*out))out->cache_file=file;
    return out;
}
void PrepareBcTexture(FtexTexture& t,TextureReadBudget* budget) {
    auto p=std::atomic_load(&policy);const auto format=t.Format();
    if(!p || !IsBc(format) || p->native[size_t(format-VK_FORMAT_BC1_RGB_UNORM_BLOCK)] || t.depth!=1 || !t.mip_count || t.mips.size()!=size_t(t.mip_count)*t.faces)return;
    uint32_t first=0;while(first<t.mip_count && t.mips[first].empty())++first;
    std::vector<BcSourceMip> mips;for(uint32_t face=0;face<t.faces;++face)for(uint32_t l=first;l<t.mip_count;++l)mips.push_back({t.MipWidth(l),t.MipHeight(l),t.mips[face*t.mip_count+l]});
    t.compatible=PrepareBcMips(format,mips,t.faces,budget);
}
}
