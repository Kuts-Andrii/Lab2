// Lab 2, variant 13, Andrii Kuts, K-28, Compiler: g++-15, C++20

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "DataGenerator.h"
#include "ReduceAlgorithms.h"

namespace {
const std::vector<std::size_t> kSizes = {100'000, 1'000'000, 10'000'000};
constexpr int kRuns = 15;
constexpr int kDefaultHardwareThreads = 4;
constexpr int kMaxK = 1024;
// Thread count for par / par_unseq
constexpr int kAutoThreads = 0;
constexpr int kNameWidth = 26;
constexpr int kColWidth = 12;
}

struct BenchTask {
    std::string name;
    int threads;
    bool custom;
    std::function<Int()> run;
};

struct BenchResult {
    std::string name;
    int threads;
    bool custom;
    double timeMs;
    Int value;
    bool correct;
};

using Results = std::vector<BenchResult>;

class ExperimentRunner {
public:
    static Results runAll(const Data& data, int hw) {
        const Int expected = std::accumulate(data.begin(), data.end(), Int{0});
        Results results;
        for (const auto& task : buildTasks(data, hw)) {
            results.push_back(measure(task, expected));
        }
        return results;
    }

    static const BenchResult* bestCustom(const Results& results) {
        const BenchResult* best = nullptr;
        for (const auto& r : results) {
            if (r.custom && (best == nullptr || r.timeMs < best->timeMs)) best = &r;
        }
        return best;
    }

private:
    static std::vector<BenchTask> buildTasks(const Data& data, int hw) {
        namespace ex = std::execution;
        const auto noPolicy = [&data] { return ReduceAlgorithms::noPolicy(data); };
        const auto policy = [&data](auto p) {
            return [&data, p] { return ReduceAlgorithms::withPolicy(data, p); };
        };

        std::vector<BenchTask> tasks = {
            {"std::reduce (no policy)", 1, false, noPolicy},
            {"std::reduce (seq)", 1, false, policy(ex::seq)},
            {"std::reduce (unseq)", 1, false, policy(ex::unseq)},
            {"std::reduce (par)", kAutoThreads, false, policy(ex::par)},
            {"std::reduce (par_unseq)", kAutoThreads, false, policy(ex::par_unseq)},
        };

        for (int k : buildKValues(hw)) {
            const auto run = [&data, k] { return ReduceAlgorithms::customParallel(data, k); };
            tasks.push_back({"custom K=" + std::to_string(k), k, true, run});
        }
        return tasks;
    }

    // Powers of 2 and multiples of hw
    static std::vector<int> buildKValues(int hw) {
        std::set<int> ks = {std::max(1, hw / 2), hw, hw * 2, hw * 4};
        for (int k = 1; k <= kMaxK; k *= 2) ks.insert(k);
        return {ks.begin(), ks.end()};
    }

    // Warm-up, then median
    static BenchResult measure(const BenchTask& task, Int expected) {
        volatile Int sink = task.run();
        (void)sink;

        std::vector<double> times;
        times.reserve(kRuns);
        Int result = 0;
        bool correct = true;
        for (int i = 0; i < kRuns; ++i) {
            const auto start = std::chrono::steady_clock::now();
            result = task.run();
            const auto stop = std::chrono::steady_clock::now();
            times.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
            correct = correct && result == expected;
        }

        std::ranges::sort(times);
        const double median = times[times.size() / 2];
        return {task.name, task.threads, task.custom, median, result, correct};
    }
};

class ReportWriter {
public:
    static void printEnvironment(std::ostream& os, int hw) {
        os << "Compiler: " << __VERSION__ << "\n";
        os << "Build: " << buildInfo() << ", hardware threads: " << hw << "\n";
        os << "Time = median of " << kRuns << " runs of one reduce call, ms";
        os << " (data loading not included)\n";
    }

