---
title: 'Noisy: The Class You Wrote a Hundred Times'
date: 2024-04-21T22:23:01-04:00
tags: [utility]
---

You have probably written a class that prints a message in all its special member functions. And like me, you probably wrote it multiple times. I decided to write it well once and for all, and share it.

<!--more-->

## Context

Recently, I was writing some code involving structured bindings and I was unsure whether it would incur unintended copy or move operations. As usual, when I am in this situation, I open up [Compiler Explorer](https://godbolt.org/) and test it. For the nth time, I ended up coding a class like this one:

```cpp
struct S {
    S() { std::cout << "ctor\n"; }
    ~S() { std::cout << "dtor\n"; }
    // ... and so on with copy and move operations
}
```

I don't know how many times I wrote this class! I thought maybe it was time I write it well, once and for all, and then reuse it when I need it. And then, I thought that I am probably not the only one having written that class over and over again, am I? Maybe this could be useful to others.

## Noisy

So I decided to upload it to GitHub. Here it is: https://github.com/VincentZalzal/noisy. I hope it can be useful to you too!

If you are a user of [Compiler Explorer](https://godbolt.org/), know that you can directly include a file via URL, which is pretty handy in this case.

```cpp
#include <https://raw.githubusercontent.com/VincentZalzal/noisy/main/noisy.h>
```

`vz::Noisy` prints a message for each call to a special member function. As a bonus, at the end of the program, it also prints a summary of the number of calls for each member function. For example, running the following program:

```cpp
vz::Noisy make_noisy() { return {}; }

int main() {
    vz::Noisy x = std::move(make_noisy());
}
```

would print:

```
Noisy( 0): default constructor
Noisy( 1): move constructor from Noisy( 0)
Noisy( 0): destructor
Noisy( 1): destructor

===== Noisy counters =====
Default constructor count:  1
Move constructor count:     1
Destructor count:           2
```

## Other use cases

While the main use case is code exploration, you can also use `vz::Noisy` for testing purposes. The counters can be queried instead of being printed to screen. For example, you could ensure a custom container does not leak objects or make unintended copies.

You can also use it to compare the number of calls to special member functions performed by two different algorithms, like `std::sort` and `std::stable_sort`.

## Conclusion

You will find more details on how to use `vz::Noisy` in the [README file](https://github.com/VincentZalzal/noisy). Do not hesitate to share your thoughts!
