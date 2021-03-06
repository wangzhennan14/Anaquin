#ifndef MERGED_HPP
#define MERGED_HPP

#include <set>
#include <map>
#include <cmath>
#include <numeric>
#include "data/data.hpp"
#include "data/itree.hpp"
#include "data/locus.hpp"
#include "tools/errors.hpp"

namespace Anaquin
{
    class MergedInterval : public Matched
    {
        public:
        
            typedef std::string IntervalID;
        
            struct Stats
            {
                Base length = 0;
                Base nonZeros = 0;
            
                inline Proportion covered() const { return static_cast<double>(nonZeros) / length; }
            };

            MergedInterval() {}
            MergedInterval(const IntervalID &id, const Locus &l) : _id(id), _l(l) {}
            MergedInterval(const IntervalID &id,
                           const Locus      &l,
                           const GeneID     &gID,
                           const TransID    &tID) :
                        _id(id), _tID(tID), _gID(gID), _l(l) {}

            // Return loci where no alignment
            std::set<Locus> zeros() const;
        
            Base map(const Locus &l, Base *lp = nullptr, Base *rp = nullptr);
        
            template <typename F> Stats stats(F f) const
            {
                Stats stats;

                for (const auto &i : _data)
                {
                    stats.nonZeros += i.second.length();
                }
                
                stats.length = _l.length();
                assert(stats.length >= stats.nonZeros);

                return stats;
            }
        
            inline Stats stats() const
            {
                return stats([&](const ChrID &id, Base i, Base j, Coverage cov)
                {
                    return true;
                });
            }

            inline void merge(const Locus &l) { _l.merge(l); }

            inline const Locus &l()       const { return _l;  }
            inline const IntervalID &id() const { return _id; }

            inline const std::string &gID() const { return _gID; }
            inline const std::string &tID() const { return _tID; }
        
            inline IntervalID name() const override { return id(); }
        
            inline std::size_t size() { return _data.size(); }
        
        //private:
        
            // The first base of the mini-region
            Base _x = NAN;
        
            // The last base of the mini-region
            Base _y = NAN;
        
            Locus _l;

            std::map<Base, Locus> _data;
        
            GeneID  _gID;
            TransID _tID;
        
            IntervalID _id;
    };
    
    template <typename T = MergedInterval> class MergedIntervals
    {
        public:
        
            struct Stats : public T::Stats
            {
                // Total number of intervals
                Counts n = 0;
                
                // Number of intervals with full coverage
                Counts f = 0;
            };
        
            typedef std::map<typename T::IntervalID, T> IntervalData;

            inline void add(const T &i)
            {
                _inters.insert(typename std::map<typename T::IntervalID, T>::value_type(i.id(), i));
            }

            /*
             * Merge the new interval with the first existing overlapping interval. New interval is
             * added if no overlapping found.
             */
            
            inline void merge(const T &i)
            {
                for (auto &j : _inters)
                {
                    if (j.second.l().overlap(i.l()))
                    {
                        j.second.merge(i.l());
                        return;
                    }
                }

                add(i);
            }
        
            inline void build()
            {
                std::vector<Interval_<T *>> loci;
            
                #define LOCUS_TO_TINTERVAL(x) Interval_<T *>(x.l().start, x.l().end, &x)
            
                for (auto &i : _inters)
                {
                    loci.push_back(LOCUS_TO_TINTERVAL(i.second));
                }
                
                A_CHECK(!loci.empty(), "No interval was built. Zero interval.");
            
                _tree = std::shared_ptr<IntervalTree<T *>>(new IntervalTree<T *> { loci });

                A_CHECK(_tree, "Failed to build interval treee");
            }
        
            inline T * find(const typename T::IntervalID &id)
            {
                return _inters.count(id) ? &(_inters.at(id)) : nullptr;
            }
        
            inline const T * find(const typename T::IntervalID &id) const
            {
                return _inters.count(id) ? &(_inters.at(id)) : nullptr;
            }

            inline T * exact(const Locus &l, std::vector<T *> *r = nullptr) const
            {
                // This could happen for chrM (no intron)
                if (!_tree)
                {
                    return nullptr;
                }
                
                auto v = _tree->findContains(l.start, l.end);

                T *t = nullptr;
    
                for (const auto &i : v)
                {
                    if (i.value->l() == l)
                    {
                        t = i.value;
                        
                        if (r)
                        {
                            r->push_back(t);
                        }
                    }
                }
            
                return t;
            }
        
            inline T * contains(const Locus &l, std::vector<T *> *r = nullptr) const
            {
                // This could happen for chrM (no intron)
                if (!_tree)
                {
                    return nullptr;
                }

                auto v = _tree->findContains(l.start, l.end);

                if (r)
                {
                    for (const auto &i : v)
                    {
                        if (i.value)
                        
                        r->push_back(i.value);
                    }
                }
            
                return v.empty() ? nullptr : v.front().value;
            }
        
            inline T * overlap(const Locus &l, std::vector<T *> *r = nullptr) const
            {
                // This could happen for chrM (no intron)
                if (!_tree)
                {
                    return nullptr;
                }

                auto v = _tree->findOverlapping(l.start, l.end);
            
                if (r)
                {
                    for (const auto &i : v)
                    {
                        r->push_back(i.value);
                    }
                }
            
                return v.empty() ? nullptr : v.front().value;
            }

            typename MergedIntervals::Stats stats() const
            {
                MergedIntervals::Stats stats;
            
                for (const auto &i : _inters)
                {
                    const auto s = i.second.stats();
                
                    stats.n++;
                    stats.length   += s.length;
                    stats.nonZeros += s.nonZeros;
                    
                    assert(s.length >= s.nonZeros);
                    
                    if (s.length == s.nonZeros)
                    {
                        stats.f++;
                    }
                }

                assert(stats.n >= stats.f);
                return stats;
            }
        
            inline const IntervalData &data() const { return _inters; }
        
            // Number of intervals
            inline Counts size() const { return _inters.size(); }
        
            inline Base length() const
            {
                return std::accumulate(_inters.begin(), _inters.end(), 0,
                        [&](int sums, const std::pair<MergedInterval::IntervalID, MergedInterval> & p)
                {
                    return sums + p.second.l().length();
                });
            }
        
        //private:
        
            std::shared_ptr<IntervalTree<T *>> _tree;
        
            IntervalData _inters;
    };

    typedef std::map<ChrID, MergedIntervals<>> MC2Intervals;
}

#endif