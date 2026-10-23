//Maestria en Electronica - TEC
//David Duarte
//Jose Zuniga
//Nasser Brown
//Proyecto 1
//main.cpp: aplicacion que une cli + wav + encoder + deco, con el mismo protocolo que el prototipo de python.
//
//Con metadatos: lee ENTRADA.wav, esconde el texto "clave=valor\n" (un bit por ventana, MSB primero) en el canal
//               elegido mediante ecos, deja sin eco las ventanas restantes y escribe SALIDA.wav.
//Sin metadatos: lee ENTRADA.wav, decodifica el canal elegido y toma como fin del mensaje la primera racha de GAP
//               ventanas sin eco (R47). Imprime en stdout los metadatos reconocidos con su categoria, y en stderr
//               los desconocidos con una advertencia.
//Los mensajes de diagnostico van a stderr para que stdout contenga solo los metadatos (R48).

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cli/cli.hpp"
#include "deco/decode.hpp"
#include "encoder/encoder.hpp"
#include "encoder/wav.hpp"

namespace {

// Longitud de ventana = WIN_MULT * max(d) (R33, R42): el menor multiplo que cumple PESQ >= 4.0 (R32) y
// recuperacion >= 95 % (R46) en las pistas de prueba, con fundido de una ventana completa entre nucleos.
constexpr std::size_t WIN_MULT = 10;
constexpr std::uint32_t FS = 44100;     // formato exigido: 44.1 kHz, 16 bits, estereo (R25)
constexpr std::size_t CHANNELS = 2;
constexpr deco::Real THR = 3.0;         // z minimo para considerar que una ventana tiene eco (R47)
constexpr std::size_t GAP = 13;         // ventanas sin eco seguidas que marcan el fin del mensaje (~195 ms)

// Categoria que acompana a cada metadato reconocido (R49).
const std::map<std::string, std::string> LABELS = {
    {"title", "Título"}, {"artist", "Artista"}, {"author", "Autor"}, {"album", "Álbum"},
    {"year", "Año"}, {"guest", "Artista invitado"}, {"producer", "Productor"},
};

void push_bits(std::vector<int>& b, unsigned v, int n) {   // MSB primero
    for (int i = n - 1; i >= 0; --i) b.push_back((v >> i) & 1);
}

// Fraccion de las ventanas [from, to) cuyo pico de R_cc supera el umbral, es decir, que tienen eco.
double echo_fraction(const std::vector<deco::Real>& z, std::size_t from, std::size_t to) {
    if (from >= to) return 0.0;
    std::size_t n = 0;
    for (std::size_t k = from; k < to; ++k) n += z[k] >= THR;
    return static_cast<double>(n) / static_cast<double>(to - from);
}

// Inicio de la primera racha de GAP ventanas sin eco; si no hay ninguna, el mensaje llega hasta el final.
std::size_t message_end(const std::vector<deco::Real>& z) {
    std::size_t run = 0;
    for (std::size_t k = 0; k < z.size(); ++k) {
        run = z[k] < THR ? run + 1 : 0;
        if (run == GAP) return k + 1 - GAP;
    }
    return z.size();
}

void check_format(const wav::Audio& a, const std::string& path) {   // R25 (los 16 bits los valida wav::read)
    if (a.fs != FS || a.ch.size() != CHANNELS)
        throw std::runtime_error(path + ": se requiere WAV de 44.1 kHz, 16 bits y estéreo (tiene " +
                                 std::to_string(a.fs) + " Hz y " + std::to_string(a.ch.size()) + " canal(es))");
}

// Separa el mensaje en lineas "clave=valor": las claves conocidas van a stdout con su categoria (R48, R49) y el
// resto a stderr con una advertencia (R50, R51).
void print_metadata(const std::string& msg) {
    std::istringstream in(msg);
    for (std::string line; std::getline(in, line);) {
        if (line.empty()) continue;
        const std::size_t eq = line.find('=');
        const auto it = eq == std::string::npos ? LABELS.end() : LABELS.find(line.substr(0, eq));
        if (it != LABELS.end())
            std::cout << it->second << ": " << line.substr(eq + 1) << '\n';
        else
            std::cerr << "ADVERTENCIA: metadato desconocido '" << line << "'\n";
    }
}

int encode(const cli::Options& opt, wav::Audio& audio, const encoder::Params& p, std::size_t L) {
    deco::Signal& x = audio.ch[opt.channel];
    std::string payload;
    for (const auto& [k, v] : opt.meta) payload += k + "=" + v + "\n";

    std::vector<int> bits;
    for (unsigned char c : payload) push_bits(bits, c, 8);
    const std::size_t capacity = x.size() / L;   // ventanas completas = bits disponibles
    if (bits.size() + GAP > capacity)              // deben quedar GAP ventanas sin eco para marcar el fin
        throw std::runtime_error("el audio es muy corto: se necesitan " + std::to_string(bits.size()) + " + " +
                                 std::to_string(GAP) + " ventanas y solo hay " + std::to_string(capacity));

    x = encoder::encode(x, bits, p, L, L);   // convolucion por ventanas + solapamiento y suma con fundido (R34, R36)
    wav::write(opt.output, audio);        // mismo formato y encabezado que la entrada (R37)
    std::fprintf(stderr, "salida   : %s (%zu bits escondidos en %zu de %zu ventanas)\n",
                 opt.output.c_str(), bits.size(), bits.size(), capacity);
    return 0;
}

int decode(const deco::Signal& x, const encoder::Params& p, std::size_t L) {
    const deco::DecodeResult r = deco::decode(x, {p.d[0], p.d[1]}, L);   // R_cc + clasificador por ventana
    const std::size_t nwin = r.symbols.size();
    std::size_t end = message_end(r.scores);   // ventanas sin eco a partir de aqui (R47)
    end -= end % 8;                            // solo bytes completos

    if (end == 0) {
        std::fprintf(stderr, "no se encontró un mensaje: las primeras ventanas no tienen eco\n");
        return 1;
    }
    std::fprintf(stderr, "mensaje  : %zu bytes; %.1f %% de sus %zu ventanas con eco, %.1f %% de las %zu "
                         "ventanas restantes reconocidas sin codificar\n",
                 end / 8, 100 * echo_fraction(r.scores, 0, end), end,
                 100 * (1 - echo_fraction(r.scores, end, nwin)), nwin - end);

    std::string msg;
    for (std::size_t i = 0; i < end; i += 8) {
        unsigned byte = 0;
        for (std::size_t j = 0; j < 8; ++j) byte = (byte << 1) | static_cast<unsigned>(r.symbols[i + j]);
        msg += static_cast<char>(byte);
    }
    print_metadata(msg);
    return 0;
}

int run(const cli::Options& opt) {
    wav::Audio audio = wav::read(opt.input);   // rechaza todo lo que no sea PCM de 16 bits
    check_format(audio, opt.input);

    const encoder::Params p = encoder::make_params(opt.delay0_us, opt.delay1_us, opt.att0_pct, opt.att1_pct, FS);
    const std::size_t L = encoder::window_length(p, WIN_MULT);
    std::fprintf(stderr, "modo     : %s\nentrada  : %s (%zu muestras)\ncanal    : %s\n"
                         "retardos : %zu / %zu muestras\nventana  : %zu muestras (%.1f bit/s)\n",
                 opt.encoding() ? "codificar" : "decodificar", opt.input.c_str(), audio.ch[0].size(),
                 opt.channel ? "right" : "left", p.d[0], p.d[1], L, static_cast<double>(FS) / L);

    return opt.encoding() ? encode(opt, audio, p, L) : decode(audio.ch[opt.channel], p, L);
}

}  // namespace

int main(int argc, char* argv[]) {
    const cli::Options opt = cli::parse_args(argc, argv);   // aborta ante parametros invalidos (R7)
    try {
        return run(opt);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
