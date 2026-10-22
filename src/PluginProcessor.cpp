#include "PluginProcessor.h"
#include "gui/PluginEditor.h"

namespace dlaw
{
namespace
{
juce::String slotId (int i) { return "p" + juce::String (i + 1).paddedLeft ('0', 2); }

constexpr float kBypassFadeSeconds = 0.010f;
constexpr float kTrimSmoothSeconds = 0.020f;
} // namespace

// =============================================================================================
SlotParameter::SlotParameter (DaylightProcessor& o, int slotIndex)
    : juce::AudioParameterFloat (juce::ParameterID { slotId (slotIndex), 1 },
                                 "P" + juce::String (slotIndex + 1),
                                 juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f),
      owner (o), slot (slotIndex)
{
}

juce::String SlotParameter::getName (int maximumStringLength) const
{
    return owner.getSlotName (slot).substring (0, maximumStringLength);
}

bool SlotParameter::isAutomatable() const
{
    if (auto* fx = owner.getEffectHost())
        return slot < fx->getNumParams();
    return true;
}

juce::String SlotParameter::getText (float v, int maximumStringLength) const
{
    return juce::String (v, 3).substring (0, maximumStringLength);
}

// =============================================================================================
DaylightProcessor::DaylightProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    for (int i = 0; i < kNumSlots; ++i)
    {
        slotNames[(size_t) i] = "-";
        auto p = std::make_unique<SlotParameter> (*this, i);
        slots[(size_t) i] = p.get();
        addParameter (p.release());
    }

    auto trimRange = juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f);
    auto dbAttr = juce::AudioParameterFloatAttributes().withLabel ("dB");

    inTrimParam = new juce::AudioParameterFloat (juce::ParameterID { "intrim", 1 }, "Input Trim", trimRange, 0.0f, dbAttr);
    outTrimParam = new juce::AudioParameterFloat (juce::ParameterID { "outtrim", 1 }, "Output Trim", trimRange, 0.0f, dbAttr);
    bypassParam = new juce::AudioParameterBool (juce::ParameterID { "bypass", 1 }, "Bypass", false);
    addParameter (inTrimParam);
    addParameter (outTrimParam);
    addParameter (bypassParam);
}

DaylightProcessor::~DaylightProcessor() = default;

bool DaylightProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void DaylightProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    maxBlock = juce::jmax (32, samplesPerBlock);

    dryF.setSize (2, maxBlock, false, true, true);
    scratchF.setSize (1, maxBlock, false, true, true);
    dryD.setSize (2, maxBlock, false, true, true);
    scratchD.setSize (1, maxBlock, false, true, true);

    inGain.reset (currentSampleRate, kTrimSmoothSeconds);
    outGain.reset (currentSampleRate, kTrimSmoothSeconds);
    inGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (inTrimParam->get()));
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outTrimParam->get()));

    bypassMix.reset (currentSampleRate, kBypassFadeSeconds);
    bypassMix.setCurrentAndTargetValue (bypassParam->get() ? 1.0f : 0.0f);

    if (effectHost != nullptr)
        effectHost->setSampleRate (currentSampleRate);

    setLatencySamples (0);
}

// =============================================================================================
void DaylightProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)  { processAudio (buffer); }
void DaylightProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&) { processAudio (buffer); }

template <typename T>
void DaylightProcessor::processAudio (juce::AudioBuffer<T>& buffer)
{
    const int numSamples = buffer.getNumSamples();

    // Never more output channels than input channels for this effect; clear any extras.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples <= 0 || buffer.getNumChannels() < 1)
        return;

    auto* fx = activeHost.load (std::memory_order_acquire);

    const float inDb = inTrimParam->get();
    const float outDb = outTrimParam->get();
    inGain.setTargetValue (inDb == 0.0f ? 1.0f : juce::Decibels::decibelsToGain (inDb));
    outGain.setTargetValue (outDb == 0.0f ? 1.0f : juce::Decibels::decibelsToGain (outDb));
    bypassMix.setTargetValue (bypassParam->get() ? 1.0f : 0.0f);

    if (fx != nullptr)
        for (int i = 0; i < fx->getNumParams(); ++i)
            fx->setParameter (i, slots[(size_t) i]->get());

    for (int start = 0; start < numSamples; start += maxBlock)
        processChunk (buffer, start, juce::jmin (maxBlock, numSamples - start), fx);
}

