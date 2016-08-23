#ifndef ANALYZER_HPP
#define ANALYZER_HPP

#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include "data/standard.hpp"
#include "stats/classify.hpp"
#include "writers/r_writer.hpp"
#include "writers/pdf_writer.hpp"
#include "writers/mock_writer.hpp"

// Defined in main.cpp
extern bool __showInfo__;

namespace Anaquin
{
    template <typename T> std::string toString(const T &x, unsigned n = 2)
    {
        std::ostringstream out;
        out << std::fixed << std::setprecision(n) << x;
        return out.str();
    }

    template <typename T> Counts count(const std::map<T, Counts> &x)
    {
        return std::accumulate(std::begin(x), std::end(x), 0, [](Counts c, const std::pair<T, Counts>& p)
        {
            return c + (p.second ? 1 : 0);
        });
    }
    
    template <typename X, typename F> Counts count(const X &x, F f)
    {
        Counts n = 0;
        
        for (const auto &i : x)
        {
            n += f(i.first, i.second);
        }
        
        return n;
    }

    template <typename T1, typename T2> T2 sum(const std::vector<T1> &x)
    {
        return std::accumulate(std::begin(x), std::end(x), T2(), [](T2 c, const T1 &p)
        {
            return c + p;
        });
    }
    
    template <typename T1, typename T2> T2 sum(const std::map<T1, T2> &x)
    {
        return std::accumulate(std::begin(x), std::end(x), T2(), [](T2 c, const std::pair<T1, T2>& p)
        {
            return c + p.second;
        });
    }
    
    struct Analyzer
    {
        // Empty Implementation
    };
    
    typedef std::map<SequinID, Counts> Hist;
    
    struct SequinStats
    {
        // Distribution of counts within sampling regions
        Hist hist;
    };

    struct AnalyzerStats
    {
        std::map<ChrID, Hist> hist;
    };
    
    struct MappingStats
    {
        inline Counts total() const
        {
            return countSyn + countGen + countNA;
        }

        inline Proportion propNA() const
        {
            return total() ? static_cast<Proportion>(countNA) / total() : NAN;
        }
        
        inline Proportion propGen() const
        {
            return total() ? static_cast<Proportion>(countGen) / total() : NAN;
        }

        inline Proportion propSyn() const
        {
            return total() ? static_cast<Proportion>(countSyn) / total() : NAN;
        }

        inline Proportion dilution() const
        {
            return (countSyn + countGen) ? static_cast<Proportion>(countSyn) / (countSyn + countGen) : NAN;
        }

        Counts countNA  = 0;
        Counts countGen = 0;
        Counts countSyn = 0;
    };

    struct AlignmentStats : public MappingStats
    {
        template <typename T, typename F> void update(const T &t, F f)
        {
            if      (!t.mapped) { countNA++;  }
            else if (!f(t))     { countSyn++; }
            else                { countGen++; }
        }

        template <typename T> void update(const T &t)
        {
            return update(t, [&](const T &t)
            {
                return !Standard::isSynthetic(t.cID);
            });
        }
    };

    struct WriterOptions
    {
        enum LogLevel
        {
            Info,
            Warn,
            Error,
        };

        // Working directory
        Path work;

        std::shared_ptr<PDFWriter> report;

        std::shared_ptr<Writer> writer = std::shared_ptr<Writer>(new MockWriter());
        std::shared_ptr<Writer> logger = std::shared_ptr<Writer>(new MockWriter());
        std::shared_ptr<Writer> output = std::shared_ptr<Writer>(new MockWriter());

        inline void warn(const std::string &s) const
        {
            logger->write("[WARN]: " + s);
            output->write("[WARN]: " + s);
        }
        
        inline void wait(const std::string &s) const
        {
            logger->write("[WAIT]: " + s);
            output->write("[WAIT]: " + s);
        }
        
        inline void analyze(const std::string &s) const
        {
            info("Analyzing: " + s);
        }
        
        inline void generate(const FileName &f) const
        {
            info("Generating: " + f);
        }
        
        inline void info(const std::string &s) const
        {
            logInfo(s);
            
            if (__showInfo__)
            {
                output->write("[INFO]: " + s);
            }
        }
        
        inline void logInfo(const std::string &s) const
        {
            logger->write("[INFO]: " + s);
        }
        
        inline void logWarn(const std::string &s) const
        {
            logger->write("[WARN]: " + s);
        }
        
        inline void error(const std::string &s) const
        {
            logger->write("[ERROR]: " + s);
            output->write("[ERROR]: " + s);
        }
        
        // Write to the standard terminal
        inline void out(const std::string &s) const { output->write(s); }
    };

    struct AnalyzerOptions : public WriterOptions
    {
        // Empty Implementation
    };

    struct IndexOptions : public AnalyzerOptions
    {
        FileName index;
    };

    struct SingleMixtureOption : public AnalyzerOptions
    {
        Mixture mix = Mix_1;
    };

    struct FuzzyOptions : public AnalyzerOptions
    {
        double fuzzy;
    };

    struct ViewerOptions : public AnalyzerOptions
    {
        Path path;
    };
    
    struct ReportOptions : public AnalyzerOptions
    {
        FileName mix;
        FileName index;
    };

    struct DoubleMixtureOptions : public AnalyzerOptions
    {
        Mixture mix_1 = Mix_1;
        Mixture mix_2 = Mix_2;
    };
}

#endif
