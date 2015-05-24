#ifndef GI_FEATURE_HPP
#define GI_FEATURE_HPP

#include "locus.hpp"
#include "biology.hpp"

namespace Spike
{
    typedef std::string FeatureID;
    
    struct Feature
    {
        inline bool overlap(const Locus &l) const
        {
            return this->l.overlap(l);
        }

        operator Locus() const     { return l;  }
        operator FeatureID() const { return id; }

        void operator=(const Feature &f)
        {
            l  = f.l;
            id = f.id;
            type = f.type;
            tID  = f.tID;
            geneID = f.geneID;
        }
        
        FeatureID id;

        // The location of the feature relative to the chromosome
        Locus l;

        RNAFeature type;
        
        // Empty if the information is unavailable
        GeneID geneID;

        // Empty if the information is unavailable
        TranscriptID tID;
    };
}

#endif