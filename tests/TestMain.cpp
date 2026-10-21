// Daylight_aw automatic tests.
//
//   DaylightTests                    run all tests (exit code 0 = everything passed)
//   DaylightTests --snapshots <dir>  also render every effect's GUI in both themes to PNG files
//
// The most important test is the null test: every effect run through Daylight_aw must give
// bit-identical output to the original Airwindows code run directly.

#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdlib>
#include <iostream>

#include "PluginProcessor.h"
#include "gui/Layout.h"
#include "gui/Settings.h"
#include "BinaryData.h"

using namespace dlaw;

namespace
{
int failures = 0, checks = 0;

void check (bool ok, const juce::String& what)
{
    ++checks;
    if (! ok)
    {
        ++failures;
        std::cout << "  FAIL: " << what << std::endl;
    }
}

template <typename T>
void fillNoise (juce::AudioBuffer<T>& b, int seed)
{
    juce::Random r (seed);
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample (ch, i, (T) ((r.nextDouble() * 2.0 - 1.0) * 0.5));
}

std::unique_ptr<DaylightProcessor> makeProcessor (double sr = 48000.0, int block = 512, bool mono = false)
{
    auto p = std::make_unique<DaylightProcessor>();
    if (mono)
    {
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (juce::AudioChannelSet::mono());
        l.outputBuses.add (juce::AudioChannelSet::mono());
        p->setBusesLayout (l);
    }
    p->setRateAndBufferSizeDetails (sr, block);
    p->prepareToPlay (sr, block);
    return p;
}

template <typename T>
void runProcessor (DaylightProcessor& p, juce::AudioBuffer<T>& buffer, int block)
{
    juce::MidiBuffer midi;
    for (int start = 0; start < buffer.getNumSamples(); start += block)
    {
        const int n = juce::jmin (block, buffer.getNumSamples() - start);
        juce::AudioBuffer<T> view (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, n);
        p.processBlock (view, midi);
    }
}

template <typename T>
double maxDiff (const juce::AudioBuffer<T>& a, const juce::AudioBuffer<T>& b, int channels)
{
    double m = 0.0;
    for (int ch = 0; ch < channels; ++ch)
        for (int i = 0; i < a.getNumSamples(); ++i)
            m = std::max (m, std::abs ((double) a.getSample (ch, i) - (double) b.getSample (ch, i)));
    return m;
}

template <typename T>
bool allFinite (const juce::AudioBuffer<T>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite ((double) b.getSample (ch, i)))
                return false;
    return true;
}

// ---------------------------------------------------------------------------------------------
template <typename T>
void nullTest (const EffectInfo& info, int block)
{
    constexpr double sr = 48000.0;
    constexpr int total = 48000 / 2;

    std::srand (1234);
    auto p = makeProcessor (sr, block);
    check (p->selectEffect (info.id), juce::String ("select ") + info.id);

    std::srand (1234);
    auto ref = info.create();
    ref->setSampleRate ((float) sr);

    // non-default settings on both
    for (int i = 0; i < info.numParams; ++i)
    {
        const float v = std::fmod (0.37f + 0.29f * (float) i, 1.0f);
        p->getSlot (i).setValueNotifyingHost (v);
        ref->setParameter (i, v);
    }

    juce::AudioBuffer<T> input (2, total), viaPlugin (2, total), direct (2, total);
    fillNoise (input, 99);
    viaPlugin.makeCopyOf (input);
    direct.makeCopyOf (input);

    std::srand (777);
    runProcessor (*p, viaPlugin, block);

    std::srand (777);
    for (int start = 0; start < total; start += block)
    {
        const int n = juce::jmin (block, total - start);
        T* io[2] = { direct.getWritePointer (0, start), direct.getWritePointer (1, start) };
        if constexpr (std::is_same_v<T, float>) ref->processReplacing (io, io, n);
        else                                    ref->processDoubleReplacing (io, io, n);
    }

    const double d = maxDiff (viaPlugin, direct, 2);
    if (d != 0.0 && std::getenv ("DLAW_DEBUG"))
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < total; ++i)
                if (viaPlugin.getSample (ch, i) != direct.getSample (ch, i))
                {
                    std::printf ("  first diff ch%d i=%d plugin=%.17g direct=%.17g in=%.17g\n", ch, i,
                                 (double) viaPlugin.getSample (ch, i), (double) direct.getSample (ch, i), (double) input.getSample (ch, i));
                    break;
                }
    // Effects with shared function-level state (see EffectInfo::hasSharedState) can't be compared
    // sample-exact between two instances in one process; the original code differs there by a
    // sub-audible noise floor only. Everything else must be bit-identical.
    const double allowed = info.hasSharedState ? 1.0e-9 : 0.0;
    check (d <= allowed, juce::String ("null test ") + info.id + (std::is_same_v<T, float> ? " float" : " double")
                         + " block " + juce::String (block) + ": max difference " + juce::String (d, 12));
    check (allFinite (viaPlugin), juce::String ("finite output ") + info.id);
}

