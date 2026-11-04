#pragma once

#include "person.h"

#include <random>
#include <string>
#include <vector>

// ======================= ГЕНЕРАТОР СЛУЧАЙНЫХ ДАННЫХ =========================

class RandomPersonGenerator {
public:
    RandomPersonGenerator();
    Person generate();

private:
    std::mt19937 rng;
    std::vector<std::string> surnames;
    std::vector<std::string> names;
    std::vector<std::string> patronymics;

    template <typename T>
    const T& pick(const std::vector<T>& vec) {
        std::uniform_int_distribution<std::size_t> dist(0, vec.size() - 1);
        return vec[dist(rng)];
    }
};