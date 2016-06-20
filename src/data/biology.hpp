#ifndef BIOLOGY_HPP
#define BIOLOGY_HPP

#include "data/types.hpp"

namespace Anaquin
{
    enum Strand
    {
        Forward,
        Backward,
    };

    enum RNAFeature
    {
        Exon,
        Gene,
        Intron,
        Transcript,
    };
    
    enum Mutation
    {
        SNP,
        Insertion,
        Deletion
    };
}

#endif