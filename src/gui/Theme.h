#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace dlaw
{
/**
    All colours / strengths / images of one theme ("tokens").
    Drawing code reads ONLY these values - never a colour written directly in code.
    See themes/README.md for the meaning of every token.
*/
struct Theme
{
    juce::String name { "Light" };

    // Paper
    juce::Colour paperColor { 0xffe2dfd7 };
    float textureStrength = 1.0f;
    juce::Colour horizonColor { 0xff2b2a28 };
    float horizonOpacity = 0.10f;

    // Text
    juce::Colour text { 0xff2b2a28 };
    juce::Colour muted { 0xff55524b };
    juce::Colour groupTitle { 0xff3a3834 };

    // Group frames
    juce::Colour frameColor { 0xff2b2a28 };
    float frameOpacity = 0.55f;
    juce::Colour frameGlowColor { 0xffece8dc };
    float frameGlowRadius = 0.0f;
    float frameGlowStrength = 0.0f;

    // Knobs, stepped knobs, switches
    juce::Colour knobFill { 0xff1b1a18 };
    juce::Colour knobEdge { 0xff0f0e0d };
    juce::Colour knobPointer { 0xffece8df };
    juce::Colour knobShadow { 0x331e1c18 };
    juce::Colour knobTicks { 0xff2b2a28 };
    juce::String knobStyle { "rough" };   // edge drawing: "rough" (cut), "soft" (pencil), "grain" (charcoal)
    /** Tests/previews only: forces a knob style on every loaded theme (empty = off). */
    static juce::String& knobStyleOverrideForTesting() { static juce::String s; return s; }

    // Style of the whole window
    bool listView = false;      // style.view "list": one fader per parameter, Airwindows order and names
    bool systemFont = false;    // style.font "system": the computer's own UI font instead of Shantell Sans
    float corner = 6.0f;        // style.corner: corner radius of bars, panels and buttons (px)
    juce::Colour topBarFill { 0x00000000 };   // style.topBar: band behind the top row (transparent = none)

    // Faders (list view and trims in list view)
    juce::Colour sliderTrack { 0xff5d5e5f };
    juce::Colour sliderHandle { 0xffdcdcdc };
    juce::Colour sliderHandleBorder { 0xff9a9a9a };
    juce::Colour sliderCentre { 0xff1b1b1b };
    juce::Colour valueText { 0xff2b2a28 };    // value text next to faders

    // Selected rows in lists (effects, categories, presets)
    juce::Colour selectFill { 0x1a2b2a28 };
    juce::Colour selectText { 0xff2b2a28 };

    // Menu bar
    juce::Colour barFill { 0xd9eeebe4 };
    juce::Colour barBorder { 0x662b2a28 };
    float barGlow = 0.0f;

    // Panels (dropdowns, About, dialogs)
    juce::Colour panelFill { 0xf7f0ede6 };
    juce::Colour panelBorder { 0x662b2a28 };
    juce::Colour panelText { 0xff2b2a28 };
    float panelGlow = 0.0f;

    // Buttons (A/B, BYPASS, tabs, stars)
    juce::Colour buttonOutline { 0xff2b2a28 };
    juce::Colour buttonActiveFill { 0xff2b2a28 };
    juce::Colour buttonActiveText { 0xfff1eee7 };

    // Logo
    juce::Colour logoColor { 0xff2b2a28 };
    float logoGlow = 0.0f;

    // Images (optional). Paper is a tiling texture; both are stored at 2x resolution.
    juce::Image paper, figure;

    /** Messages about invalid / missing values (shown in settings, never fatal). */
    juce::StringArray warnings;

    static Theme builtInLight();
    static Theme builtInDark();
    static Theme builtInOriginal();

    /** Parses theme.json text on top of 'base' (every missing/invalid token keeps base's value). */
    static Theme fromJson (const juce::String& jsonText, const Theme& base);

    /** Loads a theme folder (theme.json + paper.png + figure.png). Never throws. */
    static Theme fromFolder (const juce::File& folder, const Theme& base);

    /** Lowest text/background contrast ratio among the important pairs. */
    float minimumContrast() const;
};

/** Theme ids stored in the settings. */
enum class ThemeId { light, dark, user, original };
juce::String toString (ThemeId);
ThemeId themeIdFromString (const juce::String&);

/** User folder: Documents/big_tyme_designs/Daylight_aw */
juce::File getUserDataFolder();
juce::File getUserThemeFolder();

/** Creates the user theme folder as a copy of 'from' if it doesn't exist yet. Returns false on failure. */
bool ensureUserThemeExists (ThemeId from);
} // namespace dlaw
