#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

namespace dlaw
{
/**
    Global user preferences shared by every Daylight_aw instance:
    theme, zoom, favourites, recently used effects.

    Stored in Documents/big_tyme_designs/Daylight_aw/settings.json.
    Writes are atomic (temp file + rename). Instances in the same DAW share this object
    (SharedResourcePointer); other processes are picked up by reloadIfChanged().
*/
class GlobalSettings : public juce::ChangeBroadcaster
{
public:
    GlobalSettings();

    ThemeId getTheme() const { return theme; }
    void setTheme (ThemeId);

    int getZoom() const { return zoom; }
    void setZoom (int percent);

    bool getTooltips() const { return tooltips; }
    void setTooltips (bool on);

    bool isFavourite (const juce::String& id) const { return favourites.contains (id); }
    void toggleFavourite (const juce::String& id);
    const juce::StringArray& getFavourites() const { return favourites; }

    void addRecent (const juce::String& id);
    const juce::StringArray& getRecent() const { return recent; }

    /** Call regularly from the GUI (cheap): reloads if another process changed the file. */
    void reloadIfChanged();

    static constexpr int zoomLevels[] = { 100, 125, 150, 200 };

private:
    void load();
    void save();

    juce::File file;
    juce::Time lastModification;
    ThemeId theme = ThemeId::light;
    int zoom = 100;
    bool tooltips = false;   // off for new installs; switch on in Settings
    juce::StringArray favourites, recent;
};
} // namespace dlaw