    // x best K = time / best K time
    static void printTable(std::ostream& os, std::size_t size, const Results& results) {
        const auto* best = ExperimentRunner::bestCustom(results);
        os << "\nSequence size: " << size << "\n";
        printHeader(os);
        for (const auto& r : results) printRow(os, r, best);
    }

    static void printBestK(std::ostream& os, const Results& results, int hw) {
        const auto* best = ExperimentRunner::bestCustom(results);
        if (best == nullptr) return;
        const double ratio = static_cast<double>(best->threads) / hw;
        os << "Best K = " << best->threads << " (" << formatNumber(best->timeMs, 3) << " ms)";
        os << ", hardware threads = " << hw;
        os << ", K / hardware threads = " << formatNumber(ratio, 2) << "\n";
    }

private:
    static std::string buildInfo() {
        std::string info = std::string("-") + OPT_LABEL;
#ifdef NDEBUG
        info += ", Release (NDEBUG)";
#else
        info += ", not Release (NDEBUG undefined)";
#endif
#ifdef __OPTIMIZE__
        info += ", optimization ON";
#else
        info += ", optimization OFF";
#endif
        return info;
    }

    static std::string formatNumber(double value, int precision) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision) << value;
        return ss.str();
    }

    static std::string threadsLabel(const BenchResult& r) {
        return r.threads == kAutoThreads ? "auto" : std::to_string(r.threads);
    }

    static std::string ratioLabel(const BenchResult& r, const BenchResult* best) {
        if (!r.custom || best == nullptr) return "-";
        return formatNumber(r.timeMs / best->timeMs, 2);
    }

    static void printHeader(std::ostream& os) {
        os << std::left;
        os << std::setw(kNameWidth) << "Algorithm";
        os << std::setw(kColWidth) << "Threads";
        os << std::setw(kColWidth) << "Time, ms";
        os << std::setw(kColWidth) << "x best K";
        os << std::setw(kColWidth) << "Result";
        os << "Check\n";
    }

    static void printRow(std::ostream& os, const BenchResult& r, const BenchResult* best) {
        os << std::left;
        os << std::setw(kNameWidth) << r.name;
        os << std::setw(kColWidth) << threadsLabel(r);
        os << std::setw(kColWidth) << formatNumber(r.timeMs, 3);
        os << std::setw(kColWidth) << ratioLabel(r, best);
        os << std::setw(kColWidth) << r.value;
        os << (r.correct ? "OK" : "ERROR") << "\n";
    }
};

// Console + file
static void emit(std::ostream& log, const std::string& text) {
    std::cout << text << std::flush;
    log << text;
}

static int detectHardwareThreads() {
    const unsigned n = std::thread::hardware_concurrency();
    return n == 0 ? kDefaultHardwareThreads : static_cast<int>(n);
}

static Data loadOrGenerate(std::size_t size) {
    const std::string path = std::string(DATA_DIR) + "/data_" + std::to_string(size) + ".txt";
    if (!std::filesystem::exists(path)) {
        std::cerr << "Generating " << size << " elements -> " << path << "\n";
        DataGenerator::saveToFile(DataGenerator::generate(size), path);
    }
    return DataGenerator::loadFromFile(path);
}

int main() {
    try {
        const int hw = detectHardwareThreads();
        std::filesystem::create_directories(DATA_DIR);

        const std::string logFile = std::string(OUTPUT_DIR) + "/out_" + OPT_LABEL + ".txt";
        std::ofstream log(logFile);

        std::ostringstream header;
        ReportWriter::printEnvironment(header, hw);
        emit(log, header.str());

        for (std::size_t size : kSizes) {
            const auto data = loadOrGenerate(size);
            const auto results = ExperimentRunner::runAll(data, hw);
            std::ostringstream table;
            ReportWriter::printTable(table, size, results);
            ReportWriter::printBestK(table, results, hw);
            emit(log, table.str());
        }

        emit(log, "\nDone. Output saved to " + logFile + "\n");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
