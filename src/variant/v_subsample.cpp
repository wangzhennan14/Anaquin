#include "tools/sampling.hpp"
#include "parsers/parser_sam.hpp"
#include "writers/writer_sam.hpp"
#include "variant/v_subsample.hpp"

using namespace Anaquin;

VSubsample::Stats VSubsample::stats(const FileName &file, const Options &o)
{
    Stats stats;

    const auto &r = Standard::instance();
    o.analyze(file);

    /*
     * Generating coverage on both chromosomes
     */
     
    stats.cov = CoverageTool::stats(file, [&](const Alignment &align, const ParserProgress &)
    {
        if (align.id == r.id)
        {
            return static_cast<bool>(r.r_var.findGeno(align.l));
        }
        else if (align.id == "chr21")
        {
            return static_cast<bool>(r.r_var.findInterval("chr21", align.l));
        }

        return false;
    });

    o.info(std::to_string(stats.cov.inters.size()) + " intervals generated");

    o.info("Generating statistics for chrT");
    stats.chrT = stats.cov.inters.find(r.id)->stats();

    o.info("Generating statistics for chr21");
    stats.hg38 = stats.cov.inters.find(r.id)->stats();
    
    /*
     * Now we have the data, we'll need to compare the coverage and determine what fraction that
     * the synthetic chromosome needs to be subsampled.
     */
    
    switch (o.method)
    {
        case ArithmeticAverage:
        {
            stats.chrTC = stats.chrT.mean;
            stats.hg38C = stats.hg38.mean;
            break;
        }

        case Maximum:
        {
            stats.chrTC = stats.chrT.max;
            stats.hg38C = stats.hg38.max;
            break;
        }

        case Median:
        {
            stats.chrTC = stats.chrT.p50;
            stats.hg38C = stats.hg38.p50;
            break;
        }

        case Percentile75:
        {
            stats.chrTC = stats.chrT.p75;
            stats.hg38C = stats.hg38.p75;
            break;
        }
    }

    assert(stats.chrTC && stats.hg38C);

    if (stats.hg38C >= stats.chrTC)
    {
        // Not an error because it could happen in a simulation
        o.warn("The coverage for the genome is higher than the synthetic chromosome. This is unexpected because the genome is much wider.");
    }

    return stats;
}

// Generate and report statistics for subsampling
void VSubsample::report(const FileName &file, const Options &o)
{
    const auto stats = VSubsample::stats(file, o);
    
    /*
     * Generating summary statistics
     */
    
    o.info("Generating summary statistics");
    
    /*
     * Generating a subsampled alignment. It's expected that coverage would roughly match between
     * the genome and synthetic chromosome.
     */
    
    o.info("Subsampling the alignment");
    
    WriterSAM writer;
    writer.open("/Users/tedwong/Sources/QA/abcd.sam");
    
    SamplingTool sampler(1 - stats.fract());

    ParserSAM::parse(file, [&](const Alignment &align, const ParserSAM::AlignmentInfo &info)
    {
        const bam1_t *b    = reinterpret_cast<bam1_t *>(info.data);
        const bam_hdr_t *h = reinterpret_cast<bam_hdr_t *>(info.header);

        if (!align.i)
        {
            /*
             * This is the key, randomly write the reads with certain probability
             */

            if (sampler.select(bam_get_qname(b)))
            {
                writer.write(h, b);
            }
        }
    });
    
    writer.close();
}