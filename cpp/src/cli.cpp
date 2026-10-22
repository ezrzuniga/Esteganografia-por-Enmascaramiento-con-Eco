#include "cli/cli.hpp"

#include <getopt.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

namespace cli {
namespace {

constexpr double FS = 44100.0;

struct MetaKey { const char* name; const char* help; };
constexpr MetaKey KEYS[] = {
    {"title",    "título de la canción"},
    {"artist",   "nombre del artista intérprete"},
    {"author",   "autor original"},
    {"album",    "nombre del álbum"},
    {"year",     "año de publicación"},
    {"guest",    "artista invitado"},
    {"producer", "productor"},
};
constexpr int NKEYS = sizeof(KEYS) / sizeof(KEYS[0]);

// getopt_long returns `val` for each long option; values > 255 can't clash with short options
enum { OPT_DELAY0 = 256, OPT_DELAY1, OPT_ATT0, OPT_ATT1, OPT_CHANNEL, OPT_META };  // OPT_META + i

void usage_line(std::ostream& os, const char* prog) {
    os << "uso: " << prog << " [-h] [metadatos] [enmascaramiento] [--channel {left,right}]\n"
       << "       ENTRADA.wav [SALIDA.wav]\n";
}

// One help row, aligned like argparse.
void row(const std::string& left, const std::string& help) {
    constexpr int W = 24;
    std::cout << "  " << std::left << std::setw(W) << left;
    if (static_cast<int>(left.size()) >= W) std::cout << "\n  " << std::string(W, ' ');
    std::cout << help << '\n';
}

void print_help(const char* prog) {
    usage_line(std::cout, prog);
    std::cout << "\nEsteganografía por enmascaramiento con eco. Si se especifica al menos un\n"
                 "metadato el programa codifica; si no, decodifica.\n"
                 "\nargumentos posicionales:\n";
    row("ENTRADA.wav", "WAV original (codificar) o codificado (decodificar)");
    row("SALIDA.wav", "WAV codificado de destino (solo al codificar)");

    std::cout << "\nopciones:\n";
    row("-h, --help", "muestra esta ayuda y termina");
    row("--channel {left,right}", "canal a codificar/decodificar (default: left)");

    std::cout << "\nmetadatos (opcionales; si se repiten, vale el último):\n";
    for (const auto& k : KEYS) row(std::string("--") + k.name + " TEXTO", k.help);

    std::cout << "\nenmascaramiento (deben coincidir al codificar y decodificar):\n";
    row("--delay0 US", "retardo del eco para un bit 0 [µs] (default: 1000)");
    row("--delay1 US", "retardo del eco para un bit 1 [µs] (default: 1500)");
    row("--att0 PCT", "amplitud del eco para un bit 0 [% de la original] (default: 40)");
    row("--att1 PCT", "amplitud del eco para un bit 1 [% de la original] (default: 40)");

    std::cout << "\nejemplos:\n"
              << "  " << prog << " original.wav codificado.wav --title 'Mi canción' --year 2026\n"
              << "  " << prog << " codificado.wav --channel right\n";
}

// Equivalent of argparse's p.error(): usage + message on stderr, exit code 2.
[[noreturn]] void fail(const char* prog, const std::string& msg) {
    usage_line(std::cerr, prog);
    std::cerr << prog << ": error: " << msg << '\n';
    std::exit(2);
}

// Equivalent of type=float: the whole string must be a finite number.
double to_number(const char* prog, const char* opt, const char* s) {
    char* end = nullptr;
    errno = 0;
    const double v = std::strtod(s, &end);
    if (end == s || *end != '\0' || errno == ERANGE || !std::isfinite(v))
        fail(prog, std::string("argumento --") + opt + ": valor numérico inválido: '" + s + "'");
    return v;
}

int us_to_samples(double us) { return static_cast<int>(std::lround(us * FS / 1e6)); }

}  // namespace

Options parse_args(int argc, char* argv[]) {
    const char* prog = argv[0];
    Options o;

    std::vector<option> longopts = {
        {"help",    no_argument,       nullptr, 'h'},
        {"delay0",  required_argument, nullptr, OPT_DELAY0},
        {"delay1",  required_argument, nullptr, OPT_DELAY1},
        {"att0",    required_argument, nullptr, OPT_ATT0},
        {"att1",    required_argument, nullptr, OPT_ATT1},
        {"channel", required_argument, nullptr, OPT_CHANNEL},
    };
    for (int i = 0; i < NKEYS; ++i)                                     // R10-R16
        longopts.push_back({KEYS[i].name, required_argument, nullptr, OPT_META + i});
    longopts.push_back({nullptr, 0, nullptr, 0});                       // required terminator

    int c;
    while ((c = getopt_long(argc, argv, "h", longopts.data(), nullptr)) != -1) {
        switch (c) {
        case 'h':         print_help(prog); std::exit(0);
        case OPT_DELAY0:  o.delay0_us = to_number(prog, "delay0", optarg); break;
        case OPT_DELAY1:  o.delay1_us = to_number(prog, "delay1", optarg); break;
        case OPT_ATT0:    o.att0_pct  = to_number(prog, "att0", optarg); break;
        case OPT_ATT1:    o.att1_pct  = to_number(prog, "att1", optarg); break;
        case OPT_CHANNEL: {                                             // choices=[left, right]
            const std::string ch = optarg;
            if (ch == "left") o.channel = 0;
            else if (ch == "right") o.channel = 1;
            else fail(prog, "argumento --channel: opción inválida: '" + ch + "' (elija entre 'left', 'right')");
            break;
        }
        case '?':  // unknown option or missing value; getopt already printed why (R7)
            usage_line(std::cerr, prog);
            std::exit(2);
        default:   // metadata; a repeated key overwrites the previous value (R8, R9)
            o.meta[KEYS[c - OPT_META].name] = optarg;
        }
    }

    // positional arguments: getopt_long moves them to argv[optind..argc-1]
    const int npos = argc - optind;
    if (npos < 1) fail(prog, "se requiere el argumento: ENTRADA.wav");
    if (npos > 2) fail(prog, "argumentos no reconocidos: " + std::string(argv[optind + 2]));
    o.input = argv[optind];
    if (npos == 2) o.output = argv[optind + 1];

    // semantic validation
    o.d0 = us_to_samples(o.delay0_us);
    o.d1 = us_to_samples(o.delay1_us);
    if (std::min(o.d0, o.d1) < 1)
        fail(prog, "los retardos deben ser >= " + std::to_string(1e6 / FS) + " µs (1 muestra)");
    if (o.d0 == o.d1) fail(prog, "--delay0 y --delay1 deben diferir en al menos una muestra");
    if (!(o.att0_pct > 0 && o.att0_pct <= 100)) fail(prog, "--att0 debe estar en (0, 100]");
    if (!(o.att1_pct > 0 && o.att1_pct <= 100)) fail(prog, "--att1 debe estar en (0, 100]");

    for (const auto& [key, val] : o.meta)
        if (val.find('\n') != std::string::npos) fail(prog, "los metadatos no pueden contener saltos de línea");
    if (o.encoding() && o.output.empty()) fail(prog, "al codificar se requiere SALIDA.wav");
    if (!o.encoding() && !o.output.empty())
        fail(prog, "se dio SALIDA.wav pero ningún metadato: indique al menos uno para codificar");
    return o;
}

}  // namespace cli