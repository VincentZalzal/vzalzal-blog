+++
title = 'Readability of Logical Operators'
date = 2019-09-13T23:49:21-04:00
tags = ['readability', 'style', 'operators']
+++

Operators generally lead to a terse and familiar syntax that usually helps readability. However, this is not always the case. I want to present an alternative here, and I wonder why it isn't used more often.

<!--more-->

## Usage examples of logical operators

### Complex conditions

Assume a function used to change the sound volume of an application. The function ignores an input that is out of bounds.

```cpp
constexpr int MIN_VOLUME =  0;
constexpr int MAX_VOLUME = 11;

void setVolume(int volume) {
   if (MIN_VOLUME <= volume && volume <= MAX_VOLUME) {
      // change the volume
   }
}
```

Now, let's say you prefer early returns (which is a controversial topic in itself) to avoid indenting the whole function.

```cpp
void setVolume(int volume) {
   if (/* volume out of bounds */)
      return;

   // change the volume
}
```

What is the most readable way of writing the condition here? Applying De Morgan's law and negating the inequalities yield:

```cpp
   if (volume < MIN_VOLUME || volume > MAX_VOLUME)
```

While negating the inequalities, there is always a danger: what is the negation of `<=`? `>`, or `>=`? Also, by negating the inequalities, we lose the nice mathematical syntax where the variable is in-between its two limits.

So, maybe we could let the optimizer do the negation:

```cpp
   if (!(MIN_VOLUME <= volume && volume <= MAX_VOLUME))
```

I prefer the math syntax in general, but here, the `!` operator is easy to miss while skimming through the code. This can be minimized by adding whitespace:

```cpp
   if ( ! (MIN_VOLUME <= volume && volume <= MAX_VOLUME))
```

### Predicates

Classes usually define predicates, that is, functions returning `bool`. For example:

```cpp
void process(const std::vector<float>& vec) {
   if (!vec.empty()) {
      // process the vector
   }
}
```

### Standard library

Many classes in the standard library use `operator bool` or `operator!` to determine if an object is in a 'valid' state:

- `std::basic_ios::operator!`: checks whether an error has occurred on the associated stream
- `std::unique_ptr::operator bool`: checks whether the `unique_ptr` owns an object
- `std::optional::operator bool`: checks whether the `optional` contains a value
- etc

Using `std::optional` as an example:

```cpp
std::optional<std::string> readStuff();

void doStuff() {
   auto stuff = readStuff();
   if (!stuff)
      return;
   // continue using *stuff
}
```

## Alternative tokens

Did you know about [alternative tokens](https://en.cppreference.com/w/cpp/language/operator_alternative)? C++ allows using alternative spellings for several operators. I want to focus on the following three:

- `!`: `not`
- `&&`: `and`
- `||`: `or`

It may seem obvious, and you may have seen these alternative spellings a lot in python code, but they are almost non-existent in C++ code.

I wonder why? We seem to be using the punctuation versions out of habit, by convention, because this is what we see everywhere and this is what we have been taught. But apart from that, I wonder if there is any reason to prefer the punctuation versions over their wordy counterparts.

Don't you think the volume example is more readable when using `not`?

```cpp
void setVolume(int volume) {
   if (not (MIN_VOLUME <= volume && volume <= MAX_VOLUME))
      return;
   // ...
}
```

You could go one step further and use `and` too:

```cpp
   if (not (MIN_VOLUME <= volume and volume <= MAX_VOLUME))
```

How about this?

```cpp
void process(const std::vector<float>& vec) {
   if (not vec.empty()) {
      // ...
   }
}
```

```cpp
std::optional<std::string> readStuff();

void doStuff() {
   auto stuff = readStuff();
   if (not stuff)
      return;
   // ...
}
```

Also, depending on your IDE, tokens `and`, `or` and `not` may be highlighted as keywords, which is a nice side effect. Conversely, some IDEs can also highlight operators, so YMMV.

### Note for Visual Studio

Under Visual Studio, you need to have *Conformance mode* enabled for the alternative tokens to be recognized. This mode is now enabled by default in new projects. For older projects, look for the `/permissive-` switch.

### For operator declarations

The alternative tokens can also be used when declaring or defining an operator. For example:

```cpp
class MyClass
{
   MyClass operator not();
};
```

### Anywhere, really

The alternative tokens can be used anywhere the corresponding punctuation could be used:

```cpp
class MyClass
{
   MyClass(MyClass and); // this is a move constructor!
};
```

But please: **just say no**!

## Conclusion

I prefer using `&&` and `||`, maybe out of habit. Still, I would argue that it is easier to spot punctuation between two words, because they don't use letters, so I would not recommend using `and` or `or`.

However, I would certainly consider using `not` instead of `!`. First, it is a unary operator, so the previous argument does not hold. Also, `!` is the mathematical symbol for factorial, not logical negation (which is `¬` or `~` usually).

Did you know that using `and`, `or` and `not` was legal in C++? Would you consider using them instead of their punctuation counterparts? Have you seen them in production code?

Note: original comments can be found on [Reddit](https://www.reddit.com/r/cpp/comments/d8183w/readability_of_logical_operators/).
