//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//wav.hpp

//Lectura/escritura minima de WAV PCM de 16 bits (cualquier numero de canales). Las muestras se normalizan a [-1, 1).

#ifndef ENCODER_WAV_HPP
#define ENCODER_WAV_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "deco/types.hpp"

namespace wav {

struct Audio {
    std::uint32_t fs = 44100;
    std::vector<deco::Signal> ch;   // ch[c][n]
};

namespace detail {
inline std::uint32_t rd32(const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (std::uint32_t(p[3]) << 24); }
inline std::uint16_t rd16(const unsigned char* p) { return std::uint16_t(p[0] | (p[1] << 8)); }
inline void wr32(std::string& s, std::uint32_t v) { for (int i = 0; i < 4; ++i) s += char((v >> (8 * i)) & 0xFF); }
inline void wr16(std::string& s, std::uint16_t v) { for (int i = 0; i < 2; ++i) s += char((v >> (8 * i)) & 0xFF); }
}  // namespace detail

inline Audio read(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("no se pudo abrir: " + path);
    const std::vector<unsigned char> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (b.size() < 12 || std::memcmp(b.data(), "RIFF", 4) || std::memcmp(b.data() + 8, "WAVE", 4))
        throw std::runtime_error(path + ": no es un archivo WAV");

    Audio a;
    unsigned channels = 0, bits = 0, format = 0;
    for (std::size_t pos = 12; pos + 8 <= b.size();) {
        const std::uint32_t size = detail::rd32(&b[pos + 4]);
        const unsigned char* body = &b[pos + 8];
        const std::size_t avail = std::min<std::size_t>(size, b.size() - (pos + 8));
        if (!std::memcmp(&b[pos], "fmt ", 4) && avail >= 16) {
            format = detail::rd16(body);
            channels = detail::rd16(body + 2);
            a.fs = detail::rd32(body + 4);
            bits = detail::rd16(body + 14);
        } else if (!std::memcmp(&b[pos], "data", 4)) {
            if (channels == 0) throw std::runtime_error(path + ": falta el chunk fmt");
            if (format == 0xFFFE && avail >= 16) format = 1;   // WAVE_FORMAT_EXTENSIBLE: se asume PCM
            if (format != 1 || bits != 16) throw std::runtime_error(path + ": solo se soporta PCM de 16 bits");
            const std::size_t frames = avail / (2 * channels);
            a.ch.assign(channels, deco::Signal(frames));
            for (std::size_t n = 0; n < frames; ++n)
                for (unsigned c = 0; c < channels; ++c)
                    a.ch[c][n] = std::int16_t(detail::rd16(body + 2 * (n * channels + c))) / deco::kScale;
            return a;
        }
        pos += 8 + size + (size & 1);   // los chunks se alinean a 2 bytes
    }
    throw std::runtime_error(path + ": no contiene chunk data");
}

inline void write(const std::string& path, const Audio& a) {
    if (a.ch.empty()) throw std::invalid_argument("wav::write: sin canales");
    const std::uint16_t channels = static_cast<std::uint16_t>(a.ch.size());
    const std::size_t frames = a.ch[0].size();
    const std::uint32_t data_bytes = static_cast<std::uint32_t>(frames * channels * 2);

    std::string s = "RIFF";
    detail::wr32(s, 36 + data_bytes);
    s += "WAVEfmt ";
    detail::wr32(s, 16);
    detail::wr16(s, 1);
    detail::wr16(s, channels);
    detail::wr32(s, a.fs);
    detail::wr32(s, a.fs * channels * 2);
    detail::wr16(s, channels * 2);
    detail::wr16(s, 16);
    s += "data";
    detail::wr32(s, data_bytes);
    for (std::size_t n = 0; n < frames; ++n)
        for (std::uint16_t c = 0; c < channels; ++c) {
            const double v = std::round(a.ch[c][n] * deco::kScale);
            detail::wr16(s, static_cast<std::uint16_t>(static_cast<std::int16_t>(std::clamp(v, -32768.0, 32767.0))));
        }

    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("no se pudo escribir: " + path);
    f.write(s.data(), static_cast<std::streamsize>(s.size()));
}

}  // namespace wav
#endif
