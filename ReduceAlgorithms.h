#pragma once

#include <execution>
#include <numeric>
#include <utility>

#include "DataGenerator.h"

namespace ReduceAlgorithms {

    inline Int noPolicy(const Data& data) {
        return std::reduce(data.begin(), data.end(), Int{0});
    }

    template <typename Policy>
    Int withPolicy(const Data& data, Policy&& policy) {
        return std::reduce(std::forward<Policy>(policy), data.begin(), data.end(), Int{0});
    }

    // K chunks in K threads, then sum of partials
    Int customParallel(const Data& data, int parts);

}