void testAllEffects()
{
    std::cout << "Null tests (bit-identical to original Airwindows) ..." << std::endl;
    for (auto& e : getAllEffects())
    {
        nullTest<float> (e, 512);
        nullTest<double> (e, 333);   // odd block size on purpose
    }
}

void testSelectionAndNames()
{
    std::cout << "Selection lock and parameter names ..." << std::endl;
    auto p = makeProcessor();
    check (! p->hasEffect(), "starts empty");
    check (p->getSlot (0).getName (64) == "-", "unused slot name before selection");
    check (p->selectEffect ("totape9"), "select totape9");
    check (p->getSlot (0).getName (64) == "Input", "slot 1 named after the effect");
    check (p->getSlot (9).getName (64) == "-", "unused slot named -");
    check (! p->getSlot (40).isAutomatable(), "unused slot not automatable");
    check (! p->selectEffect ("galactic"), "second selection refused (locked)");
    check (p->getProgramName (0) == "ToTape9", "program name = effect name");
    check (! p->selectEffect ("doesnotexist"), "unknown id refused");
}

void testPassThroughAndBypass()
{
    std::cout << "Pass-through, bypass, trims, mono ..." << std::endl;
    {
        auto p = makeProcessor();
        juce::AudioBuffer<float> in (2, 4096), out (2, 4096);
        fillNoise (in, 5);
        out.makeCopyOf (in);
        runProcessor (*p, out, 512);
        check (maxDiff (in, out, 2) == 0.0, "no effect chosen = untouched pass-through");
    }
    {
        auto p = makeProcessor();
        p->selectEffect ("density2");
        p->getBypass().setValueNotifyingHost (1.0f);
        juce::AudioBuffer<float> warm (2, 4096);
        fillNoise (warm, 1);
        runProcessor (*p, warm, 512); // crossfade finishes here

        juce::AudioBuffer<float> in (2, 4096), out (2, 4096);
        fillNoise (in, 6);
        out.makeCopyOf (in);
        runProcessor (*p, out, 512);
        check (maxDiff (in, out, 2) == 0.0, "bypassed = untouched");
    }
    {
        auto p = makeProcessor();
        p->selectEffect ("purestgain");
        p->getOutputTrim().setValueNotifyingHost (p->getOutputTrim().convertTo0to1 (6.0f));
        juce::AudioBuffer<float> in (2, 9600), out (2, 9600);
        fillNoise (in, 7);
        out.makeCopyOf (in);
        runProcessor (*p, out, 512);
        // after smoothing, +6 dB = x1.995
        const float ratio = out.getSample (0, 9000) / in.getSample (0, 9000);
        check (std::abs (ratio - juce::Decibels::decibelsToGain (6.0f)) < 0.01f, "output trim +6 dB (ratio " + juce::String (ratio) + ")");
    }
    {
        // mono track: left output equals the stereo result for identical L/R input
        std::srand (55);
        auto mono = makeProcessor (48000.0, 512, true);
        mono->selectEffect ("totape9");
        std::srand (55);
        auto stereo = makeProcessor();
        stereo->selectEffect ("totape9");

        juce::AudioBuffer<float> m (1, 4800), s (2, 4800);
        fillNoise (m, 8);
        s.copyFrom (0, 0, m, 0, 0, 4800);
        s.copyFrom (1, 0, m, 0, 0, 4800);
        std::srand (9); runProcessor (*mono, m, 512);
        std::srand (9); runProcessor (*stereo, s, 512);
        check (maxDiff (m, s, 1) == 0.0, "mono = left channel of stereo with identical input");
        check (mono->getTotalNumInputChannels() == 1, "mono layout accepted");
    }
}