template <typename T>
void DaylightProcessor::applyGain (juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>& g,
                                   T* l, T* r, int n) noexcept
{
    if (! g.isSmoothing())
    {
        const float v = g.getCurrentValue();
        if (v == 1.0f)
            return; // exactly 0 dB: audio stays bit-identical

        for (int i = 0; i < n; ++i)
        {
            l[i] *= (T) v;
            if (r != nullptr) r[i] *= (T) v;
        }
        return;
    }

    for (int i = 0; i < n; ++i)
    {
        const auto v = (T) g.getNextValue();
        l[i] *= v;
        if (r != nullptr) r[i] *= v;
    }
}

template <typename T>
void DaylightProcessor::processChunk (juce::AudioBuffer<T>& buffer, int start, int n, EffectHost* fx)
{
    const bool stereo = buffer.getNumChannels() > 1 && getTotalNumInputChannels() > 1;
    T* left = buffer.getWritePointer (0, start);
    T* right = stereo ? buffer.getWritePointer (1, start) : nullptr;

    const bool fullyBypassed = ! bypassMix.isSmoothing() && bypassMix.getCurrentValue() >= 1.0f;

    if (fx == nullptr || fullyBypassed)
    {
        // Untouched pass-through. Keep the smoothers' timing consistent.
        inGain.skip (n);
        outGain.skip (n);
        bypassMix.skip (n);
        return;
    }

    auto& dry = [this]() -> juce::AudioBuffer<T>& {
        if constexpr (std::is_same_v<T, float>) return dryF; else return dryD;
    }();
    auto& scratch = [this]() -> juce::AudioBuffer<T>& {
        if constexpr (std::is_same_v<T, float>) return scratchF; else return scratchD;
    }();

    const bool crossfading = bypassMix.isSmoothing() || bypassMix.getCurrentValue() > 0.0f;

    if (crossfading)
    {
        juce::FloatVectorOperations::copy (dry.getWritePointer (0), left, n);
        if (right != nullptr)
            juce::FloatVectorOperations::copy (dry.getWritePointer (1), right, n);
    }

    applyGain (inGain, left, right, n);

    // Airwindows effects are stereo. On a mono track the same signal feeds both inputs
    // and the left output is used.
    T* r = right;
    if (r == nullptr)
    {
        r = scratch.getWritePointer (0);
        juce::FloatVectorOperations::copy (r, left, n);
    }

    T* ins[2] = { left, r };
    T* outs[2] = { left, r };
    fx->process (ins, outs, n);

    applyGain (outGain, left, right, n);

    if (crossfading)
    {
        const T* d0 = dry.getReadPointer (0);
        const T* d1 = dry.getReadPointer (1);
        for (int i = 0; i < n; ++i)
        {
            const auto m = (T) bypassMix.getNextValue();
            left[i] = left[i] * ((T) 1 - m) + d0[i] * m;
            if (right != nullptr)
                right[i] = right[i] * ((T) 1 - m) + d1[i] * m;
        }
    }
}

// =============================================================================================
juce::String DaylightProcessor::getSlotName (int slot) const
{
    const juce::SpinLock::ScopedLockType sl (nameLock);
    return slotNames[(size_t) slot];
}

void DaylightProcessor::setSlotNamesFromEffect()
{
    {
        const juce::SpinLock::ScopedLockType sl (nameLock);
        for (int i = 0; i < kNumSlots; ++i)
            slotNames[(size_t) i] = (effectHost != nullptr && i < effectHost->getNumParams())
                                        ? effectHost->getParamNames()[i]
                                        : juce::String ("-");
    }
    updateHostDisplay (ChangeDetails().withParameterInfoChanged (true).withProgramChanged (true));
}

const juce::String DaylightProcessor::getProgramName (int)
{
    if (effectHost != nullptr)
        return presetName.isEmpty() ? juce::String (effectHost->getInfo().name)
                                    : juce::String (effectHost->getInfo().name) + " - " + presetName;
    if (hasUnknownEffect())
        return unknownEffectId;
    return "Choose effect";
}

bool DaylightProcessor::selectEffect (const std::string& effectId)
{
    if (effectHost != nullptr || hasUnknownEffect())
        return false; // locked: one effect per instance

    auto* info = findEffect (effectId);
    if (info == nullptr)
        return false;

    auto host = std::make_unique<EffectHost> (*info, currentSampleRate);

    for (int i = 0; i < host->getNumParams(); ++i)
        slots[(size_t) i]->setValueNotifyingHost (host->getDefault (i));

    effectHost = std::move (host);
    abValues = { captureEffectValues(), captureEffectValues() };
    activeAB = 0;
    activeHost.store (effectHost.get(), std::memory_order_release);

    setSlotNamesFromEffect();
    sendChangeMessage();
    return true;
}

