---
layout: post
title: "`constexpr std::string` update"
date: 2023-10-13 00:01:00 +0000
tags:
  constexpr
  cpplang-slack
  library-design
  llvm
---

Last month I posted ["Just how `constexpr` is C++20's `std::string`?"](/blog/2023/09/08/constexpr-string-firewall/) (2023-09-08),
showing some surprising behavior in libc++. As of a week ago, that surprising behavior
is gone!

The same day my post hit Hacker News,
James Y. Knight filed [libstdc++ bug 111351](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111351)
asking for libstdc++ to match libc++'s more conservative behavior by disabling its
[SSO](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#sbo-soo-sso) at constexpr time.
After some pushback there, Knight (quite awesomely, in my view) pivoted to create
not only a bug report but [an actual patch](https://github.com/llvm/llvm-project/pull/66576)
for libc++ completely implementing constexpr SSO! His pull request was opened 2023-09-16
and merged into trunk on 2023-10-10.

We can even see the change immediately benefiting projects that consume Clang nightlies.
For example: Clang can detect unused global `static` variables only when their initializers are
constant-expressions (because dynamic initialization and destruction themselves count as "uses"
that disguise the otherwise-unusedness of the variable).
So Clang is now [able](https://godbolt.org/z/GEbrTjcnc) to detect unused
`static const std::string` globals up to 22 characters in length —
see [Chromium commit 879cb86](https://github.com/chromium/chromium/commit/879cb86187c04e9e941c03174765e5030a5a9cba).

The main point of my post (the "firewall" between constexpr evaluation and runtime
execution) is still as valid as ever — perhaps even more so, now that libc++'s special cases
are eliminated. But I thought it would be worth revisiting each of that post's example
snippets with libc++ trunk and seeing how they've changed.

---

    constexpr std::string s = "William Shakespeare";

This 19-character constinit string is now legal on libc++ (though not on libstdc++ nor Microsoft STL,
because their SSO limit is lower).

---

    constexpr std::vector<int> v1 = {1, 2, 3};
    constexpr std::vector<int> v2 = {};

`v1` remains invalid and `v2` remains valid, of course.
But now we also have:

    constexpr std::string s1 = {'1', '2', '3'};
    constexpr std::string s2 = {};

Both `s1` and `s2` are now valid. Before the patch, libc++ considered them both
invalid. ("Both?" Yes; remember that even an empty `string` requires storage for
its trailing null byte. Last month's libc++ refused to use SSO even for that one byte.
This is now fixed.)

---

libstdc++ rejects the following code ([Godbolt](https://godbolt.org/z/1ErrKjdbq))
due to its pointer-into-self `string` representation, while Microsoft <b>and now also libc++</b> accept.

    int main() {
      static constexpr std::string abc = "abc"; // OK
      constexpr std::string def = "def";        // Error!
    }

---

Finally:

    constinit std::string s = "";

This definition (at global scope) is now accepted by all three vendors.
