---
layout: post
title: 'How to build LLVM from source'
date: 2018-04-16 00:02:00 +0000
tags:
  how-to
  llvm
---

If I write this down, maybe I won't forget it next time!

There are two plausible sources of the LLVM codebase:
`https://llvm.org/git/$FOO.git` and `https://github.com/llvm-mirror/$FOO.git`.
The GitHub mirror seems to be faster than `llvm.org`; plus, we're
going to want to fork it on GitHub to track our changes.

(However, to submit patches to LLVM projects, you must use
[the official Phabricator](https://reviews.llvm.org/differential/diff/create/);
don't submit GitHub pull requests against the `llvm-mirror` mirrors!)


## Step 1: Fork!

Go to your GitHub account and fork each of the following repos:

- [llvm-mirror/llvm](https://github.com/llvm-mirror/llvm)
- [llvm-mirror/clang](https://github.com/llvm-mirror/clang)


## Step 2: Get the code!

Locally clone the repos to the right places.

    cd $ROOT               ; git clone git@github.com:llvm-mirror/llvm
    cd $ROOT/llvm/tools    ; git clone git@github.com:llvm-mirror/clang

Here are the right places for some other useful (but unnecessary) repos.

    cd $ROOT/llvm/projects          ; git clone git@github.com:llvm-mirror/libcxx
    cd $ROOT/llvm/projects          ; git clone git@github.com:llvm-mirror/compiler-rt
    cd $ROOT/llvm/tools             ; git clone git@github.com:llvm-mirror/lldb
    cd $ROOT/llvm/tools/clang/tools ; git clone git@github.com:llvm-mirror/clang-tools-extra extra

(Notice that `clang-tools-extra`'s repo name doesn't match its expected directory name.
We snuck an `extra` parameter onto the end of that `git clone` line.)

This is a good time to set up the `.git/config` for each of the repos
you just cloned (for example, `$ROOT/llvm/.git/config`).
I set it up this way:

    [remote "origin"]
        url = git@github.com:Quuxplusone/llvm.git
        fetch = +refs/heads/master:refs/remotes/origin/master
    [remote "upstream"]
        url = git@github.com:llvm-mirror/llvm.git
        fetch = +refs/heads/master:refs/remotes/upstream/master
    [branch "master"]
        remote = upstream
        merge = refs/heads/master

This gives me two remotes: one named `upstream` whence I can pull,
and one named `origin` whither I can push. My local `master` and `origin/master`
will both track `upstream/master`. Anything I do in my local repo, I will do in a
feature branch; my feature branches will track `origin`.


## Step 3: Build!

    mkdir -p $ROOT/build
    cd $ROOT/build
    cmake -G 'Unix Makefiles' \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
    make -j5 clang
    make -j5 check-clang

If you omit `-DCMAKE_BUILD_TYPE=RelWithDebInfo`, this first part will still work, but you'll produce
a "debug-build" version of `clang` that is super slow, and then the "bootstrap" step below will
take days instead of minutes. So watch out for that.

`make -j5 clang` takes about 80 minutes on my laptop.
`make -j5 check-clang` takes another 37 minutes
(27 minutes to build `clang-tidy` for some reason, and then another 10 minutes to run the actual tests).

Making `clang` will build both `clang` and `clang++`.
Making `cxx` will build `libc++` (and also `libc++abi`, which is included in the `libcxx` repo).
Making `check-$FOO` will build and run the test suite for `$FOO`:

    make -j5 check-clang check-cxx

If something goes wrong, you can usually recover via

    rm $ROOT/build/CMakeCache.txt

and, absolute worst case, you can blow away `$ROOT/build` and start over.


## Step 4: Bootstrap!

This is where it might get non-portable for people who aren't on OS X, I'm not sure.
Here we will *not* be installing Clang over top of the system compiler
(super dangerous!); but we *will* instruct CMake to build Clang using
the previously built Clang.

There is apparently [an official way to bootstrap Clang](https://llvm.org/docs/AdvancedBuilds.html):

    cd $ROOT/build
    make -j5 clang
    cmake -G 'Unix Makefiles' \
        -DCLANG_ENABLE_BOOTSTRAP=On \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
    make -j5 stage2

However, when I get to `make -j5 stage2`, it fails with a CMake error:

    Host Clang must be able to find libstdc++4.8 or newer!

So when I bootstrap Clang, I use this crude approach:

    cd $ROOT/build
    cmake -G 'Unix Makefiles' \
        -E env \
        CXXFLAGS="-cxx-isystem /Library/Developer/CommandLineTools/usr/include/c++/v1" \
        CXX="$ROOT/llvm/build/bin/clang++" \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
    cp bin/clang-7 clang-ok
    find ../include/ -name '*.h' | xargs touch
    make -j5 clang

(This takes the same 80 minutes as the original `make -j5 clang` did.)

(If you have checked out `libcxx`, you can try substituting
`$ROOT/llvm/projects/libcxx/include` for `/Library/Developer/CommandLineTools/usr/include/c++/v1`.)

This crude approach will of course overwrite your "good" `build/bin/clang++`
(the one that successfully compiled Clang) with a new version (which for all
you know might *not* compile Clang), so that's why I did `cp bin/clang-7 clang-ok`
before running `make -j5 clang` the second time.
(Make sure you `cp` the actual executable and not just a symlink to it!)
