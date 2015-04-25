#ifndef GI_CLASSIFY_HPP
#define GI_CLASSIFY_HPP

#include "standard.hpp"
#include <ss/ml/confusion.hpp>

namespace Spike
{
    class Confusion : public SS::Confusion
    {
        public:
            inline Counts& nq() const { return _nq; }
            inline Counts& nr() const { return _nr; }

            inline Counts &tn() const { throw std::runtime_error("tn() is unsupported"); }

            inline Counts &fp() const
            {
                assert(_nq - _tp == _fp);
                return _fp;
            }

            inline Percentage sn() const
            {
                assert(_nr);
                
                // Adjust for fn... Refer to the wikipedia for more details
                _fn = _nr - _tp;
                
                return SS::Confusion::sn();
            }
        
            /*
             * The usual formula: tn / (tn + fp) would not work. We don't know
             * tn, furthermore fp would have been dominated by tn. The formula
             * below is consistent to cufflink's recommendation. Technically,
             * we're not calculating specificity but positive predication value.
             */

            inline Percentage sp() const
            {
                return ((tp() + fp()) && fp() != n()) ? tp() / (tp() + fp()) : NAN;
            }

        private:
            mutable Counts _nq = 0;
            mutable Counts _nr = 0;
    };
    
    inline bool tfp(bool cond, Confusion *m1, Confusion *m2 = NULL)
    {
        if (cond)
        {
            if (m1) { m1->tp()++; }
            if (m2) { m2->tp()++; }
        }
        else
        {
            if (m1) { m1->fp()++; }
            if (m2) { m2->fp()++; }
        }

        return cond;
    }

    template <typename Iter, typename T> bool find(const Iter &iter, const T &t)
    {
        for (auto i: iter)
        {
            if (i.l.contains(t.l))
            {
                return true;
            }
        }

        return false;
    }
    
    template <typename Iter, typename T> bool find_map(const Iter &map, const T &t)
    {
        for (auto i: map)
        {
            if (i.second.l.contains(t.l))
            {
                return true;
            }
        }

        return false;
    }

    /*
     * Binary classification. Negativity isn't required because in a typical experiment
     * the dilution would be so low that negativity dominates positivity.
     */

    template <typename T, typename Positivity>
    bool classify(Confusion &m, const T &t, Positivity p)
    {
        static const auto &s = Standard::instance();

        if (t.id == s.id && s.l.contains(t.l))
        {
            // Regardless of whether it's tp, it's counted as an experiment (query) unit
            m.nq()++;

            if (p(t))
            {
                m.tp()++;
                return true;
            }
            else
            {
                m.fp()++;
            }
        }

        return false;
    }
}

#endif