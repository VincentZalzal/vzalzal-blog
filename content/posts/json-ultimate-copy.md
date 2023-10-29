---
title: 'nlohmann::json: The Ultimate Copy Tool?'
date: 2023-10-22T21:53:27-04:00
tags: [hack, ranges]
---

The `nlohmann::json` library is a C++ JSON parser and generator that is very nice to use. But I ended up using it recently for something completely unrelated to JSON!

<!--more-->

## `nlohmann::json`

If you haven't heard of [`nlohmann::json`](https://github.com/nlohmann/json) before, I suggest you give it a try next time you need to deal with JSON in C++. The library is header-only, is very flexible, and has a modern C++ API.

However, I won't delve into the details of how to use the library, as it has good documentation, and it is not the point of this article.

## Context

I work in a codebase where there are a few instances of custom sequence containers like `std::vector`, for good and not-so-good reasons. Depending on what parts of the codebase I'm working on, I might end up calling a function that provides a container of one type and having to pass it to a function expecting a container of a different type.

To make things simple, I'll use `std::vector`, `std::deque`, `std::list`, and `std::array` as possible sequence containers, but the article applies to non-standard containers as well.

## The problem

Now, imagine you have the following:

```cpp
// in compute.h
std::deque<float> compute_values();

// in summarize.h
float summarize_values(const std::vector<float>& values);
```

You need to compose the two functions, but the type mismatch prevents it. Let's assume that the development cost of changing the API of those functions is not worth it for now, and copying the data over is acceptable.

In this case, it's easy, you don't even need a separate line to construct the intermediate vector:

```cpp
const std::deque<float> values = compute_values();
return summarize_values( {values.begin(), values.end()} );
```

That's when you have a 1D sequence. What happens when you have, say, a 3D sequence, i.e. mismatched triply-nested sequences?

```cpp
using out_t = std::list< std::deque< std::array<float, 3> > >;
out_t compute_values();

using in_t = std::vector< std::vector< std::vector<float> > >;
float summarize_values(const in_t& values);
```

There are many ways to go about it, but my first instinct was to write this:

```cpp
const out_t values = compute_values();

in_t values_copy;
values_copy.reserve(values.size());
for (const auto& values_sub1 : values) {
    auto& values_copy_sub1 = values_copy.emplace_back();
    values_copy_sub1.reserve(values_sub1.size());
    for (const auto& values_sub2 : values_sub1) {
        auto& values_copy_sub2 = values_copy_sub1.emplace_back();
        values_copy_sub2.reserve(values_sub2.size());
        for (float value : values_sub2) {
            values_copy_sub2.push_back(value);
        }
    }
}

return summarize_values(values_copy);
```

How does this code make you feel? Is your answer 🤮? I know I don't like it! I find it verbose and error-prone. There must be a better way.

## The hacky solution

Enters `nlohmann::json`. Behold, the ultimate copy function:

```cpp
template <typename To, typename From>
To copy_to(const From& something) {
    return nlohmann::json(something).template get<To>();
}
```

Which allows me to write this:

```cpp
return summarize_values( copy_to<in_t>( compute_values() ) );
```

Witchcraft, I hear you say? Almost 😉 This impressive result comes from the recursive nature of the `nlohmann::json` library implementation. When constructing a JSON object from `something`, if the thing is a sequence, then each item in the sequence is itself constructed as a JSON object in the same way.

Even better: you can implement JSON support for user-defined types by defining two simple functions. The custom vector-like types I was dealing with in the first place already had JSON support, so this solution worked seamlessly.

Also, while maybe not the most efficient solution, note that there is no file access, nor any conversion between floats and strings involved here. The function is copying the same floats into an intermediate in-memory representation.

Finally, the call to `get<To>()` copies data from this intermediate representation into the new, final representation. Not bad, for a one-liner!

It can even perform some conversions. Be warned though, lossy conversions are allowed:

```cpp
const std::vector<float> vecf = {1.1f, 2.2f, 3.3f};
const auto vecd = copy_to< std::vector<double> >(vecf);
```

I was happy with this clever hack, considering I had multiple different 3D sequences to convert. In any case, this was temporary code that would not get pushed, so this was the perfect solution to save time.

## The real solution: `std::ranges::to`

While writing this article, I realized that [`std::ranges::to`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1206r7.pdf), new in C++23, could potentially solve the issue, in a much cleaner and standard way. Does it work? Let's try:

```cpp
return summarize_values( compute_values() | std::ranges::to<in_t>() );
```

Yes, it works 🎉 Even the conversion works:

```cpp
const auto vecd = vecf | std::ranges::to<std::vector<double>>();
```

At the time of writing, almost all major compilers have already implemented `std::ranges::to`. It is only a matter of time until the ultimate copy tool will be readily available in the standard library.

## Code

You can play with the code in this article on [Compiler Explorer](https://godbolt.org/z/W69sjr38h).
