// Tested on x86-64 clang 17.0.1
// with options -O3 -stdlib=libc++ -std=c++23 -Wall -Wextra -pedantic -Werror

#include <array>
#include <deque>
#include <iostream>
#include <list>
#include <ranges>
#include <vector>

#include <nlohmann/json.hpp>

namespace seq1d {

// in compute.h
std::deque<float> compute_values();

// in summarize.h
float summarize_values(const std::vector<float>& values);

std::deque<float> compute_values() {
    return {3, 1, 4, 1, 6};
}

float summarize_values(const std::vector<float>& values) {
    return values[1];
}

float test() {
    const std::deque<float> values = compute_values();
    return summarize_values({values.begin(), values.end()});
}

}

namespace seq3d {

using out_t = std::list<std::deque<std::array<float, 3>>>;
out_t compute_values();

using in_t = std::vector<std::vector<std::vector<float>>>;
float summarize_values(const in_t& values);

out_t compute_values() {
    return {
        {
            {1.1f, 2.2f, 3.3f},
            {4.4f, 5.5f, 6.6f}
        },
        {
            {11.1f, 22.2f, 33.3f},
            {44.4f, 55.5f, 66.6f}
        },
    };
}

float summarize_values(const in_t& values) {
    return values[1][1][1];
}

float test() {
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
}

template <typename To, typename From>
To copy_to(const From& something) {
    return nlohmann::json(something).template get<To>();
}

float test_json() {
    return summarize_values(copy_to<in_t>(compute_values()));
}

// Warning: can be lossy (e.g. float -> int)
double test_conv() {
    const std::vector<float> vecf = {1.1f, 2.2f, 3.3f};
    const auto vecd = copy_to<std::vector<double>>(vecf);
    return vecd[1];
}

float test_ranges() {
    return summarize_values( compute_values() | std::ranges::to<in_t>() );
}

double test_conv_ranges() {
    const std::vector<float> vecf = { 1.1f, 2.2f, 3.3f };
    const auto vecd = vecf | std::ranges::to<std::vector<double>>();
    return vecd[1];
}

}

int main() {
    std::cout << "seq1d:               " << seq1d::test() << '\n';
    std::cout << "seq3d:               " << seq3d::test() << '\n';
    std::cout << "seq3d (json):        " << seq3d::test_json() << '\n';
    std::cout << "conversion:          " << seq3d::test_conv() << '\n';
    std::cout << "seq3d (ranges):      " << seq3d::test_ranges() << '\n';
    std::cout << "conversion (ranges): " << seq3d::test_conv_ranges() << '\n';
}
