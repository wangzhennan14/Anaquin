#include <ss/stats.hpp>
#include "tools/random.hpp"
#include "VarQuin/v_sample2.hpp"
#include "writers/writer_sam.hpp"
#include "readers/reader_bam.hpp"

// Defined in main.cpp
extern Anaquin::FileName BedRef();

using namespace Anaquin;

typedef std::map<ChrID, std::map<Locus, Proportion>> NormFactors;

static ReaderBam::Stats sample(const FileName &file,
                               const NormFactors &norms,
                               VSample2::Stats &stats,
                               const VSample2::Options &o)
{
    typedef std::map<ChrID, std::map<Locus, std::shared_ptr<RandomSelection>>> Selection;
    
    /*
     * Initalize independnet random generators for every sampling region
     */
    
    Selection select;
    
    for (const auto &i : norms)
    {
        for (const auto &j : i.second)
        {
            assert(j.second >= 0 && j.second <= 1.0 && !isnan(j.second));
            
            // Create independent random generator for each region
            select[i.first][j.first] = std::shared_ptr<RandomSelection>(new RandomSelection(1.0 - j.second));
        }
    }

    assert(select.size() == norms.size());

    o.info("Sampling: " + file);
    
    WriterSAM writer;
    writer.openTerm();

    // Subsampling regions
    const auto sampled = Standard::instance().r_var.dInters();
    
    return ReaderBam::stats(file, sampled, [&](const ParserSAM::Data &x, const ParserSAM::Info &info)
    {
        if (info.p.i && !(info.p.i % 1000000))
        {
            o.logInfo(std::to_string(info.p.i));
        }
        
        auto shouldSampled = !x.mapped;
        
        if (!shouldSampled)
        {
            Interval *inter;
            
            /*
             * Should that be contains or overlap? We prefer overlaps because any read that is overlapped
             * into the regions still give valuable information and sequencing depth.
             */

            if (sampled.count(x.cID) && (inter = sampled.at(x.cID).overlap(x.l)))
            {
                if (select.at(x.cID).at(inter->l())->select(x.name))
                {
                    shouldSampled = true;
                }
            }
            else
            {
                // Never sample for reads outside the regions
                shouldSampled = true;
            }
        }
        
        if (shouldSampled)
        {
            stats.totAfter.countSyn++;
            
            // Write SAM read to console
            writer.write(x);
            
            return true;
        }

        return false;
    });
}

template <typename Stats> Coverage stats2cov(const VSample2::Method meth, const Stats &stats)
{
    switch (meth)
    {
        case VSample2::Method::Mean:   { return stats.mean; }
        case VSample2::Method::Median: { return stats.p50;  }

        /*
         * Prop and Reads specifies a fixed proportion to subsample. It's not actually a measure to report coverage.
         */

        case VSample2::Method::Prop:
        case VSample2::Method::Reads: { return stats.mean; }
    }
}

