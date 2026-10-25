#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <future>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "engine/assets/ftex.h"
#include "engine/assets/texture_preparation_stream.h"
#include "engine/core/log.h"
#include "engine/core/pathcode.h"
#include "engine/fs/mods.h"

namespace pt {
bool LogEnabled() { return false; }
void LogWrite(LogLevel, std::string_view) {}
static std::unordered_map<uint64_t, std::vector<uint8_t>> files;
static std::atomic<uint32_t> archive_reads{0}, stream_reads{0};
static uint64_t stream_code;
QarArchive::~QarArchive() {}
std::optional<std::vector<uint8_t>> QarArchive::Read(uint64_t code) const {
    ++archive_reads;
    if (code == stream_code) ++stream_reads;
    auto it = files.find(code);
    return it == files.end() ? std::nullopt : std::optional(it->second);
}
namespace mods {
const ModSet* Active() { return nullptr; }
std::optional<std::vector<uint8_t>> ReadOverride(std::string_view) { return std::nullopt; }
}
struct TextureMip { uint32_t width, height; std::span<const uint8_t> data; };
bool EnhancedTextureEligible(const std::string&, const FtexTexture&) { return false; }
uint32_t FormatBlockBytes(VkFormat, bool&);
struct TextureManager {
    static constexpr uint32_t kWhite = 0;
    struct Upload { VkFormat format; std::vector<std::vector<uint8_t>> bytes; };
    std::unordered_map<std::string, uint32_t> by_name_;
    std::unordered_map<std::string, std::shared_future<PreparedTextureResult>> decoding_;
    std::unordered_map<std::string, std::shared_ptr<const FtexTexture>> prepared_;
    std::unordered_map<uint32_t, std::string> fox_sources_;
    std::vector<Upload> uploads;
    std::unordered_map<std::string, std::string> failed_textures_;
    std::deque<std::string> decode_order_;
    std::unique_ptr<TexturePreparationStream> decode_stream_;
    struct PendingPreparation { std::shared_future<PreparedTextureResult> future; std::function<void(const std::string&)> prioritize; };
    std::unordered_map<std::string, PendingPreparation> pending_preparations_;
    double read_ms_ = 0, upload_ms_ = 0;
    struct ReadStats { uint64_t prepared = 0, ahead = 0, direct = 0, stream_ready = 0, budget_fallback = 0, read_fallback = 0, unprepared = 0; double stream_wait_ms = 0; } read_stats_;
    std::unordered_set<std::string> worker_prepared_;
    std::unordered_map<std::string, uint8_t> prepared_variants_;
    bool enhanced_enabled_ = false;
    uint32_t Create(const std::string& name, VkFormat format, std::span<const TextureMip> mips, uint32_t, bool, std::shared_ptr<const BcMipChain>) {
        Upload upload{format, {}};
        for (const auto& mip : mips) upload.bytes.emplace_back(mip.data.begin(), mip.data.end());
        uploads.push_back(std::move(upload));
        return by_name_[name] = uint32_t(uploads.size());
    }
    uint32_t LoadModImage(const QarArchive&, const std::string&, const std::string&, const std::vector<uint8_t>&, bool) { return 0; }
    void LoadEnhancedTexture(uint32_t, const std::string&, const FtexTexture*) {}
    uint32_t LoadFox(const QarArchive&, const std::string&, bool* = nullptr, bool = false);
    void DecodeAhead(const QarArchive&, const std::vector<std::string>&);
    uint32_t PumpDecoded(const QarArchive&, uint32_t);
    void StopDecodeWorkers();
    bool AdoptDecoded(const std::string&, std::shared_ptr<FtexTexture>);
    void DropDecoded(const std::vector<std::string>&);
};
template<class T> void Put(std::vector<uint8_t>& out, size_t at, T value) { std::memcpy(out.data() + at, &value, sizeof(value)); }
}  // namespace pt

