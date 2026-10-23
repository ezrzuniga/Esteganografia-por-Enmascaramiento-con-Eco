#pragma once
#include <map>
#include <string>

namespace cli {

struct Options {
    std::string input;                          // R29, R40
    std::string output;                         // R30; empty in decoder mode
    std::map<std::string, std::string> meta;    // R10-R16; empty -> decoder (R28, R39)
    double delay0_us = 1000, delay1_us = 1500;  // R17, R19-R21
    double att0_pct = 40, att1_pct = 40;        // R22-R24
    int channel = 0;                            // 0 = left, 1 = right (R26, R27)
    int d0 = 0, d1 = 0;                         // delays in samples

    bool encoding() const { return !meta.empty(); }
};

// Parses argv; on error prints a message to stderr and exits (R7).
Options parse_args(int argc, char* argv[]);

}  // namespace cli