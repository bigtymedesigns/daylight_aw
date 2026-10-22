#pragma once

#include <juce_core/juce_core.h>
#include <limits>
#include <memory>
#include <vector>

#include "EffectRegistry.h"

namespace dlaw
{
/**
    Wraps one chosen Airwindows effect.

    Two instances of the effect are created:
      - the AUDIO instance is only ever touched by the audio thread (after construction),
      - the DISPLAY instance is only used on the message thread to turn values into text.
    This keeps the original DSP code completely untouched and free of data races.
*/
class EffectHost
{
public:
    EffectHost (const EffectInfo& info, double sampleRate);

    const EffectInfo& getInfo() const noexcept { return info; }
    int getNumParams() const noexcept { return numParams; }
    const juce::StringArray& getParamNames() const noexcept { return paramNames; }
    float getDefault (int index) const { return defaults[(size_t) index]; }

    // ---- audio thread (or while audio is stopped) ----------------------------------------
    void setSampleRate (double sr) noexcept;

    /** Pushes a parameter value into the audio instance if it changed. */
    void setParameter (int index, float value) noexcept
    {
        if (! juce::exactlyEqual (lastPushed[(size_t) index], value))   // NaN at start: always pushes
        {
            lastPushed[(size_t) index] = value;
            audioFx->setParameter (index, value);
        }
    }

    void process (float** in, float** out, int numSamples) noexcept   { audioFx->processReplacing (in, out, numSamples); }
    void process (double** in, double** out, int numSamples) noexcept { audioFx->processDoubleReplacing (in, out, numSamples); }

    // ---- message thread only ---------------------------------------------------------------
    /** Text for a value exactly as the original effect shows it (e.g. "0.5000", "Spring"), plus its label. */
    juce::String getDisplayText (int index, float value);

    /** Number of distinct choices if the effect shows named choices for this parameter, else 0. */
    int detectSteps (int index);

    /** Converts typed text to a 0..1 value using the effect's own conversion when it has one. */
    bool textToValue (int index, const juce::String& text, float& result);

private:
    int probeSteps (int index);
    std::vector<int> stepCache;   // message thread only; -1 = not probed yet

    const EffectInfo& info;
    std::unique_ptr<AirwinConsolidatedBase> audioFx, displayFx;
    int numParams = 0;
    juce::StringArray paramNames;
    std::vector<float> defaults, lastPushed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectHost)
};
} // namespace dlaw