int main() {
    using namespace pt;
    const std::string stem = "/Assets/synthetic";
    std::vector<uint8_t> header(0x50);
    std::memcpy(header.data(), "FTEX", 4);
    Put<uint16_t>(header, 8, 2); Put<uint16_t>(header, 10, 4); Put<uint16_t>(header, 12, 4);
    Put<uint16_t>(header, 14, 1); Put<uint8_t>(header, 16, 1); Put<uint32_t>(header, 28, 2);
    Put<uint32_t>(header, 0x44, 8); Put<uint8_t>(header, 0x4D, 1);
    const std::vector<uint8_t> red{0, 248, 0, 248, 0, 0, 0, 0};
    files[PathCode64(stem + ".ftex")] = header;
    stream_code = PathCode64(stem + ".1.ftexs"); files[stream_code] = red;
    QarArchive qar;
    TextureReadBudget small(7); FtexTexture rejected;
    assert(!LoadFtex(qar, stem, rejected, &small));
    assert(small.Used() == 0 && stream_reads == 0 && rejected.mips.empty());
    TextureReadBudget exact(8); auto prepared = std::make_shared<FtexTexture>();
    assert(LoadFtex(qar, stem, *prepared, &exact) && exact.Used() == 8 && stream_reads == 1);
    assert(prepared->mips[0] == red && prepared->Format() == VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
    assert(prepared->Format(true) == VK_FORMAT_BC1_RGBA_UNORM_BLOCK && prepared->Srgb());
    for (bool raw_first : {false, true}) {
        TextureManager manager; assert(manager.AdoptDecoded(stem, prepared));
        const uint32_t before = archive_reads;
        bool ok = false;
        auto a = manager.LoadFox(qar, stem, &ok, raw_first); assert(ok);
        auto b = manager.LoadFox(qar, stem, &ok, !raw_first); assert(ok && a != b);
        assert(archive_reads == before && manager.uploads.size() == 2);
        assert(manager.uploads[raw_first ? 1 : 0].format == VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
        assert(manager.uploads[raw_first ? 0 : 1].format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
        for (const auto& upload : manager.uploads) assert(upload.bytes[0] == red);
        assert(prepared->Srgb() && prepared->mips[0] == red);
        assert(manager.LoadFox(qar, stem, &ok, raw_first) == a && archive_reads == before);
        manager.DropDecoded({stem}); assert(manager.prepared_.empty());
    }
    {
        TextureManager manager;
        std::promise<PreparedTextureResult> slow;
        manager.decoding_["slow"] = slow.get_future().share();
        {
            std::promise<PreparedTextureResult> fast;
            manager.decoding_[stem] = fast.get_future().share();
            fast.set_value({prepared, TexturePreparationFailure::None});
        }
        manager.decode_order_ = {"slow", stem};
        manager.prepared_variants_[stem] = 1;
        const auto reads = archive_reads.load();
        assert(manager.PumpDecoded(qar, 1) == 1);
        assert(manager.by_name_.contains(stem) && manager.decoding_.contains("slow"));
        assert(manager.prepared_.empty() && manager.worker_prepared_.empty() && archive_reads == reads);
        slow.set_value({{}, TexturePreparationFailure::Read});
        assert(manager.PumpDecoded(qar, 1) == 0 && manager.decode_order_.empty());
    }
    {
        TextureManager manager;
        manager.prepared_variants_[stem] = 1;
        manager.DecodeAhead(qar, {stem, stem});
        assert(manager.decoding_.size() == 1 && manager.decode_order_.size() == 1);
        assert(manager.decoding_.at(stem).wait_for(std::chrono::seconds(3)) == std::future_status::ready);
        const auto reads = archive_reads.load();
        assert(manager.PumpDecoded(qar, 1) == 1 && archive_reads == reads);
        assert(manager.prepared_.empty() && manager.worker_prepared_.empty());
        manager.StopDecodeWorkers();
    }
    {
        TextureManager manager;
        std::promise<PreparedTextureResult> failed;
        failed.set_value({{},TexturePreparationFailure::Read});
        manager.pending_preparations_["/Assets/missing"]={failed.get_future().share(),[](const std::string&){}};
        const auto before=archive_reads.load();bool ok=true;
        assert(manager.LoadFox(qar,"/Assets/missing",&ok)==TextureManager::kWhite&&!ok);
        assert(archive_reads==before); // Worker failure + absent archive header is final.
        assert(manager.failed_textures_.at("/Assets/missing")=="missing FTEX header");
        assert(manager.LoadFox(qar,"/Assets/missing",&ok)==TextureManager::kWhite&&!ok&&archive_reads==before);
    }
    TextureReadBudget concurrent(96000); std::atomic<uint32_t> successes{0};
    std::vector<std::thread> workers;
    for (int i = 0; i < 8; ++i) workers.emplace_back([&] { for (int n = 0; n < 1000; ++n) if (concurrent.Reserve(1000)) ++successes; });
    for (auto& worker : workers) worker.join();
    assert(successes == 96 && concurrent.Used() == 96000 && !concurrent.Reserve(UINT64_MAX));
    header.resize(0x40); files[PathCode64(stem + ".ftex")] = header;
    TextureReadBudget malformed(1024); auto before = stream_reads.load();
    assert(!LoadFtex(qar, stem, rejected, &malformed) && malformed.Used() == 0 && stream_reads == before);
    puts("Production texture preparation passed: pre-stream budget rejection, concurrent limits, immutable raw/sRGB sharing, byte equality and malformed headers");
}
