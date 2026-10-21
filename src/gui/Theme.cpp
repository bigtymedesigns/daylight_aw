#include "Theme.h"
#include "BinaryData.h"
#include "../Identity.h"

namespace dlaw
{
namespace
{
juce::String resourceText (const char* name)
{
    int size = 0;
    if (auto* data = BinaryData::getNamedResource (name, size))
        return juce::String::fromUTF8 (data, size);
    return {};
}

juce::Image resourceImage (const char* name)
{
    int size = 0;
    if (auto* data = BinaryData::getNamedResource (name, size))
        return juce::ImageCache::getFromMemory (data, size);
    return {};
}

bool parseColour (const juce::var& v, juce::Colour& out)
{
    if (! v.isString())
        return false;

    auto s = v.toString().trim();
    if (! s.startsWithChar ('#'))
        return false;

    s = s.substring (1);
    if (! s.containsOnly ("0123456789abcdefABCDEF") || (s.length() != 6 && s.length() != 8))
        return false;

    const auto value = (juce::uint32) s.getHexValue64();
    out = s.length() == 6 ? juce::Colour (0xff000000u | value)
                          : juce::Colour ((value << 24) | (value >> 8)); // #RRGGBBAA -> ARGB
    return true;
}

bool parseNumber (const juce::var& v, float& out, float lo, float hi)
{
    if (! (v.isDouble() || v.isInt() || v.isInt64()))
        return false;
    const auto f = (float) (double) v;
    if (! std::isfinite (f))
        return false;
    out = juce::jlimit (lo, hi, f);
    return true;
}

float luminance (juce::Colour c)
{
    auto lin = [] (float u) { return u <= 0.03928f ? u / 12.92f : std::pow ((u + 0.055f) / 1.055f, 2.4f); };
    return 0.2126f * lin (c.getFloatRed()) + 0.7152f * lin (c.getFloatGreen()) + 0.0722f * lin (c.getFloatBlue());
}

float contrast (juce::Colour a, juce::Colour b)
{
    const auto la = luminance (a), lb = luminance (b);
    return (juce::jmax (la, lb) + 0.05f) / (juce::jmin (la, lb) + 0.05f);
}

juce::Colour over (juce::Colour top, juce::Colour bottom)
{
    return bottom.interpolatedWith (top.withAlpha (1.0f), top.getFloatAlpha());
}
} // namespace

// ---------------------------------------------------------------------------------------------
Theme Theme::fromJson (const juce::String& jsonText, const Theme& base)
{
    Theme t = base;
    t.warnings.clear();

    auto root = juce::JSON::parse (jsonText);
    if (! root.isObject())
    {
        t.warnings.add ("theme.json could not be read - using defaults.");
        return t;
    }

    if (auto n = root.getProperty ("name", {}); n.isString())
        t.name = n.toString();

    auto tokens = root.getProperty ("tokens", {});
    if (! tokens.isObject())
    {
        t.warnings.add ("theme.json has no \"tokens\" section - using defaults.");
        return t;
    }

    auto colour = [&] (const char* key, juce::Colour& target)
    {
        auto v = tokens.getProperty (key, {});
        if (v.isVoid()) return;
        if (! parseColour (v, target))
            t.warnings.add (juce::String (key) + ": invalid colour (use #RRGGBB or #RRGGBBAA)");
    };
    auto number = [&] (const char* key, float& target, float lo, float hi)
    {
        auto v = tokens.getProperty (key, {});
        if (v.isVoid()) return;
        if (! parseNumber (v, target, lo, hi))
            t.warnings.add (juce::String (key) + ": invalid number");
    };

    colour ("paper.color", t.paperColor);
    number ("paper.textureStrength", t.textureStrength, 0.0f, 1.0f);
    colour ("horizon.color", t.horizonColor);
    number ("horizon.opacity", t.horizonOpacity, 0.0f, 1.0f);

    colour ("ink.text", t.text);
    colour ("ink.muted", t.muted);
    colour ("ink.groupTitle", t.groupTitle);

    colour ("frame.color", t.frameColor);
    number ("frame.opacity", t.frameOpacity, 0.0f, 1.0f);
    colour ("frame.glowColor", t.frameGlowColor);
    number ("frame.glowRadius", t.frameGlowRadius, 0.0f, 40.0f);
    number ("frame.glowStrength", t.frameGlowStrength, 0.0f, 1.0f);

    colour ("knob.fill", t.knobFill);
    colour ("knob.edge", t.knobEdge);
    colour ("knob.pointer", t.knobPointer);
    colour ("knob.shadow", t.knobShadow);
    colour ("knob.ticks", t.knobTicks);

    colour ("bar.fill", t.barFill);
    colour ("bar.border", t.barBorder);
    number ("bar.glow", t.barGlow, 0.0f, 1.0f);

    colour ("panel.fill", t.panelFill);
    colour ("panel.border", t.panelBorder);
    colour ("panel.text", t.panelText);
    number ("panel.glow", t.panelGlow, 0.0f, 1.0f);

    colour ("button.outline", t.buttonOutline);
    colour ("button.activeFill", t.buttonActiveFill);
    colour ("button.activeText", t.buttonActiveText);

    colour ("logo.color", t.logoColor);
    number ("logo.glow", t.logoGlow, 0.0f, 1.0f);

    return t;
}

Theme Theme::builtInLight()
{
    static const Theme cached = []
    {
        Theme t = fromJson (resourceText ("light_theme_json"), Theme {});
        t.paper = resourceImage ("light_paper_png");
        t.figure = resourceImage ("light_figure_png");
        return t;
    }();
    return cached;
}

Theme Theme::builtInDark()
{
    static const Theme cached = []
    {
        Theme t = fromJson (resourceText ("dark_theme_json"), builtInLight());
        t.paper = resourceImage ("dark_paper_png");
        t.figure = resourceImage ("dark_figure_png");
        return t;
    }();
    return cached;
}

Theme Theme::fromFolder (const juce::File& folder, const Theme& base)
{
    auto json = folder.getChildFile ("theme.json");
    Theme t = json.existsAsFile() ? fromJson (json.loadFileAsString(), base) : base;

    if (! json.existsAsFile())
        t.warnings.add ("theme.json not found - using defaults.");

    auto loadImage = [&] (const char* fileName) -> juce::Image
    {
        auto f = folder.getChildFile (fileName);
        if (! f.existsAsFile())
            return {};
        auto img = juce::ImageFileFormat::loadFrom (f);
        if (! img.isValid() || img.getWidth() > 4096 || img.getHeight() > 4096)
        {
            t.warnings.add (juce::String (fileName) + " could not be used (missing, broken or larger than 4096 px).");
            return {};
        }
        return img;
    };

    t.paper = loadImage ("paper.png");
    t.figure = loadImage ("figure.png");

    if (t.minimumContrast() < 4.5f)
        t.warnings.add ("Low contrast: some text may be hard to read (" + juce::String (t.minimumContrast(), 1) + ":1, aim for 4.5:1).");

    return t;
}

float Theme::minimumContrast() const
{
    const auto bg = paperColor;
    const auto panel = over (panelFill, bg);
    const auto bar = over (barFill, bg);
    return juce::jmin (contrast (text, bg), contrast (panelText, panel), contrast (text, bar), contrast (groupTitle, bg));
}

// ---------------------------------------------------------------------------------------------
juce::String toString (ThemeId id)
{
    switch (id)
    {
        case ThemeId::dark: return "dark";
        case ThemeId::user: return "user";
        case ThemeId::light: break;
    }
    return "light";
}

ThemeId themeIdFromString (const juce::String& s)
{
    if (s == "dark") return ThemeId::dark;
    if (s == "user") return ThemeId::user;
    return ThemeId::light;
}

juce::File getUserDataFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile (kVendorName)
        .getChildFile (kPluginName);
}

