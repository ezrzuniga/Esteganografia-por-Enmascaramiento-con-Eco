//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//encoder_test.cpp: overlap-add == convolucion de la señal completa, y round-trip con el decodificador.
//
//Compilar desde la raiz del repo:  g++ -std=c++17 -O2 -Icpp/include cpp/tests/encoder_test.cpp -o encoder_test

#include <cmath>
#include <cstdio>
#include <random>

#include "encoder/encoder.hpp"
#include "deco/decode.hpp"

int main() {
    const double FS = 44100;
    const std::size_t WIN_MULT = 16;
    const encoder::Params p = encoder::make_params(1000, 1500, 40, 40, FS);
    const std::size_t L = encoder::window_length(p, WIN_MULT);
    std::printf("delays = (%zu, %zu) samples, window = %zu samples (%.1f ms), rate = %.1f bit/s\n",
                p.d[0], p.d[1], L, 1e3 * L / FS, FS / L);

    std::mt19937 gen(1234);
    std::normal_distribution<double> noise(0.0, 1.0);
    deco::Signal x(8 * L + 123);
    for (auto& s : x) s = noise(gen);

    int fails = 0;
    for (int b = 0; b < 2; ++b) {
        const auto y = encoder::encode(x, std::vector<int>(x.size() / L + 1, b), p, L);
        const auto h = encoder::kernel(p.d[b], p.a[b]);
        double err = 0;
        for (std::size_t n = 0; n < x.size(); ++n) {   // convolucion directa, truncada a len(x)
            double ref = 0;
            for (std::size_t j = 0; j < h.size() && j <= n; ++j) ref += h[j] * x[n - j];
            err = std::fmax(err, std::fabs(ref - y[n]));
        }
        std::printf("bit %d: max |overlap-add - convolve| = %.2e\n", b, err);
        if (err > 1e-9) ++fails;
    }

    const std::vector<int> bits = {1, 0, 1, 1, 0, 0, 1, 0};
    const auto y = encoder::encode(x, bits, p, L);
    const auto r = deco::decode(y, {p.d[0], p.d[1]}, L);
    int wrong = 0;
    for (std::size_t i = 0; i < bits.size(); ++i) wrong += (r.symbols[i] != bits[i]);
    std::printf("round-trip: %d / %zu bits wrong\n", wrong, bits.size());
    if (wrong) ++fails;

    std::puts(fails ? "FAIL" : "overlap-add == full-signal convolution; OK");
    return fails;
}
