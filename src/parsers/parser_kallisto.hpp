#ifndef PARSER_KALLISTO_HPP
#define PARSER_KALLISTO_HPP

#include "data/data.hpp"
#include "data/tokens.hpp"
#include "data/reader.hpp"
#include "data/convert.hpp"
#include "data/standard.hpp"
#include "parsers/parser.hpp"

namespace Anaquin
{
    struct ParserKallisto
    {
        enum Field
        {
            TargetID,
            Length,
            EffLength,
            EstCounts,
            TPM,
        };

        struct Data
        {
            ChrID cID;
            
            // Eg: R1_101_1
            IsoformID id;

            // Estimated abundance
            Coverage abund;
        };
        
        static bool isKallisto(const Reader &r)
        {
            std::string line;
            std::vector<Token> toks;
            
            // Read the header
            if (r.nextLine(line))
            {
                Tokens::split(line, "\t", toks);
                
                if (toks.size() == 5         &&
                    toks[0]  == "target_id"  &&
                    toks[1]  == "length"     &&
                    toks[2]  == "eff_length" &&
                    toks[3]  == "est_counts" &&
                    toks[4]  == "tpm")
                {
                    return true;
                }
            }

            return false;
        }

        static void parse(const Reader &rr, std::function<void(const Data &, const ParserProgress &)> f)
        {
            protectParse("Kallisto format", [&]()
            {
                const auto &r = Standard::instance().r_rna;

                Data d;
                ParserProgress p;
                
                Line line;
                std::vector<Token> toks;
                
                while (rr.nextLine(line))
                {
                    if (p.i++ == 0)
                    {
                        continue;
                    }
                    
                    Tokens::split(line, "\t", toks);
                    
                    d.id = toks[TargetID];
                    
                    if (r.findTrans(ChrIS, d.id))
                    {
                        d.cID = ChrIS;
                    }
                    else
                    {
                        // We don't know exactly where it is...
                        d.cID = Geno;
                    }

                    d.abund = s2d(toks[TPM]);
                    
                    f(d, p);
                }
            });
        }
    };
}

#endif