juce::File getUserThemeFolder()
{
    return getUserDataFolder().getChildFile ("Themes").getChildFile ("User");
}

bool ensureUserThemeExists (ThemeId from)
{
    auto folder = getUserThemeFolder();
    if (folder.getChildFile ("theme.json").existsAsFile())
        return true;

    if (! folder.createDirectory())
        return false;

    const bool dark = from == ThemeId::dark;
    auto write = [&] (const char* resource, const char* fileName)
    {
        int size = 0;
        if (auto* data = BinaryData::getNamedResource (resource, size))
            return folder.getChildFile (fileName).replaceWithData (data, (size_t) size);
        return false;
    };

    bool ok = write (dark ? "dark_theme_json" : "light_theme_json", "theme.json");
    write (dark ? "dark_paper_png" : "light_paper_png", "paper.png");
    write (dark ? "dark_figure_png" : "light_figure_png", "figure.png");
    write ("theme_readme_md", "README.md");

    // Give the copy its own name so it is recognisable.
    auto jsonFile = folder.getChildFile ("theme.json");
    if (ok)
    {
        auto parsed = juce::JSON::parse (jsonFile.loadFileAsString());
        if (auto* obj = parsed.getDynamicObject())
        {
            obj->setProperty ("name", "User");
            obj->setProperty ("author", "");
            ok = jsonFile.replaceWithText (juce::JSON::toString (parsed));
        }
    }
    return ok;
}
} // namespace dlaw
