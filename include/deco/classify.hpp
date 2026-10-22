//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//Classify.hpp 

// Esta funcion se encarga de elegir cuál de los retardos candidatos de "d" destaca más en "r". Para esto se toma como referencia
// de ruido la ventana [max(d)/2 + 1 : 3*max(d)] (recortada a la longitud de r, igual que el slicing de Python). Luego se calcula 
// el z-score robusto (r[d_i] - mediana) / (MAD + eps) para cada candidato d_i, y se devuelve el índice del candidato con el 
// z-score más alto, junto con su z-score.
// En caso que d se encuentre fuera de rango, o la ventana de referencia esté vacía, lanza std::invalid_argument.

#ifndef DECO_CLASSIFY_HPP
#define DECO_CLASSIFY_HPP

#include <algorithm>   // max_element, min
#include <cmath>       // abs
#include <stdexcept>   // std::invalid_argument
#include <utility>     // std::pair

#include "types.hpp"
#include "median.hpp"

namespace deco {
inline std::pair<int, Real> classify(const Signal& r, const Indices& d) {
    if (d.empty()) throw std::invalid_argument("classify: d is empty");

    const std::size_t dmax = *std::max_element(d.begin(), d.end());

    //[lo,hi] ventana de referencia
    const std::size_t lo = dmax / 2 + 1;
    const std::size_t hi = std::min(3 * dmax, r.size());
    if (lo >= hi) throw std::invalid_argument("classify: empty reference window");

    const Signal ref(r.begin() + lo, r.begin() + hi);

    // Median and median-absolute-deviation of the reference.
    const Real med = detail::median(ref);
    Signal dev(ref.size());
    for (std::size_t i = 0; i < ref.size(); ++i) dev[i] = std::abs(ref[i] - med);
    const Real mad = detail::median(dev);

    // Robust z-score of every candidate lag.
    Signal z(d.size());
    for (std::size_t i = 0; i < d.size(); ++i) {
        if (d[i] >= r.size()) throw std::invalid_argument("classify: lag out of range");
        z[i] = (r[d[i]] - med) / (mad + kEps);
    }

    // First maximum wins on ties (same as numpy.argmax).
    const auto it = std::max_element(z.begin(), z.end());
    return {static_cast<int>(it - z.begin()), *it};
}
} 
#endif