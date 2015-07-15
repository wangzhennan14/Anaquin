#include "ladder/l_diffs.hpp"
#include "ladder/l_abund.hpp"
#include <ss/regression/lm.hpp>

using namespace Anaquin;

LDiffs::Stats LDiffs::analyze(const std::string &fileA, const std::string &fileB, const Options &options)
{
    LDiffs::Stats stats;

    // Copy the pointers across
    auto opt = LAbund::Options();
    
    opt.writer = options.writer;
    opt.logger = options.writer;
    opt.output = options.writer;

    options.info("Analyzing mixuture A: " + fileA);
    const auto a = LAbund::analyze(fileA, opt);

    opt.mix = MixB;
    options.info("Analyzing mixuture B: " + fileB);
    const auto b = LAbund::analyze(fileB, opt);

    const auto &s = Standard::instance();

    options.info("Merging mixtures");
    options.logInfo((boost::format("%1% sequins in mix A") % a.actual.size()).str());
    options.logInfo((boost::format("%1% sequins in mix B") % b.actual.size()).str());
    
    /*
     * Try for each detected sequin
     */

    for (const auto &i : a.actual)
    {
        const auto &id = i.first;

        // Don't bother unless the sequin is detected in both mixtures
        if (!b.actual.at(id))
        {
            options.warn((boost::format("Warning: %1% defined in mixture A but not in mixture B") % id).str());
            continue;
        }

        const auto baseID = s.l_map.at(id);

        // Calculate known fold change between mixture A and B
        const auto known = (b.expect.at(id) / a.expect.at(id));

        // Calculate actual fold change between mixture A and B
        const auto actual = (b.adjusted.at(id) / a.adjusted.at(id));

        options.logInfo((boost::format("%1%\t%2%\t%3%") % id % known % actual).str());
        
        std::cout << id << " " << known << "  " << actual << std::endl;

        stats.z.push_back(id);
        stats.x.push_back(log(known));
        stats.y.push_back(log(actual));
    }
    
    options.logInfo("Checking for sequins in mix B but not in mix A");

    /*
     * Print a warning message for each sequin detected in B but not in A
     */
    
    for (const auto &i : b.actual)
    {
        const auto &id = i.first;

        if (!a.actual.at(id))
        {
            const auto msg = (boost::format("Warning: %1% defined in mixture B but not in mixture A") % id).str();
            options.out(msg);
            options.warn(msg);
        }
    }

    
    
    
    
    
    
    AnalyzeReporter::linear(stats, "ladder_diffs", "FPKM", options.writer);

    /*
     * Generate a CSV for differential
     */

    options.info("Writing differential CSV");

    auto writeHist = [&](const std::string &file,
                         const std::map<SequinID, Counts>   &abund,
                         const std::map<SequinID, Coverage> &expect,
                         const std::map<SequinID, Coverage> &actual,
                         const std::map<SequinID, Coverage> &correct)
    {
        const std::string format = "%1%\t%2%\t%3%\t%4%\t%5%";
        
        options.writer->open(file);
        options.writer->write((boost::format(format) % "ID" % "abund" % "expect" % "observed" % "adjusted").str());
        
        /*
         * The argument abund is a histogram of abundance before normalization. It's directly taken off from
         * the alignment file. Not all sequins would be detected, in fact anything could be in the histogram.
         */
        
        assert(expect.size() == actual.size() && expect.size() == correct.size());
        
        for (const auto &i : correct)
        {
            // Eg: GA322_B
            const auto id = i.first;
            
            if (abund.count(id))
            {
                options.writer->write((boost::format(format) % id
                                       % abund.at(id)
                                       % expect.at(id)
                                       % actual.at(id)
                                       % correct.at(id)).str());
            }
            else
            {
                options.writer->write((boost::format(format) % id
                                       % "NA"
                                       % "NA"
                                       % "NA"
                                       % "NA").str());
            }
        }
        
        options.writer->close();
    };
    
    options.info("Generating histogram");
//    writeHist("ladder_hist.csv", stats.abund, stats.expect, stats.actual, stats.adjusted);

	return stats;
}