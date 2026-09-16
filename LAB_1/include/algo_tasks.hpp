#pragma once

#include <vector>

namespace algo_tasks {

/// Проверка целого числа на простоту.
bool isPrime(int n) noexcept;

/// (a) Все простые числа в v возводит в квадрат. Возвращает новый вектор.
std::vector<int> squarePrimes(std::vector<int> v);

/// (b) Сортирует v: сначала нечётные по возрастанию, затем чётные по убыванию.
void sortOddAscEvenDesc(std::vector<int>& v);

/// (c) Уникальные элементы v, попадающие в диапазон [lo, hi] (включительно).
///     Результат отсортирован по возрастанию.
std::vector<int> uniqueInRange(const std::vector<int>& v, int lo, int hi);

} // namespace algo_tasks