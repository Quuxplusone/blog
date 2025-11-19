---
layout: post
title: '"Cumbersome" labeled loops and the C preprocessor'
date: 2024-12-22 00:01:00 +0000
tags:
  language-design
  preprocessor
  proposal
  wg21
---

In my last post [I wrote](/blog/2024/12/20/labeled-loops/) that
[N3377](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3377.pdf)'s proposed syntax
for labeled loops in C was not only unprecedented among programming languages but also
"cumbersome." What did I mean by that?

Look at N3377's own example:

<table>
<tr><th>N3355 (adopted)</th><th>N3377 (proposed)</th></tr>
<tr><td style="font-size: 70%;"><pre>
#define UPDATE_MAP(MAP, ELEM, NEWVAL)\
  do { \
    MapGroup *BucketItr = MAP.Lists;\
    <b>OUTER:</b> while (BucketItr) {\
      if (BucketItr->Hash == HASH(ELEM)) {\
        MapNode *NodeItr = BucketItr->Nodes;\
        while (NodeItr) {\
          if (NodeItr->Key == ELEM) {\
            NodeItr->Value = NEWVAL;\
            break <b>OUTER</b>;\
          }\
          NodeItr = NodeItr->Next;\
        }\
        BucketItr = BucketItr->Next;\
      }\
    }\
  } while(0)
</pre></td><td style="font-size: 70%;"><pre>
#define UPDATE_MAP(MAP, ELEM, NEWVAL)\
  do { \
    MapGroup *BucketItr = MAP.Lists;\
    while <b>OUTER</b> (BucketItr) {\
      if (BucketItr->Hash == HASH(ELEM)) {\
        MapNode *NodeItr = BucketItr->Nodes;\
        while (NodeItr) {\
          if (NodeItr->Key == ELEM) {\
            NodeItr->Value = NEWVAL;\
            break <b>OUTER</b>;\
          }\
          NodeItr = NodeItr->Next;\
        }\
        BucketItr = BucketItr->Next;\
      }\
    }\
  } while(0)
</pre></td></tr>
</table>

AFAIK this isn't taken directly from any real codebase, but it's representative of a genre
I know well: macro-based C hash table implementations. I maintain
[uthash](https://troydhanson.github.io/uthash/userguide.html), a macro-based C hash table
implementation. Uthash contains macros like [this](https://github.com/troydhanson/uthash/blob/master/src/uthash.h#L1063-L1065):

    #define HASH_ITER(hh,head,el,tmp) \
      for (((el)=(head)), ((tmp)=DECLTYPE(el)((head!=NULL)?(head)->hh.next:NULL)); \
           (el) != NULL; \
           ((el)=(tmp)), ((tmp)=DECLTYPE(el)((tmp!=NULL)?(tmp)->hh.next:NULL)))

which allows client code to iterate over a hash table, like this
([Godbolt](https://godbolt.org/z/MPsK5G41G); [backup](/blog/code/2024-12-22-uthash-labeled-loop.c)):

    HASH_ITER(hh, table, row, tmp) {
      for (int y = 0; y < 4; ++y) {
        printf("x=%s y=%d\n", row->name, y);
        if (row->columns[y] != 0) {
          goto BREAK_OUTER;
        }
      }
    }
    BREAK_OUTER:
    puts("Exited outer loop");

Now, if C gains labeled loops per [N3355 "Named loops"](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3355.htm),
then we can eliminate the `goto` by rewriting with a labeled loop, like this:

    OUTER:
    HASH_ITER(hh, table, row, tmp) {
      for (int y = 0; y < 4; ++y) {
        printf("x=%s y=%d\n", row->name, y);
        if (row->columns[y] != 0) {
          break OUTER;
        }
      }
    }
    puts("Exited outer loop");

All you need for this is a new C compiler; you don't need to update your version of the uthash library.
So N3355 fits into the existing C ecosystem pretty well.

Contrariwise, [N3377](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3377.pdf)'s proposed syntax (which I call "cumbersome")
wouldn't Just Work with any existing version of uthash. Clients hoping to eliminate `goto` from their code
would have to do something like:

    #define HASH_ITER(hh,head,el,tmp) \
      HASH_ITER_WITH_LABEL(,hh,head,el,tmp)

    #define HASH_ITER_WITH_LABEL(label,hh,head,el,tmp) \
      for label (((el)=(head)), ((tmp)=DECLTYPE(el)((head!=NULL)?(head)->hh.next:NULL)); \
           (el) != NULL; \
           ((el)=(tmp)), ((tmp)=DECLTYPE(el)((tmp!=NULL)?(tmp)->hh.next:NULL)))

    HASH_ITER_WITH_LABEL(OUTER, hh, table, row, tmp) {
      for (int y = 0; y < 4; ++y) {
        printf("x=%s y=%d\n", row->name, y);
        if (row->columns[y] != 0) {
          break OUTER;
        }
      }
    }
    puts("Exited outer loop");

And then they'd have to push the `HASH_ITER_WITH_LABEL` macro upstream to uthash trunk; and they'd have to upgrade
their version of uthash; before they could start using the labeled-loop feature at all. Meanwhile, clients still
on C23 or earlier would find that the new version of uthash defines a new macro `HASH_ITER_WITH_LABEL`, but
any attempt to use it would cause a syntax error (until they upgraded their compiler, that is).
That's a bad user experience for those clients, and it would be a bad library-maintainer experience, too.
Given the choice, I'd rather have N3355 than N3377.
