#include <catch.hpp>
#include "parsers/parser_kallisto.hpp"

using namespace Anaquin;

TEST_CASE("ParseKallisto_Test")
{
    std::vector<ParseKallisto::Data> x;
    
    ParseKallisto::parse(Reader("tests/data/abundance.tsv"), [&](const ParseKallisto::Data &d, const ParserProgress &)
    {
        x.push_back(d);
    });

    REQUIRE(x.size() == 164);

    REQUIRE(x[0].id    == "R1_101_1");
    REQUIRE(x[0].abund == 517.276);
    REQUIRE(x[1].id    == "R1_101_2");
    REQUIRE(x[1].abund == 109.724);
    REQUIRE(x[2].id    == "R1_102_1");
    REQUIRE(x[2].abund == 123.318);
}