// clang++ -std=c++23 -O2 blqs-bench.cpp -lbenchmark -L... -I... -DBLQS_RELOCATE=0
//              TC            TR            NTC           NTR
// std::sort    376ms, 5.9s   413ms, 6.5s   372ms, 5.8s   436ms, 6.3s
// blqs::sort   170ms, 3.5s   233ms, 6.2s   160ms, 4.3s   239ms, 6.7s
//
// clang++ -std=c++23 -O2 blqs-bench.cpp -lbenchmark -L... -I... -DBLQS_RELOCATE=1
//              TC            TR            NTC           NTR
// std::sort    385ms, 5.8s   411ms, 6.5s   372ms, 5.8s   435ms, 6.2s
// blqs::sort   165ms, 3.4s   201ms, 4.0s   162ms, 4.4s   220ms, 6.3s
//

#include <algorithm>
#include <benchmark/benchmark.h>
#include <cassert>
#include <compare>
#include <random>
#include <utility>
#include <vector>

#include "blqs.h"

using Int = unsigned;

struct TC {
  Int *p_;
  void *ctrl_;
  explicit TC() = default;
  explicit TC(Int& i) : p_(&i) {}
  friend auto operator<=>(const TC& a, const TC& b) { return *a.p_ <=> *b.p_; }
};

struct TR {
  std::shared_ptr<Int> p_;
  explicit TR() = default;
  explicit TR(Int& i) : p_(&i, [](Int*){}) {}
  friend auto operator<=>(const TR& a, const TR& b) { return *a.p_ <=> *b.p_; }
};

struct NTC : TC {
  using TC::TC;
  ~NTC() {}
};

struct NTR : TR {
  using TR::TR;
  NTR(NTR&&) = default;
  NTR(const NTR&) = default;
  NTR& operator=(NTR&&) = default;
  NTR& operator=(const NTR&) = default;
  ~NTR() {}
};

auto bigdata = []() {
  std::array<Int, 50'000'000> a;
  std::mt19937 g;
  std::generate(a.begin(), a.end(), std::ref(g));
  return a;
}();

template<class T>
std::vector<T> get_sample_data(size_t n) {
  assert(n <= bigdata.size());
  auto v = std::vector<T>(bigdata.data(), bigdata.data() + n);
  return v;
}

template<class T>
void BM_stdsort(benchmark::State& state) {
  auto vo = get_sample_data<T>(state.range(0));
  for (auto _ : state) {
    state.PauseTiming();
    auto v = vo;
    state.ResumeTiming();
    benchmark::DoNotOptimize(v);
    std::sort(v.data(), v.data() + v.size());
    benchmark::DoNotOptimize(v);
    assert(std::is_sorted(v.begin(), v.end()));
  }
}

template<class T>
void BM_blqsort(benchmark::State& state) {
  auto vo = get_sample_data<T>(state.range(0));
  for (auto _ : state) {
    state.PauseTiming(); 
    auto v = vo;  
    state.ResumeTiming();
    benchmark::DoNotOptimize(v);
    blqs::sort(v.data(), v.data() + v.size());
    benchmark::DoNotOptimize(v);
    assert(std::is_sorted(v.begin(), v.end()));
  }
}

BENCHMARK(BM_stdsort<TC>)->Arg(5'000'000)->Arg(50'000'000);
BENCHMARK(BM_stdsort<TR>)->Arg(5'000'000)->Arg(50'000'000);
BENCHMARK(BM_stdsort<NTC>)->Arg(5'000'000)->Arg(50'000'000);
BENCHMARK(BM_stdsort<NTR>)->Arg(5'000'000)->Arg(50'000'000);

BENCHMARK(BM_blqsort<TC>)->Arg(5'000'000)->Arg(50'000'000);
BENCHMARK(BM_blqsort<TR>)->Arg(5'000'000)->Arg(50'000'000);
BENCHMARK(BM_blqsort<NTC>)->Arg(5'000'000)->Arg(50'000'000);
BENCHMARK(BM_blqsort<NTR>)->Arg(5'000'000)->Arg(50'000'000);

BENCHMARK_MAIN();
