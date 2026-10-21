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
    bool isDither;           // output trim should stay at 0 dB
    bool hasSharedState;     // original code keeps tiny state shared by all instances (detected by the generator)
    int numParams;
    std::function<std::unique_ptr<AirwinConsolidatedBase>()> create;
};

/** Generated list (src/generated/EffectList.gen.cpp). */
const std::vector<EffectInfo>& getAllEffects();

/** Returns nullptr when the id is unknown (e.g. a session from a newer version). */
const EffectInfo* findEffect (const std::string& id);
} // namespace dlaw
