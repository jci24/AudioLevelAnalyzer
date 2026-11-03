#pragma once

#include <cmath>
#include <iostream>

namespace tests
{
// Use a relative tolerance because floating-point calculations may round.
// An expected zero must match zero exactly.
inline bool expectNear(const char* name, double actual, double expected)
{
    const double tolerance = 0.000001 * std::abs(static_cast<double>(expected));
    const bool passed = std::isfinite(actual)
        && std::abs(static_cast<double>(actual) - expected) <= tolerance;

    std::cout << (passed ? "PASS: " : "FAIL: ") << name
              << " (expected " << expected << ", got " << actual << ")\n";
    return passed;
}

inline bool expectTrue(const char* name, bool condition)
{
    std::cout << (condition ? "PASS: " : "FAIL: ") << name << '\n';
    return condition;
}
}
