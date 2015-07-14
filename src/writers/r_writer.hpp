#ifndef GI_R_WRITER_HPP
#define GI_R_WRITER_HPP

#include <string>
#include <vector>
#include <sstream>
#include <numeric>
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

// Defined in main.cpp
extern std::string date();

// Defined in main.cpp
extern std::string __full_command__;

extern std::string LinearR();

namespace Anaquin
{
    struct RWriter
    {
        template <typename T> static std::string write
                    (const std::vector<T> &x,
                     const std::vector<T> &y,
                     const std::vector<SequinID> &z,
                     const std::string &units,
                     T s)
        {
            // The default color is simply black
            std::vector<ColorID> c(z.size(), "black");

            // Apply the default drawing color
            return RWriter::write(x, y, z, c, units, s);
        }

        template <typename T> static std::string write
                    (const std::vector<T> &x,
                     const std::vector<T> &y,
                     const std::vector<SequinID> &z,
                     const std::vector<ColorID>  &c,
                     const std::string units,
                     T s)
        {
            using boost::algorithm::join;
            using boost::adaptors::transformed;

            std::stringstream ss;
            ss << LinearR();

            const auto xs = join(x | transformed(static_cast<std::string(*)(double)>(std::to_string)), ", ");
            const auto ys = join(y | transformed(static_cast<std::string(*)(double)>(std::to_string)), ", ");
            const auto zs = (boost::format("'%1%'") % boost::algorithm::join(z, "','")).str();
            const auto cs = (boost::format("'%1%'") % boost::algorithm::join(c, "','")).str();

            return (boost::format(ss.str()) % date() % __full_command__ % xs % ys % zs % units).str();
        }
    };
}

#endif