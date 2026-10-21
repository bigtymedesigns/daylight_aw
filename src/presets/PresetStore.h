#pragma once

#include <juce_core/juce_core.h>
#include <utility>
#include <vector>

#include "../dsp/EffectRegistry.h"

namespace dlaw
{
/** One user preset, as read from or written to a .dlpreset file. */
struct PresetData
{
    juce::String name;          // from the file name
    juce::String effectId;
    int effectVersion = 0;
    std::vector<std::pair<juce::String, float>> values;   // parameter name -> 0..1
};

/**
    User presets on disk, one small readable JSON file per preset:

        Documents/big_tyme_designs/Daylight_aw/Presets/<Effect name>/<Preset name>.dlpreset

    Values are stored by parameter name, so presets survive parameter reordering.
    Trims, bypass and A/B are not part of a preset. Writes are atomic (temp file + rename).
    No GUI code here, so everything is testable.
*/
class PresetStore
{
public:
    static constexpr const char* extension = ".dlpreset";
    static constexpr int formatVersion = 1;

    explicit PresetStore (juce::File rootFolder);

    /** Documents/big_tyme_designs/Daylight_aw/Presets */
    static juce::File defaultRoot();
    /** Tests only: redirect defaultRoot() to a temporary folder (empty file = normal location). */
    static void setDefaultRootForTesting (const juce::File&);

    juce::File getRoot() const { return root; }
    juce::File folderFor (const EffectInfo&) const;
    juce::File fileFor (const EffectInfo&, const juce::String& presetName) const;

    /** Preset files for this effect, sorted by name (natural order, case-insensitive). */
    juce::Array<juce::File> list (const EffectInfo&) const;

    /** Makes a name safe for a file name on Windows and macOS. Empty if nothing usable is left. */
    static juce::String sanitiseName (const juce::String& raw);

    /** Writes (or replaces) a preset. Returns false on any failure (nothing half-written). */
    bool save (const EffectInfo&, const juce::String& presetName,
               const juce::StringArray& paramNames, const std::vector<float>& values) const;

    /** Reads a preset. Returns false for missing, broken or foreign files (never throws). */
    static bool load (const juce::File&, PresetData& result);

    /** Renames a preset file. Fails if the new name is empty or already taken. */
    bool rename (const juce::File& presetFile, const juce::String& newName, juce::File& renamed) const;

    /** Moves a preset to the system trash (so it can be recovered), deleting it if that's not possible. */
    static bool remove (const juce::File& presetFile);

private:
    juce::File root;
};
} // namespace dlaw
