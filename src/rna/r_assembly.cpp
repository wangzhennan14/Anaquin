#include "r_assembly.hpp"
#include "expression.hpp"
#include <boost/format.hpp>
#include "parsers/parser_gtf.hpp"
#include <iostream>
using namespace Spike;

template <typename F> static void extractIntrons(const std::map<SequinID, std::vector<Feature>> &x, F f)
{
    Feature ir;

    for (const auto & ts : x)
    {
        for (auto i = 0; i < ts.second.size(); i++)
        {
            if (i)
            {
                if (ts.second[i-1].iID == ts.second[i].iID)
                {
                    ir = ts.second[i];
                    ir.l = Locus(ts.second[i - 1].l.end + 1, ts.second[i].l.start - 1);
                    f(ts.second[i-1], ts.second[i], ir);
                }
            }
        }
    }
}

RAssemblyStats RAssembly::analyze(const std::string &file, const Options &options)
{
    RAssemblyStats stats;
    const auto &s = Standard::instance();

    // The structure depends on the mixture
    const auto seqs = s.r_sequin(options.mix);

    std::map<SequinID, std::vector<Feature>> q_exons_;
    std::vector<Feature> q_exons;

    ParserGTF::parse(file, [&](const Feature &f)
    {
        if (options.filters.count(f.iID))
        {
            return;
        }

        switch (f.type)
        {
            case Exon:
            {
                q_exons.push_back(f);
                q_exons_[f.iID].push_back(f);

                /*
                 * Classify at the exon level
                 */

                if (classify(stats.me, f, [&](const Feature &)
                {
                    return find(s.r_exons, f, ExactRule);
                }))
                {
                    stats.e_lc.at(f.l)++;
                    stats.ce.at(f.iID)++;
                }

                break;
            }

            case Transcript:
            {
                /*
                 * Classify at the transctipt level
                 */

                if (classify(stats.mt, f, [&](const Feature &)
                {
                    return find_map(seqs, f, ExactRule);
                }))
                {
                    stats.t_lc.at(f.iID)++;
                    stats.ct.at(f.iID)++;
                }

                break;
            }

            default:
            {
                break;
            }
        }
    });

    /*
     * Sort the exons in the query since there is no guarantee that those are sorted
     */
    
    for (auto &i : q_exons_)
    {
        CHECK_AND_SORT(i.second);
    }

    extractIntrons(q_exons_, [&](const Feature &, const Feature &, Feature &i)
                   {
                       /*
                        * Classify at the intron level
                        */
                       
                       if (classify(stats.mi, i, [&](const Feature &)
                       {
                           return find(s.r_introns, i, ExactRule);
                       }))
                       {
                           stats.i_lc.at(i.l)++;
                           stats.ci[i.iID]++;
                       }
                   });
    
    /*
     * Classify at the base level
     */

    countBase(s.r_l_exons, q_exons, stats.mb);

    /*
     * Setting the known references
     */
    
    count_ref(stats.e_lc, stats.me.nr);
    count_ref(stats.i_lc, stats.mi.nr);

    // The number of sequins is also the number of known transcripts
    stats.mt.nr = seqs.size();
    
    // The known base length is the total length of all known exons
    stats.mb.nr = s.r_c_exons;

    /*
     * Calculate for the LOS
     */

    stats.se = Expression::analyze(stats.ce, seqs);
    stats.st = Expression::analyze(stats.ct, seqs);
    stats.sb = Expression::analyze(stats.cb, seqs);
    stats.si = Expression::analyze(stats.ci, seqs);

    /*
     * Report for the statistics
     */

    AnalyzeReporter::report("assembly.base.stats", stats.mb, stats.sb, stats.cb, options.writer);
    AnalyzeReporter::report("assembly.exons.stats", stats.me, stats.se, stats.ce, options.writer);
    AnalyzeReporter::report("assembly.intron.stats", stats.mi, stats.si, stats.ci, options.writer);
    AnalyzeReporter::report("assembly.transcripts.stats", stats.mt, stats.st, stats.ct, options.writer);

    return stats;
}