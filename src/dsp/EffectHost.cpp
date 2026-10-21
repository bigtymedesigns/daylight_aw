#include "EffectHost.h"

#include <mutex>
#include <set>

namespace dlaw
{
namespace
{
juce::String cleanText (const char* raw)
{
    return juce::String (juce::CharPointer_UTF8 (raw)).trim();
}

bool isNumeric (const juce::String& s)
{
    auto t = s.trim();
    if (t.isEmpty())
        return false;
    if (t == "-inf" || t == "inf")
        return true;
    return t.containsOnly ("0123456789.-+eE ");
}
} // namespace

EffectHost::EffectHost (const EffectInfo& infoToUse, double sampleRate)
    : info (infoToUse)
{
    static std::once_flag once;
    std::call_once (once, [] { if (AirwinConsolidatedBase::defaultSampleRate < 2000.0f) AirwinConsolidatedBase::defaultSampleRate = 44100.0f; });

    // The audio instance is created first on purpose: the tests rely on this order.
    audioFx   = info.create();
    displayFx = info.create();

    numParams = info.numParams;
    defaults.resize ((size_t) numParams);
    lastPushed.assign ((size_t) numParams, std::numeric_limits<float>::quiet_NaN());

    for (int i = 0; i < numParams; ++i)
    {
        char buf[kVstMaxParamStrLen + 8] = {};
        displayFx->getParameterName (i, buf);
        paramNames.add (cleanText (buf));
        defaults[(size_t) i] = displayFx->getParameter (i);
    }

    setSampleRate (sampleRate);
}

void EffectHost::setSampleRate (double sr) noexcept
{
    const auto rate = (float) (sr > 2000.0 ? sr : 44100.0);
    audioFx->setSampleRate (rate);
    displayFx->setSampleRate (rate);
}

juce::String EffectHost::getDisplayText (int index, float value)
{
    if (index < 0 || index >= numParams)
        return {};

    displayFx->setParameter (index, value);

    char text[kVstMaxParamStrLen + 8] = {};
    char label[kVstMaxParamStrLen + 8] = {};
    displayFx->getParameterDisplay (index, text);
    displayFx->getParameterLabel (index, label);

    auto t = cleanText (text);
    auto l = cleanText (label);
    return l.isEmpty() ? t : t + " " + l;
}

int EffectHost::detectSteps (int index)
{
    if (index < 0 || index >= numParams)
        return 0;

    const float original = displayFx->getParameter (index);
    std::set<juce::String> distinct;
    bool allText = true;

    for (int k = 0; k <= 400 && allText; ++k)
    {
        displayFx->setParameter (index, (float) k / 400.0f);
        char text[kVstMaxParamStrLen + 8] = {};
        displayFx->getParameterDisplay (index, text);
        auto t = cleanText (text);

        if (isNumeric (t))
            allText = false;
        else
            distinct.insert (t);
    }

    displayFx->setParameter (index, original);

    if (! allText || distinct.size() < 2 || distinct.size() > 32)
        return 0;

    return (int) distinct.size();
}

bool EffectHost::textToValue (int index, const juce::String& text, float& result)
{
    if (index < 0 || index >= numParams)
        return false;

    if (displayFx->canConvertParameterTextToValue (index))
    {
        float v = 0.0f;
        if (displayFx->parameterTextToValue (index, text.toRawUTF8(), v))
        {
            result = juce::jlimit (0.0f, 1.0f, v);
            return true;
        }
    }

    // Fallback: plain number 0..1
    auto t = text.trim();
    if (t.isNotEmpty() && isNumeric (t))
    {
        result = juce::jlimit (0.0f, 1.0f, t.getFloatValue());
        return true;
    }

    return false;
}
} // namespace dlaw
