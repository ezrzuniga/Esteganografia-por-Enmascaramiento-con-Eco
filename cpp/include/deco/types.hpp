//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//types.hpp

//Aiases de tipos y constantes numericas compartidas por todos los headers de deco.

#ifndef DECO_TYPES_HPP
#define DECO_TYPES_HPP

#include <complex>   // std::complex
#include <cstddef>   // std::size_t
#include <vector>    // std::vector

namespace deco {


//Se puede cambiar el valor de Real = <> para cambiar la libreria al timpo de datos float / long double.
using Real    = double;                       // tipo de muestra que representa la señal de audio
using Complex = std::complex<Real>;           // bin de frecuencia (complejo)
using Signal  = std::vector<Real>;            // señal de audio 1-D
using Indices = std::vector<std::size_t>;     // conjunto de retrasos candidatos ("d" en Python)

//CONSTANTES NUMERICAS
constexpr Real kPi = 3.14159265358979323846264338327950288;
constexpr Real kEps = 1e-12;
constexpr Real kScale = 32768.0;
}
#endif
