//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//rcc.hpp
// RCC Real Cepstrum-like: transforma de una del cuadro.

// Pasos de la transformacion de un cuadro de entrada `v_in` a un cuadro de salida `out`:
//   1. Normalizar la trama a RMS = 2^15.
//   2. Aplicar una ventana de Hann (definición de numpy.hanning, simétrica).
//   3. Calcular el logaritmo de la magnitud del espectro (unilateral, N/2+1 bins).
//   4. Elevarlo al cuadrado y aplicar la transformada inversa para volver a una secuencia real de longitud N.
// Devuelve un vector de la misma longitud que `v_in`.

#ifndef DECO_RCC_HPP
#define DECO_RCC_HPP

#include <cmath>

#include "types.hpp"
#include "fft.hpp"

namespace deco {


inline Signal rcc(const Signal& v_in) {
    const std::size_t N = v_in.size();
    if (N == 0) return {};

    // Paso 1 
    Real acc = 0.0;
    for (Real x : v_in) acc += x * x;
    const Real rms = std::sqrt(acc / static_cast<Real>(N));
    const Real gain = kScale / (rms + kEps);

    //Paso 2
    std::vector<Complex> X(N);
    for (std::size_t n = 0; n < N; ++n) {
        const Real w = (N == 1)
            ? 1.0
            : 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<Real>(n) / static_cast<Real>(N - 1));
        X[n] = Complex(v_in[n] * gain * w, 0.0);
    }

    // Paso 3
    detail::fft_any(X, false);
    const std::size_t half = N / 2;
    std::vector<Real> C2(half + 1);
    for (std::size_t k = 0; k <= half; ++k) {
        const Real c = std::log(std::abs(X[k]) + kEps);
        C2[k] = c * c;
    }

    // Paso 4
    std::vector<Complex> S(N);
    for (std::size_t k = 0; k <= half; ++k) S[k] = Complex(C2[k], 0.0);
    for (std::size_t k = 1; k < N - half; ++k) S[N - k] = S[k];

    detail::fft_any(S, true);

    Signal out(N);
    for (std::size_t n = 0; n < N; ++n)
        out[n] = S[n].real() / static_cast<Real>(N);
    return out;
}

}  // namespace deco

#endif  // DECO_RCC_HPP
