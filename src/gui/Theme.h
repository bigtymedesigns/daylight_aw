#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace dlaw
{
/**
    All colours and sizes of one theme ("tokens").
    Drawing code reads ONLY these values - never a colour written directly in code.
    The meaning of every token: themes/HOW_TO_MAKE_YOUR_OWN_THEME.txt (copied into the user's theme folder).
    The values below are the built-in Default ("deep space") look; themes/default/theme.json holds the same.
*/
struct Theme
{
    juce::String name { "Default" };

    // Window background: a flat colour with two soft corner halos
    struct Halo
    {
        juce::Colour colour;
        float opacity;     // 0..1 at the centre, fading to nothing at the edge
        float x, y;        // centre, as a fraction of the window width / height
        float radius;      // as a fraction of the distance to the farthest corner
    };
    juce::Colour background { 0xff07080b };
    Halo halo1 { juce::Colour (0xff2fc4a5), 0.20f, 0.10f, 0.22f, 0.42f };
    Halo halo2 { juce::Colour (0xffc657ec), 0.17f, 0.90f, 0.88f, 0.45f };
    juce::Colour topBarLine { 0x12ffffff };    // line under the top bar

    // Text
    juce::Colour text { 0xffe6e8eb };          // general text, menu button, messages
    juce::Colour muted { 0xff9da1a7 };         // labels (IN, OUT, BYPASS), hints
    juce::Colour nameText { 0xffd9dce0 };      // control names
    juce::Colour value { 0xffc3c6cb };         // control values
    juce::Colour groupTitle { 0xff2fc4a5 };    // group titles
    juce::Colour accent { 0xff2fc4a5 };        // trim value when not 0 dB, links

    // Group panels
    juce::Colour groupFill { 0x590a0c10 };
    juce::Colour groupBorder { 0x17ffffff };

    // Knobs, stepped knobs, switches
    juce::String knobStyle { "ring" };         // "ring": outline only; "round": filled disc
    juce::Colour knobFill { 0x00000000 };
    juce::Colour knobRing { 0x4dffffff };
    juce::Colour knobPointer { 0xffc657ec };
    juce::Colour knobTicks { 0x4dffffff };

    // Buttons (menu button, A/B, Save preset, dropdowns, bypass, page tabs)
    juce::Colour buttonFill { 0x05ffffff };
    juce::Colour buttonBorder { 0x24ffffff };
    juce::Colour buttonText { 0xffe6e8eb };
    juce::Colour buttonActiveFill { 0xff2fc4a5 };   // A/B, current page, bypass on: green
    juce::Colour buttonActiveText { 0xff06130f };

    // Panels (effect browser, presets + settings, About, Effect info, dialogs, menus)
    juce::Colour panelFill { 0xf70d0f13 };
    juce::Colour panelBorder { 0x21ffffff };
    juce::Colour panelText { 0xffe6e8eb };
    juce::Colour selectFill { 0x242fc4a5 };    // selected rows
    juce::Colour selectText { 0xff7fe8cf };

    juce::Colour logo { 0xffeef0f3 };
    float corner = 6.0f;                       // buttons and panels; group panels use corner + 2

    /** Messages about invalid values (shown in the settings panel, never fatal). */
    juce::StringArray warnings;

    static Theme builtInDefault();

    /** Parses theme.json text on top of 'base' (every missing/invalid token keeps base's value). */
    static Theme fromJson (const juce::String& jsonText, const Theme& base);

    /** Loads a theme folder (theme.json). Never throws. */
    static Theme fromFolder (const juce::File& folder, const Theme& base);

    /** Lowest text/background contrast ratio among the important pairs. */
    float minimumContrast() const;
};

/** Theme ids stored in the settings. Anything else (old Light, Dark, Original) reads as Default. */
enum class ThemeId { def, user };
juce::String toString (ThemeId);
ThemeId themeIdFromString (const juce::String&);

/** User folder: Documents/big_tyme_designs/Daylight_aw */
juce::File getUserDataFolder();
juce::File getUserThemeFolder();
/** Tests only: use another user folder (empty file = normal location). Set before anything reads it. */
void setUserDataFolderForTesting (const juce::File&);

inline constexpr const char* kThemeGuideFileName = "How to make your own theme.txt";

/**
    Makes sure the User theme folder holds a theme for this version: a copy of Default plus the guide.
    A theme from Daylight_aw 1.0 (made for the old Light/Dark look) is moved, untouched, into
    "old_1.0" inside the folder first; nothing is ever deleted. Returns false on failure.
*/
bool ensureUserThemeExists();
} // namespace dlaw
