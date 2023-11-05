---
title: 'Unique Serial Number'
date: 2023-11-08T07:26:06-04:00
tags: [random, math]
math: true
---

While solving coding exercises, I stumbled upon a problem for which I found an interesting solution I wanted to share, with some nice mathematics involved. 

<!--more-->

## The Problem: Unique serial number

Here is the problem statement:

> Implement a class that generates serial numbers of the form `AB123`, that is, two letters followed by three digits. Each time a new serial number is generated, it should be different from all previously returned values, until all possible serial numbers have been generated. Generate the serial numbers in a pseudorandom order.

The solution I ended up implementing was not widespread among the submitted programs, so I thought I would share it here. Also, it is a good excuse to sneak in some group theory on the blog 😉

For simplicity, let's ignore what happens after all serial numbers have been exhausted. Let's use the following API:

```cpp
class SerialNumberGenerator {
public:
    std::string next();
};
```

## Common solutions

I'll now go through some possible solutions.

### The hash table solution

By far the most common C++ solution I've seen follows this pattern:

- Generate two random letters and three random digits using some random number generator.
- Check whether the generated serial number exists in the `std::unordered_set` of previous numbers.
- If so, try again; if not, add the serial number to the set and return it.

I have a feeling that coding interview questions have trained us to overuse hash tables, as they are usually part of the expected answer. However, in this case, I think the solution above is far from the best. The more serial numbers are generated, the slower it gets. At the limit, trying to generate the last remaining serial number might take a while, as you will try to stumble upon it randomly in a loop.

