#pragma once
#include <cmath>
namespace pt {
struct MobileBloomKernel { float radius; float fraction; };
inline MobileBloomKernel MobileBloomStep(int requested, int iterations, int i) {
    const int start=i*requested/iterations, end=(i+1)*requested/iterations;
    float variance=0.0f;
    for(int j=start;j<end;++j) { const float radius=j+0.5f; variance+=radius*radius; }
    return {std::sqrt(variance),static_cast<float>(end-start)/requested};
}
}
