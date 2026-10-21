#include "Settings.h"

namespace dlaw
{
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

    theme = themeIdFromString (root.getProperty ("theme", "light").toString());

    const int z = (int) root.getProperty ("zoom", 100);
    zoom = 100;
    for (auto level : zoomLevels)
        if (level == z)
            zoom = z;

    auto readList = [] (const juce::var& v)
    {
        juce::StringArray list;
        if (auto* arr = v.getArray())
            for (auto& item : *arr)
                if (item.isString())
                    list.addIfNotAlreadyThere (item.toString());
        return list;
    };
    favourites = readList (root.getProperty ("favourites", {}));
    recent = readList (root.getProperty ("recent", {}));
    while (recent.size() > 10)
        recent.remove (recent.size() - 1);
}

void GlobalSettings::save()
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("formatVersion", 1);
    obj->setProperty ("theme", toString (theme));
    obj->setProperty ("zoom", zoom);

    juce::Array<juce::var> fav, rec;
    for (auto& s : favourites) fav.add (s);
    for (auto& s : recent) rec.add (s);
    obj->setProperty ("favourites", fav);
    obj->setProperty ("recent", rec);

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
} // namespace dlaw
