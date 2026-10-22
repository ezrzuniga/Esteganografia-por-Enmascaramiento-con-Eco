//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//main.cpp: versión de prueba (sin WAV todavía)

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <random>

#include "cli/cli.hpp"
#include "deco/decode.hpp"

constexpr std::size_t WIN_MULT = 16;   // window length = WIN_MULT * max delay (R33, R42)

int main(int argc, char* argv[]) {
    const cli::Options opt = cli::parse_args(argc, argv);   // exits on bad arguments (R7)
    const std::size_t L = WIN_MULT * static_cast<std::size_t>(std::max(opt.d0, opt.d1));

    std::cout << "modo     : " << (opt.encoding() ? "codificar" : "decodificar") << '\n'
              << "entrada  : " << opt.input << '\n'
              << "salida   : " << (opt.output.empty() ? "-" : opt.output) << '\n'
              << "canal    : " << (opt.channel ? "right" : "left") << '\n'
              << "retardos : " << opt.d0 << " / " << opt.d1 << " muestras\n"
              << "ventana  : " << L << " muestras\n";
    for (const auto& [k, v] : opt.meta) std::cout << "meta     : " << k << " = " << v << '\n';

    // Smoke test of the decoder: white noise with a bit-1 echo in every window.
    const double a1 = opt.att1_pct / 100.0;
    const std::size_t t1 = static_cast<std::size_t>(opt.d1);
    std::mt19937 gen(1234);
    std::normal_distribution<double> noise(0.0, 1.0);

    deco::Signal x(4 * L), y(4 * L);
    for (auto& s : x) s = noise(gen);
    for (std::size_t n = 0; n < y.size(); ++n)                       // y = x * (d(n) + a1 d(n - t1))
        y[n] = x[n] + (n >= t1 ? a1 * x[n - t1] : 0.0);

    const deco::Indices d = {static_cast<std::size_t>(opt.d0), t1};
    const deco::DecodeResult r = deco::decode(y, d, L);

    std::cout << "\nprueba del decodificador (se espera bit = 1):\n";
    for (std::size_t i = 0; i < r.symbols.size(); ++i)
        std::printf("  ventana %zu: bit=%d  z=%.2f\n", i, r.symbols[i], r.scores[i]);
    return 0;
}