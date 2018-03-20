#include <utility>

template<size_t I> struct priority_tag : priority_tag<I-1> {};
template<> struct priority_tag<0> {};

namespace CNL_customization {
    template<class T> struct do_halve;
}
namespace CNL_detail {
    template<class T>
    auto detail_halve(T t, priority_tag<0>) -> decltype(t / 2)
    {
        // lower priority (priority 0)
        return t / 2;
    }

    template<class T>
    auto detail_halve(T t, priority_tag<1>) -> decltype(do_halve(t))
    {
        // higher priority (priority 1)
        return do_halve(t);
    }

    template<class T>
    auto detail_halve(T t, priority_tag<2>) -> decltype(CNL_customization::do_halve<T>::_(t))
    {
        // highest priority (priority 2)
        return CNL_customization::do_halve<T>::_(t);
    }
}

namespace CNL {
    inline constexpr auto cpo_halve = [](auto&& t)
        -> decltype(CNL_detail::detail_halve(std::forward<decltype(t)>(t), priority_tag<2>{}))
    {
        return CNL_detail::detail_halve(std::forward<decltype(t)>(t), priority_tag<2>{});
    };
}

#include <algorithm>
#include <cassert>
#include <vector>

namespace Other {
    class Bignum {
        int value;
    public:
        Bignum(int v) : value(v) {}
        bool operator==(Bignum rhs) const { return value == rhs.value; }
        void divide_by(int v) {
            value /= v;
        }
    };
}

namespace users {
    struct Number {
        int value;
        explicit Number(int v) : value(v) {}
        bool operator==(Number rhs) const { return value == rhs.value; }
    };
    Number do_halve(const Number& n) {
        return Number(n.value / 2);
    }
}

namespace CNL_customization {
    template<>
    struct do_halve<Other::Bignum> {
        static Other::Bignum _(Other::Bignum b) {
            b.divide_by(2);
            return b;
        }
    };
}

int main()
{
    if (true) {
        std::vector<int> vec = {1, 2, 3, 4, 5};
        std::transform(vec.begin(), vec.end(), vec.begin(), CNL::cpo_halve);
        assert((vec == std::vector<int>{0, 1, 1, 2, 2}));
    }
    if (true) {
        std::vector<users::Number> vec = { users::Number(1), users::Number(2), users::Number(3) };
        std::transform(vec.begin(), vec.end(), vec.begin(), CNL::cpo_halve);
        assert((vec == std::vector<users::Number>{users::Number(0), users::Number(1), users::Number(1)}));
    }
    if (true) {
        std::vector<Other::Bignum> vec = { 1, 2, 3 };
        std::transform(vec.begin(), vec.end(), vec.begin(), CNL::cpo_halve);
        assert((vec == std::vector<Other::Bignum>{ 0, 1, 1 }));
    }
}
