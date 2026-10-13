// Lab 2, Andrii Kuts, K-28, Compiler: g++-15, C++20

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "DataGenerator.h"
#include "ReduceAlgorithms.h"

namespace {
const std::vector<std::size_t> kSizes = {100'000, 1'000'000, 10'000'000};
const std::string kDataDir = "data";
constexpr int kRuns = 5;
constexpr int kDefaultHardwareThreads = 4;
constexpr int kLargeK[] = {128, 256, 512, 1024};
constexpr int kNameWidth = 26;
constexpr int kColWidth = 12;
}

// Пишем сразу в консоль и в файл
class Tee {
public:
    Tee(std::ostream& a, std::ostream& b) : a_(a), b_(b) {}

    template <typename T>
    Tee& operator<<(const T& v) {
        a_ << v;
        b_ << v;
        return *this;
    }

private:
    std::ostream& a_;
    std::ostream& b_;
};

struct BenchResult {
    std::size_t size;
    std::string name;
    int threads;
    bool custom;
    double timeMs;
    Int value;
    bool correct;
};

class ExperimentRunner {
public:
    static std::vector<BenchResult> runAll(const Data& data, int hardwareThreads) {
        const Int expected = std::accumulate(data.begin(), data.end(), Int{0});
        std::vector<BenchResult> results;
        runStandard(data, expected, results);
        runCustom(data, expected, hardwareThreads, results);
        return results;
    }

    static std::vector<int> buildKValues(int hw) {
        std::set<int> ks = {1, 2, 4, std::max(1, hw / 2), hw, hw * 2, hw * 4};
        ks.insert(std::begin(kLargeK), std::end(kLargeK));
        return {ks.begin(), ks.end()};
    }

    static const BenchResult* bestCustom(const std::vector<BenchResult>& results) {
        const BenchResult* best = nullptr;
        for (const auto& r : results)
            if (r.custom && (best == nullptr || r.timeMs < best->timeMs)) best = &r;
        return best;
    }

private:
    static BenchResult measure(const std::string& name, std::size_t size, int threads, bool custom,
                               const std::function<Int()>& task, Int expected) {
        volatile Int sink = task();
        (void)sink;

        std::vector<double> times;
        times.reserve(kRuns);
        Int result = 0;
        for (int i = 0; i < kRuns; ++i) {
            const auto start = std::chrono::steady_clock::now();
            result = task();
            const auto stop = std::chrono::steady_clock::now();
            times.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
        }
        std::sort(times.begin(), times.end());
        return {size, name, threads, custom, times[times.size() / 2], result, result == expected};
    }

    static void runStandard(const Data& data, Int expected, std::vector<BenchResult>& out) {
        const auto n = data.size();
        out.push_back(measure("std::reduce (no policy)", n, 1, false,
                              [&] { return ReduceAlgorithms::noPolicy(data); }, expected));
        out.push_back(measure("std::reduce (seq)", n, 1, false,
                              [&] { return ReduceAlgorithms::withPolicy(data, std::execution::seq); }, expected));
        out.push_back(measure("std::reduce (unseq)", n, 1, false,
                              [&] { return ReduceAlgorithms::withPolicy(data, std::execution::unseq); }, expected));
        out.push_back(measure("std::reduce (par)", n, 0, false,
                              [&] { return ReduceAlgorithms::withPolicy(data, std::execution::par); }, expected));
        out.push_back(measure("std::reduce (par_unseq)", n, 0, false,
                              [&] { return ReduceAlgorithms::withPolicy(data, std::execution::par_unseq); }, expected));
    }

    static void runCustom(const Data& data, Int expected, int hw, std::vector<BenchResult>& out) {
        for (int k : buildKValues(hw)) {
            out.push_back(measure("custom K=" + std::to_string(k), data.size(), k, true,
                                  [&] { return ReduceAlgorithms::customParallel(data, k); }, expected));
        }
    }
};

class ReportWriter {
public:
    // Теперь шаблон — принимает и std::ostream, и Tee
    template <typename Stream>
    static void printEnvironment(Stream& os, int hw, const std::string& label) {
#if defined(_MSC_VER)
        os << "Compiler: MSVC " << _MSC_FULL_VER << "\n";
#else
        os << "Compiler: " << __VERSION__ << "\n";
#endif
        os << "Run: " << label << ", hardware threads: " << hw << "\n"
           << "Time = median of " << kRuns
           << " runs of one reduce call, ms (data loading not included)\n";
    }

