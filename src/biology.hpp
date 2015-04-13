#ifndef GI_BIOLOGY_HPP
#define GI_BIOLOGY_HPP

namespace Spike
{
    template <typename T1, typename T2> bool contains(const T1 &t1, const T2 &t2)
    {
        return t1.l.contains(t2.l);
    }

    /*
     * Search for a particular element that matches the given loci. Returns the status and the element
     * if it succeed.
     */

    template <typename Iter, typename T, typename F> bool find(const Iter &begin, const Iter &end, const T &t, F &r)
    {
        for (auto i = begin; i < end; i++)
        {
            if (i->l.contains(t.l))
            {
                r = *i;
                return true;
            }
        }
        
        return false;
    }
    
    /*
     * Intron is a sequence within a gene that is removed by RNA splicing. This function calculates
     * the gaps between the given exons and assume them be introns (also known as spliced junctions).
     */
    
    template <typename Iter, typename F> void extractIntrons(const Iter &exons, F f)
    {
        for (auto i = 0; i < exons.size(); i++)
        {
            if (i)
            {
                f(exons[i - 1].end, exons[i].start);
            }
        }
    }
}

#endif