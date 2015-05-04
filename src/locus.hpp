#ifndef GI_LOCUS_HPP
#define GI_LOCUS_HPP

#include <set>
#include <map>
#include <list>
#include <vector>
#include <assert.h>
#include "types.hpp"

namespace Spike
{
    struct Locus
    {
        Locus(const Locus &l1, const Locus &l2)
        {
            end   = std::max(l1.end,   l2.end);
            start = std::min(l1.start, l2.start);
        }

        Locus(BasePair start = 0, BasePair end = 0) : start(start), end(end) {}

        static bool overlap(const std::vector<Locus> &ls)
        {
            for (auto i = 0; i < ls.size(); i++)
            {
                for (auto j = i + 1; j < ls.size(); j++)
                {
                    if (ls[i].overlap(ls[j]))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        template <typename T> static std::vector<Locus> merge(const std::vector<T> &x)
        {
            std::vector<Locus> sorted;

            // std::copy doesn't allow implicit type conversion...
            for (auto i : x)
            {
                sorted.push_back(i);
            }

            std::sort(sorted.begin(), sorted.end(), [&](const Locus &l1, const Locus &l2)
            {
                return (l1.start < l2.start) || (l1.start == l2.start && l1.end < l2.end);
            });
            
            std::vector<Locus> merged;
            
            for (auto i = 0; i < sorted.size();)
            {
                // We'll need the index for skipping i
                auto j = i + 1;
                
                Locus super = sorted[i];
                
                // Look forward until the end of an overlap region
                for (; j < sorted.size(); j++)
                {
                    if (sorted[j].overlap(super))
                    {
                        super = super + sorted[j];
                    }
                    else
                    {
                        break;
                    }
                }

                // Construct the super-loci for the region
                merged.push_back(super);

                i = j;
            }
            
            return merged;
        }
        
        inline BasePair length() const { return (end - start + 1); }

        inline BasePair overlap(const Locus &l) const
        {
            if (l.start > end || start > l.end)
            {
                return 0;
            }
            else if (start <= l.start && end >= l.end)
            {
                return l.length();
            }
            else if (start >= l.start && end <= l.end)
            {
                return length();
            }
            else if (end >= l.end)
            {
                return l.end - start + 1;
            }
            else
            {
                return end - l.start + 1;
            }
        }

        inline bool contains(const Locus &q) const
        {
            return (q.start >= start && q.end <= end);
        }

        inline Locus operator+(const Locus &l) const { return Locus(std::min(start, l.start), std::max(end, l.end));}
        inline bool operator==(const Locus &l) const { return start == l.start && end == l.end; }
        inline bool operator<(const Locus &l)  const { return start < l.start || (start == l.start && end < l.end); }

        BasePair start, end;
    };
}

#endif