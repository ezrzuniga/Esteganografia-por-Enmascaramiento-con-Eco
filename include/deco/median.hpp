//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//median.hpp

//Mediana con el promedio de dos valores medios para tamaños pares. Toma el vector POR VALOR porque lo reordena.

#ifndef DECO_MEDIAN_HPP
#define DECO_MEDIAN_HPP

#include <algorithm>   // nth_element, max_element
#include <stdexcept>   // std::invalid_argument
#include <vector>

#include "types.hpp"

namespace deco {
namespace detail {
inline Real median(std::vector<Real> v) {
    if (v.empty()) throw std::invalid_argument("median of empty range");
    const std::size_t n = v.size();
    const std::size_t mid = n / 2;

    // da valor al elemento del medio superior en v[mid].
    std::nth_element(v.begin(), v.begin() + mid, v.end());
    const Real hi = v[mid];
    if (n % 2 == 1) return hi;                            // Par: el valor del medio
    // Impar: la mitad baja es el máximo de todo lo que está a la izquierda de `mid`.
    const Real lo = *std::max_element(v.begin(), v.begin() + mid);
    return 0.5 * (lo + hi);
}
}
}

#endif  // DECO_MEDIAN_HPP