void testState()
{
    std::cout << "Session state ..." << std::endl;
    juce::MemoryBlock saved;
    {
        auto p = makeProcessor();
        p->selectEffect ("totape9");
        p->getSlot (2).setValueNotifyingHost (0.123f);
        p->getInputTrim().setValueNotifyingHost (p->getInputTrim().convertTo0to1 (3.0f));
        p->switchAB (1);
        p->getSlot (2).setValueNotifyingHost (0.9f);
        p->lastTab = 0;
        p->getStateInformation (saved);
    }
    {
        auto p = makeProcessor();
        p->setStateInformation (saved.getData(), (int) saved.getSize());
        check (p->hasEffect() && juce::String (p->getEffectHost()->getInfo().id) == "totape9", "effect restored");
        check (std::abs (p->getSlot (2).get() - 0.9f) < 1e-6f, "parameter restored");
        check (std::abs (p->getInputTrim().get() - 3.0f) < 0.051f, "trim restored");
        check (p->getActiveAB() == 1, "A/B slot restored");
        p->switchAB (0);
        check (std::abs (p->getSlot (2).get() - 0.123f) < 1e-5f, "A memory restored");
    }
    {
        // unknown effect from a newer version: passes audio through and keeps the state verbatim
        const juce::String future = "<DaylightState formatVersion=\"7\" effectId=\"futurefx\"><Param name=\"X\" index=\"0\" value=\"0.3\"/></DaylightState>";
        auto p = makeProcessor();
        p->setStateInformation (future.toRawUTF8(), (int) future.getNumBytesAsUTF8());
        check (p->hasUnknownEffect(), "unknown effect detected");
        juce::MemoryBlock again;
        p->getStateInformation (again);
        check (again.toString() == future, "unknown state kept verbatim");
        check (! p->selectEffect ("totape9"), "cannot replace an unknown effect");

        juce::AudioBuffer<float> in (2, 1024), out (2, 1024);
        fillNoise (in, 3);
        out.makeCopyOf (in);
        runProcessor (*p, out, 256);
        check (maxDiff (in, out, 2) == 0.0, "unknown effect = pass-through");
    }
    {
        auto p = makeProcessor();
        const char junk[] = "\x01\x02<<not xml";
        p->setStateInformation (junk, (int) sizeof (junk));
        check (! p->hasEffect() && ! p->hasUnknownEffect(), "broken state ignored safely");
    }
}

void testLayouts()
{
    std::cout << "Layouts ..." << std::endl;
    int small = 0, large = 0, tabbed = 0;
    for (auto& e : getAllEffects())
    {
        EffectHost host (e, 48000.0);
        std::vector<ParamFacts> facts;
        for (int i = 0; i < host.getNumParams(); ++i)
            facts.push_back ({ host.getParamNames()[i], host.detectSteps (i) });

        int size = 0;
        const char* data = BinaryData::getNamedResource ((juce::String (e.id) + "_json").toRawUTF8(), size);
        const auto json = data != nullptr ? juce::String::fromUTF8 (data, size) : juce::String();

        auto layout = buildLayout (facts, json);
        check (layout.warnings.isEmpty(), juce::String ("layout without warnings: ") + e.id + " " + layout.warnings.joinIntoString ("; "));
        check (layout.numControls() == host.getNumParams(), juce::String ("every parameter on screen: ") + e.id);

        for (auto& page : layout.pages)
        {
            check ((int) page.rows.size() <= kRowsPerPage, juce::String ("max 3 rows per page: ") + e.id);
            for (auto& row : page.rows)
            {
                int n = 0;
                for (auto& g : row.groups) n += (int) g.controls.size();
                check (n <= kControlsPerRow, juce::String ("max 5 per row: ") + e.id);
            }
        }

        if (layout.pages.size() > 1) ++tabbed;
        if (layout.small) ++small; else ++large;
    }
    std::cout << "  small " << small << ", large " << large << ", with tabs " << tabbed << std::endl;

    // Stepped / switch detection on known effects
    for (auto [id, index, steps] : { std::tuple { "pocketverbs", 0, 6 }, std::tuple { "adclip8", 2, 8 },
                                     std::tuple { "monitoring3", 0, 17 }, std::tuple { "notjustanotherdither", 0, 2 },
                                     std::tuple { "totape9", 0, 0 } })
    {
        EffectHost host (*findEffect (id), 48000.0);
        check (host.detectSteps (index) == steps, juce::String ("steps detected for ") + id + ": " + juce::String (host.detectSteps (index)));
    }
}

void renderSnapshots (const juce::File& dir)
{
    std::cout << "Rendering GUI snapshots to " << dir.getFullPathName() << " ..." << std::endl;
    dir.createDirectory();
    juce::SharedResourcePointer<GlobalSettings> settings;
    const auto originalTheme = settings->getTheme();

    for (auto theme : { ThemeId::light, ThemeId::dark })
    {
        settings->setTheme (theme);

        auto shoot = [&] (DaylightProcessor& p, const juce::String& name)
        {
            std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            juce::PNGImageFormat png;
            auto f = dir.getChildFile (name + "_" + toString (theme) + ".png");
            f.deleteFile();
            juce::FileOutputStream os (f);
            png.writeImageToStream (img, os);
        };

        {
            auto p = makeProcessor();
            shoot (*p, "00_startup");
        }
        for (auto& e : getAllEffects())
        {
            auto p = makeProcessor();
            p->selectEffect (e.id);
            shoot (*p, e.id);
        }
    }
    settings->setTheme (originalTheme);
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    testSelectionAndNames();
    testPassThroughAndBypass();
    testState();
    testLayouts();
    testAllEffects();

    for (int i = 1; i + 1 < argc; ++i)
        if (juce::String (argv[i]) == "--snapshots")
            renderSnapshots (juce::File::getCurrentWorkingDirectory().getChildFile (argv[i + 1]));

    std::cout << "\n" << checks << " checks, " << failures << " failed" << std::endl;
    return failures == 0 ? 0 : 1;
}
