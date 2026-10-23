//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//encoder.hpp

//Codificador: port a C++ de encode()/kernel() del prototipo de python. Cada ventana de L muestras se convoluciona
//con el kernel h = d(n) + a d(n - delay) del bit correspondiente y se suma con overlap-add (la cola de N-1 muestras
//se derrama a la ventana siguiente). Las ventanas sin bit (k >= bits.size()) pasan sin modificar.

#ifndef ENCODER_ENCODER_HPP
#define ENCODER_ENCODER_HPP

#include <algorithm>   // std::max
#include <array>
#include <cmath>       // std::lround
#include <cstddef>
#include <stdexcept>   // std::invalid_argument
#include <vector>

#include "deco/types.hpp"

namespace encoder {

using deco::Real;
using deco::Signal;

// Parametros del codificador: retardos (muestras) y amplitudes del eco para bit 0 / bit 1.
struct Params {
    std::array<std::size_t, 2> d;   // retardos en muestras
    std::array<Real, 2>        a;   // amplitudes (fraccion del original)
};

// Convierte retardos en microsegundos y atenuaciones en % a Params (igual que el prototipo).
inline Params make_params(double delay0_us, double delay1_us, double att0_pct, double att1_pct, double fs) {
    Params p;
    p.d = {static_cast<std::size_t>(std::lround(delay0_us * fs / 1e6)),
           static_cast<std::size_t>(std::lround(delay1_us * fs / 1e6))};
    p.a = {att0_pct / 100.0, att1_pct / 100.0};
    return p;
}

// Longitud de ventana: WIN_MULT * max(d).
inline std::size_t window_length(const Params& p, std::size_t win_mult) {
    return win_mult * std::max(p.d[0], p.d[1]);
}

// h[0] = 1, h[delay] = amp, el resto 0.
inline Signal kernel(std::size_t delay, Real amp) {
    Signal h(delay + 1, 0.0);
    h[0] = 1.0;
    h[delay] += amp;   // += por si delay == 0 (h[0] = 1 + amp, igual que la convolucion)
    return h;
}

// Convolucion completa (longitud x.size() + h.size() - 1) acumulada en out[offset...].
inline void convolve_add(const Signal& x, std::size_t begin, std::size_t len, const Signal& h,
                         Signal& out, std::size_t offset) {
    for (std::size_t i = 0; i < len; ++i)
        for (std::size_t j = 0; j < h.size(); ++j)
            if (offset + i + j < out.size()) out[offset + i + j] += x[begin + i] * h[j];
}

// Fundido de `fade` muestras (par, <= L) centrado en cada frontera: rampas sin^2/cos^2 que suman 1, asi que con el
// mismo nucleo en todas las ventanas el resultado es la convolucion completa. fade = 0: cambio abrupto de nucleo.
inline Signal encode(const Signal& x, const std::vector<int>& bits, const Params& p, std::size_t L,
                     std::size_t fade = 0) {
    if (L == 0) throw std::invalid_argument("encode: L must be > 0");
    if (fade > L || fade % 2) throw std::invalid_argument("encode: fade must be even and <= L");
    const Signal h[2] = {kernel(p.d[0], p.a[0]), kernel(p.d[1], p.a[1])};
    const std::size_t N = x.size(), half = fade / 2, nw = (N + L - 1) / L;

    Signal y(N, 0.0);   // y se trunca a len(x): la cola final se descarta
    Signal v;
    for (std::size_t k = 0; k < nw; ++k) {
        const std::size_t s = k * L, e = s + L;
        const std::size_t lo = s >= half ? s - half : 0, hi = std::min(N, e + half);
        v.assign(x.begin() + lo, x.begin() + hi);
        for (std::size_t n = lo; n < hi && fade; ++n) {
            if (k > 0 && n < s + half) {
                const Real t = std::sin(deco::kPi / 2 * (static_cast<Real>(n - (s - half)) + 0.5) / static_cast<Real>(fade));
                v[n - lo] *= t * t;
            }
            if (k + 1 < nw && n >= e - half) {
                const Real t = std::cos(deco::kPi / 2 * (static_cast<Real>(n - (e - half)) + 0.5) / static_cast<Real>(fade));
                v[n - lo] *= t * t;
            }
        }
        if (k < bits.size()) {
            if (bits[k] != 0 && bits[k] != 1) throw std::invalid_argument("encode: bits must be 0 or 1");
            convolve_add(v, 0, v.size(), h[bits[k]], y, lo);
        } else {
            for (std::size_t i = 0; i < v.size(); ++i) y[lo + i] += v[i];
        }
    }
    return y;
}

}  // namespace encoder
#endif
