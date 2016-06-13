#ifndef R_CUFFDIFF_HPP
#define R_CUFFDIFF_HPP

#include "stats/analyzer.hpp"

namespace Anaquin
{
    struct RCuffdiff
    {
        typedef AnalyzerOptions Options;

        static void analyze(const FileName &, const FileName &, const Options &o = Options());
        static void report(const FileName &, const Options &o = Options());
    };
}

#endif