VSample2::Stats VSample2::analyze(const FileName &gen, const FileName &seq, const Options &o)
{
    o.analyze(gen);
    o.analyze(seq);

    const auto &r = Standard::instance().r_var;
    
    VSample2::Stats stats;
    
    // Regions to subsample
    const auto refs = r.dInters();
    
    A_CHECK(!refs.empty(), "Empty reference sampling regions");
    
    // Checking genomic alignments before subsampling
    const auto gStats = ReaderBam::stats(gen, refs, [&](const ParserSAM::Data &x, const ParserSAM::Info &info)
    {
        if (info.p.i && !(info.p.i % 1000000))
        {
            o.logWait(std::to_string(info.p.i));
        }
        
        if (x.mapped)
        {
            stats.totBefore.countGen++;
        }
        
        return true;
    });
    
    // Checking synthetic alignments before subsampling
    const auto sStats = ReaderBam::stats(seq, refs, [&](const ParserSAM::Data &x, const ParserSAM::Info &info)
    {
        if (info.p.i && !(info.p.i % 1000000))
        {
            o.logWait(std::to_string(info.p.i));
        }
        
        if (x.mapped)
        {
            stats.totBefore.countSyn++;
        }
        
        return true;
    });
    
    // Normalization for each region
    NormFactors norms;

    // Required for summary statistics
    std::vector<double> allNorms;

    std::vector<double> allAfterSynC;
    std::vector<double> allBeforeGenC;
    std::vector<double> allBeforeSynC;
    
    // For each chromosome...
    for (auto &i : refs)
    {
        // Eg: chr1
        const auto cID = i.first;

        // For each region...
        for (auto &j : i.second.data())
        {
            const auto &l = j.second.l();
            
            // Genomic statistics within the region
            const auto gs = gStats.inters.at(cID).find(l.key())->stats();
            
            // Synthetic statistics within the region
            const auto ss = sStats.inters.at(cID).find(l.key())->stats();
            
            o.info("Calculating coverage for the synthetic and genome");
            
            /*
             * Now we have the data, we'll need to compare the coverage and determine the fraction that
             * the synthetic alignments needs to be sampled.
             */
            
            const auto synC = stats2cov(o.meth, ss);
            const auto genC = stats2cov(o.meth, gs);

            Proportion norm = NAN;

            switch (o.meth)
            {
                case VSample2::Method::Mean:
                case VSample2::Method::Median: { norm = synC ? std::min(genC / synC, 1.0) : NAN; break; }
                case VSample2::Method::Prop:
                {
                    A_ASSERT(!isnan(o.p));
                    norm = o.p;
                    break;
                }

                case VSample2::Method::Reads:
                {
                    A_ASSERT(!isnan(o.reads));

                    // Number of reads in the region
                    const auto aligns = ss.aligns;

                    // Try to keep subsample the fixed number of reads
                    norm = o.reads >= aligns ? 1.0 : ((Proportion) o.reads) / aligns;

                    break;
                }
            }
            
            allBeforeGenC.push_back(genC);
            allBeforeSynC.push_back(synC);
            
            if (isnan(norm))
            {
                o.logWarn((boost::format("Normalization is NAN for %1%:%2%-%3%") % i.first
                                                                              % l.start
                                                                              % l.end).str());
                
                // We can't just use NAN...
                norm = 0.0;
            }
            else if (norm == 1.0)
            {
                o.logWarn((boost::format("Normalization is 1 for %1%:%2%-%3%") % i.first
                                                                            % l.start
                                                                            % l.end).str());
            }
            else
            {
                o.logInfo((boost::format("Normalization for %1%-%2% - %3%") % i.first
                                                                            % l.start
                                                                            % l.end).str());
            }
            
            if (!genC) { stats.noGAlign++; }
            if (!synC) { stats.noSAlign++; }
            
            stats.c2v[cID][l].rID    = j.first;
            stats.c2v[cID][l].gen    = genC;
            stats.c2v[cID][l].before = synC;
            stats.c2v[cID][l].norm   = norms[i.first][l] = norm;
            
            allNorms.push_back(norm);
        }
    }
    
    // Now, we have the normalization factors. We can proceed with subsampling.
    const auto after = sample(seq, norms, stats, o);
    
    /*
     * Assume our subsampling is working, let's check the coverage for every region.
     */
    
    // For each chromosome...
    for (auto &i : after.inters)
    {
        // For each region...
        for (auto &j : i.second.data())
        {
            stats.count++;
            const auto &l = j.second.l();
            
            // Coverage after subsampling
            const auto cov = stats2cov(o.meth, j.second.stats());

            stats.c2v[i.first][l].after = cov;
            
            // Required for generating summary statistics
            allAfterSynC.push_back(cov);
        }
    }
    
    stats.beforeGen = SS::mean(allBeforeGenC);
    stats.beforeSyn = SS::mean(allBeforeSynC);
    stats.afterGen  = stats.beforeGen;
    stats.afterSyn  = SS::mean(allAfterSynC);
    
    stats.normSD   = SS::getSD(allNorms);
    stats.normAver = SS::mean(allNorms);
    
    stats.totAfter.countGen = stats.totBefore.countGen;
    
    stats.sampAfter.countGen  = gStats.countGen;
    stats.sampBefore.countGen =  gStats.countGen;
    
    // Remember, the synthetic reads have been mapped to the forward genome
    stats.sampBefore.countSyn = sStats.countGen;

    // Remember, the synthetic reads have been mapped to the forward genome
    stats.sampAfter.countSyn = after.countGen;

    return stats;
}

