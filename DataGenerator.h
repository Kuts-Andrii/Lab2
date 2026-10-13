#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using Int = std::int64_t;
using Data = std::vector<Int>;

// File format: line 1 - N (number of elements), next - N integers separated by spaces.
class DataGenerator {
public:
    static constexpr std::uint64_t kSeed = 42;
    static constexpr Int kMinValue = 1;
    static constexpr Int kMaxValue = 100;

    static Data generate(std::size_t size);
    static void saveToFile(const Data& data, const std::string& filename);
    static Data loadFromFile(const std::string& filename);
};