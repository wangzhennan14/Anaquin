#include "FusQuin/FUSQuin.hpp"
#include "FusQuin/f_discover.hpp"

using namespace Anaquin;

extern Scripts PlotFROC();

FDiscover::Stats FDiscover::analyze(const FileName &file, const FDiscover::Options &o)
{
    const auto &r = Standard::instance().r_fus;
    
    FDiscover::Stats stats;
    
    stats.data[Geno];
    stats.data[ChrT].hist = r.fusionHist();

    FUSQuin::analyze<FDiscover::Options>(file, o, [&](const FUSQuin::Match &match)
    {
        switch (match.label)
        {
            case FUSQuin::Label::Positive:
            {
                stats.data[ChrT].tps.push_back(match);
                stats.data[ChrT].hist[match.known->id]++;
                break;
            }
                
            case FUSQuin::Label::GenoChrT:
            case FUSQuin::Label::Negative:
            {
                stats.data[ChrT].fps.push_back(match);
                break;
            }
                
            case FUSQuin::Label::Geno:
            {
                stats.data[Geno].fps.push_back(match);
                break;
            }
        }
    });
    
    /*
     * Find out all the missing fusions (only chrT for now)
     */

    for (auto &i : stats.data)
    {
        if (i.first == ChrT)
        {
            for (const auto &j : i.second.hist)
            {
                if (!j.second)
                {
                    i.second.fns.push_back(*r.findFusion(j.first));
                }
            }
        }
    }

    return stats;
}

static void writeSummary(const FileName &file, const FDiscover::Stats &stats, const FDiscover::Options &o)
{
    const auto &r = Standard::instance().r_fus;

    const auto summary = "Summary for input: %1%\n\n"
                         "   ***\n"
                         "   *** Number of fusions detected in the synthetic and genome\n"
                         "   ***\n\n"
                         "   Synthetic: %2% fusions\n"
                         "   Genome:    %3% fusions\n\n"
                         "   ***\n"
                         "   *** Reference annotation (Synthetic)\n"
                         "   ***\n\n"
                         "   File: %4%\n\n"
                         "   Synthetic: %5% fusions\n\n"
                         "   ************************************************************\n"
                         "   ***                                                      ***\n"
                         "   ***        Statistics for the synthetic chromosome       ***\n"
                         "   ***                                                      ***\n"
                         "   ************************************************************\n\n"
                         "   True Positives:  %6% fusions\n"
                         "   False Positives: %7% fusions\n\n"
                         "   ***\n"
                         "   *** Performance metrics\n"
                         "   ***\n\n"
                         "   FPR:       : %8%\n"
                         "   Sensitivity: %9%\n"
                         "   Precision:   %10%\n\n";
    
    o.writer->open(file);
    o.writer->write((boost::format(summary) % file
                                            % stats.countDetect(ChrT)
                                            % stats.countDetect(Geno)
                                            % o.rChrT
                                            % r.countFusion()
                                            % stats.countTP(ChrT)
                                            % stats.countFP(ChrT)
                                            % "?"
                                            % stats.sn(ChrT)
                                            % stats.pc(ChrT)).str());
    o.writer->close();
}

static void writeQuery(const FileName &file, const ChrID &cID, const FDiscover::Stats &stats, const FDiscover::Options &o)
{
    const auto &data  = stats.data.at(cID);
    const auto format = "%1%\t%2%\t%3%\t%4%";

    o.writer->open(file);
    o.writer->write((boost::format(format) % "seq"
                                           % "label"
                                           % "pos1"
                                           % "pos2").str());

    for (const auto &tp : data.tps)
    {
        o.writer->write((boost::format(format) % tp.known->id
                                               % "TP"
                                               % tp.query.l1
                                               % tp.query.l2).str());
    }

    for (const auto &fp : data.fps)
    {
        o.writer->write((boost::format(format) % "-"
                                               % "FP"
                                               % fp.query.l1
                                               % fp.query.l2).str());
    }
}

static void writeQuins(const FileName &file, const FDiscover::Stats &stats, const FDiscover::Options &o)
{
    o.writer->open(file);
    
    const auto format = "%1%\t%2%";
    
    o.writer->write((boost::format(format) % "seq"
                                           % "counts").str());

    for (const auto &i : stats.data.at(ChrT).hist)
    {
        o.writer->write((boost::format(format) % i.first
                                               % i.second).str());
    }
    
    o.writer->close();
}

void FDiscover::report(const FileName &file, const FDiscover::Options &o)
{
    const auto stats = analyze(file, o);

    o.info("Generating statistics");
    
    /*
     * Generating summary statistics
     */

    writeSummary("FusDiscover_summary.stats", stats, o);

    /*
     * Generating sequin statistics
     */
    
    writeQuins("FusDiscover_quins.stats", stats, o);
    
    /*
     * Generating statistics for the query
     */
    
    writeQuery("FusDiscover_query.stats", ChrT, stats, o);
    
    /*
     * Generating ROC plot
     */
    
    o.writer->open("FusDiscover_ROC.R");
    //o.writer->write(RWriter::createScript("FusDiscover_query.stats", PlotFROC()));
    o.writer->close();
}