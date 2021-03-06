#ifndef SS_STATS_HPP
#define SS_STATS_HPP

#include <vector>
#include <numeric>
#include <limits.h>
#include <algorithm>
#include <functional>
#include <ss/data/data.hpp>
#include <ss/data/errors.hpp>
#include <ss/internal/stats.hpp>

namespace SS
{
    /*
     * Produce sample quantiles corresponding to the given probabilities. The smallest observation corresponds to a
     * probability of 0 and the largest to a probability of 1.
     */

    template <typename T> typename T::value_type quantile(const T &x, P probs)
    {
        return Internal::quantile(x, probs);
    }
    
    template <typename T> typename T::value_type cov(const T &x, const T &y)
    {
        return Internal::cov(x, y);
    }

    template <class T> typename T::value_type getVariance(const T &x)
    {
        return Internal::cov(x, x);
    }
    
    // Computes the standard deviation
    template <typename T> typename T::value_type getSD(const T &x)
    {
        return sqrt(getVariance(x));
    }
    
    // Computes the arithmetic mean
    template <typename T> typename T::value_type mean(const T &x)
    {
        return (1.0 * Internal::sum(x)) / Internal::count(x);
    }

    template <typename T> Real corrPearson(const T &x, const T &y)
    {
        SS_ASSERT(x.size() == y.size(), "Incompatible dimensions");
        return Internal::corrPearson(x, y);
    }
    
    template <typename T> Real corrSpearman(const T &x, const T &y)
    {
        SS_ASSERT(x.size() == y.size(), "Incompatible dimensions");
        return Internal::corrSpearman(x, y);
    }

    enum TestType
    {
        Less,
        Greater,
        TwoSided,
    };
    
    namespace Internal
    {
        template <typename T> Real critical(T t, P conf, TestType type)
        {
            const auto alpha = 1.0 - conf;
            
            if (type == TwoSided)
            {
                return t(1.0 - (0.5 * alpha));
            }
            else if (type == Greater)
            {
                return t(1.0 - alpha);
            }
            else
            {
                return -t(1.0 - alpha);
            }
        }
        
        inline P pval(Real x, P cdf, TestType type)
        {
            if (type == TwoSided)
            {
                return 2.0 * (cdf > 0.5 ? 1.0 - cdf : cdf);
            }
            else if (type == Greater)
            {
                return 1.0 - cdf;
            }
            else
            {
                return cdf;
            }
        }
    }
}

#endif