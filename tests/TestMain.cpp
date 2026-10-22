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
#include <thread>
#include <atomic>

#include "gui/PluginEditor.h"

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


// ---------------------------------------------------------------------------------------------
void testPresets()
{
    std::cout << "Presets ..." << std::endl;
    juce::TemporaryFile tempDir;
    const auto root = tempDir.getFile();
    root.createDirectory();
    PresetStore store (root);

    check (PresetStore::sanitiseName ("a/b:c*d") == "a b c d", "sanitise: illegal characters");
    check (PresetStore::sanitiseName ("  ..hidden. ") == "hidden", "sanitise: dots and spaces");
    check (PresetStore::sanitiseName ("CON") == "CON_", "sanitise: Windows reserved name");
    check (PresetStore::sanitiseName ("  ").isEmpty(), "sanitise: empty");
    check (PresetStore::sanitiseName (juce::String::repeatedString ("x", 100)).length() == 64, "sanitise: length limit");

    const auto& info = *findEffect ("totape9");
    auto p = makeProcessor();
    p->selectEffect ("totape9");
    auto* fx = p->getEffectHost();

    std::vector<float> values;
    for (int i = 0; i < fx->getNumParams(); ++i)
        values.push_back (std::fmod (0.123f + 0.311f * (float) i, 1.0f));

    check (store.save (info, "Warm Tape", fx->getParamNames(), values), "save preset");
    check (store.save (info, "Preset 10", fx->getParamNames(), values), "save second preset");
    check (store.save (info, "Preset 2", fx->getParamNames(), values), "save third preset");
    check (! store.save (info, "", fx->getParamNames(), values), "refuse empty name");

    auto files = store.list (info);
    check (files.size() == 3, "list presets");
    if (files.size() == 3)
        check (files[0].getFileNameWithoutExtension() == "Preset 2" && files[1].getFileNameWithoutExtension() == "Preset 10",
               "natural sort order");

    PresetData data;
    check (PresetStore::load (store.fileFor (info, "Warm Tape"), data), "load preset");
    check (data.name == "Warm Tape" && data.effectId == "totape9", "preset name and effect");
    check ((int) data.values.size() == fx->getNumParams(), "all values stored");

    // apply: exact values, by name
    check (p->applyPreset (data), "apply preset");
    bool exact = true;
    for (int i = 0; i < fx->getNumParams(); ++i)
        exact = exact && p->getSlot (i).get() == values[(size_t) i];
    check (exact, "preset values restored exactly");
    check (p->getPresetName() == "Warm Tape" && ! p->isPresetModified(), "preset loaded, not modified");

    p->getSlot (0).setValueNotifyingHost (0.9f);
    check (p->isPresetModified(), "edit marks preset modified");

    // session remembers preset name + modified state
    juce::MemoryBlock mb;
    p->getStateInformation (mb);
    auto p2 = makeProcessor();
    p2->setStateInformation (mb.getData(), (int) mb.getSize());
    check (p2->getPresetName() == "Warm Tape" && p2->isPresetModified(), "session keeps preset name and '*'");

    // unknown names ignored, missing names -> defaults
    PresetData partial;
    partial.name = "Partial";
    partial.effectId = "totape9";
    partial.values = { { "Input", 0.25f }, { "NoSuchParam", 0.9f } };
    check (p->applyPreset (partial), "apply partial preset");
    check (p->getSlot (0).get() == 0.25f, "named value applied");
    check (p->getSlot (1).get() == fx->getDefault (1), "missing value -> effect default");

    PresetData other = data;
    other.effectId = "galactic";
    check (! p->applyPreset (other), "refuse preset of another effect");

    // reset clears the preset name
    p->resetToDefaults();
    check (p->getPresetName().isEmpty(), "reset clears preset name");

    // broken / foreign files
    auto broken = root.getChildFile ("broken.dlpreset");
    broken.replaceWithText ("{ not json");
    check (! PresetStore::load (broken, data), "broken file rejected");
    broken.replaceWithText ("{\"hello\": 1}");
    check (! PresetStore::load (broken, data), "foreign json rejected");
    check (! PresetStore::load (root.getChildFile ("missing.dlpreset"), data), "missing file rejected");

    // rename
    juce::File renamed;
    check (! store.rename (store.fileFor (info, "Preset 2"), "Warm Tape", renamed), "rename onto existing refused");
    check (store.rename (store.fileFor (info, "Preset 2"), "Bright", renamed) && renamed.existsAsFile(), "rename");
    check (store.rename (renamed, "BRIGHT", renamed) && renamed.getFileNameWithoutExtension() == "BRIGHT", "rename case only");
    {
        bool onDisk = false;
        for (auto& f : store.list (info))
            onDisk = onDisk || f.getFileNameWithoutExtension() == "BRIGHT";   // exact spelling, as the OS lists it
        check (onDisk, "case-only rename visible on disk");
    }

    check (PresetStore::remove (renamed) && ! renamed.existsAsFile(), "remove preset");
    check (store.list (info).size() == 2, "list after remove");

    root.deleteRecursively();
}

void testEffectDocs()
{
    std::cout << "Effect info texts ..." << std::endl;
    for (auto& e : getAllEffects())
    {
        const auto doc = InfoPanel::docFor (e.id);
        check (doc.length() > 200 && doc.startsWith ("#"), juce::String ("Airwindopedia text for ") + e.id);
    }
}


