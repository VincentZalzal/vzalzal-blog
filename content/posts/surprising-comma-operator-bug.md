---
title: 'A Surprising Bug Involving the Comma Operator'
date: 2023-10-09T20:51:15-04:00
tags: [bug, operators]
---

Recently, I had to investigate a bug that I found interesting enough I wanted to share the story, bringing my blog back to life in the process!

<!--more-->

## Context

The codebase I work in involves a lot of linear algebra, so it is no surprise it has its own matrix class. To simplify things, I'll distill all classes down to the simplest form that still illustrates the bug, and avoid showing complete implementations here.

Suppose you have a class to represent a (mathematical) vector of four coordinates:

```cpp
template <typename T>
struct Vec4 {
    T values[4];
    // ...
};

using Vec4d = Vec4<double>;
```

The class is inspired by [Eigen](https://eigen.tuxfamily.org/), which provides an "interesting" way to initialize objects:

```cpp
Vec4d vec;
vec << 1, 2, 3, 4; // vec == [1, 2, 3, 4]
```

How would you go about implementing support for such a syntax? By overloading both `operator<<` and `operator,`!

##  The comma operator

This story involves the comma operator, i.e. `operator,`. The first time I heard about the comma operator was while reading *More Effective C++* by Scott Meyers, and it was to tell me not to overload it. Nevertheless, existing code exists, and you might find some overload of this operator in your codebase too, so it is good to know how the built-in operator works.

The built-in comma operator first evaluates the expression on the left, then the expression on the right, then returns the result of the expression on the right. 

Scott Meyers' reason not to overload `operator,` was related to the guarantee (or lack thereof) on the order of evaluation of the arguments. But in our case, the problem is different.

## Implementation of the comma initializer

Here is how one can implement the initialization syntax above:

```cpp
template <typename T> struct CommaInit;

template <typename T>
struct Vec4 {
    T values[4];
    CommaInit<T> operator<<(T value) { values[0] = value; return {this, 1}; }
};

template <typename T>
struct CommaInit {
    Vec4<T>* vec;
    size_t   idx;
    CommaInit operator,(T value) { vec->values[idx++] = value; return *this; }
};
```

Pretty simple, really. The `Vec4` class provides an `operator<<` that initializes the first value, then returns a temporary object of type `CommaInit` to track the index of the next element to initialize. In turn, the `CommaInit` class overloads `operator,` to initialize the next value and increment the index.

## Usage

Since you work a lot with 3D points in [homogeneous coordinates](https://en.wikipedia.org/wiki/Homogeneous_coordinates), you implement a helper function to create a 3D point along the X axis:

```cpp
template <typename T>
Vec4<T> point_along_x(T x) {
    Vec4<T> vec;
    vec << x, 0, 0, 1;
    return vec;
}
```

When you test it, all goes well:

```cpp
Vec4d vec = point_along_x(1.618); // [1.618, 0, 0, 1]
```

## User-defined types

Let's say you want to use the `Vec4` class, but not with a built-in type. For the sake of simplicity again, assume you want to use an existing third-party safe numerics library that provides the following type:

```cpp
namespace safe {
    struct f64 {
        double value = 0;

        f64() = default;
        explicit f64(double v) : value(v) {}
    };
}

using Vec4_f64 = Vec4<safe::f64>;
```

When you test your code with `safe::f64` though, you get a surprising result:

```cpp
Vec4_f64 vec = point_along_x(safe::f64(1.618)); // [1.618, 0, 0, 0] !!
```

What is happening here‽ Why is the last coordinate 0 instead of 1? Try and see if you can figure it out.

## A fundamental difference

There is a fundamental difference between the built-in `operator,` and the other binary operators. Let's compare below:

```cpp
struct A {};
struct B {};
B b1 = A() + B();  // error: no match for 'operator+'
B b2 = (A(), B()); // compiles!
```

**The built-in `operator,` will accept any type without the user having to overload it!**

 This means that there is a danger, when you overload `operator,`, that the built-in one gets called instead of your version if the arguments are not exactly right.

## The bug

In `point_along_x()`, `vec << x` returns a temporary object of type `CommaInit<safe::f64>`. Its `operator,` takes a parameter of type `safe::f64`.

`(vec << x), 0` tries to call `operator,(CommaInit<safe::f64>, int)`. The compiler checks whether `CommaInit<safe::f64>::operator,` should be part of overload resolution. However, because an int cannot be converted to a `safe::f64` (its constructor is `explicit`!), it is rejected. Thus, the built-in `operator,` gets called, which simply returns the right-hand side, that is, the integer 0.

After that, the remaining two calls to `operator,` only involve integers, and thus do nothing. Since the default constructor for `safe::f64` initializes its value to 0, we get 0 for all components except the first one. Ouch!

## Possible solutions

As stated, the user-defined comma operator was rejected because the constructor for `safe::f64` is `explicit`. If you have control over that class, making the conversion implicit would solve the problem, although this is not a great solution for type safety.

A better solution would be to explicitly call the constructor in `point_along_x`:

```cpp
template <typename T>
Vec4<T> point_along_x(T x) {
    Vec4<T> vec;
    vec << x, T(0), T(0), T(1); // explicit constructor calls
    return vec;
}
```

## Enable all the warnings!

This bug would have been caught, had the compiler warning level been higher. For example, if using `--Wall --Werror` on gcc, you would get this:

```txt
error: right operand of comma operator has no effect [-Werror=unused-value]
```

## Code

You can play with the code in this article on [Compiler Explorer](https://godbolt.org/z/KMTTjzvj7).

## Conclusion

This is yet again a compelling example of using all the tools at your disposal, like higher warning levels, to catch bugs at compile-time. But more importantly, it is also yet another reason not to overload the comma operator!
