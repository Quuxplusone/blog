---
layout: post
title: 'Copy Elision in Throw Statements'
date: 2018-04-09 00:01:00 +0000
tags:
  copy-elision
  move-semantics
  sufficiently-smart-compiler
---

Today it was brought to my attention (on Slack) that although copy elision
is permitted in `throw` statements, no major C++ compiler performs that elision.

> When certain criteria are met, an implementation is allowed to omit the copy/move
> construction of a class object, even if the constructor selected for the copy/move
> operation and/or the destructor for the object have side effects [...]
>
> In a _throw-expression_, when the operand is the name of a non-volatile automatic
> object (other than a function or catch-clause parameter) whose scope does not extend
> beyond the end of the innermost enclosing try-block (if there is one), the copy/move
> operation from the operand to the exception object can be omitted by constructing the
> automatic object directly into the exception object[.]

[Here is our test case:](https://godbolt.org/g/ZX3MJG)

    struct Widget {
        int data[1000];
        Widget() noexcept;
        Widget(Widget&&) noexcept;
    };

    void elision_permitted_but_it_doesnt_happen() {
        Widget ex;
        throw ex;
    }

    void elision_not_permitted() {
        Widget ex;
        throw static_cast<Widget&&>(ex);
    }

The latter function is *required* to make one call to `Widget`'s zero-argument constructor
followed by one call to the move constructor. The former function is permitted to make
those calls; but it is permitted alternatively to make just one call to the zero-argument
constructor and *no* moves (by constructing `ex` directly into the exception slot).

On the Itanium ABI (Linux and OS X), it's fairly intuitive why the compiler (Clang
or GCC) doesn't elide the move. The assembly looks like this:

    subq   $4008, %rsp
    leaq   8(%rsp), %r14
    movq   %r14, %rdi
    callq  _ZN6WidgetC1Ev  # zero-argument constructor
    movl   $4000, %edi
    callq  __cxa_allocate_exception  # get space for the exception slot
    movq   %rax, %rbx
    movq   %rax, %rdi
    movq   %r14, %rsi
    callq  _ZN6WidgetC1EOS_  # move-constructor
    xorl   %edx, %edx
    movq   %rbx, %rdi
    movl   $_ZTI6Widget, %esi  # typeid(Widget)
    callq  __cxa_throw  # the actual throw machinery

In order to perform copy elision here, we would have to hoist the call to
`__cxa_allocate_exception` *above* the call to `Widget`'s zero-argument constructor.
Changing the order of two function calls is generally a dangerous operation;
the compiler cannot do it unless it is sure that the two functions don't interact
in any way — such as by touching global data, or printing messages to the screen whose
order can be observed, or by one of them terminating the program so that the other
one never gets to run. In fact, `__cxa_allocate_exception` *can* terminate the program!

So the null hypothesis here is that Clang and GCC are not "sufficiently smart"
to understand that `Widget::Widget()` does not *meaningfully* interact with
`__cxa_allocate_exception()`.

Let's teach the compiler that `Widget::Widget()` has no side effects
and therefore cannot interact with `__cxa_allocate_exception()` at all:

    Widget() noexcept = default;

Clang and GCC still don't care —
[they continue to codegen a call to the move constructor](https://godbolt.org/g/MReynW),
regardless. Hmm. Okay, maybe they just don't understand that copy elision is permitted
in this context (because no compiler dev has ever bothered to teach them that fact).

So that's the story on the Itanium ABI.

On MSVC (Windows), the exception-throwing mechanism is organized differently.
On MSVC, the codegen looks like this:

    subq     $8040, %rsp
    leaq     32(%rsp), %rcx
    callq    ??0Widget@@QEAA@XZ  # zero-argument constructor
    leaq     32(%rsp), %rdx
    leaq     4032(%rsp), %rcx  # the exception slot resides on the stack
    callq    ??0Widget@@QEAA@$QEAU0@@Z  # move constructor
    leaq     4032(%rsp), %rcx
    leaq     $_TI1?AUWidget@@, %rdx  # typeid(Widget)
    callq    _CxxThrowException  # the actual throw machinery

Here there is no side-effecting code between the call to `Widget()` and the
call to `Widget(Widget&&)`. MSVC does not have to be "smart" to see
that the move-constructor is redundant. It merely has to be taught that
move-elision is _possible_ in this case.

Maybe someday somebody will teach all three of these compilers how to do
copy-elision on throw statements.
