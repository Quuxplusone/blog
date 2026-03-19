---
layout: post
title: "Chromium's `span`-over-initializer-list success story"
date: 2026-03-19 00:02:00 +0000
tags:
  c++-style
  initializer-list
  parameter-only-types
  proposal
excerpt: |
  Previously: ["`span` should have a converting constructor from `initializer_list`"](/blog/2021/10/03/p2447-span-from-initializer-list/)
  (2021-10-03). This converting constructor was added by [P2447](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2447r6.html)
  for C++26. Way back in 2024, Peter Kasting added the same constructor to Chromium's
  [`base::span`](https://github.com/chromium/chromium/blob/4aa6967/base/containers/span.h#L1166-L1171) —
  he emailed me about it at the time — but I was only recently reminded that in
  [the /r/cpp thread](https://old.reddit.com/r/cpp/comments/1n8fbg8/showcasing_underappreciated_proposals/)
  about the feature he'd written:

  > Yup, this change was so useful it led to me doing a ton of reworking of Chromium's
  > `base::span` just so I could implement it there.

  Speaking of [ambiguity](/blog/2026/03/19/seven-types-of-ambiguity/): out of context that comment _could_ be taken
  as sarcasm. What programmer enjoys "doing a ton of reworking just" to implement a single new constructor?
  Did he mean the change was so _useful_, or, like, "_so_ useful"? :) So it's worthwhile to track down
  [pkasting's actual commit](https://github.com/chromium/chromium/commit/7a129f92f54dafe6c3ef98030ebbdbc2704d3411)
  from November 2024 and see all the places he sincerely did clean up as a result.
---

{% raw %}
Previously: ["`span` should have a converting constructor from `initializer_list`"](/blog/2021/10/03/p2447-span-from-initializer-list/)
(2021-10-03). This converting constructor was added by [P2447](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2447r6.html)
for C++26. Way back in 2024, Peter Kasting added the same constructor to Chromium's
[`base::span`](https://github.com/chromium/chromium/blob/4aa6967/base/containers/span.h#L1166-L1171) —
he emailed me about it at the time — but I was only recently reminded that in
[the /r/cpp thread](https://old.reddit.com/r/cpp/comments/1n8fbg8/showcasing_underappreciated_proposals/)
about the feature he'd written:

> Yup, this change was so useful it led to me doing a ton of reworking of Chromium's
> `base::span` just so I could implement it there.

Speaking of [ambiguity](/blog/2026/03/19/seven-types-of-ambiguity/): out of context that comment _could_ be taken
as sarcasm. What programmer enjoys "doing a ton of reworking just" to implement a single new constructor?
Did he mean the change was so _useful_, or, like, "_so_ useful"? :) So it's worthwhile to track down
[pkasting's actual commit](https://github.com/chromium/chromium/commit/7a129f92f54dafe6c3ef98030ebbdbc2704d3411)
from November 2024 and see all the places he sincerely did clean up as a result.

What follows is a "close reading" of all the client call sites changed in Chromium commit
[7a129f92f5](https://github.com/chromium/chromium/commit/7a129f92f54dafe6c3ef98030ebbdbc2704d3411).

---

    std::vector<scoped_refptr<const Cert>> certs(
        {kcer_cert_0, kcer_cert_1, kcer_cert_2, kcer_cert_3, kcer_cert_3,
         kcer_cert_2, kcer_cert_1, kcer_cert_0, kcer_cert_0, kcer_cert_2,
         kcer_cert_3, kcer_cert_1});
    CertCache cache(certs);

becomes simply

    CertCache cache({kcer_cert_0, kcer_cert_1, kcer_cert_2, kcer_cert_3,
                     kcer_cert_3, kcer_cert_2, kcer_cert_1, kcer_cert_0,
                     kcer_cert_0, kcer_cert_2, kcer_cert_3, kcer_cert_1});

This is the poster-child use-case: the new code directly views a stack-allocated
`initializer_list`, where the old code had wasted time and memory copying the contents
of that `initializer_list` into a heap-allocated `vector`.
This being test code, we don't really care about the new code's improved efficiency,
but we do care about its improved readability and convenience.

---

    ASSERT_TRUE(ConfigureAppContainerSandbox(
        std::array<const base::FilePath*, 2>{&pathA, &pathB}));

becomes simply

    ASSERT_TRUE(ConfigureAppContainerSandbox({&pathA, &pathB}));

---


    EXPECT_THAT(MapThenFilterStrings(
                    {{"en", "de"}},
                    base::BindRepeating(~~~~)),
                IsEmpty());

replaces its double-braces with single-braces.

---

    FetchImagesForURLs(base::span_from_ref(card_art_url),
                       base::span({AutofillImageFetcherBase::ImageSize::kSmall,
                                   AutofillImageFetcherBase::ImageSize::kLarge}));

becomes simply

    FetchImagesForURLs(base::span_from_ref(card_art_url),
                       {AutofillImageFetcherBase::ImageSize::kSmall,
                        AutofillImageFetcherBase::ImageSize::kLarge});

Notice that these preceding three examples all had the same _intent_ — to view a
fixed list of two items — but in the absence of natural syntax they invented three
different workarounds to imperfectly express their intent. (Temporary `std::array`;
doubled curly braces; explicit cast to `base::span`.) All three converged on the natural
syntax as soon as it became available.
Two of them benefit from [P2752](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2752r3.html), too.

---

There were two "failure stories" in Peter's commit, both due to the
new constructor's lack of CTAD. (I still don't think anyone should
ever use CTAD, and LEWG was a little scared of adding it here anyway.)
For example Peter rewrote

    if (base::span(box.type) == base::span({'f', 't', 'y', 'p'}))

into

    if (base::span(box.type) == base::span<const char>({'f', 't', 'y', 'p'}))

Now, you might think after P2447 this could have become simply

    if (base::span(box.type) == {'f', 't', 'y', 'p'})

but sadly no, for [historical reasons](https://stackoverflow.com/questions/11420448/initializer-lists-and-rhs-of-operators)
a braced initializer list is grammatically disallowed
after most C++ operators (the exceptions being [`co_yield`](https://eel.is/c++draft/expr.yield#nt:yield-expression)
and [the assignment operators](https://eel.is/c++draft/expr.assign#nt:assignment-expression)).
I myself would probably have written one of

    if (std::string_view(box.type, 4) == "ftyp")

    if (memcmp(box.type, "ftyp", 4) == 0)

In the other "failure case," Peter rewrote

    hosts[DnsHostsKey("localhost", ADDRESS_FAMILY_IPV4)] =
        IPAddress({192, 168, 1, 1});

into

    hosts[DnsHostsKey("localhost", ADDRESS_FAMILY_IPV4)] =
        IPAddress(base::span<const uint8_t>({192, 168, 1, 1}));

The trick here is that the old code ([Godbolt](https://godbolt.org/z/hfYTqh3GW))
wasn't actually constructing a `span` at all; it was calling
[`IPAddress`’s four-argument converting constructor](https://github.com/chromium/chromium/blob/3afe88cc17a748340a53c3eea07fb706a5054af7/net/base/ip_address.h#L135-L137)
followed by a redundant explicit cast to `IPAddress`.
Personally I would have preserved the old behavior and improved readability
at the same time by simply removing the curly braces:

    hosts[DnsHostsKey("localhost", ADDRESS_FAMILY_IPV4)] =
        IPAddress(192, 168, 1, 1);

{% endraw %}
