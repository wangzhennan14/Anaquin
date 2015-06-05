#include "data/tokens.hpp"
#include "meta/m_assembly.hpp"
#include "parsers/parser_blat.hpp"

using namespace Spike;

Velvet::VelvetStats Velvet::analyze(const std::string &contig, const std::string &blat)
{
    Velvet::VelvetStats stats;
    std::map<std::string, ParserBlat::BlatLine> psl;

    // Perform a pairwise alignment with blat
    ParserBlat::parse(blat, [&](const ParserBlat::BlatLine &l, const ParserProgress &p)
    {
        psl[l.qName] = l;
    });

    assert(!psl.empty());
    
    /*
     * Read coverage from the contig file. The format looks like:
     *
     *      >NODE_77460_length_31_cov_1.129032
     */

    std::vector<std::string> tokens;
    
    ParserFA::parse(contig, [&](const FALine &l, const ParserProgress &)
    {
        Tokens::split(l.id, "_", tokens);
        Node node;
        
        // Eg: NODE_77460
        node.id = l.id;

        if (psl.count(l.id))
        {
            node.sequin = psl[l.id].tName;
        }

        // Coverage of the node in k-mer
        node.cov = stof(tokens[tokens.size() - 1]);

        stats.nodes.push_back(node);
    });

    return stats;
}

MAssemblyStats MAssembly::analyze(const std::string &file, const Options &options)
{
    MAssemblyStats stats = DNAsssembly::stats<MAssemblyStats>(file);

    if (!options.psl.empty())
    {
        std::cout << "Using an aligment file: " << options.psl << std::endl;
        
        // Calculate velvet-specific statistics
        const auto contigs = Velvet::analyze(file, options.psl);
        
        std::cout << "Creating a abundance plot" << std::endl;
        
        /*
         * Plot the coverage realtive to the known concentration (in attamoles/ul) of each assembled contig.
         */

        std::vector<double> x, y;
        std::vector<std::string> z;
        
        for (const auto &node : contigs.nodes)
        {
            if (!node.sequin.empty())
            {
                const auto &seq = Standard::instance().m_seq_A.at(node.sequin);
                
                // Known concentration from the matching sequin
                const auto known = seq.abund();
                
                // Measured coverage
                const auto measured = node.cov;
                
                x.push_back(log(known));
                y.push_back(log(measured));
                z.push_back(node.sequin);
            }
        }
        
        // Generate a R script for a plot of abundance
        AnalyzeReporter::script("abundance.R", x, y, z, options.writer);
    }
    
    /*
     * Write out assembly results
     */

    const std::string format = "%1%\t%2%\t%3%\t%4%\t%5%\t%6%";

    options.writer->open("assembly.stats");
    options.writer->write((boost::format(format) % "N20" % "N50" % "N80" % "min" % "mean" % "max").str());
    options.writer->write((boost::format(format) % stats.N20
                                                 % stats.N50
                                                 % stats.N80
                                                 % stats.min
                                                 % stats.mean
                                                 % stats.max).str());
    options.writer->close();

    return stats;;
}