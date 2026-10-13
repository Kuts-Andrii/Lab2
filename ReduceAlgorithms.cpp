#include "ReduceAlgorithms.h"

#include <algorithm>
#include <cstddef>
#include <system_error>
#include <thread>
#include <vector>

Int ReduceAlgorithms::customParallel(const Data& data, int parts) {
    const std::size_t n = data.size();
    if (n == 0) return 0;

    std::size_t k = parts < 1 ? 1 : static_cast<std::size_t>(parts);
    k = std::min(k, n);

    const std::size_t base = n / k;
    const std::size_t extra = n % k;

    Data partial(k, 0);
    std::vector<std::thread> workers;
    workers.reserve(k);

    std::size_t offset = 0;
    for (std::size_t i = 0; i < k; ++i) {
        const std::size_t len = base + (i < extra ? 1 : 0);
        const auto first = data.begin() + static_cast<std::ptrdiff_t>(offset);
        const auto last = first + static_cast<std::ptrdiff_t>(len);
        try {
            workers.emplace_back([&partial, first, last, i] {
                partial[i] = std::reduce(std::execution::seq, first, last, Int{0});
            });
        } catch (const std::system_error&) {
            partial[i] = std::reduce(std::execution::seq, first, last, Int{0});
        }
        offset += len;
    }
    for (auto& w : workers) w.join();

    return std::reduce(std::execution::seq, partial.begin(), partial.end(), Int{0});
}