    template <typename Stream>
    static void printTable(Stream& os, std::size_t size, const std::vector<BenchResult>& results) {
        const auto* best = ExperimentRunner::bestCustom(results);
        os << "\nSequence size: " << size << "\n"
           << std::left << std::setw(kNameWidth) << "Algorithm"
           << std::setw(kColWidth) << "Threads"
           << std::setw(kColWidth) << "Time, ms"
           << std::setw(kColWidth) << "x best K"
           << std::setw(kColWidth) << "Result" << "Check\n";

        for (const auto& r : results) {
            os << std::left << std::setw(kNameWidth) << r.name
               << std::setw(kColWidth) << threadsLabel(r)
               << std::setw(kColWidth) << std::fixed << std::setprecision(3) << r.timeMs;
            if (r.custom && best != nullptr)
                os << std::setw(kColWidth) << std::setprecision(2) << r.timeMs / best->timeMs;
            else
                os << std::setw(kColWidth) << "-";
            os << std::setw(kColWidth) << r.value << (r.correct ? "OK" : "ERROR") << "\n";
        }
    }

    template <typename Stream>
    static void printBestK(Stream& os, const std::vector<BenchResult>& results, int hw) {
        const auto* best = ExperimentRunner::bestCustom(results);
        if (best == nullptr) return;
        os << "Best K = " << best->threads << " (" << std::fixed << std::setprecision(3)
           << best->timeMs << " ms), hardware threads = " << hw
           << ", K / hardware threads = " << std::setprecision(2)
           << static_cast<double>(best->threads) / hw << "\n";
    }

    // Отдельный файл с таблицей — обычный std::ostream
    static void saveResults(const std::vector<BenchResult>& results, const std::string& filename) {
        std::ofstream out(filename);
        out << std::left << std::setw(kNameWidth) << "Algorithm"
            << std::setw(kColWidth) << "Threads"
            << std::setw(kColWidth) << "Time_ms"
            << std::setw(kColWidth) << "Result"
            << "Check\n";
        for (const auto& r : results) {
            out << std::left << std::setw(kNameWidth) << r.name
                << std::setw(kColWidth) << threadsLabel(r)
                << std::setw(kColWidth) << std::fixed << std::setprecision(3) << r.timeMs
                << std::setw(kColWidth) << r.value
                << (r.correct ? "OK" : "ERROR") << "\n";
        }
    }

private:
    static std::string threadsLabel(const BenchResult& r) {
        return r.threads == 0 ? "auto" : std::to_string(r.threads);
    }
};

static int detectHardwareThreads() {
    const unsigned n = std::thread::hardware_concurrency();
    return n == 0 ? kDefaultHardwareThreads : static_cast<int>(n);
}

static Data loadOrGenerate(std::size_t size) {
    const std::string path = kDataDir + "/data_" + std::to_string(size) + ".txt";
    if (!std::filesystem::exists(path)) {
        std::cerr << "Generating " << size << " elements -> " << path << "\n";
        DataGenerator::saveToFile(DataGenerator::generate(size), path);
    }
    return DataGenerator::loadFromFile(path);
}

int main(int argc, char** argv) {
    try {
        const std::string label = argc > 1 ? argv[1] : "run";
        const int hw = detectHardwareThreads();

        std::filesystem::create_directories(kDataDir);

        // Полный лог: консоль + out_<label>.txt
        std::ofstream log("out_" + label + ".txt");
        Tee out(std::cout, log);

        ReportWriter::printEnvironment(out, hw, label);

        std::vector<BenchResult> all;
        for (std::size_t size : kSizes) {
            const auto data = loadOrGenerate(size);
            const auto results = ExperimentRunner::runAll(data, hw);
            ReportWriter::printTable(out, size, results);
            ReportWriter::printBestK(out, results, hw);
            all.insert(all.end(), results.begin(), results.end());
        }

        // Таблица результатов в отдельный txt
        const std::string resultsFile = "results_" + label + ".txt";
        ReportWriter::saveResults(all, resultsFile);
        out << "\nDone. Results saved to " << resultsFile << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}