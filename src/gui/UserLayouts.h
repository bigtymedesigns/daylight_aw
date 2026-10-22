#pragma once

#include <juce_core/juce_core.h>
#include "Layout.h"

namespace dlaw
{
class EffectHost;

/**
    Custom layouts made by the user, without rebuilding the plugin:

        Documents/big_tyme_designs/Daylight_aw/Layouts/<effect id>.json

    Same format as the built-in layouts/<id>.json (see docs/LAYOUTS.md, also copied into that folder).
    Priority: user file > built-in file > automatic layout. A broken or incomplete file can never
    hide a parameter: problems are repaired and listed, missing parameters are appended.
*/
namespace userlayouts
{
juce::File folder();
/** Tests only: use another folder (empty file = normal location). */
void setFolderForTesting (const juce::File&);
juce::File fileFor (const juce::String& effectId);

/** Built-in layout JSON for an effect (empty if it has none). */
juce::String builtInJson (const juce::String& effectId);

/** The JSON the editor should use, and whether it came from the user's file. */
juce::String effectiveJson (const juce::String& effectId, bool& isUser);

/** Writes the user's file as a copy of the layout currently shown (no-op if it already exists). */
bool create (EffectHost& fx);

/** Moves the user's file to the trash (back to the built-in layout). */
bool remove (const juce::String& effectId);

/** Layout -> JSON in the layout-file format (used to start a user's file from what is on screen). */
juce::String toJson (const EffectLayout& layout, const std::vector<ParamFacts>& params);

/** What the editor last loaded for an effect (shown in Settings). Message thread only. */
struct Status { bool custom = false; juce::StringArray warnings; };
Status& status (const juce::String& effectId);
} // namespace userlayouts
} // namespace dlaw
