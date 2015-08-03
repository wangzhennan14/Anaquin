#ifndef GI_F_DISCOVER_HPP
#define GI_F_DISCOVER_HPP

#include "stats/analyzer.hpp"

namespace Anaquin
{
    struct FDiscover
    {
        struct Options : public SingleMixtureOptions
        {
            // GCC requires it...
            Options() {}

            Software soft = Software::TopHat;
        };

        struct Stats : ModelStats
        {
            // Overall performance
            Confusion m;

            // Fraction of reference fusion detected
            double covered;

            // Distribution of the sequins
            SequinHist h = Analyzer::seqHist();

            // Sequins failed to detect in the experiment
            MissingSequins miss;
        };

        static Stats analyze(const std::string &, const Options &options = Options());
    };
}

#endif