---
title: "C++ templates: a language in the language"
date: 2019-09-09T09:06:28-04:00
tags: [templates, SFINAE, speculative, readability]
---

C++ templates are a wonderful tool, with unforeseen capabilities at the time of their inception. They have grown into a sub-language inside of C++, with their own branching and looping constructs.

<!--more-->

I think this is hurting C++ in the long run, and we should focus on making template metaprogramming look like normal code.

## Reminiscence

I remember the first time I stumbled upon Erwin Unruh's [first template metaprogram](http://www.erwin-unruh.de/primorig.html). Using C++ templates, he showed it was possible to compute the prime numbers at compile-time. This completely blew my mind.

Executing code at compile-time held a lot of potential. I dived into C++ templates. I was using them in all my projects at University. I even wrote a series of articles in the students' journal about it.

Later, at work, I used C++ templates in framework code. I became *the template guy*.

What I did not realized at the time is that many fellow software developers are intimidated by C++ template metaprogramming, and will prefer avoiding a complicated-looking framework altogether and reimplement the functionality, instead of embracing it. For a company with a large codebase, it becomes a problem. Why this unease? I think it is mainly due to syntax.

## Disillusionment

Never underestimate the power of syntax! C++ templates were created as class and function generators: you provide a type or value, and you get a specific version of the class or function. It was not initially intended to have flexible branching and looping constructs.

Yes, C++ templates were proven to be Turing-complete. However, their syntax is drastically different than 'normal' C++. It is a language inside the C++ language.

It is at that point that I started to favor code that is clear for the majority, rather than clever code, and I reduced my usage of templates. But around the same time came C++11 and C++14, and template metaprogramming got really serious.

My opinion is that template metaprogramming is a very powerful tool, but not well integrated to the language, and C++11 and C++14 did not help. Now, only experts can stay up-to-date and really understand the intricacies of all the type traits and SFINAE tricks.

For the future of C++, I really hope that template metaprogramming looks more like traditional non-template code.

## Examples

### `std::void_t`

