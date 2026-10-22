//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//decode.hpp

//Decodificador : Esta funcion se encarga de dividir la señal de entrada "y" en tramas consecutivas de longitud "L"
// (la ultima trama parcial se descarta), luego aplica rcc + classify a cada trama, y devuelve los indices de simbolos 
// y sus scores por trama.

#ifndef DECO_DECODE_HPP
#define DECO_DECODE_HPP

#include <stdexcept>   // std::invalid_argument
#include <vector>

#include "types.hpp"
#include "rcc.hpp"
#include "classify.hpp"

namespace deco {

// salida por cuadro [reemplaza en c++ las dos columnas de salida de numpy observadas en el prototipo de python]
struct DecodeResult {
    std::vector<int>  symbols;   // best candidate index per frame
    std::vector<Real> scores;    // its robust z-score per frame
};

inline DecodeResult decode(const Signal& y, const Indices& d, std::size_t L) {
    DecodeResult res;
    if (L == 0) throw std::invalid_argument("decode: L must be > 0");

    for (std::size_t s = 0; s + L <= y.size(); s += L) {
        const Signal frame(y.begin() + s, y.begin() + s + L);   // y[s:s+L]
        const auto c = classify(rcc(frame), d);                 // per-frame decision
        res.symbols.push_back(c.first);
        res.scores.push_back(c.second);
    }
    return res;
}

}
#endif