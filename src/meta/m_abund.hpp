#ifndef M_ABUNDANCE_HPP
#define M_ABUNDANCE_HPP

#include "meta/m_assembly.hpp"

namespace Anaquin
{
    struct MAbundance
    {
        template <typename Options> static void logOptions(const Options &o)
        {
            switch (o.coverage)
            {
                case WendySmooth:    { o.info("Wendy Smoothing");           break; }
                case KMerCov_Contig: { o.info("K-mer coverage per contig"); break; }
                case KMerCov_Sequin: { o.info("K-mer coverage per sequin"); break; }
            }
        };

        enum CoverageMethod
        {
            WendySmooth,
            KMerCov_Contig, // K-mer coverage relative to the size of the contig
            KMerCov_Sequin, // K-mer coverage relative to the size of the sequin
        };

        template <typename Options, typename Stats, typename DStats> static Point calculate
                    (Stats &stats,
                     const MBlat::Stats &bStats,
                     const DStats &dStats,
                     const SequinID &id,
                     MetaAlignment &align,
                     const Options &o,
                     CoverageMethod cov = WendySmooth)
        {
            /*
             * Plot the coverage relative to the known concentration for each assembled contig
             */
            
            if (!align.contigs.empty())
            {
                stats.h.at(id)++;
                
                // Known concentration
                const auto known = align.seq->abund(Mix_1, false);
                
                /*
                 * Measure concentration for this MetaQuin. Average out the coverage for each aligned contig.
                 */
                
                Concentration measured = 0;
                
                // Sum of k-mer lengths for all sequins
                Base sumKLength = 0;
                
                for (auto i = 0; i < align.contigs.size(); i++)
                {
                    if (!bStats.aligns.count(align.contigs[i].id))
                    {
                        continue;
                    }
                    else if (!dStats.contigs.count(align.contigs[i].id))
                    {
                        o.warn((boost::format("%1% not found in the input file") % align.contigs[i].id).str());
                        continue;
                    }
                    
                    const auto &contig = dStats.contigs.at(align.contigs[i].id);

                    assert(align.seq->l.length());
                    assert(contig.k_cov && contig.k_len);
                    
                    sumKLength += contig.k_len;
                    
                    switch (cov)
                    {
                        case WendySmooth:    { measured += contig.k_len * contig.k_cov;          break; }
                        case KMerCov_Contig: { measured += contig.k_cov / contig.k_len;          break; }
                        case KMerCov_Sequin: { measured += contig.k_cov / align.seq->l.length(); break; }
                    }
                    
                    /*
                     * Calculate for the average depth for alignment and sequin
                     */
                    
                    align.depthAlign  += align.contigs[i].l.length() * contig.k_cov / align.contigs[i].l.length();
                    align.depthSequin += align.contigs[i].l.length() * contig.k_cov;
                }
                
                if (o.coverage == WendySmooth)
                {
                    measured = measured / sumKLength;
                }
                
                if (measured)
                {
                    assert(align.seq->l.length());
                    align.depthSequin = align.depthSequin / align.seq->l.length();
                    return Point(known, measured);
                }
            }
            
            return Point(0 ,0);
        }
        
        struct Stats : public LinearStats, public MappingStats
        {
            Limit ss;

            // Distribution of the sequins
            SequinHist h = Standard::instance().r_meta.hist();
        };

        struct Options : public MAssembly::Options
        {
            // Required by the GCC compiler ...
            Options() {}

            // How the measured coverage is computed
            CoverageMethod coverage = WendySmooth;
        };
        
        static Stats analyze(const FileName &, const Options &o = Options());
        static void  report (const FileName &, const Options &o = Options());
    };
}

#endif