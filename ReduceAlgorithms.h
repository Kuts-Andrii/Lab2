#pragma once

#include <execution>
#include <numeric>
#include <utility>

#include "DataGenerator.h"

class ReduceAlgorithms {
public:
    static Int noPolicy(const Data& data) {
        return std::reduce(data.begin(), data.end(), Int{0});
    }

    template <typename Policy>
    static Int withPolicy(const Data& data, Policy&& policy) {
        return std::reduce(std::forward<Policy>(policy), data.begin(), data.end(), Int{0});
    }

    static Int customParallel(const Data& data, int parts);
};