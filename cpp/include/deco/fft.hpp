//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//fft.hpp

#ifndef DECO_FFT_HPP
#define DECO_FFT_HPP

#include <cmath>     // cos, sin
#include <utility>   // std::swap
#include <vector>

#include "types.hpp"

namespace deco {
namespace detail {

//FFT de radix-2 iterativa en el lugar. El tamaño de `a` DEBE ser una potencia de dos.
// `inverse` selecciona el signo del exponente; NO se aplica escalado 1/N aquí.

inline void fft_pow2(std::vector<Complex>& a, bool inverse) {
    const std::size_t n = a.size();

    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;   // propagate the carry
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }

    for (std::size_t len = 2; len <= n; len <<= 1) {
        const Real ang = 2.0 * kPi / static_cast<Real>(len) * (inverse ? 1.0 : -1.0);
        const Complex wlen(std::cos(ang), std::sin(ang));
        for (std::size_t i = 0; i < n; i += len) {
            Complex w(1.0, 0.0);
            for (std::size_t k = 0; k < len / 2; ++k) {
                const Complex u = a[i + k];
                const Complex v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

// DFT arbitrario. Usa radix-2 cuando es posible, de lo contrario usa el algoritmo Bluestein's
// chirp-z (que reduce cualquier longitud a convoluciones de orden dos).

inline void fft_any(std::vector<Complex>& a, bool inverse) {
    const std::size_t n = a.size();
    if (n <= 1) return;                                   // trivial cases

    if ((n & (n - 1)) == 0) {                             // power of two
        fft_pow2(a, inverse);
        return;
    }

    // Chirp w[k] = exp(sign * i*pi*k^2/n). k^2 es reducido mod 2n para mantener el 
    //argumento pequeño y preservar la precisión para n grande.
    const Real sign = inverse ? 1.0 : -1.0;
    std::vector<Complex> w(n);
    for (std::size_t k = 0; k < n; ++k) {
        const std::size_t k2 = (k * k) % (2 * n);
        const Real ang = sign * kPi * static_cast<Real>(k2) / static_cast<Real>(n);
        w[k] = Complex(std::cos(ang), std::sin(ang));
    }

    // Tamanno de convolucion: menores a oreden 2.
    std::size_t m = 1;
    while (m < 2 * n - 1) m <<= 1;

    std::vector<Complex> A(m, Complex(0, 0)), B(m, Complex(0, 0));
    for (std::size_t k = 0; k < n; ++k) A[k] = a[k] * w[k];
    B[0] = std::conj(w[0]);
    for (std::size_t k = 1; k < n; ++k) B[k] = B[m - k] = std::conj(w[k]);

    fft_pow2(A, false);
    fft_pow2(B, false);
    for (std::size_t i = 0; i < m; ++i) A[i] *= B[i];
    fft_pow2(A, true);

    for (std::size_t k = 0; k < n; ++k)
        a[k] = A[k] * w[k] / static_cast<Real>(m);
}
}
}

#endif  // DECO_FFT_HPP
