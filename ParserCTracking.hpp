#ifndef AS_PARSER_C_TRACKING_HPP
#define AS_PARSER_C_TRACKING_HPP

#include "Types.hpp"
#include <functional>

enum CTrackingStatus
{
    OK,
    HIData
};

typedef std::string TrackID;

struct CTracking
{
    TrackID id;

    FPKM fpkm;
    FPKM lFPKM;
    FPKM uFPKM;

    CTrackingStatus status;
};

struct ParserCTracking
{
    static bool parse(const std::string &file, std::function<void (const CTracking &)>);
};

#endif