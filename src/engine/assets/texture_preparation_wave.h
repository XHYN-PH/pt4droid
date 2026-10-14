#pragma once
#include "engine/assets/ftex.h"
#include "engine/core/parallel_for.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pt {
using PreparedTextureWave = std::vector<std::pair<std::string, std::shared_ptr<FtexTexture>>>;
// Only the two readers allocate payloads. The shared budget covers every live
// wave; a lease survives adoption, and releases after the last raw/sRGB user.
template<class Load>
PreparedTextureWave PrepareTextureWave(const std::vector<std::string>& paths,
                                      const std::shared_ptr<TextureReadBudget>& budget, Load&& load) {
    PreparedTextureWave results(paths.size());
    ParallelChunks(paths.size(), 2, [&](size_t, size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            auto lease = std::make_shared<TextureReadBudget>(96ull * 1024 * 1024, budget);
            auto ftex = std::shared_ptr<FtexTexture>(new FtexTexture, [lease](FtexTexture* value) { delete value; });
            if (load(paths[i], *ftex, lease.get())) results[i] = {paths[i], std::move(ftex)};
        }
    });
    return results;
}
}
