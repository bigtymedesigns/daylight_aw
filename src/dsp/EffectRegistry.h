#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "airwin_consolidated_base.h"

namespace dlaw
{
/** One Airwindows effect that is compiled into the plugin (generated from manifest/effects.json). */
struct EffectInfo
{
    const char* id;          // permanent Daylight ID, never changes
    const char* name;        // Airwindows name shown to the user
    const char* category;
    const char* description;
    int version;             // bumped when the pinned upstream code changes the sound
    const char* since;       // Daylight_aw version that first included this effect ("0.1.0")
    const char* released;    // date Airwindows first published it ("2026-03-14")
    const char* family;      // versions of one effect share a family ("ToTape" for ToTape5 ... ToTape9)
    double tailSeconds;      // reverb/delay tail reported to the host, so it is never cut off
    bool isDither;           // output trim should stay at 0 dB
    bool hasSharedState;     // original code keeps tiny state shared by all instances (detected by the generator)
    bool isNew;              // added to Daylight_aw in this update ("New" folder; hidden when empty)
    bool isRecent;           // released by Airwindows in the 6 months before the newest effect ("Last 6 months")
    bool isLatest;           // newest version of its family (shown in the "Latest versions only" view)
    int numParams;
    std::function<std::unique_ptr<AirwinConsolidatedBase>()> create;
};

/** Generated list (src/generated/EffectList.gen.cpp). */
const std::vector<EffectInfo>& getAllEffects();

/** Returns nullptr when the id is unknown (e.g. a session from a newer version). */
const EffectInfo* findEffect (const std::string& id);

/** Compares "1.2.3" style versions: negative, zero or positive like strcmp. */
int compareVersions (const std::string& a, const std::string& b);
} // namespace dlaw