I think template metaprogramming that involves [SFINAE](https://en.cppreference.com/w/cpp/language/sfinae) is what many developers would consider arcane. Take for example the code sample for `std::void_t` on [cppreference](https://en.cppreference.com/w/cpp/types/void_t):

```cpp
template <typename T, typename = void>
struct is_iterable : std::false_type {};
template <typename T>
struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()),
                                  decltype(std::declval<T>().end())>>
    : std::true_type {};

int main()
{
    std::cout << is_iterable<std::vector<double>>::value; // prints 1
}
```

So many things are confusing here... First, a `struct` is used as a mechanism to create a compile-time function. Something that is supposed to be used to group data is actually used to compute a compile-time value and holds no data.

That structure has two template parameters, but the second one is not *really* a parameter to the 'function'. The second parameter (which is not even named) is used so that, when instantiated with a specific type T, if it would fail to compile, that specialization is ignored. This is SFINAE in action. And this is why the `= void` is needed in the primary template.

So really, the second parameter is used for branching, depending on whether the type T supports the `begin()` and `end()` member functions.

Also, `decltype` and `declval` hurt readability here. `declval` is needed only because we can't be sure T is default-constructible. We need a way to instantiate a T. Finally, `decltype` is needed to transform the values of `begin()` and `end()` back to their respective types, because this is what `void_t` expects.

Now, imagine some C# programmer is developing a UI on top of a C++ library and has to debug inside of the C++ code. Most C# (or Java, or Python, etc) programmer would not have too much difficulty understanding usual business logic C++ code. But if they have to understand the sample above, it is another story. Why? Because it does not look like a C-inspired language at all.

Here is my take on rewriting the code sample, but with the usual C-inspired constructs.

```cpp
// Wishful thinking, won't compile
bool is_iterable(std::type_t T) { // type represented as value
   return is_compilable {         // language construct that returns a bool
      T.instance().begin();       // T.instance() is clearer than std::declval<T>()
      T.instance().end();
   };
}

int main() {
   // normal function call, evaluated at compile-time
   std::cout << is_iterable(std::vector<double>); // implicit cast to std::type_t
}
```

I think most programmers would intuitively understand the code above, because it uses the same constructs as normal code, apart from `is_compilable`. It's been a while now that we've been looking for a standard library solution for something like `is_compilable`. Maybe it would be worth it to define it as a language construct instead.

**Edit 2019-09-09**: redditer /u/sphere991 [pointed out](https://www.reddit.com/r/cpp/comments/d1qrsm/c_templates_a_language_in_the_language/ezr305w?utm_source=share&utm_medium=web2x) that C++20 concepts will simplify a lot of template metaprogramming. The example above reduces to:

```cpp
template <class T>
concept is_iterable = requires (T& x) {
    x.begin();
    x.end();
};

int main() {
    std::cout << is_iterable<std::vector<double>>;
}
```

I am more hopeful now, thanks to concepts!

### `std::enable_if`

[cppreference](https://en.cppreference.com/w/cpp/types/enable_if) has a similar example for `std::enable_if`:

```cpp
// the partial specialization of A is enabled via a template parameter
template<class T, class Enable = void>
class A {}; // primary template

template<class T>
class A<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
}; // specialization for floating point types

int main()
{
    A<int> a1; // OK, matches the primary template
    A<double> a2; // OK, matches the partial specialization
}
```

Again, structures are abused here, in the form of type traits. `std::enable_if` is a branching construct, while `std::is_floating_point` would fit nicely as a member function of a `type_t` class for types.

What I would like here is some kind of `if constexpr` at global scope. Something like:

```cpp
// Wishful thinking, won't compile
template <typename T>
if (T.is_floating_point()) {
   class A {}; // primary template
} else {
   class A {}; // specialization for floating point types
}
```

If I'm not mistaken, C++20 concepts will help in the example above, but there are many cases where a construct that would conditionally inject code anywhere would be very useful.

### Fold expressions

Since the introduction of variadic templates, the need to manipulate parameter packs while avoiding recursion fostered the addition of new language constructs, like `sizeof...`. Fold expressions were introduced in C++17 to facilitate iteration. Taking the code sample on [cppreference](https://en.cppreference.com/w/cpp/language/fold):

```cpp
template<typename ...Args>
void printer(Args&&... args) {
    (std::cout << ... << args) << '\n';
}
```

Those `...` were non-existent prior to C++11, and even now are reserved for templates only. While this feature is elegant, it is surprising when compared to the rest of the language.

What are we really trying to achieve here? Iteration (fold expression) over a list of parameters of possibly different types (parameter pack). Note that `...` is used as language construct in the former case, and as type in the latter.

I would rewrite this using some kind of 'constexpr range-based for loop':

```cpp
// Wishful thinking, won't compile
void printer(pack_t args) { // magical type! List of pairs <type, arg>
   for (auto&& arg : args)  // type of arg would change here
      std::cout << arg;
   std::cout << '\n';
}
```

Another option would be to support `...` for non-template code also, as an alternative to for loops. Adoption would be faster, and at least, there would be consistency between the template and non-template worlds.

## Future

I think `constexpr if` and the addition of more and more `constexpr` support is the way to go. It allows compile-time programming using standard syntax.

If we also have some class type (like `type_t` above) used to describe types as values, this would simplify a lot of code, and reduce the need for more and more type_traits. There are currently multiple proposals for static reflection.

I've heard about the [Boost.Hana](https://www.boost.org/doc/libs/1_71_0/libs/hana/doc/html/index.html) library from Louis Dionne, but haven't had the time to look at it. If I understand correctly, it allows computation on type using standard syntax, which looks promising. I'd be curious of any feedback on using this library in production code.

## Conclusion

C++ template metaprogramming is already complex enough. If we want more people using this powerful tool, we should strive for a syntax that looks like normal code, and avoid having to learn a second language. Not every C++ developer should be a C++ expert.

Do you use metaprogramming at work? How well is it received by your colleagues?

Note: original comments can be found on [Reddit](https://www.reddit.com/r/cpp/comments/d1qrsm/c_templates_a_language_in_the_language/).