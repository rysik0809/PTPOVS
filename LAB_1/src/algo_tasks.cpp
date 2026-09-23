#include "algo_tasks.hpp"

#include <algorithm>
#include <set>
#include <utility>

namespace algo_tasks {

bool isPrime(int n) noexcept
{
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;

    for (int d = 3; 1LL * d * d <= n; d += 2)
        if (n % d == 0) return false;

    return true;
}

std::vector<int> squarePrimes(std::vector<int> v)
{
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

                  if (aOdd != bOdd) return aOdd;
                  return aOdd ? a < b : a > b;
              });
}

std::vector<int> uniqueInRange(const std::vector<int>& v, int lo, int hi)
{
    if (lo > hi) std::swap(lo, hi);

    std::set<int> s;
    for (int x : v)
        if (lo <= x && x <= hi)
            s.insert(x);

    return {s.begin(), s.end()};
}

} // namespace algo_tasks