#include "algo_tasks.hpp"

#include <algorithm>
#include <iterator>

namespace algo_tasks {

bool isPrime(int n) noexcept
{
    if (n < 2) return false;
    if (n < 4) return true;      // 2, 3
    if (n % 2 == 0) return false;

    for (int d = 3; 1LL * d * d <= n; d += 2) 
    {
        if (n % d == 0) return false;
    }
    return true;
}

std::vector<int> squarePrimes(std::vector<int> v)
{
    // std::transform — алгоритм модификации элементов.
    std::transform(v.begin(), v.end(), v.begin(),
                   [](int x) { return isPrime(x) ? x * x : x; });
    return v;
}

void sortOddAscEvenDesc(std::vector<int>& v)
{
    std::sort(v.begin(), v.end(),
              [](int a, int b)
              {
                  const bool aOdd = (a % 2) != 0;
                  const bool bOdd = (b % 2) != 0;

                  if (aOdd != bOdd) return aOdd;   // нечётные впереди
                  if (aOdd)         return a < b;  // нечётные — по возрастанию
                  return a > b;                     // чётные — по убыванию
              });
}

std::vector<int> uniqueInRange(const std::vector<int>& v, int lo, int hi)
{
    if (lo > hi) std::swap(lo, hi);

    std::vector<int> out;

    std::copy_if(v.begin(), v.end(), std::back_inserter(out),
                 [=](int x) { return x >= lo && x <= hi; });

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());

    return out;
}

} // namespace algo_tasks