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
    bool isDither;           // output trim should stay at 0 dB
    bool hasSharedState;     // original code keeps tiny state shared by all instances (detected by the generator)
    int numParams;
    std::function<std::unique_ptr<AirwinConsolidatedBase>()> create;
};

/** Generated list (src/generated/EffectList.gen.cpp). */
const std::vector<EffectInfo>& getAllEffects();

/** Returns nullptr when the id is unknown (e.g. a session from a newer version). */
const EffectInfo* findEffect (const std::string& id);

/** True for effects added in the newest batch (the highest 'since' version): the "New" category. */
bool isNewEffect (const EffectInfo&);

/** Compares "1.2.3" style versions: negative, zero or positive like strcmp. */
int compareVersions (const std::string& a, const std::string& b);
} // namespace dlaw
