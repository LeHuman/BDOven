#include "util.h"
#include <array>

#pragma once

namespace Reflow {

namespace State {
struct State_t {
    const char *name;
};
const State_t RAMP_UP("Ramping up");
const State_t PREHEAT("Pre-heating");
const State_t SOAK("Soaking");
const State_t REFLOW("Reflowing");
const State_t COOL("Cooling");
} // namespace State

struct Alloy_t {
    const char *str;
};

namespace Alloy {
Alloy_t SnPb("Sn/Pb");
Alloy_t Sn63Pb37("Sn63/Pb37");
Alloy_t Sn62Pb36Ag2("Sn62/Pb36/Ag2");
Alloy_t SnAgCu("Sn/Ag/Cu");
Alloy_t Sn965Ag3Cu05("Sn96.5/Ag3/Cu0.5");
} // namespace Alloy

using Timing = std::tuple<State::State_t, int, int>;

struct ReflowProfile {
    const char *name;
    const Alloy_t mix;
    const Timing *timing;
    const int sz;
    std::vector<double> Xs;
    std::vector<double> Ys;
    ReflowProfile(const char *name, const Alloy_t mix, const Timing timing[], const int sz) : name(name), mix(mix), timing(timing), sz(sz) {
        for (int i = 0; i < sz; i++) {
            Xs.push_back(std::get<1>(timing[i]));
            Ys.push_back(std::get<2>(timing[i]));
        }
    }
};

int title(const ReflowProfile *profile, char *buf, int sz) {
    return snprintf(buf, sz, "%s - %s", profile->name, profile->mix.str);
}

const Timing getPoint(const ReflowProfile *profile, int sec) {
    int i = 0;
    for (int x : profile->Xs) {
        if (x >= sec)
            break;
        i++;
    }
    return profile->timing[i];
}

const Timing stateString(const ReflowProfile *profile, float temp, int sec, char *buf, int sz) {
    const Timing ret = getPoint(profile, sec);
    char t_buf[8];
    ftoa(temp, t_buf, 8);
    snprintf(buf, sz, "%s %sF° @ %is", std::get<0>(ret).name, t_buf, sec);
    return ret;
}

const Timing getPeak(const ReflowProfile *profile) {
    int i = 0, ci = 0;
    double max = -1;
    for (double y : profile->Ys) {
        if (y > max) {
            max = y;
            ci = i;
        }
        i++;
    }
    return profile->timing[ci];
}

const Timing getLast(const ReflowProfile *profile) {
    return profile->timing[profile->sz - 1];
}

const Timing CHIPQUIK_Sn965Ag3Cu05_7[] = {
    {State::RAMP_UP, 0, 25},
    {State::RAMP_UP, 90, 150},
    {State::PREHEAT, 180, 175},
    {State::SOAK, 210, 217},
    {State::REFLOW, 240, 249},
    {State::REFLOW, 270, 217},
    {State::COOL, 281, 175},
};

const Timing KESTER_SnAgCu_6[] = {
    {State::RAMP_UP, 0, 25},
    {State::PREHEAT, 90, 150},
    {State::SOAK, 180, 210},
    {State::REFLOW, 210, 245},
    {State::REFLOW, 240, 210},
    {State::COOL, 270, 150},
};

const Timing KESTER_SnPb_6[] = {
    {State::RAMP_UP, 0, 25},
    {State::PREHEAT, 90, 150},
    {State::SOAK, 180, 180},
    {State::REFLOW, 210, 219},
    {State::REFLOW, 240, 180},
    {State::COOL, 270, 150},
};

const std::vector<ReflowProfile> PROFILES = {
    {"CHIPQUIK", Alloy::Sn965Ag3Cu05, CHIPQUIK_Sn965Ag3Cu05_7, 7},
    {"KESTER", Alloy::SnAgCu, KESTER_SnAgCu_6, 6},
    {"KESTER", Alloy::Sn63Pb37, KESTER_SnPb_6, 6},
    {"KESTER", Alloy::Sn62Pb36Ag2, KESTER_SnPb_6, 6},
};

} // namespace Reflow
