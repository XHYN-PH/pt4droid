#pragma once

#include <array>
#include <cstddef>

namespace pt {

// Same first N elements as a stable sort, with bounded storage. Equal candidates
// retain their source order, including the tie at the last selected element.
template <typename T, size_t N> class StableTop {
public:
    template <typename Before> void Insert(const T& value, Before before) {
        size_t at = 0;
        while (at < size_ && !before(value, values_[at])) ++at;
        if (at == N) return;
        const size_t end = size_ < N ? size_++ : N - 1;
        for (size_t i = end; i > at; --i) values_[i] = values_[i - 1];
        values_[at] = value;
    }
    size_t Size() const { return size_; }
    const T& operator[](size_t i) const { return values_[i]; }

private:
    static_assert(N > 0);
    std::array<T, N> values_{};
    size_t size_ = 0;
};

}  // namespace pt
