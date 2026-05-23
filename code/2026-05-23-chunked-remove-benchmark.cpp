
#include <algorithm>
#include <benchmark/benchmark.h>
#include <random>
#include <utility>
#include <vector>

#if BENCHMARK_COUNTS
size_t cps = 0;
#endif

template <class FwdIt, class Pred>
constexpr FwdIt smooth_remove_if(FwdIt first, FwdIt last, Pred pred) {
  FwdIt dfirst = std::find_if(first, last, pred);
  if (dfirst != last) {
    for (first = std::next(dfirst); first != last; ++first) {
      if (!pred(*first)) {
        *dfirst++ = std::move(*first);
      }
    }
  }
  return dfirst;
}

template <class FwdIt, class Pred>
constexpr FwdIt chunky_remove_if(FwdIt first, FwdIt last, Pred pred) {
  FwdIt dfirst = std::find_if(first, last, pred);
  if (dfirst != last) {
    for (first = std::next(dfirst); first != last; ++first) {
      if (!pred(*first)) {
        FwdIt sfirst = first;
        first = std::find_if(std::next(first), last, pred);
        dfirst = std::move(sfirst, first, dfirst);
#if BENCHMARK_COUNTS
        ++cps;
#endif
        if (first == last) break;
      }
    }
  }
  return dfirst;
}

template<int In>
auto get_pred() {
  if constexpr (In == 1024) {
    return [](int x) { return !(x & 0x3FF); };
  } else if constexpr (In == 128) {
    return [](int x) { return !(x & 0x7F); };
  } else if constexpr (In == 8) {
    return [](int x) { return !(x & 0x7); };
  } else if constexpr (In == 2) {
    return [](int x) { return !(x & 0x1); };
  }
}

std::vector<unsigned> get_sample_data(size_t n) {
  auto v = std::vector<unsigned>(n);
  std::mt19937 g;
  std::generate(v.begin(), v.end(), std::ref(g));
  return v;
}

#if BENCHMARK_COUNTS

size_t mas = 0;
struct Instrumented {
  int i_;
  explicit Instrumented(int i) : i_(i) {}
  Instrumented(Instrumented&&) = default;
  Instrumented& operator=(Instrumented&& rhs) noexcept { i_ = rhs.i_; ++mas; return *this; }
};

template<int In>
void test() {
  printf("For 1 in %d:\n", In);
  size_t ps = 0;
  auto pred = [&ps, p=get_pred<In>()](Instrumented& i) { ++ps; return p(i.i_); };
  {
    std::vector<unsigned> vi = get_sample_data(1'000'000);
    std::vector<Instrumented> v(vi.begin(), vi.end());
    mas = ps = 0;
    (void)std::remove_if(v.begin(), v.end(), pred);
    printf("std: %zu move-assignments, %zu predicate calls\n", mas, ps);
  }
  {
    std::vector<unsigned> vi = get_sample_data(1'000'000);
    std::vector<Instrumented> v(vi.begin(), vi.end());
    mas = ps = 0;
    (void)smooth_remove_if(v.begin(), v.end(), pred);
    printf("smooth: %zu move-assignments, %zu predicate calls\n", mas, ps);
  }
  {
    std::vector<unsigned> vi = get_sample_data(1'000'000);
    std::vector<Instrumented> v(vi.begin(), vi.end());
    mas = cps = ps = 0;
    (void)chunky_remove_if(v.begin(), v.end(), pred);
    printf("chunky: %zu memmoves (replacing %zu move-assignments), %zu predicate calls\n", cps, mas, ps);
  }
}

int main() {
  test<2>();
  test<8>();
  test<128>();
  test<1024>();
}

#else

#define DEFINE_BENCHMARK(BM_benchname, algoname_remove_if) \
template<int In> \
void BM_benchname(benchmark::State& state) { \
  auto vo = get_sample_data(state.range(0)); \
  auto pred = get_pred<In>(); \
  for (auto _ : state) { \
    state.PauseTiming(); \
    auto v = vo; \
    state.ResumeTiming(); \
    benchmark::DoNotOptimize(v); \
    auto it = algoname_remove_if(v.begin(), v.end(), pred); \
    benchmark::DoNotOptimize(v); \
    benchmark::DoNotOptimize(it); \
  } \
}

DEFINE_BENCHMARK(BM_std, std::remove_if)
DEFINE_BENCHMARK(BM_smooth, smooth_remove_if)
DEFINE_BENCHMARK(BM_chunky, chunky_remove_if)

BENCHMARK(BM_std<    2>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_std<    8>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_std<  128>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_std< 1024>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);

BENCHMARK(BM_smooth<    2>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_smooth<    8>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_smooth<  128>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_smooth< 1024>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);

BENCHMARK(BM_chunky<    2>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_chunky<    8>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_chunky<  128>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
BENCHMARK(BM_chunky< 1024>)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);

BENCHMARK_MAIN();

#endif // BENCHMARK_COUNTS
