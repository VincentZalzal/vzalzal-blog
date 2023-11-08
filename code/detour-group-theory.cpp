// Tested on x86-64 gcc 13.2
// with options -O3 -std=c++23 -Wall -Wextra -pedantic -Werror

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <set>

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

class Z256Generator {
public:
    int next() {
        state_ = (state_ + 191) % 256;
        return state_;
    }
private:
    int state_ = 42;
};

void test_Z256Generator() {
    std::set<int> s;
    Z256Generator g;
    for (int i = 0; i < 256; i++)
        s.insert(g.next());
    assert(s.size() == 256);
}

void test_SerialNumberGenerator() {
    std::set<std::string> s;
    SerialNumberGenerator g;
    for (int i = 0; i < 676000; i++)
        s.insert(g.next());
    assert(s.size() == 676000);
}

int main() {
    test_Z256Generator();
    test_SerialNumberGenerator();

    SerialNumberGenerator g;
    std::cout << g.next() << ' ' << g.next() << '\n';
}
