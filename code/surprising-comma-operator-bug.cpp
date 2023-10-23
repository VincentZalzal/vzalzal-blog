// Tested on x86-64 gcc 13.2 with -O3

#include <cstddef>
#include <iostream>

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

template <typename T>
std::ostream& operator<<(std::ostream& os, const Vec4<T>& vec) {
    return os << '[' << vec.values[0] << ", " << vec.values[1] << ", " << vec.values[2] << ", " << vec.values[3] << ']';
}

namespace safe {
    struct f64 {
        double value = 0;

        f64() = default;
        explicit f64(double v) : value(v) {}  // removing explicit would avoid the bug (but not a great solution)
    };

    std::ostream& operator<<(std::ostream& os, f64 d) {
        return os << d.value;
    }
}

using Vec4d    = Vec4<double>;
using Vec4_f64 = Vec4<safe::f64>;

template <typename T>
Vec4<T> point_along_x(T x) {
    Vec4<T> vec;
    vec << x, 0, 0, 1;
    // vec << x, T(0), T(0), T(1); would work here
    return vec;
}

int main() {
    {
        Vec4d vec;
        vec << 1, 2, 3, 4; // vec == [1, 2, 3, 4]
        std::cout << vec << std::endl;
    }

    {
        Vec4d vec = point_along_x(1.618); // [1.618, 0, 0, 1]
        std::cout << vec << std::endl;
    }

    {
        // Adding -Wall -Werror would catch this!
        Vec4_f64 vec = point_along_x(safe::f64(1.618)); // [1.618, 0, 0, 0] !!
        std::cout << vec << std::endl;
    }

    {
        struct A {};
        struct B {};
        // B b1 = A() + B(); // error: no match for 'operator+'
        B b2 = (A(), B()); // compiles!
    }
}
