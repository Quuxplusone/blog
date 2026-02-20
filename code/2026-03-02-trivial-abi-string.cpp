// A libstdc++-style String, which contains a pointer-to-self.
// This String, being non-trivially relocatable, is a bad candidate for [[clang::trivial_abi]].
//
// The following will all work fine, physically speaking:
//   clang++ -DNO -DSTRING='"hello world"' -O{0,1,2}
//   clang++ -DNO -DSTRING='"hello"' -O{0,1,2}
//   clang++ -DYES -DSTRING='"hello world"' -O{0,1,2}  -- because the SSO buffer is never used
//   clang++ -DYES -DSTRING='"hello"' -O0              -- because special member functions are not omitted
//
// But the following will segfault:
//   clang++ -DYES -DSTRING='"hello"' -O{1,2}
//
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <utility>

#ifdef YES
 #define MAYBE_TRIVIAL [[clang::trivial_abi]]
#else
 #define MAYBE_TRIVIAL
#endif

struct MAYBE_TRIVIAL String {
    char *p_;
    char sso_[8];
    String(const char *s) {
        size_t n = strlen(s);
        if (n < 8) {
            strcpy(sso_, s);
            p_ = sso_;
        } else {
            p_ = strdup(s);
        }
    }
    String(const String& rhs) : String(rhs.p_) {}
    void swap(String& rhs) noexcept {
        std::swap(sso_, rhs.sso_);
        char *tmp = (p_ == sso_) ? rhs.sso_ : p_;
        p_ = (rhs.p_ == sso_) ? sso_ : rhs.p_;
        rhs.p_ = tmp;
    }
    String& operator=(const String& rhs) {
        String copy = rhs;
        copy.swap(*this);
        return *this;
    }
    ~String() {
        if (p_ != sso_) free(p_);
    }
};

String produce() { return STRING; }
void consume(String s) { }
int main() {
    consume(produce());
    consume(produce());
}
