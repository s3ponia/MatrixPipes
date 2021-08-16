//
// Created by slava on 16.08.2021.
//

#ifndef MATRIXPIPE_FLATMAP_H
#define MATRIXPIPE_FLATMAP_H

#include <cstddef>
#include <array>

template<class KeyType, class ValueType, std::size_t MaxKeyPairs = 10>
class FlatMap {
public:
    constexpr FlatMap() = default;

    template<std::size_t N>
    constexpr explicit FlatMap(const std::pair<KeyType, ValueType> (&init_arr)[N]) : count(N) {
        static_assert(N <= MaxKeyPairs);

        for (std::size_t i = 0; i < N; ++i) {
            keys[i] = init_arr[i].first;
            values[i] = init_arr[i].second;
        }
    }

    constexpr const ValueType &operator[](const KeyType &key) const {
        for (std::size_t i = 0; i < count; ++i) {
            if (key == keys[i]) {
                return values[i];
            }
        }
        throw std::runtime_error{"value is not found"};
    }

private:
    std::array<KeyType, MaxKeyPairs> keys;
    std::array<ValueType, MaxKeyPairs> values;
    std::size_t count{};
};


#endif //MATRIXPIPE_FLATMAP_H
