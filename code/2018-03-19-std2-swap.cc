#include <utility>

namespace std2 {
    template<size_t I> struct priority_tag : priority_tag<I-1> {};
    template<> struct priority_tag<0> {};
}

namespace std2::customization {
    template<class T, class U> struct swapper;
}
namespace std2::detail {

    template<class T>
    auto detail_swap(T& a, T& b, priority_tag<0>)
        -> decltype(void(a = std::move(b)), void(T(std::move(a))))
    {
        T temp(a);
        a = std::move(b);
        b = std::move(temp);
    }
    
    template<class T, class U>
    auto detail_swap(T&& a, U&& b, priority_tag<1>)
        -> decltype(void(swap(std::forward<T>(a), std::forward<U>(b))))
    {
        swap(std::forward<T>(a), std::forward<U>(b));
    }

    template<class T, class U>
    auto detail_swap(T&& a, U&& b, priority_tag<2>)
        -> decltype(void(std::forward<T>(a).swap(std::forward<U>(b))))
    {
        std::forward<T>(a).swap(std::forward<U>(b));
    }

    template<class T, class U>
    auto detail_swap(T&& a, U&& b, priority_tag<3>)
        -> decltype(void(std2::customization::swapper<T&&,U&&>::_(std::forward<T>(a), std::forward<U>(b))))
    {
        std2::customization::swapper<T&&,U&&>::_(std::forward<T>(a), std::forward<U>(b));
    }
}

namespace std2 {
    inline constexpr auto swap = [](auto&& a, auto&& b)
        -> decltype(std2::detail::detail_swap(std::forward<decltype(a)>(a), std::forward<decltype(b)>(b), priority_tag<3>{}))
    {
        return std2::detail::detail_swap(std::forward<decltype(a)>(a), std::forward<decltype(b)>(b), priority_tag<3>{});
    };
}

#include <assert.h>
#include <stdio.h>

struct Gadget {
    int value;
    explicit Gadget(int v) : value(v) {}
    void swap(Gadget& rhs) { puts("Swapped Gadgets!"); std2::swap(value, rhs.value); }
};

namespace Other {
    struct Wodget {
        int value;
        explicit Wodget(int v) : value(v) {}
        Wodget(const Wodget&) = delete;
    };
}

namespace std2::customization {
    template<>
    struct swapper<Other::Wodget&, Other::Wodget&> {
        static void _(Other::Wodget& a, Other::Wodget& b) {
            puts("Swapped Wodgets!");
            std2::swap(a.value, b.value);
        }
    };
}

int main()
{
    if (true) {
        int a = 1;
        int b = 2;
        std2::swap(a,b);
        assert(a == 2 && b == 1);
    }
    if (true) {
        Gadget x(1);
        Gadget y(2);
        std2::swap(x, y);
        assert(x.value == 2 && y.value == 1);
    }
    if (true) {
        Other::Wodget x(1);
        Other::Wodget y(2);
        std2::swap(x, y);
        assert(x.value == 2 && y.value == 1);
    }
}
