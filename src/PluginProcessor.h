#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

#include "Identity.h"
#include "dsp/EffectHost.h"
#include "presets/PresetStore.h"

namespace dlaw
{
class DaylightProcessor;

/** A generic host-visible parameter slot whose name follows the chosen effect. */
class SlotParameter final : public juce::AudioParameterFloat
{
public:
    SlotParameter (DaylightProcessor& owner, int slotIndex);
    juce::String getName (int maximumStringLength) const override;
    bool isAutomatable() const override;
    juce::String getText (float normalisedValue, int maximumStringLength) const override;

private:
    DaylightProcessor& owner;
    const int slot;
};

class DaylightProcessor final : public juce::AudioProcessor,
                                public juce::ChangeBroadcaster
{
public:
    DaylightProcessor();
    ~DaylightProcessor() override;

    // ---- juce::AudioProcessor -------------------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return kPluginName; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    // ---- Daylight --------------------------------------------------------------------------
    /** Chooses the effect. Only possible once per instance (message thread). */
    bool selectEffect (const std::string& effectId);

    bool hasEffect() const noexcept { return effectHost != nullptr; }
    EffectHost* getEffectHost() const noexcept { return effectHost.get(); }

    /** Session was saved with an effect this version doesn't know (needs a newer Daylight_aw). */
    bool hasUnknownEffect() const noexcept { return unknownEffectId.isNotEmpty(); }
    juce::String getUnknownEffectId() const { return unknownEffectId; }

    SlotParameter& getSlot (int index) noexcept { return *slots[(size_t) index]; }
    juce::AudioParameterFloat& getInputTrim() noexcept  { return *inTrimParam; }
    juce::AudioParameterFloat& getOutputTrim() noexcept { return *outTrimParam; }
    juce::AudioParameterBool& getBypass() noexcept      { return *bypassParam; }

    juce::String getSlotName (int slot) const;

    /** Resets all of the effect's parameters to the effect's own defaults (message thread). */
    void resetToDefaults();

    // ---- presets (message thread) --------------------------------------------------------------
    /** Applies a preset of this effect (parameters by name; missing ones go to the effect default). */
    bool applyPreset (const PresetData&);
    /** Records that the current values were just saved as this preset. */
    void markPresetSaved (const juce::String& name);
    /** Called when the current preset was renamed or deleted on disk. */
    void setPresetName (const juce::String& name) { presetName = name; sendChangeMessage(); }
    juce::String getPresetName() const { return presetName; }
    /** True when a preset is loaded and any value differs from it ("*" in the menu bar). */
    bool isPresetModified() const;

    // ---- A/B (message thread) --------------------------------------------------------------
    int getActiveAB() const noexcept { return activeAB; }
    void switchAB (int which);

    // ---- per-instance GUI state saved with the session ---------------------------------------
    int lastTab = 0;

private:
    template <typename T> void processAudio (juce::AudioBuffer<T>& buffer);
    template <typename T> void processChunk (juce::AudioBuffer<T>& buffer, int start, int length, EffectHost* fx);
    template <typename T> static void applyGain (juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>& g,
                                                 T* l, T* r, int n) noexcept;

    void setSlotNamesFromEffect();
    std::vector<float> captureEffectValues() const;
    void applyEffectValues (const std::vector<float>& values);

    std::array<SlotParameter*, (size_t) kNumSlots> slots {};
    juce::AudioParameterFloat* inTrimParam = nullptr;
    juce::AudioParameterFloat* outTrimParam = nullptr;
    juce::AudioParameterBool* bypassParam = nullptr;

    std::unique_ptr<EffectHost> effectHost;          // owned, created on the message thread, never replaced
    std::atomic<EffectHost*> activeHost { nullptr };  // what the audio thread uses

    juce::String unknownEffectId;
    juce::String unknownStateXml;                     // kept verbatim so saving never loses it

    mutable juce::SpinLock nameLock;
    std::array<juce::String, (size_t) kNumSlots> slotNames;

    juce::String presetName;
    std::vector<float> presetSnapshot;

    std::array<std::vector<float>, 2> abValues;
    int activeAB = 0;

    double currentSampleRate = 44100.0;
    int maxBlock = 512;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inGain { 1.0f }, outGain { 1.0f };
    juce::SmoothedValue<float> bypassMix { 0.0f };

    juce::AudioBuffer<float> dryF, scratchF;
    juce::AudioBuffer<double> dryD, scratchD;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DaylightProcessor)
};
} // namespace dlaw
