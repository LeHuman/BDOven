namespace Reflow {

enum STATE {
    RAMP_UP,
    PREHEAT,
    SOAK,
    REFLOW,
    COOL,
};

const char *REFLOW_STATE_STR[] = {
    "Ramping up",
    "Pre-heating",
    "Soaking",
    "Reflowing",
    "Cooling",
};

struct ReflowProfile {
    const char *name;
    const char *alloy_target;
    const int **timing; // STATE enum, seconds, celsius
    const int len;
    ReflowProfile(const char *name, const char *alloy_target, const int timing[][3], const int len) : name(name), alloy_target(alloy_target), timing((const int **)timing), len(len) {
    }
    // const int *getPoint(int sec) {
    // }
};

// http://www.chipquik.com/datasheets/SMD291SNL50T3.pdf
const int REFLOW_PROFILE_CHIPQUIK_TIMING_SNAGCU[][3] = {
    {RAMP_UP, 0, 25},
    {RAMP_UP, 90, 150},
    {PREHEAT, 180, 175},
    {SOAK, 210, 217},
    {REFLOW, 240, 249},
    {REFLOW, 270, 217},
    {COOL, 281, 175},
};
// https://www.kester.com/Portals/0/Documents/Knowledge%20Base/Reflow%20Profile%20for%20SnAgCu%20Alloys.pdf
const int REFLOW_PROFILE_KESTER_TIMING_SNAGCU[][3] = {
    {RAMP_UP, 0, 25},
    {PREHEAT, 90, 150},
    {SOAK, 180, 210},
    {REFLOW, 210, 245},
    {REFLOW, 240, 210},
    {COOL, 270, 150},
};
// https://www.kester.com/Portals/0/Documents/Knowledge%20Base/Standard_Profile.pdf
const int REFLOW_PROFILE_KESTER_TIMING_SNPB[][3] = {
    {RAMP_UP, 0, 25},
    {PREHEAT, 90, 150},
    {SOAK, 180, 180},
    {REFLOW, 210, 219},
    {REFLOW, 240, 180},
    {COOL, 270, 150},
};

static const ReflowProfile REFLOW_PROFILE_CHIPQUIK_SNAGCU("CHIPQUIK", "Sn96.5/Ag3.0/Cu0.5", REFLOW_PROFILE_CHIPQUIK_TIMING_SNAGCU, 7);
static const ReflowProfile REFLOW_PROFILE_KESTER_SNAGCU("KESTER", "Sn/Ag/Cu", REFLOW_PROFILE_KESTER_TIMING_SNAGCU, 6);
static const ReflowProfile REFLOW_PROFILE_KESTER_SNPB_0("KESTER", "n63/Pb37", REFLOW_PROFILE_KESTER_TIMING_SNPB, 6);
static const ReflowProfile REFLOW_PROFILE_KESTER_SNPB_1("KESTER", "Sn62/Pb36/Ag02", REFLOW_PROFILE_KESTER_TIMING_SNPB, 6);

const ReflowProfile REFLOW_PROFILES[] = {
    REFLOW_PROFILE_CHIPQUIK_SNAGCU,
    REFLOW_PROFILE_KESTER_SNAGCU,
    REFLOW_PROFILE_KESTER_SNPB_0,
    REFLOW_PROFILE_KESTER_SNPB_1,
};

} // namespace Reflow
