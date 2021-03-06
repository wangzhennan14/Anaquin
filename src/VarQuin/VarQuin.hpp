#ifndef VARQUIN_HPP
#define VARQUIN_HPP

#include "data/data.hpp"
#include "data/standard.hpp"
#include "parsers/parser_vcf.hpp"
#include "parsers/parser_variants.hpp"
#include <boost/algorithm/string/predicate.hpp>

namespace Anaquin
{
    struct VariantStats
    {
        // Number of SNPs detected
        Counts n_snp;

        // Number of indels detected
        Counts n_ind;
    };
    
    struct VariantMatch
    {
        // The called variant
        CalledVariant query;

        // Matched by position?
        const Variant *match = nullptr;
        
        // Matched by variant allele? Only if position is matched.
        bool alt;
        
        // Matched by reference allele? Only if position is matched.
        bool ref;
    };

    // Eg: chrev1, chrev10 etc...
    inline bool isReverseGenome(const ChrID &cID)
    {
        A_ASSERT(!cID.empty());
        return cID.find("rev") != std::string::npos;
    }

    inline ChrID toReverse(const ChrID &cID)
    {
        auto x = cID;
        
        // Eg; chr2 to chrev2
        boost::replace_all(x, "chr", "chrev");
        
        A_ASSERT(!x.empty());
        
        return x;
    }

    // Eg: ST_007_R, GS_047 etc...
    inline bool isVarQuin(const SequinID &id)
    {
        if (id.find("GS_") != std::string::npos)
        {
            return true;
        }
        else if (id.find("GI_") != std::string::npos)
        {
            return true;
        }
        else if (id.find("_R") != std::string::npos)
        {
            return true;
        }
        else if (id.find("_V") != std::string::npos)
        {
            return true;
        }
        else if (id.find("CS_") != std::string::npos)
        {
            return true;
        }
        else if (id.find("CI_") != std::string::npos)
        {
            return true;
        }
        else if (id.find("rev") != std::string::npos)
        {
            return true;
        }

        return false;
    }

    inline std::string type2str(Mutation type)
    {
        switch (type)
        {
            case Mutation::SNP:       { return "SNP";       }
            case Mutation::Deletion:  { return "Deletion";  }
            case Mutation::Insertion: { return "Insertion"; }
        }
    }

    inline bool isRefID(const SequinID &id)
    {
        if (boost::algorithm::ends_with(id, "_R"))
        {
            return true;
        }
        else if (boost::algorithm::ends_with(id, "_V"))
        {
            return false;
        }
        
        throw std::runtime_error("Unknown sequin: " + id);
    }
    
    // Eg: D1_1_R to D1_1
    inline SequinID baseID(const SequinID &id)
    {
        auto tmp = id;

        boost::replace_all(tmp, "_R", "");
        boost::replace_all(tmp, "_V", "");
        
        return tmp;
    }
    
    inline SequinID refID(const SequinID &id)
    {
        return baseID(id) + "_R";
    }
    
    inline SequinID varID(const SequinID &id)
    {
        return baseID(id) + "_V";
    }

    /*
     * Common framework for parsing and matching called variants
     */

    template <typename F, typename Input> void parseVariants(const FileName &file, Input input, F f)
    {   
        const auto &r = Standard::instance().r_var;

        VariantMatch m;

        auto match = [&](const CalledVariant &query)
        {
            m.query = query;
            m.match = nullptr;

            const auto isSyn = isVarQuin(query.cID);

            if (isSyn || Standard::isGenomic(query.cID))
            {
                // Can we match by position?
                m.match = r.findVar(query.cID, query.l);

                if (m.match)
                {
                    m.ref = m.match->ref == query.ref;
                    m.alt = m.match->alt == query.alt;
                }
            }

            return m;
        };

        switch (input)
        {
            case Input::VCFInput:
            {
                ParserVCF::parse(file, [&](const ParserVCF::Data &d, const ParserProgress &)
                {
                    f(match(d));
                });

                break;
            }

            case Input::TxtInput:
            {
                ParserVariant::parse(file, [&](const ParserVariant::Data &d, const ParserProgress &)
                {
                    f(match(d));
                });

                break;
            }
        }
    }
}

#endif