Let $N = 26^2 \times 10^3 = 676 000$ be the number of possible serial numbers. Assuming you want to call `next()` $N$ times, I worked out that this solution would require on the order of $O(N \log N)$ tries to find all the numbers, which makes the time complexity of a single call to `next()` amortized $O(\log N)$ (but don't quote me on that 😅). Also, the hash table would require $O(N)$ bytes of memory. We can do better than that.

### The random shuffle solution

One can observe that calling `next()` $N$ times generates a permutation of the sequence of all $N$ serial numbers. Thus, a solution is to generate one such permutation in advance, and then return the next element in the permutation on each call to `next()`. Here is one way to do it:

- Fill a `std::vector` with all possible serial numbers in order (for example, using five nested for loops).
- Call [`std::shuffle`](https://en.cppreference.com/w/cpp/algorithm/random_shuffle) on the vector.
- Keep the index of the serial number to return in the following call to `next()`.

The construction of the shuffled vector requires $O(N)$ operations upfront, which might or might not be acceptable. At runtime, calling `next()` is $O(1)$. We still require $O(N)$ bytes of memory.

## Some group theory

We could improve on the previous solution if we could find a way to generate a permutation as we go. Is there some kind of update formula to generate a serial number given the previous one, such that we go through all possible serial numbers exactly once? That's where group theory comes in.

### Cyclic groups

To be succinct, in group theory, a group is a set of elements under some binary operation. For example, the set of integers in $[0, 256)$ under addition modulo 256 forms a group, that I'll denote informally as $\mathbb{Z}\_{256}$.

A group is [cyclic](https://en.wikipedia.org/wiki/Cyclic_group) if it is possible to generate all the elements of the group from a single one, by applying the binary operation repeatedly. That element is called a generator of the group. As an obvious example, 1 would be a generator of $\mathbb{Z}\_{256}$, as adding 1 repeatedly will cycle through all the elements of the group. 2 would not be a generator, as adding 2 does not change the parity of a number, and thus it would generate the subset of either even or odd integers in $\mathbb{Z}\_{256}$.

But 1 is not the only possible generator of $\mathbb{Z}\_{256}$. For example, 191 would also be a valid generator. Starting from 0, adding 191 repeatedly (mod 256) would yield the sequence 0, 191, 126, 61, 252, 187, ..., 130, 65, and then back to 0, going through all the 256 possible values. It is possible to prove that any element that is relatively prime to 256 would also be a valid generator.

So if the serial numbers we are trying to generate were instead the numbers 0 through 255, we could generate a permutation of all the numbers on the fly easily. Pick some element (it doesn't matter which) as the initial state of the generator, then add 191 modulo 256 in `next()`:

```cpp
class Z256Generator {
public:
    int next() {
        state_ = (state_ + 191) % 256;
        return state_;
    }
private:
    int state_ = 42;
};
```

### Better randomness

`Z256Generator` generates a sequence that we could barely call pseudorandom. We require a group of $676 000$ elements (the count of serial numbers) using a different operation with better randomness. Then, it would just be a matter of mapping each integer in $[0, 676 000)$ to a unique serial number of the form `AB123` to complete the assignment.

It just so happens that the C++ Standard Library has exactly what we need: [`std::linear_congruential_engine`](https://en.cppreference.com/w/cpp/numeric/random/linear_congruential_engine).

## The LCG solution

A [linear congruential generator](https://en.wikipedia.org/wiki/Linear_congruential_generator) (LCG) uses an affine transformation instead of simple addition:

$$x_{i+1} \leftarrow (a x_i + c) \mod m$$

If we choose parameters $a$, $c$, and $m$ carefully, it is possible to ensure each integer in $[0, 676 000)$ is generated once, in good pseudorandom order.

### Parameter selection

In our case, $m = 676 000$ is the number of elements in the group. To select $a$ and $c$, [refer to Wikipedia](https://en.wikipedia.org/wiki/Linear_congruential_generator#c_%E2%89%A0_0):

1. $c \neq 0$ (because $m$ is neither prime nor a power of 2).
2. $m$ and $c$ are relatively prime.
3. $a-1$ is divisible by all prime factors of $m$.
4. $a-1$ is divisible by 4 (because $m$ is divisible by 4).
5. $a−1$ should not be any more divisible by prime factors of $m$ than necessary to ensure good randomness.

1 is a common choice for $c$.

$m$ can be decomposed into $2^5 \times 5^3 \times 13^2$. Selecting $a-1 = 2^2 \times 5 \times 13$ meets the remaining criteria, yielding $a = 261$.

### Mapping to serial numbers

A serial number can be interpreted as a 5-digit number, the first two digits in base 26 and the last three digits in base 10. Converting an integer in $[0, 676 000)$ to a serial number then boils down to a base conversion, similar to what you would do to convert a number to hexadecimal, for example.

### The full LCG solution

Here is the code for the full solution. It has a complexity of $O(1)$, both for time and memory. As you can see, most of the code is used to map an integer to a serial number.

```cpp
class SerialNumberGenerator {
public:
    std::string next() {
        std::string serial;
        const auto add_digit = [&serial](int val, int base, char offset) {
            const auto res = std::div(val, base);
            serial.push_back(offset + res.rem);
            return res.quot;
        };

        auto value = static_cast<int>(lcg_());
        
        value = add_digit(value, 26, 'A');
        value = add_digit(value, 26, 'A');
        value = add_digit(value, 10, '0');
        value = add_digit(value, 10, '0');
        value = add_digit(value, 10, '0');

        return serial;
    }

private:
    using Lcg = std::linear_congruential_engine<std::uint_fast32_t, 261, 1, 676'000>;
    Lcg lcg_{123'456};
};
```

## Conclusion

I hope you enjoyed this little detour into group theory. I think using an LCG is an elegant solution to the problem, albeit not an obvious one. In production code, this would warrant a sizable comment block to make the lives of future maintainers easier.

As usual, you can play with the code in this article on [Compiler Explorer](https://godbolt.org/z/MavxsP6n8).

### Side note: Exercism

I changed the wording of the original problem, [*Robot Name*](https://exercism.org/tracks/cpp/exercises/robot-name), which can be found on the [Exercism](https://exercism.org) website. If you want to learn a programming language, I would recommend you try it out. It has some nice features that set it apart from similar sites I've tried:

- You can solve problems either using the online IDE or using your local IDE and their command-line program to submit your solution.
- Problems are geared towards language basics and are not focussing on e.g. implementing a self-balancing tree or other classical interview problems.
- After solving a problem, you can look at other users' solutions, which usually focus more on good style than obscure cleverness or terseness.
- You can ask for mentoring from another user.

This promotion is not sponsored in any way. I just had a great experience and wanted to share.