static void generateCSV(const FileName &file, const VSample2::Stats &stats, const VSample2::Options &o)
{
    o.generate(file);

    const auto format = boost::format("%1%\t%2%\t%3%\t%4%\t%5%\t%6%\t%7%\t%8%");
    
    o.generate(file);
    o.writer->open(file);
    o.writer->write((boost::format(format) % "ID"
                                           % "ChrID"
                                           % "Start"
                                           % "End"
                                           % "Genome"
                                           % "Before"
                                           % "After"
                                           % "Norm").str());

    // For each chromosome...
    for (const auto &i : stats.c2v)
    {
        // For each variant...
        for (const auto &j : i.second)
        {
            o.writer->write((boost::format(format) % j.second.rID
                                                   % i.first
                                                   % j.first.start
                                                   % j.first.end
                                                   % j.second.gen
                                                   % j.second.before
                                                   % j.second.after
                                                   % j.second.norm).str());            
        }
    }
    
    o.writer->close();
}

static void generateSummary(const FileName &file,
                            const FileName &gen,
                            const FileName &seq,
                            const VSample2::Stats &stats,
                            const VSample2::Options &o)
{
    o.generate(file);
    
    auto meth2Str = [&]()
    {
        switch (o.meth)
        {
            case VSample2::Method::Mean:   { return "Mean";       }
            case VSample2::Method::Median: { return "Median";     }
            case VSample2::Method::Reads:  { return "Reads";      }
            case VSample2::Method::Prop:   { return "Proportion"; }
        }
    };

    const auto summary = "-------VarSubsample Summary Statistics\n\n"
                         "       Reference annotation file: %1%\n"
                         "       Alignment file (genome):  %2%\n"
                         "       Alignment file (sequins): %3%\n\n"
                         "-------Reference regions\n\n"
                         "       Variant regions: %4% regions\n"
                         "       Method: %5%\n\n"
                         "-------Total alignments (before subsampling)\n\n"
                         "       Synthetic: %6%\n"
                         "       Genome:    %7%\n\n"
                         "-------Total alignments (after subsampling)\n\n"
                         "       Synthetic: %8%\n"
                         "       Genome:    %9%\n\n"
                         "-------Alignments within sampling regions (before subsampling)\n\n"
                         "       Synthetic: %10%\n"
                         "       Genome:    %11%\n\n"
                         "-------Alignments within sampling regions (after subsampling)\n\n"
                         "       Synthetic: %12%\n"
                         "       Genome:    %13%\n\n"
                         "       Normalization: %14% \u00B1 %15%\n\n"
                         "-------Before subsampling (within sampling regions)\n\n"
                         "       Synthetic coverage (average): %16%\n"
                         "       Genome coverage (average):    %17%\n\n"
                         "-------After subsampling (within sampling regions)\n\n"
                         "       Synthetic coverage (average): %18%\n"
                         "       Genome coverage (average):    %19%\n";
    
    o.generate(file);
    o.writer->open(file);
    o.writer->write((boost::format(summary) % BedRef()                  // 1
                                            % gen                       // 2
                                            % seq                       // 3
                                            % stats.count               // 4
                                            % meth2Str()                // 5
                                            % stats.totBefore.countSyn  // 6
                                            % stats.totBefore.countGen  // 7
                                            % stats.totAfter.countSyn   // 8
                                            % stats.totAfter.countGen   // 9
                                            % stats.sampBefore.countSyn // 10
                                            % stats.sampBefore.countGen // 11
                                            % stats.sampAfter.countSyn  // 12
                                            % stats.sampAfter.countGen  // 13
                                            % stats.normAver            // 14
                                            % stats.normSD              // 15
                                            % stats.beforeSyn           // 16
                                            % stats.beforeGen           // 17
                                            % stats.afterSyn            // 18
                                            % stats.afterGen            // 19
                     ).str());
    o.writer->close();
}

void VSample2::report(const std::vector<FileName> &files, const Options &o)
{
    const auto stats = analyze(files[0], files[1], o);
    
    /*
     * Generating VarSubsample_summary.stats
     */
    
    generateSummary("VarSubsample_summary.stats", files[0], files[1], stats, o);

    /*
     * Generating VarSubsample_sequins.csv
     */

    generateCSV("VarSubsample_sequins.csv", stats, o);
}