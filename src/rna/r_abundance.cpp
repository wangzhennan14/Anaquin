#include <iostream>
#include <ss/c.hpp>
#include "expression.hpp"
#include "r_abundance.hpp"
#include "writers/r_writer.hpp"
#include "parsers/parser_tmap.hpp"
#include "parsers/parser_tracking.hpp"
#include <ss/regression/linear_model.hpp>

using namespace SS;
using namespace Spike;

static const std::string GTracking = "genes.fpkm_tracking";
static const std::string ITracking = "isoforms.fpkm_tracking";

static bool suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

RAbundanceStats RAbundance::analyze(const std::string &file, const Options &options)
{
    RAbundanceStats stats;
    const auto &s = Standard::instance();

    auto c = RAnalyzer::isoformCounter();

    if (suffix(file, ".tmap"))
    {
        ParserTMap::parse(file, [&](const TMap &t, const ParserProgress &)
        {
            if (s.r_seqs_iA.count(t.refID))
            {
                c[t.refID]++;
                
                if (t.fpkm)
                {
                    const auto &i = s.r_seqs_iA.at(t.refID);

                    stats.x.push_back(log(i.abund(true)));
                    stats.y.push_back(log(t.fpkm));
                    stats.z.push_back(t.refID);
                }
            }
        });

        stats.s = Expression::analyze(c, s.r_sequin(options.mix));
    }
    else
    {
        if (file.find(GTracking) != std::string::npos && file.find(ITracking) != std::string::npos)
        {
            throw std::runtime_error((boost::format("Unknown file. It must be %1% or %2%")
                                        % GTracking % ITracking).str());
        }

        ParserTracking::parse(file, [&](const Tracking &t, const ParserProgress &)
        {
            if (file.find(ITracking) != std::string::npos)
            {
                c[t.trackID]++;
                assert(s.r_seqs_iA.count(t.trackID));
                
                if (t.fpkm)
                {
                    const auto &i = s.r_seqs_iA.at(t.trackID);
                    
                    stats.x.push_back(log(i.abund(true)));
                    stats.y.push_back(log(t.fpkm));
                    stats.z.push_back(t.trackID);
                }

                stats.s = Expression::analyze(c, s.r_sequin(options.mix));
            }
            else
            {
                c[t.geneID]++;
                assert(s.r_seqs_gA.count(t.geneID));
                const auto &m = s.r_seqs_gA.at(t.geneID);

                if (t.fpkm)
                {
                    /*
                     * The x-axis would be the known concentration for each gene,
                     * the y-axis would be the expression (RPKM) reported.
                     */
                    
                    stats.x.push_back(log(m.abund(true)));
                    stats.y.push_back(log(t.fpkm));
                    stats.z.push_back(t.geneID);
                }

                stats.s = Expression::analyze(c, s.r_pair(options.mix));
            }
        });
    }

    assert(!stats.x.empty() && stats.x.size() == stats.y.size() && stats.y.size() == stats.z.size());

    stats.linear();
    AnalyzeReporter::report("abundance.stats", "abundance.R", stats, c, options.writer);
    
    return stats;
}