// ---------------------------------------------------------------------------------------------
// Audio thread processing while the "GUI" changes everything it can. Under ThreadSanitizer
// (CI job) this proves there are no data races between the audio and message threads.
void testThreadedStress()
{
    std::cout << "Threaded stress (audio + GUI threads) ..." << std::endl;
    auto p = makeProcessor (48000.0, 256);
    p->selectEffect ("consolexchannel");

    std::atomic<bool> stop { false };
    std::atomic<int> blocks { 0 };
    bool finite = true;

    std::thread audio ([&]
    {
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer midi;
        juce::Random rnd (1);
        while (! stop.load())
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 256; ++i)
                    buf.setSample (ch, i, rnd.nextFloat() * 0.5f - 0.25f);
            p->processBlock (buf, midi);
            finite = finite && allFinite (buf);
            ++blocks;
        }
    });

    juce::Random rnd (2);
    const auto end = juce::Time::getMillisecondCounter() + 600;
    int round = 0;
    while (juce::Time::getMillisecondCounter() < end)
    {
        const int n = p->getEffectHost()->getNumParams();
        p->getSlot (rnd.nextInt (n)).setValueNotifyingHost (rnd.nextFloat());
        p->getInputTrim().setValueNotifyingHost (rnd.nextFloat());
        p->getOutputTrim().setValueNotifyingHost (rnd.nextFloat());
        if (round % 7 == 0) p->getBypass().setValueNotifyingHost (p->getBypass().get() ? 0.0f : 1.0f);
        if (round % 11 == 0) p->switchAB (round % 2);
        if (round % 13 == 0)
        {
            juce::MemoryBlock mb;
            p->getStateInformation (mb);
            p->setStateInformation (mb.getData(), (int) mb.getSize());
        }
        if (round % 17 == 0) p->resetToDefaults();
        ++round;
        juce::Thread::sleep (1);
    }

    stop = true;
    audio.join();
    check (blocks.load() > 10, "audio thread kept running (" + juce::String (blocks.load()) + " blocks)");
    check (finite, "output stayed finite under concurrent changes");
}


// ---------------------------------------------------------------------------------------------
// Sessions saved by every released version must load forever. Each release adds a fixture
// (tests/fixtures/session_<version>_*.xml); this test loads them all.
void testOldSessions (const juce::File& fixtures)
{
    std::cout << "Sessions from older versions ..." << std::endl;
    auto files = fixtures.findChildFiles (juce::File::findFiles, false, "session_*.xml");
    check (files.size() > 0, "fixtures found in " + fixtures.getFullPathName());

    for (auto& f : files)
    {
        const auto text = f.loadFileAsString();
        auto xml = juce::parseXML (text);
        check (xml != nullptr, "fixture parses: " + f.getFileName());
        if (xml == nullptr)
            continue;

        auto p = makeProcessor();
        p->setStateInformation (text.toRawUTF8(), (int) text.getNumBytesAsUTF8());
        const auto id = xml->getStringAttribute ("effectId");
        check (p->hasEffect() && juce::String (p->getEffectHost()->getInfo().id) == id, "effect restored: " + f.getFileName());

        bool valuesOk = p->hasEffect();
        for (auto* e : xml->getChildWithTagNameIterator ("Param"))
            if (p->hasEffect())
            {
                const int idx = p->getEffectHost()->getParamNames().indexOf (e->getStringAttribute ("name"));
                valuesOk = valuesOk && idx >= 0
                        && std::abs (p->getSlot (idx).get() - (float) e->getDoubleAttribute ("value")) < 1.0e-6f;
            }
        check (valuesOk, "parameter values restored: " + f.getFileName());

        if (auto* trims = xml->getChildByName ("Trims"))
            check (std::abs (p->getInputTrim().get() - (float) trims->getDoubleAttribute ("in")) < 1.0e-4f, "trims restored: " + f.getFileName());
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

        auto shoot = [&] (DaylightProcessor& p, const juce::String& name, int panel = 0)
        {
            std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
            if (auto* d = dynamic_cast<DaylightEditor*> (ed.get()))
            {
                if (panel == 1) d->showSettingsPanel();
                if (panel == 2) d->showInfoPanel();
            }
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

        {
            // presets + settings panel, with a few presets (in a temporary folder)
            juce::TemporaryFile tempDir;
            PresetStore::setDefaultRootForTesting (tempDir.getFile());
            auto p = makeProcessor();
            p->selectEffect ("totape9");
            auto* fx = p->getEffectHost();
            std::vector<float> v;
            for (int i = 0; i < fx->getNumParams(); ++i)
                v.push_back (p->getSlot (i).get());
            PresetStore store (PresetStore::defaultRoot());
            for (auto n : { "Warm Tape", "Bright Bus", "Lo-fi Cassette" })
                store.save (fx->getInfo(), n, fx->getParamNames(), v);
            PresetData d;
            PresetStore::load (store.fileFor (fx->getInfo(), "Warm Tape"), d);
            p->applyPreset (d);
            p->getSlot (0).setValueNotifyingHost (0.8f);
            shoot (*p, "zz_totape9_settings", 1);
            shoot (*p, "zz_totape9_info", 2);

            auto small = makeProcessor();
            small->selectEffect ("deess");
            shoot (*small, "zz_deess_settings", 1);
            shoot (*small, "zz_deess_info", 2);

            tempDir.getFile().deleteRecursively();
            PresetStore::setDefaultRootForTesting ({});
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
    testPresets();
    testEffectDocs();
    testThreadedStress();
    testOldSessions (juce::File (DAYLIGHT_FIXTURES_DIR));
    testAllEffects();

    for (int i = 1; i + 1 < argc; ++i)
        if (juce::String (argv[i]) == "--snapshots")
            renderSnapshots (juce::File::getCurrentWorkingDirectory().getChildFile (argv[i + 1]));

    std::cout << "\n" << checks << " checks, " << failures << " failed" << std::endl;
    return failures == 0 ? 0 : 1;
}
