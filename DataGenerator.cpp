#include "DataGenerator.h"

#include <fstream>
#include <random>
#include <stdexcept>

Data DataGenerator::generate(std::size_t size) {
    std::mt19937_64 gen(kSeed);
    std::uniform_int_distribution<Int> dist(kMinValue, kMaxValue);
    Data data(size);
    for (auto& v : data) v = dist(gen);
    return data;
}

void DataGenerator::saveToFile(const Data& data, const std::string& filename) {
    std::ofstream out(filename);
    if (!out) throw std::runtime_error("cannot create file: " + filename);
    out << data.size() << "\n";
    for (auto v : data) out << v << ' ';
}

Data DataGenerator::loadFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) throw std::runtime_error("cannot open file: " + filename);
    std::size_t size = 0;
    if (!(in >> size)) throw std::runtime_error("bad header in file: " + filename);
    Data data(size);
    for (auto& v : data)
        if (!(in >> v)) throw std::runtime_error("bad data in file: " + filename);
    return data;
}