#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "Theme.h"

namespace dlaw
{
struct EffectInfo;

/**
    Global user preferences shared by every Daylight_aw instance:
    theme, zoom, favourites, favourites collections, recently used effects, the browser view.

    Stored in Documents/big_tyme_designs/Daylight_aw/settings.json.
    Writes are atomic (temp file + rename). Instances in the same DAW share this object
    (SharedResourcePointer); other processes are picked up by reloadIfChanged().
    Unknown keys from older versions (e.g. "tooltips") are ignored.
*/
class GlobalSettings : public juce::ChangeBroadcaster
{
public:
    GlobalSettings();

    ThemeId getTheme() const { return theme; }
    void setTheme (ThemeId);

    int getZoom() const { return zoom; }
    void setZoom (int percent);

    // ---- favourites + collections ----------------------------------------------------------
    bool isFavourite (const juce::String& id) const { return favourites.contains (id); }
    void toggleFavourite (const juce::String& id);
    const juce::StringArray& getFavourites() const { return favourites; }

    /** A user-made collection under Favourites ("Clippers", "Mix bus"). Tags: an effect can be in
        several. Deleting a collection never touches the effects or the favourites. */
    struct Collection { juce::String name; juce::StringArray ids; };
    const std::vector<Collection>& getCollections() const { return collections; }
    int findCollection (const juce::String& name) const;          // -1 when there is none
    /** Adds a collection; returns its index. The name is made unique ("Clippers 2"). */
    int addCollection (const juce::String& name);
    void renameCollection (int index, const juce::String& newName);
    void removeCollection (int index);
    void moveCollection (int index, int delta);                    // -1 = up, +1 = down
    bool isInCollection (int index, const juce::String& id) const;
    void toggleInCollection (int index, const juce::String& id);
    bool isInAnyCollection (const juce::String& id) const;

    // ---- recently used --------------------------------------------------------------------
    void addRecent (const juce::String& id);
    const juce::StringArray& getRecent() const { return recent; }

    // ---- effect browser view ----------------------------------------------------------------
    /** "Latest versions only" (default) or all versions. */
    bool getLatestOnly() const { return latestOnly; }
    void setLatestOnly (bool);

    /** Whether the browser lists this effect in the current view: always in "All versions";
        in "Latest versions only" the newest of each family, plus any older version that is a
        favourite or in a collection (so it stays at hand until it is removed from those). */
    bool isListed (const EffectInfo&) const;

    /** Call regularly from the GUI (cheap): reloads if another process changed the file. */
    void reloadIfChanged();

    static constexpr int zoomLevels[] = { 100, 125, 150, 200 };

private:
    void load();
    void save();
    juce::String uniqueName (const juce::String& wanted, int ignoreIndex) const;

    juce::File file;
    juce::Time lastModification;
    ThemeId theme = ThemeId::def;
    int zoom = 100;
    bool latestOnly = true;
    juce::StringArray favourites, recent;
    std::vector<Collection> collections;
};
} // namespace dlaw
