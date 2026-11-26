#include "Settings.h"
#include "../dsp/EffectRegistry.h"

namespace dlaw
{
namespace
{
juce::StringArray readList (const juce::var& v)
{
    juce::StringArray list;
    if (auto* arr = v.getArray())
        for (auto& item : *arr)
            if (item.isString() && item.toString().isNotEmpty())
                list.addIfNotAlreadyThere (item.toString());
    return list;
}

juce::var toVar (const juce::StringArray& list)
{
    juce::Array<juce::var> arr;
    for (auto& s : list)
        arr.add (s);
    return arr;
}
} // namespace

GlobalSettings::GlobalSettings()
    : file (getUserDataFolder().getChildFile ("settings.json"))
{
    load();
}

void GlobalSettings::load()
{
    if (! file.existsAsFile())
        return;

    lastModification = file.getLastModificationTime();
    auto root = juce::JSON::parse (file.loadFileAsString());
    if (! root.isObject())
        return; // broken file: keep defaults, it will be rewritten on the next change

    theme = themeIdFromString (root.getProperty ("theme", "default").toString());

    const int z = (int) root.getProperty ("zoom", 100);
    zoom = 100;
    for (auto level : zoomLevels)
        if (level == z)
            zoom = z;

    latestOnly = (bool) root.getProperty ("latestOnly", true);
    favourites = readList (root.getProperty ("favourites", {}));
    recent = readList (root.getProperty ("recent", {}));
    while (recent.size() > 10)
        recent.remove (recent.size() - 1);

    collections.clear();
    if (auto* arr = root.getProperty ("collections", {}).getArray())
        for (auto& c : *arr)
        {
            const auto name = c.getProperty ("name", {}).toString().trim();
            if (name.isEmpty() || findCollection (name) >= 0)
                continue;
            collections.push_back ({ name.substring (0, 40), readList (c.getProperty ("effects", {})) });
        }
}

void GlobalSettings::save()
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("formatVersion", 2);
    obj->setProperty ("theme", toString (theme));
    obj->setProperty ("zoom", zoom);
    obj->setProperty ("latestOnly", latestOnly);
    obj->setProperty ("favourites", toVar (favourites));

    juce::Array<juce::var> cols;
    for (auto& c : collections)
    {
        auto* co = new juce::DynamicObject();
        co->setProperty ("name", c.name);
        co->setProperty ("effects", toVar (c.ids));
        cols.add (juce::var (co));
    }
    obj->setProperty ("collections", cols);
    obj->setProperty ("recent", toVar (recent));

    file.getParentDirectory().createDirectory();
    if (file.replaceWithText (juce::JSON::toString (juce::var (obj))))   // atomic: temp file + rename
        lastModification = file.getLastModificationTime();

    sendChangeMessage();
}

void GlobalSettings::reloadIfChanged()
{
    if (file.existsAsFile() && file.getLastModificationTime() != lastModification)
    {
        load();
        sendChangeMessage();
    }
}

void GlobalSettings::setTheme (ThemeId t)
{
    if (t != theme) { theme = t; save(); }
}

void GlobalSettings::setZoom (int percent)
{
    for (auto level : zoomLevels)
        if (level == percent && percent != zoom)
        {
            zoom = percent;
            save();
        }
}

void GlobalSettings::setLatestOnly (bool on)
{
    if (on != latestOnly) { latestOnly = on; save(); }
}

bool GlobalSettings::isListed (const EffectInfo& e) const
{
    if (! latestOnly || e.isLatest)
        return true;
    const juce::String id (e.id);
    return isFavourite (id) || isInAnyCollection (id);
}

void GlobalSettings::toggleFavourite (const juce::String& id)
{
    if (favourites.contains (id))
        favourites.removeString (id);
    else
        favourites.add (id);
    save();
}

void GlobalSettings::addRecent (const juce::String& id)
{
    recent.removeString (id);
    recent.insert (0, id);
    while (recent.size() > 10)
        recent.remove (recent.size() - 1);
    save();
}

// ---- collections ----------------------------------------------------------------------------
int GlobalSettings::findCollection (const juce::String& name) const
{
    for (size_t i = 0; i < collections.size(); ++i)
        if (collections[i].name.equalsIgnoreCase (name.trim()))
            return (int) i;
    return -1;
}

juce::String GlobalSettings::uniqueName (const juce::String& wanted, int ignoreIndex) const
{
    auto base = wanted.trim().substring (0, 40);
    if (base.isEmpty())
        base = "Collection";
    auto taken = [&] (const juce::String& n)
    {
        const int i = findCollection (n);
        return i >= 0 && i != ignoreIndex;
    };
    auto name = base;
    for (int n = 2; taken (name); ++n)
        name = base + " " + juce::String (n);
    return name;
}

int GlobalSettings::addCollection (const juce::String& name)
{
    collections.push_back ({ uniqueName (name, -1), {} });
    save();
    return (int) collections.size() - 1;
}

void GlobalSettings::renameCollection (int index, const juce::String& newName)
{
    if (! juce::isPositiveAndBelow (index, (int) collections.size()) || newName.trim().isEmpty())
        return;
    collections[(size_t) index].name = uniqueName (newName, index);
    save();
}

void GlobalSettings::removeCollection (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) collections.size()))
        return;
    collections.erase (collections.begin() + index);
    save();
}

void GlobalSettings::moveCollection (int index, int delta)
{
    const int target = index + delta;
    if (! juce::isPositiveAndBelow (index, (int) collections.size()) || ! juce::isPositiveAndBelow (target, (int) collections.size()))
        return;
    std::swap (collections[(size_t) index], collections[(size_t) target]);
    save();
}

bool GlobalSettings::isInCollection (int index, const juce::String& id) const
{
    return juce::isPositiveAndBelow (index, (int) collections.size()) && collections[(size_t) index].ids.contains (id);
}

void GlobalSettings::toggleInCollection (int index, const juce::String& id)
{
    if (! juce::isPositiveAndBelow (index, (int) collections.size()))
        return;
    auto& ids = collections[(size_t) index].ids;
    if (ids.contains (id))
        ids.removeString (id);
    else
        ids.add (id);
    save();
}

bool GlobalSettings::isInAnyCollection (const juce::String& id) const
{
    for (auto& c : collections)
        if (c.ids.contains (id))
            return true;
    return false;
}
} // namespace dlaw
