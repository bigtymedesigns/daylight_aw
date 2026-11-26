// Daylight_aw automatic tests.
//
//   DaylightTests                    run all tests (exit code 0 = everything passed)
//   DaylightTests --snapshots <dir>  also render every effect's GUI (Default theme) and the panels to PNG files
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
    check (! p->selectEffect ("galactic3"), "second selection refused (locked)");
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
        p->selectEffect ("tube2");
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
    other.effectId = "galactic3";
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

void testThemes()
{
    std::cout << "Themes ..." << std::endl;
    const auto d = Theme::builtInDefault();
    check (d.warnings.isEmpty() && d.name == "Default" && d.knobStyle == "ring", "Default theme loads without warnings, ring knobs");
    check (d.background == juce::Colour (0xff07080b) && d.knobPointer == juce::Colour (0xffc657ec)
           && d.groupTitle == juce::Colour (0xff2fc4a5) && d.halo1.opacity > 0.0f && d.halo2.opacity > 0.0f,
           "Default theme = the deep-space look (background, violet pointer, teal titles, two halos)");
    check (d.minimumContrast() >= 4.5f, "Default theme text contrast >= 4.5:1");

    for (auto old : { "light", "dark", "original", "", "nonsense", "default" })
        check (themeIdFromString (old) == ThemeId::def, juce::String ("old/unknown theme setting -> Default: ") + old);
    check (themeIdFromString ("user") == ThemeId::user && themeIdFromString (toString (ThemeId::user)) == ThemeId::user, "User theme id round trip");

    auto broken = Theme::fromJson ("{ broken", d);
    check (! broken.warnings.isEmpty() && broken.background == d.background, "broken theme.json -> Default values + warning");
    auto partly = Theme::fromJson (R"({"tokens":{"background.color":"#102030","knob.pointer":"red","knob.style":"wobbly","halo1.opacity":5}})", d);
    check (partly.background == juce::Colour (0xff102030) && partly.knobPointer == d.knobPointer && partly.knobStyle == "ring"
           && juce::exactlyEqual (partly.halo1.opacity, 1.0f) && partly.warnings.size() == 2,
           "theme tokens: valid ones used, invalid ones fall back with a warning, numbers clamped");

    // User theme folder: a 1.0 theme is moved aside, never deleted; a 1.1 theme is left alone.
    auto folder = getUserThemeFolder();
    folder.deleteRecursively();
    folder.createDirectory();
    folder.getChildFile ("theme.json").replaceWithText (R"({"formatVersion": 1, "name": "User", "tokens": {"paper.color": "#E2DFD7"}})");
    folder.getChildFile ("paper.png").replaceWithText ("png");
    check (ensureUserThemeExists(), "user theme: migrate 1.0 theme");
    const auto old = folder.getChildFile ("old_1.0");
    check (old.getChildFile ("theme.json").loadFileAsString().contains ("paper.color") && old.getChildFile ("paper.png").existsAsFile(),
           "user theme: the 1.0 files are kept in old_1.0");
    const auto fresh = juce::JSON::parse (folder.getChildFile ("theme.json").loadFileAsString());
    check ((int) fresh.getProperty ("formatVersion", 0) == 2 && fresh.getProperty ("name", {}).toString() == "User"
           && folder.getChildFile (kThemeGuideFileName).existsAsFile(), "user theme: new copy of Default + guide");
    auto user = Theme::fromFolder (folder, d);
    check (user.warnings.isEmpty() && user.background == d.background, "user theme copy loads cleanly and looks like Default");

    folder.getChildFile ("theme.json").replaceWithText (R"({"formatVersion": 2, "tokens": {"background.color": "#000000"}})");
    check (ensureUserThemeExists() && ! folder.getChildFile ("old_1.0 (2)").exists()
           && folder.getChildFile ("theme.json").loadFileAsString().contains ("#000000"), "user theme: a 1.1 theme is never replaced");
    folder.getChildFile ("theme.json").replaceWithText ("{ broken");
    check (ensureUserThemeExists() && folder.getChildFile ("theme.json").loadFileAsString() == "{ broken",
           "user theme: a broken file is left for the user to fix");
    folder.deleteRecursively();
}

void testBrowserData()
{
    std::cout << "Effect browser: New, Last 6 months, families ..." << std::endl;
    check (compareVersions ("0.1.0", "0.1.0") == 0 && compareVersions ("0.2.0", "0.10.0") < 0
           && compareVersions ("1.0.0", "0.9.9") > 0, "version compare");
    int newCount = 0, recent = 0, latest = 0;
    juce::String newest;
    for (auto& e : getAllEffects())
    {
        check (compareVersions (e.since, kVersion) <= 0, juce::String ("'since' not newer than this build: ") + e.id);
        if (e.isNew) ++newCount;
        if (e.isRecent) ++recent;
        if (e.isLatest) ++latest;
        if (juce::String (e.released) > newest) newest = e.released;
    }
    check (newCount == 2 && findEffect ("punchydeluxe2")->isNew && findEffect ("pointydeluxe2")->isNew,
           "New = the 2 effects added in 1.1.0 (" + juce::String (newCount) + ")");
    check (recent == 24, "Last 6 months: 24 effects (" + juce::String (recent) + ")");
    check (newest == "2026-11-14", "newest release date in the manifest");
    check (findEffect ("punchydeluxe2")->isLatest && ! findEffect ("punchydeluxe")->isLatest
           && findEffect ("pointydeluxe2")->isLatest && ! findEffect ("pointydeluxe")->isLatest,
           "PunchyDeluxe2 / PointyDeluxe2 are the latest of their families");
    check (findEffect ("ultralight3")->isRecent && findEffect ("bezeq4")->isRecent && ! findEffect ("purestgain")->isRecent,
           "Last 6 months reaches 6 months back from the newest effect");

    check (latest == 425, "425 families, one latest version each (" + juce::String (latest) + ")");
    check (findEffect ("totape9")->isLatest && ! findEffect ("totape8")->isLatest && ! findEffect ("totape5")->isLatest,
           "ToTape9 is the latest ToTape");
    check (juce::String (findEffect ("totape8")->family) == "ToTape", "ToTape8 belongs to the ToTape family");
    check (findEffect ("desk4")->isLatest && findEffect ("desk")->isLatest, "Desk4 is its own effect, not Desk version 4");
    check (findEffect ("kplate140")->isLatest && findEffect ("kplate240")->isLatest, "kPlate140 and kPlate240 are two effects");
    check (findEffect ("channel9")->isLatest && ! findEffect ("channel4")->isLatest, "Channel9 is the latest Channel");
    check (findEffect ("consolexchannel")->isLatest && findEffect ("consolex2channel")->isLatest,
           "numbers inside a name don't make versions (ConsoleXChannel / ConsoleX2Channel)");
}

void testSettingsAndCollections()
{
    std::cout << "Settings, collections, Latest versions only ..." << std::endl;
    auto file = getUserDataFolder().getChildFile ("settings.json");
    file.getParentDirectory().createDirectory();

    // an old settings file (1.0): Light theme, tooltips on, an older version as favourite
    file.replaceWithText (R"({"formatVersion":1,"theme":"light","zoom":125,"tooltips":true,"favourites":["totape8","nonexistent"],"recent":["galactic3"]})");
    {
        GlobalSettings s;
        check (s.getTheme() == ThemeId::def && s.getZoom() == 125 && s.getLatestOnly(), "1.0 settings: Light -> Default, zoom kept, Latest view on");
        check (s.getFavourites().contains ("totape8") && s.getCollections().empty(), "1.0 settings: favourites kept, no collections");

        const auto* t8 = findEffect ("totape8");
        const auto* t9 = findEffect ("totape9");
        check (s.isListed (*t9) && s.isListed (*t8), "Latest view: a favourited older version stays listed");
        s.toggleFavourite ("totape8");
        check (! s.isListed (*t8), "Latest view: removed from favourites -> older version hidden");

        const int clip = s.addCollection ("Clippers");
        const int bus = s.addCollection ("Mix bus");
        const int dup = s.addCollection ("clippers");
        check (s.getCollections().size() == 3 && s.getCollections()[(size_t) dup].name == "clippers 2", "collection names are made unique");
        s.toggleInCollection (clip, "totape8");
        s.toggleInCollection (bus, "totape8");
        s.toggleInCollection (bus, "galactic3");
        check (s.isInCollection (clip, "totape8") && s.isInCollection (bus, "totape8"), "one effect can be in several collections");
        check (s.isListed (*t8), "Latest view: an older version in a collection stays listed");
        s.renameCollection (bus, "Bus");
        s.moveCollection (bus, -1);
        check (s.getCollections()[0].name == "Bus" && s.getCollections()[1].name == "Clippers", "rename + move collection");
        s.removeCollection (s.findCollection ("Clippers"));
        check (s.findCollection ("Clippers") < 0 && s.isInCollection (s.findCollection ("Bus"), "totape8"),
               "deleting a collection leaves the effects in other collections");
        s.toggleInCollection (s.findCollection ("Bus"), "totape8");
        check (! s.isListed (*t8), "Latest view: in no favourite and no collection -> hidden again");
        s.setLatestOnly (false);
        check (s.isListed (*t8), "All versions view lists everything");
        s.toggleFavourite ("galactic3");
    }
    {
        GlobalSettings s;   // read back from the file
        check (! s.getLatestOnly() && s.findCollection ("Bus") >= 0 && s.isInCollection (s.findCollection ("Bus"), "galactic3")
               && s.isFavourite ("galactic3") && s.getRecent().contains ("galactic3"), "settings + collections saved and read back");
        check (! file.loadFileAsString().contains ("tooltips"), "the old tooltips setting is gone from the file");
    }
    file.replaceWithText (R"({"collections":[{"name":"A","effects":["totape9",3,null]},{"name":"a"},{"name":""}],"latestOnly":"yes"})");
    {
        GlobalSettings s;
        check (s.getCollections().size() == 1 && s.getCollections()[0].ids.size() == 1, "odd collections data is ignored safely");
    }
    file.deleteFile();
}

void testBrowserPanel()
{
    std::cout << "Effect browser panel ..." << std::endl;
    getUserDataFolder().getChildFile ("settings.json").deleteFile();
    juce::SharedResourcePointer<GlobalSettings> settings;
    auto p = makeProcessor();
    UiContext ctx { *p, *settings, Theme::builtInDefault() };
    SelectorPanel panel (ctx, [] (const EffectInfo&) {});
    panel.setSize (676, 495);

    auto ids = [&]
    {
        juce::StringArray a;
        for (auto* e : panel.getItems()) a.add (e->id);
        return a;
    };

    check (panel.getFolderKeys().contains ("New") && panel.getFolderKeys().contains ("Last6"), "New and Last 6 months shown");
    panel.showFolder ("New");
    check (panel.getItems().size() == 2, "New lists the 2 added effects");
    panel.showFolder ("Last6");
    bool sorted = true;
    for (size_t i = 1; i < panel.getItems().size(); ++i)
        sorted = sorted && juce::String (panel.getItems()[i - 1]->released) >= juce::String (panel.getItems()[i]->released);
    size_t expected = 0;
    for (auto& e : getAllEffects())
        if (e.isRecent && settings->isListed (e)) ++expected;
    check (panel.getItems().size() == expected && expected < 24 && sorted,
           "Last 6 months: newest first, older versions hidden in the Latest view (" + juce::String ((int) expected) + ")");
    settings->setLatestOnly (false);
    panel.refresh();
    check (panel.getItems().size() == 24, "Last 6 months, all versions: 24 effects");

    settings->setLatestOnly (true);
    panel.showFolder ("Tape");
    check (ids().contains ("totape9") && ! ids().contains ("totape8"), "Latest view: Tape folder shows ToTape9, not ToTape8");
    settings->toggleFavourite ("totape8");
    panel.refresh();
    check (ids().contains ("totape8"), "Latest view: a favourited ToTape8 shows in its category");
    panel.showFolder ("Favourites");
    check (ids().contains ("totape8"), "Favourites always show what is in them");
    settings->toggleFavourite ("totape8");
    panel.showFolder ("Tape");
    check (! ids().contains ("totape8"), "Latest view: ToTape8 hidden again once it is no favourite");
    settings->setLatestOnly (false);
    panel.refresh();
    check (ids().contains ("totape8") && ids().contains ("totape5"), "All versions: every ToTape");

    const int c = settings->addCollection ("Clippers");
    settings->toggleInCollection (c, "adclip8");
    settings->toggleInCollection (c, "nonexistent");
    panel.refresh();
    check (panel.getFolderKeys().contains ("col:Clippers"), "a collection appears under Favourites");
    panel.showFolder ("col:Clippers");
    check (ids() == juce::StringArray ("adclip8"), "collection lists its effects; unknown ids are skipped");
    settings->removeCollection (c);
    panel.refresh();
    check (! panel.getFolderKeys().contains ("col:Clippers"), "a deleted collection disappears");
    settings->setLatestOnly (true);
    getUserDataFolder().getChildFile ("settings.json").deleteFile();
}

void testChangeEffect()
{
    std::cout << "Change effect ..." << std::endl;
    auto p = makeProcessor();
    check (p->selectEffect ("totape9"), "first effect chosen");
    check (! p->selectEffect ("galactic3"), "selectEffect never replaces a chosen effect");

    juce::AudioBuffer<float> warm (2, 4800);
    fillNoise (warm, 1);
    runProcessor (*p, warm, 512);
    p->getSlot (0).setValueNotifyingHost (0.9f);
    p->switchAB (1);

    std::srand (1234);   // the original effects seed their dither noise with rand(): same seed for both instances
    check (p->changeEffect ("galactic3"), "change ToTape9 -> Galactic3");
    auto* fx = p->getEffectHost();
    check (juce::String (fx->getInfo().id) == "galactic3" && p->getActiveAB() == 0 && p->getPresetName().isEmpty(),
           "after a change: new effect, A/B and preset name reset");
    check (p->getSlotName (0) == fx->getParamNames()[0], "the host sees the new parameter names");
    bool defaults = true;
    for (int i = 0; i < fx->getNumParams(); ++i)
        defaults = defaults && juce::exactlyEqual (p->getSlot (i).get(), fx->getDefault (i));
    check (defaults, "the new effect starts at its own defaults");

    auto fresh = makeProcessor();
    std::srand (1234);
    fresh->selectEffect ("galactic3");
    juce::AudioBuffer<float> a (2, 24000), b (2, 24000);
    fillNoise (a, 7);
    b.makeCopyOf (a);
    std::srand (777);
    runProcessor (*p, a, 512);
    std::srand (777);
    runProcessor (*fresh, b, 512);
    check (maxDiff (a, b, 2) == 0.0, "after a change the new effect is bit-identical to a fresh instance");
    check (p->getTailLengthSeconds() > 0.0 && juce::exactlyEqual (p->getTailLengthSeconds(), fresh->getTailLengthSeconds()),
           "the tail reported to the DAW follows the new effect");

    juce::MemoryBlock saved;
    p->getStateInformation (saved);
    auto q = makeProcessor();
    q->setStateInformation (saved.getData(), (int) saved.getSize());
    check (q->hasEffect() && juce::String (q->getEffectHost()->getInfo().id) == "galactic3", "a changed effect is saved with the session");

    // A session / undo state of another effect, loaded into an instance: it switches to that effect.
    auto r = makeProcessor();
    r->selectEffect ("totape9");
    r->getSlot (1).setValueNotifyingHost (0.25f);
    juce::MemoryBlock other;
    r->getStateInformation (other);
    p->setStateInformation (other.getData(), (int) other.getSize());
    check (juce::String (p->getEffectHost()->getInfo().id) == "totape9" && juce::exactlyEqual (p->getSlot (1).get(), 0.25f),
           "loading a ToTape9 state into a Galactic3 instance restores ToTape9 with its values (DAW undo)");
    check (! p->changeEffect ("nonexistent") && juce::String (p->getEffectHost()->getInfo().id) == "totape9",
           "an unknown id changes nothing");

    // Changing while the audio thread runs (blocks under the callback lock, like a DAW) and the host saves.
    std::atomic<bool> stop { false }, finite { true };
    std::atomic<int> blocks { 0 };
    std::thread audio ([&]
    {
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer midi;
        int seed = 0;
        while (! stop)
        {
            fillNoise (buf, ++seed);
            {
                const juce::ScopedLock sl (p->getCallbackLock());
                p->processBlock (buf, midi);
            }
            if (! allFinite (buf)) finite = false;
            ++blocks;
        }
    });
    const char* ids[] = { "galactic3", "totape9", "kcathedral5", "pop3", "debess", "punchydeluxe2" };
    for (int i = 0; i < 30; ++i)
    {
        p->changeEffect (ids[i % 6]);
        juce::MemoryBlock mb;
        p->getStateInformation (mb);
        std::this_thread::sleep_for (std::chrono::milliseconds (3));
    }
    stop = true;
    audio.join();
    check (finite && blocks > 0 && juce::String (p->getEffectHost()->getInfo().id) == "punchydeluxe2",
           "30 effect changes while audio runs: no crash, no NaN/inf (" + juce::String (blocks.load()) + " blocks)");
}

void testEffectDocs()
{
    std::cout << "Effect info texts ..." << std::endl;
    int withDoc = 0;
    for (auto& e : getAllEffects())
    {
        // A few very new effects have no Airwindopedia text yet: the panel then shows the description.
        const auto doc = InfoPanel::docFor (e.id);
        if (doc.isEmpty())
            continue;
        ++withDoc;
        check (doc.length() > 200 && doc.startsWith ("#"), juce::String ("Airwindopedia text for ") + e.id);
    }
    check (withDoc >= (int) getAllEffects().size() - 20, "almost every effect has its Airwindopedia text");
}


// ---------------------------------------------------------------------------------------------
// Audio thread processing while the "GUI" changes everything it can. Under ThreadSanitizer
// (CI job) this proves there are no data races between the audio and message threads.
void testThreadedStress()
{
    std::cout << "Threaded stress (audio + GUI threads) ..." << std::endl;
    auto p = makeProcessor (48000.0, 256);
    p->selectEffect ("stonefirecomp");

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
                const int idx = e->getIntAttribute ("index", -1);   // same index + name (names can repeat)
                valuesOk = valuesOk && p->getEffectHost()->getParamNames()[idx] == e->getStringAttribute ("name");
                valuesOk = valuesOk && idx >= 0
                        && std::abs (p->getSlot (idx).get() - (float) e->getDoubleAttribute ("value")) < 1.0e-6f;
            }
        check (valuesOk, "parameter values restored: " + f.getFileName());

        if (auto* trims = xml->getChildByName ("Trims"))
            check (std::abs (p->getInputTrim().get() - (float) trims->getDoubleAttribute ("in")) < 1.0e-4f, "trims restored: " + f.getFileName());
    }
}


// ---------------------------------------------------------------------------------------------
void testStateExtras()
{
    std::cout << "State extras (exact A/B, unknown -> known, chunked blocks) ..." << std::endl;

    {
        auto rev = makeProcessor(); rev->selectEffect ("galactic3");
        auto tape = makeProcessor(); tape->selectEffect ("totape9");
        check (rev->getTailLengthSeconds() >= 5.0 && tape->getTailLengthSeconds() < 0.001, "reverb tail reported to host");
    }

    // A/B values survive a save exactly (not rounded)
    {
        auto p = makeProcessor();
        p->selectEffect ("galactic3");
        p->getSlot (0).setValueNotifyingHost (0.12345679f);
        p->switchAB (1);
        p->getSlot (0).setValueNotifyingHost (0.9f);
        juce::MemoryBlock mb;
        p->getStateInformation (mb);
        auto q = makeProcessor();
        q->setStateInformation (mb.getData(), (int) mb.getSize());
        q->switchAB (0);
        check (q->getSlot (0).get() == 0.12345679f, "A/B slot restored bit-exactly");
    }

    // effects that use a parameter name twice (StoneFireComp: two "Attack"s) keep every value
    {
        juce::TemporaryFile tmp;
        PresetStore store (tmp.getFile());
        auto p = makeProcessor();
        p->selectEffect ("stonefirecomp");
        auto* fx = p->getEffectHost();
        std::vector<float> v;
        for (int i = 0; i < fx->getNumParams(); ++i) v.push_back (0.05f + 0.09f * (float) i);
        store.save (fx->getInfo(), "Dup", fx->getParamNames(), v);
        PresetData d;
        PresetStore::load (store.fileFor (fx->getInfo(), "Dup"), d);
        p->applyPreset (d);
        bool ok = true;
        for (int i = 0; i < fx->getNumParams(); ++i) ok = ok && p->getSlot (i).get() == v[(size_t) i];
        check (ok, "preset with repeated parameter names restores every slot");
        juce::MemoryBlock mb;
        p->getStateInformation (mb);
        auto q = makeProcessor();
        q->setStateInformation (mb.getData(), (int) mb.getSize());
        ok = true;
        for (int i = 0; i < fx->getNumParams(); ++i) ok = ok && q->getSlot (i).get() == v[(size_t) i];
        check (ok, "session with repeated parameter names restores every slot");
        tmp.getFile().deleteRecursively();
    }

    // a session of a known effect replaces the "needs a newer version" placeholder
    {
        auto p = makeProcessor();
        const juce::String unknown = "<DaylightState formatVersion=\"1\" effectId=\"fromthefuture\"/>";
        p->setStateInformation (unknown.toRawUTF8(), (int) unknown.getNumBytesAsUTF8());
        check (p->hasUnknownEffect(), "unknown effect placeholder");
        auto donor = makeProcessor();
        donor->selectEffect ("debess");
        juce::MemoryBlock mb;
        donor->getStateInformation (mb);
        p->setStateInformation (mb.getData(), (int) mb.getSize());
        check (! p->hasUnknownEffect() && p->hasEffect() && juce::String (p->getEffectHost()->getInfo().id) == "debess",
               "known state replaces unknown placeholder");
    }

    // host blocks larger than prepared (chunking) and mono double stay bit-identical
    for (auto id : { "totape9", "galactic3" })
    {
        const auto* info = findEffect (id);
        std::srand (4321);
        auto p = makeProcessor (48000.0, 256);
        p->selectEffect (id);
        std::srand (4321);
        auto ref = info->create();
        ref->setSampleRate (48000.0f);

        juce::AudioBuffer<float> a (2, 4096), b;
        fillNoise (a, 7);
        b.makeCopyOf (a);
        std::srand (99);
        runProcessor (*p, a, 1024);   // 4x the prepared size
        std::srand (99);
        for (int start = 0; start < 4096; start += 256)
        {
            float* io[2] = { b.getWritePointer (0, start), b.getWritePointer (1, start) };
            ref->processReplacing (io, io, 256);
        }
        check (maxDiff (a, b, 2) == 0.0, juce::String ("chunked big host blocks bit-identical: ") + id);
    }
}


void testLayouts()
{
    std::cout << "Layouts ..." << std::endl;
    int sizes[4] = {}, tabbed = 0;
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

        // window size follows the packed rows: 1 Small, 2 Medium, 3 Large; pages only in Large
        const int expectRows = layout.pages.size() > 1 ? 3 : layout.pages.empty() ? 1 : juce::jmax (1, (int) layout.pages[0].rows.size());
        check (layout.rows == expectRows, juce::String ("window rows = packed rows: ") + e.id);
        if (layout.hasTabs()) ++tabbed;
        ++sizes[juce::jlimit (1, 3, layout.rows)];
    }
    std::cout << "  small " << sizes[1] << ", medium " << sizes[2] << ", large " << sizes[3] << ", with tabs " << tabbed << std::endl;
    check (sizes[2] > 0, "some effects use the Medium window");

    {
        EffectHost host (*findEffect ("totape9"), 48000.0);
        std::vector<ParamFacts> facts;
        for (int i = 0; i < host.getNumParams(); ++i)
            facts.push_back ({ host.getParamNames()[i], host.detectSteps (i) });
        const auto l = buildLayout (facts, builtInLayoutJson ("totape9"));
        check (l.rows == 2 && windowHeightForRows (l.rows) == kMediumHeight, "ToTape9 opens Medium (2 rows)");
    }

    // Stepped / switch detection on known effects
    for (auto [id, index, steps] : { std::tuple { "golem", 2, 6 }, std::tuple { "adclip9", 4, 3 },
                                     std::tuple { "monitoring3", 0, 17 }, std::tuple { "pauldither", 0, 2 },
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

    // Every effect in the Default theme; a sample in the User theme (a copy of Default: proves it loads).
    const juce::StringArray sample { "totape9", "stonefirecomp", "debess", "cliponly3", "golem", "monitoring3",
                                     "pauldither", "kcathedral5", "consolexchannel", "galactic3" };
    ensureUserThemeExists();
    for (auto theme : { ThemeId::def, ThemeId::user })
    {
        settings->setTheme (theme);

        auto shoot = [&] (DaylightProcessor& p, const juce::String& name, int panel = 0)
        {
            std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
            if (auto* d = dynamic_cast<DaylightEditor*> (ed.get()))
            {
                if (panel == 1) d->showSettingsPanel();
                if (panel == 2) d->showInfoPanel();
                if (panel == 3) d->showAbout();
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
            if (theme != ThemeId::def && ! sample.contains (e.id))
                continue;
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
            shoot (*p, "zz_totape9_about", 3);
            p->getInputTrim().setValueNotifyingHost (p->getInputTrim().convertTo0to1 (-2.0f));
            p->getBypass().setValueNotifyingHost (1.0f);
            shoot (*p, "zz_totape9_trim_bypass");

            auto small = makeProcessor();
            small->selectEffect ("debess");
            shoot (*small, "zz_debess_settings", 1);
            shoot (*small, "zz_debess_info", 2);

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

    // Never touch the real Documents/big_tyme_designs folder: settings, themes and favourites
    // written by the tests go to a temporary folder.
    juce::TemporaryFile userFolder;
    userFolder.getFile().createDirectory();
    setUserDataFolderForTesting (userFolder.getFile());
    struct CleanUp { juce::File f; ~CleanUp() { f.deleteRecursively(); } } cleanUp { userFolder.getFile() };

    if (argc > 1 && juce::String (argv[1]) == "--list-steps")
    {
        // diagnostic: which parameters show named choices (stepped knobs / switches)
        for (auto& e : getAllEffects())
        {
            EffectHost host (e, 48000.0);
            for (int i = 0; i < host.getNumParams(); ++i)
                if (const int n = host.detectSteps (i); n > 0)
                    std::cout << e.id << " " << i << " " << host.getParamNames()[i] << " " << n << std::endl;
        }
        return 0;
    }

    testSelectionAndNames();
    testPassThroughAndBypass();
    testState();
    testLayouts();
    testPresets();
    testEffectDocs();
    testBrowserData();
    testThemes();
    testSettingsAndCollections();
    testBrowserPanel();
    testChangeEffect();
    testStateExtras();
    testThreadedStress();
    testOldSessions (juce::File (DAYLIGHT_FIXTURES_DIR));
    testAllEffects();

    for (int i = 1; i + 1 < argc; ++i)
        if (juce::String (argv[i]) == "--manual")
        {
            // pictures for the PDF manual, 2x resolution
            const auto dir = juce::File::getCurrentWorkingDirectory().getChildFile (argv[i + 1]);
            dir.createDirectory();
            juce::SharedResourcePointer<GlobalSettings> settings;
            const auto original = settings->getTheme();
            juce::TemporaryFile tempDir;
            PresetStore::setDefaultRootForTesting (tempDir.getFile());

            auto shot = [&] (ThemeId theme, const char* id, const juce::String& name, int panel)
            {
                settings->setTheme (theme);
                auto p = makeProcessor();
                if (id != nullptr) p->selectEffect (id);
                if (panel == 1 && p->hasEffect())
                {
                    auto* fx = p->getEffectHost();
                    std::vector<float> v;
                    for (int k = 0; k < fx->getNumParams(); ++k) v.push_back (p->getSlot (k).get());
                    PresetStore store (PresetStore::defaultRoot());
                    for (auto n : { "Warm Tape", "Bright Bus", "Lo-fi Cassette" })
                        store.save (fx->getInfo(), n, fx->getParamNames(), v);
                    PresetData d;
                    PresetStore::load (store.fileFor (fx->getInfo(), "Warm Tape"), d);
                    p->applyPreset (d);
                }
                std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
                if (auto* e = dynamic_cast<DaylightEditor*> (ed.get()))
                {
                    if (panel == 1) e->showSettingsPanel();
                    if (panel == 2) e->showInfoPanel();
                }
                auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
                auto f = dir.getChildFile (name + ".png");
                f.deleteFile();
                juce::FileOutputStream os (f);
                juce::PNGImageFormat().writeImageToStream (img, os);
            };
            shot (ThemeId::def, nullptr, "selector", 0);
            shot (ThemeId::def, "totape9", "effect", 0);
            shot (ThemeId::def, "stonefirecomp", "large", 0);
            shot (ThemeId::def, "totape9", "settings", 1);

            PresetStore::setDefaultRootForTesting ({});
            tempDir.getFile().deleteRecursively();
            settings->setTheme (original);
        }

    for (int i = 1; i + 1 < argc; ++i)
        if (juce::String (argv[i]) == "--controls")
        {
            // design review: effects showing every control type, every theme, 2x resolution
            const auto dir = juce::File::getCurrentWorkingDirectory().getChildFile (argv[i + 1]);
            dir.createDirectory();
            juce::SharedResourcePointer<GlobalSettings> settings;
            const auto original = settings->getTheme();
            for (auto theme : { ThemeId::def })
            {
                settings->setTheme (theme);
                for (auto id : { "totape9", "golem", "monitoring3", "pauldither", "debess" })
                {
                    auto p = makeProcessor();
                    p->selectEffect (id);
                    std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
                    if (juce::String (id) == "debess")
                        if (auto* d = dynamic_cast<DaylightEditor*> (ed.get()))
                            d->showSettingsPanel();
                    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
                    auto f = dir.getChildFile (juce::String (id) + "_" + toString (theme) + ".png");
                    f.deleteFile();
                    juce::FileOutputStream os (f);
                    juce::PNGImageFormat().writeImageToStream (img, os);
                }
            }
            settings->setTheme (original);
        }

    for (int i = 1; i + 1 < argc; ++i)
        if (juce::String (argv[i]) == "--snapshots")
            renderSnapshots (juce::File::getCurrentWorkingDirectory().getChildFile (argv[i + 1]));

    std::cout << "\n" << checks << " checks, " << failures << " failed" << std::endl;
    return failures == 0 ? 0 : 1;
}
