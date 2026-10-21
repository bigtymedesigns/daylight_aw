#include "PresetStore.h"
#include "../Identity.h"

#include <algorithm>
#include <cmath>

namespace dlaw
{
PresetStore::PresetStore (juce::File rootFolder) : root (std::move (rootFolder)) {}

namespace
{
juce::File& testRoot()
{
    static juce::File f;
    return f;
}
} // namespace

void PresetStore::setDefaultRootForTesting (const juce::File& f) { testRoot() = f; }

juce::File PresetStore::defaultRoot()
{
    if (testRoot() != juce::File())
        return testRoot();
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile (kVendorName)
        .getChildFile (kPluginName)
        .getChildFile ("Presets");
}

juce::File PresetStore::folderFor (const EffectInfo& info) const
{
    return root.getChildFile (sanitiseName (info.name));
}

juce::File PresetStore::fileFor (const EffectInfo& info, const juce::String& presetName) const
{
    return folderFor (info).getChildFile (sanitiseName (presetName) + extension);
}

juce::Array<juce::File> PresetStore::list (const EffectInfo& info) const
{
    auto files = folderFor (info).findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension);

    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
    {
        return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0;
    });
    return files;
}

juce::String PresetStore::sanitiseName (const juce::String& raw)
{
    juce::String s;
    for (auto c : raw)
    {
        if (c < 32 || juce::String ("\\/:*?\"<>|").containsChar (c))
            s << ' ';
        else
            s << c;
    }

    // collapse whitespace, no leading dots (hidden files), no trailing dots/spaces (Windows)
    s = s.replace ("\t", " ");
    while (s.contains ("  "))
        s = s.replace ("  ", " ");
    s = s.trim();
    while (s.startsWithChar ('.'))
        s = s.substring (1).trimStart();
    while (s.endsWithChar ('.') || s.endsWithChar (' '))
        s = s.dropLastCharacters (1);

    // reserved device names on Windows
    static const juce::StringArray reserved { "CON", "PRN", "AUX", "NUL",
                                              "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                                              "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
    if (reserved.contains (s, true))
        s << "_";

    return s.substring (0, 64).trimEnd();
}

bool PresetStore::save (const EffectInfo& info, const juce::String& presetName,
                        const juce::StringArray& paramNames, const std::vector<float>& values) const
{
    const auto clean = sanitiseName (presetName);
    if (clean.isEmpty() || paramNames.size() != (int) values.size())
        return false;

    auto folder = folderFor (info);
    if (! folder.createDirectory())
        return false;

    juce::Array<juce::var> vals;
    for (int i = 0; i < paramNames.size(); ++i)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("name", paramNames[i]);
        o->setProperty ("value", (double) juce::jlimit (0.0f, 1.0f, values[(size_t) i]));
        vals.add (juce::var (o));
    }

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("format", formatVersion);
    obj->setProperty ("plugin", kPluginName);
    obj->setProperty ("pluginVersion", kVersion);
    obj->setProperty ("effect", info.id);
    obj->setProperty ("effectName", info.name);
    obj->setProperty ("effectVersion", info.version);
    obj->setProperty ("values", vals);

    return fileFor (info, clean).replaceWithText (juce::JSON::toString (juce::var (obj)));   // atomic
}

bool PresetStore::load (const juce::File& file, PresetData& result)
{
    if (! file.existsAsFile() || file.getSize() > 1024 * 1024)
        return false;

    juce::var root;
    if (juce::JSON::parse (file.loadFileAsString(), root).failed() || ! root.isObject())
        return false;

    const auto effect = root.getProperty ("effect", {}).toString();
    const auto* vals = root.getProperty ("values", {}).getArray();
    if (effect.isEmpty() || vals == nullptr)
        return false;

    PresetData d;
    d.name = file.getFileNameWithoutExtension();
    d.effectId = effect;
    d.effectVersion = (int) root.getProperty ("effectVersion", 0);

    for (auto& v : *vals)
    {
        const auto name = v.getProperty ("name", {}).toString();
        const auto value = v.getProperty ("value", {});
        if (name.isEmpty() || ! (value.isDouble() || value.isInt() || value.isInt64()))
            continue;
        const double x = (double) value;
        if (! std::isfinite (x))
            continue;
        d.values.emplace_back (name, (float) juce::jlimit (0.0, 1.0, x));
    }

    result = std::move (d);
    return true;
}

bool PresetStore::rename (const juce::File& presetFile, const juce::String& newName, juce::File& renamed) const
{
    const auto clean = sanitiseName (newName);
    if (clean.isEmpty() || ! presetFile.existsAsFile())
        return false;

    auto target = presetFile.getSiblingFile (clean + extension);
    if (target == presetFile)
    {
        renamed = presetFile;
        return true;
    }

    // Allow changing only the letter case of the same file (case-insensitive file systems).
    const bool sameIgnoringCase = target.getFileName().equalsIgnoreCase (presetFile.getFileName());
    if (target.exists() && ! sameIgnoringCase)
        return false;

    if (sameIgnoringCase)
    {
        auto temp = presetFile.getSiblingFile (clean + ".renaming.tmp");
        if (! presetFile.moveFileTo (temp) || ! temp.moveFileTo (target))
            return false;
    }
    else if (! presetFile.moveFileTo (target))
        return false;

    renamed = target;
    return true;
}

bool PresetStore::remove (const juce::File& presetFile)
{
    if (! presetFile.existsAsFile())
        return false;
    return presetFile.moveToTrash() || presetFile.deleteFile();
}
} // namespace dlaw
