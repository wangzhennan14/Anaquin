#include "fusion/f_discover.hpp"

using namespace Anaquin;

FDiscover::Stats FDiscover::analyze(const std::string &file, const FDiscover::Options &options)
{
    const auto stats  = FAnalyzer::analyze(file, options);
    const auto format = "%1%\t%2%\t%3%\t%4%\t%5%\t%6%";

    options.info("Generating statistics");

    options.writer->open("FusionDiscover_summary.stats");
    options.writer->write((boost::format(format) % "sn"
                                                 % "sp"
                                                 % "coverage"
                                                 % "los"
                                                 % "partner_5"
                                                 % "partner_3").str());
    options.writer->write((boost::format(format) % stats.m.sn()
                                                 % stats.m.sp()
                                                 % stats.m.sp()
                                                 % stats.s.abund
                                                 % 0
                                                 % 0).str());
    options.writer->close();

    /*
     * The statistics we need are the subset that we've got, excluding the linear modeling
     */
    
    FDiscover::Stats d_stats;
    
    d_stats.m = stats.m;
    d_stats.h = stats.h;
    
    return d_stats;
}