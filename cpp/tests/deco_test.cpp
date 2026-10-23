//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
// deco_demo.cpp
// Testbench de deco/*
// Compilar desde la raiz del repo:  g++ -std=c++17 -O2 -Icpp/include cpp/tests/deco_test.cpp -o deco_test

#include <cstdio>
#include <cmath>
#include <random>

#include "deco/decode.hpp"

int main() {
    const deco::Indices d = {40, 55, 70, 90};
    const std::size_t L = 2048;
    std::mt19937 gen(1234);
    std::normal_distribution<double> noise(0.0, 1.0);
    deco::Signal y(L * 4);
    for (std::size_t s = 0; s < y.size(); s += L) {
        for (std::size_t n = 0; n < L; ++n) {
            y[s + n] = noise(gen);
            if (n >= d[2]) y[s + n] += 0.8 * y[s + n - d[2]];   // echo
        }
    }
    const deco::DecodeResult r = deco::decode(y, d, L);
    for (std::size_t i = 0; i < r.symbols.size(); ++i)
        std::printf("frame %zu: symbol=%d score=%.3f\n", i, r.symbols[i], r.scores[i]);
    return 0;
}
