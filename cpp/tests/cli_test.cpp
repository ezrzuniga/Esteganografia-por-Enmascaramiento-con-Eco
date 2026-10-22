#include <iostream>

#include "cli/cli.hpp"

int main(int argc, char* argv[]) {
    const cli::Options o = cli::parse_args(argc, argv);

    std::cout << "modo     : " << (o.encoding() ? "codificar" : "decodificar") << '\n'
              << "entrada  : " << o.input << '\n'
              << "salida   : " << (o.output.empty() ? "-" : o.output) << '\n'
              << "canal    : " << (o.channel ? "right" : "left") << '\n'
              << "delay0/1 : " << o.delay0_us << " / " << o.delay1_us << " us  -> "
              << o.d0 << " / " << o.d1 << " muestras\n"
              << "att0/1   : " << o.att0_pct << " / " << o.att1_pct << " %\n";
    for (const auto& [k, v] : o.meta) std::cout << "metadato : " << k << " = " << v << '\n';
    return 0;
}