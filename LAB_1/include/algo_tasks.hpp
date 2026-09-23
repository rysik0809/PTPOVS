#pragma once

#include <vector>

namespace algo_tasks {

/// Проверка целого на простоту
bool isPrime(int n) noexcept;

/// Возвращает новый вектор: простые числа возведены в квадрат,
/// остальные без изменений
std::vector<int> squarePrimes(std::vector<int> v);

/// Сортирует на месте: нечётные по возрастанию, затем чётные
/// по убыванию. Принимает по ссылке — без копий и аллокаций
void sortOddAscEvenDesc(std::vector<int>& v);

/// Возвращает уникальные элементы v из [lo, hi], отсортированные
/// по возрастанию. lo и hi можно передавать в любом порядке
std::vector<int> uniqueInRange(const std::vector<int>& v, int lo, int hi);

} // namespace algo_tasks