std::vector<float> DaylightProcessor::captureEffectValues() const
{
    std::vector<float> v;
    if (effectHost != nullptr)
        for (int i = 0; i < effectHost->getNumParams(); ++i)
            v.push_back (slots[(size_t) i]->get());
    return v;
}

void DaylightProcessor::applyEffectValues (const std::vector<float>& values)
{
    if (effectHost == nullptr)
        return;

    for (int i = 0; i < effectHost->getNumParams() && i < (int) values.size(); ++i)
    {
        auto* p = slots[(size_t) i];
        p->beginChangeGesture();
        p->setValueNotifyingHost (values[(size_t) i]);
        p->endChangeGesture();
    }
}

void DaylightProcessor::resetToDefaults()
{
    if (effectHost == nullptr)
        return;

    std::vector<float> d;
    for (int i = 0; i < effectHost->getNumParams(); ++i)
        d.push_back (effectHost->getDefault (i));
    applyEffectValues (d);
    presetName = {};
    presetSnapshot.clear();
    presetNameChanged();
}

bool DaylightProcessor::applyPreset (const PresetData& preset)
{
    if (effectHost == nullptr || preset.effectId != effectHost->getInfo().id)
        return false;

    const auto& names = effectHost->getParamNames();
    std::vector<float> v;
    for (int i = 0; i < effectHost->getNumParams(); ++i)
        v.push_back (effectHost->getDefault (i));

    for (const auto& [name, value] : preset.values)
    {
        const int index = names.indexOf (name);
        if (index >= 0)
            v[(size_t) index] = value;
    }

    applyEffectValues (v);
    presetName = preset.name;
    presetSnapshot = captureEffectValues();
    presetNameChanged();
    return true;
}

void DaylightProcessor::markPresetSaved (const juce::String& name)
{
    presetName = name;
    presetSnapshot = captureEffectValues();
    presetNameChanged();
}

void DaylightProcessor::presetNameChanged()
{
    // the DAW shows the program name ("ToTape9 - Warm Tape") in its preset field
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
    sendChangeMessage();
}

bool DaylightProcessor::isPresetModified() const
{
    if (presetName.isEmpty())
        return false;

    const auto now = captureEffectValues();
    if (now.size() != presetSnapshot.size())
        return true;

    for (size_t i = 0; i < now.size(); ++i)
        if (std::abs (now[i] - presetSnapshot[i]) > 1.0e-6f)
            return true;

    return false;
}

void DaylightProcessor::switchAB (int which)
{
    if (effectHost == nullptr || which == activeAB || which < 0 || which > 1)
        return;

    abValues[(size_t) activeAB] = captureEffectValues();
    activeAB = which;
    applyEffectValues (abValues[(size_t) activeAB]);
    sendChangeMessage();
}

// =============================================================================================
// State: <DaylightState formatVersion=.. pluginVersion=.. effectId=.. effectVersion=..>
//          <Param name=".." index=".." value=".."/> ...
//          <Trims in=".." out=".."/> <UI tab=".."/> <AB active=".." a=".." b=".."/>
void DaylightProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (hasUnknownEffect())
    {
        // Return exactly what we were given so a newer version can read it again.
        destData.replaceAll (unknownStateXml.toRawUTF8(), unknownStateXml.getNumBytesAsUTF8());
        return;
    }

    juce::XmlElement root ("DaylightState");
    root.setAttribute ("formatVersion", kStateFormatVersion);
    root.setAttribute ("pluginVersion", kVersion);

    if (effectHost != nullptr)
    {
        root.setAttribute ("effectId", effectHost->getInfo().id);
        root.setAttribute ("effectVersion", effectHost->getInfo().version);

        for (int i = 0; i < effectHost->getNumParams(); ++i)
        {
            auto* p = root.createNewChildElement ("Param");
            p->setAttribute ("name", effectHost->getParamNames()[i]);
            p->setAttribute ("index", i);
            p->setAttribute ("value", (double) slots[(size_t) i]->get());
        }

        auto abNow = abValues;
        abNow[(size_t) activeAB] = captureEffectValues();
        auto* ab = root.createNewChildElement ("AB");
        ab->setAttribute ("active", activeAB);
        for (int k = 0; k < 2; ++k)
        {
            juce::StringArray vals;
            for (auto v : abNow[(size_t) k])
                vals.add (juce::String (v, 7));
            ab->setAttribute (k == 0 ? "a" : "b", vals.joinIntoString (","));
        }
    }

    auto* trims = root.createNewChildElement ("Trims");
    trims->setAttribute ("in", (double) inTrimParam->get());
    trims->setAttribute ("out", (double) outTrimParam->get());

    auto* ui = root.createNewChildElement ("UI");
    ui->setAttribute ("tab", lastTab);

    if (presetName.isNotEmpty())
    {
        auto* pr = root.createNewChildElement ("Preset");
        pr->setAttribute ("name", presetName);
        juce::StringArray vals;
        for (auto v : presetSnapshot)
            vals.add (juce::String (v, 7));
        pr->setAttribute ("snapshot", vals.joinIntoString (","));
    }

    const auto text = root.toString (juce::XmlElement::TextFormat().singleLine());
    destData.replaceAll (text.toRawUTF8(), text.getNumBytesAsUTF8());
}

void DaylightProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    const auto text = juce::String::fromUTF8 (static_cast<const char*> (data), sizeInBytes);
    auto xml = juce::parseXML (text);

    if (xml == nullptr || ! xml->hasTagName ("DaylightState"))
        return; // not ours / broken: ignore, never crash

    // Future format versions would be migrated here (formatVersion > kStateFormatVersion = newer).
    const auto effectId = xml->getStringAttribute ("effectId");

    if (effectId.isNotEmpty() && effectHost == nullptr)
    {
        if (findEffect (effectId.toStdString()) == nullptr)
        {
            unknownEffectId = effectId;
            unknownStateXml = text;
            sendChangeMessage();
            return;
        }

        selectEffect (effectId.toStdString());
    }

    if (effectHost != nullptr && effectId == effectHost->getInfo().id)
    {
        const auto& names = effectHost->getParamNames();
        for (auto* p : xml->getChildWithTagNameIterator ("Param"))
        {
            // Match by parameter name first (safe across updates), index as fallback.
            int index = names.indexOf (p->getStringAttribute ("name"));
            if (index < 0)
            {
                const int fallback = p->getIntAttribute ("index", -1);
                if (fallback >= 0 && fallback < effectHost->getNumParams() && names[fallback] == p->getStringAttribute ("name"))
                    index = fallback;
            }
            if (index >= 0)
                slots[(size_t) index]->setValueNotifyingHost ((float) juce::jlimit (0.0, 1.0, p->getDoubleAttribute ("value", 0.5)));
        }

        if (auto* ab = xml->getChildByName ("AB"))
        {
            for (int k = 0; k < 2; ++k)
            {
                juce::StringArray vals;
                vals.addTokens (ab->getStringAttribute (k == 0 ? "a" : "b"), ",", {});
                std::vector<float> v;
                for (auto& s : vals)
                    v.push_back (juce::jlimit (0.0f, 1.0f, s.getFloatValue()));
                if ((int) v.size() == effectHost->getNumParams())
                    abValues[(size_t) k] = v;
            }
            activeAB = juce::jlimit (0, 1, ab->getIntAttribute ("active", 0));
        }
    }

    if (auto* trims = xml->getChildByName ("Trims"))
    {
        inTrimParam->setValueNotifyingHost (inTrimParam->convertTo0to1 ((float) trims->getDoubleAttribute ("in", 0.0)));
        outTrimParam->setValueNotifyingHost (outTrimParam->convertTo0to1 ((float) trims->getDoubleAttribute ("out", 0.0)));
    }

    if (auto* ui = xml->getChildByName ("UI"))
        lastTab = juce::jmax (0, ui->getIntAttribute ("tab", 0));

    presetName = {};
    presetSnapshot.clear();
    if (auto* pr = xml->getChildByName ("Preset"); pr != nullptr && effectHost != nullptr)
    {
        juce::StringArray vals;
        vals.addTokens (pr->getStringAttribute ("snapshot"), ",", {});
        std::vector<float> v;
        for (auto& x : vals)
            v.push_back (juce::jlimit (0.0f, 1.0f, x.getFloatValue()));
        if ((int) v.size() == effectHost->getNumParams())
        {
            presetName = pr->getStringAttribute ("name").substring (0, 64);
            presetSnapshot = std::move (v);
        }
    }

    sendChangeMessage();
}

juce::AudioProcessorEditor* DaylightProcessor::createEditor()
{
    return new DaylightEditor (*this);
}
} // namespace dlaw

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new dlaw::DaylightProcessor();
}
