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

juce::File& testUserFolder()
{
    static juce::File f;
    return f;
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
        t.warnings.add ("theme.json could not be read - using the Default values.");
        return t;
    }

    if (auto n = root.getProperty ("name", {}); n.isString())
        t.name = n.toString();

    auto tokens = root.getProperty ("tokens", {});
    if (! tokens.isObject())
    {
        t.warnings.add ("theme.json has no \"tokens\" section - using the Default values.");
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
    auto halo = [&] (const juce::String& prefix, Halo& h)
    {
        colour ((prefix + ".color").toRawUTF8(), h.colour);
        number ((prefix + ".opacity").toRawUTF8(), h.opacity, 0.0f, 1.0f);
        number ((prefix + ".x").toRawUTF8(), h.x, -0.5f, 1.5f);
        number ((prefix + ".y").toRawUTF8(), h.y, -0.5f, 1.5f);
        number ((prefix + ".radius").toRawUTF8(), h.radius, 0.0f, 2.0f);
    };

    colour ("background.color", t.background);
    halo ("halo1", t.halo1);
    halo ("halo2", t.halo2);
    colour ("topBar.line", t.topBarLine);

    colour ("ink.text", t.text);
    colour ("ink.muted", t.muted);
    colour ("ink.name", t.nameText);
    colour ("ink.value", t.value);
    colour ("ink.groupTitle", t.groupTitle);
    colour ("ink.accent", t.accent);

    colour ("group.fill", t.groupFill);
    colour ("group.border", t.groupBorder);

    {
        const auto v = tokens.getProperty ("knob.style", {});
        if (! v.isVoid())
        {
            const auto style = v.toString().trim().toLowerCase();
            if (style == "ring" || style == "round")
                t.knobStyle = style;
            else
                t.warnings.add ("knob.style: use \"ring\" or \"round\"");
        }
    }
    colour ("knob.fill", t.knobFill);
    colour ("knob.ring", t.knobRing);
    colour ("knob.pointer", t.knobPointer);
    colour ("knob.ticks", t.knobTicks);

    colour ("button.fill", t.buttonFill);
    colour ("button.border", t.buttonBorder);
    colour ("button.text", t.buttonText);
    colour ("button.activeFill", t.buttonActiveFill);
    colour ("button.activeText", t.buttonActiveText);

    colour ("panel.fill", t.panelFill);
    colour ("panel.border", t.panelBorder);
    colour ("panel.text", t.panelText);
    colour ("select.fill", t.selectFill);
    colour ("select.text", t.selectText);

    colour ("logo.color", t.logo);
    number ("style.corner", t.corner, 0.0f, 12.0f);

    return t;
}

Theme Theme::builtInDefault()
{
    static const Theme cached = fromJson (resourceText ("default_theme_json"), Theme {});
    return cached;
}

Theme Theme::fromFolder (const juce::File& folder, const Theme& base)
{
    auto json = folder.getChildFile ("theme.json");
    Theme t = json.existsAsFile() && json.getSize() < 256 * 1024 ? fromJson (json.loadFileAsString(), base) : base;

    if (! json.existsAsFile())
        t.warnings.add ("theme.json not found - using the Default values.");

    if (t.minimumContrast() < 4.5f)
        t.warnings.add ("Low contrast: some text may be hard to read (" + juce::String (t.minimumContrast(), 1) + ":1, aim for 4.5:1).");

    return t;
}

float Theme::minimumContrast() const
{
    const auto bg = background;
    const auto panel = over (panelFill, bg);
    return juce::jmin (contrast (text, bg), contrast (nameText, bg), contrast (panelText, panel), contrast (groupTitle, bg));
}

// ---------------------------------------------------------------------------------------------
juce::String toString (ThemeId id)
{
    return id == ThemeId::user ? "user" : "default";
}

ThemeId themeIdFromString (const juce::String& s)
{
    return s == "user" ? ThemeId::user : ThemeId::def;   // old "light", "dark", "original" -> Default
}

void setUserDataFolderForTesting (const juce::File& f) { testUserFolder() = f; }

juce::File getUserDataFolder()
{
    if (testUserFolder() != juce::File())
        return testUserFolder();
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile (kVendorName)
        .getChildFile (kPluginName);
}

juce::File getUserThemeFolder()
{
    return getUserDataFolder().getChildFile ("Themes").getChildFile ("User");
}

bool ensureUserThemeExists()
{
    auto folder = getUserThemeFolder();
    auto jsonFile = folder.getChildFile ("theme.json");
    auto guide = folder.getChildFile (kThemeGuideFileName);

    if (jsonFile.existsAsFile())
    {
        const auto root = juce::JSON::parse (jsonFile.loadFileAsString());
        // Only a theme that explicitly comes from Daylight_aw 1.0 (formatVersion 1) is moved aside;
        // a broken or hand-made file stays where it is (it falls back safely to the Default values).
        const bool fromVersion1 = root.isObject() && (int) root.getProperty ("formatVersion", 2) < 2;
        if (! fromVersion1)
        {
            if (! guide.existsAsFile())
                guide.replaceWithText (resourceText ("theme_guide_txt"));
            return true;
        }

        auto old = folder.getNonexistentChildFile ("old_1.0", {}, true);   // "old_1.0", then "old_1.0 (2)" ...
        if (! old.createDirectory())
            return false;
        for (auto& f : folder.findChildFiles (juce::File::findFiles, false))
            if (! f.moveFileTo (old.getChildFile (f.getFileName())))
                return false;
    }
    else if (! folder.createDirectory())
    {
        return false;
    }

    auto text = resourceText ("default_theme_json")
                    .replace ("\"name\": \"Default\"", "\"name\": \"User\"")
                    .replace ("\"author\": \"big_tyme_designs\"", "\"author\": \"\"");
    return jsonFile.replaceWithText (text) && guide.replaceWithText (resourceText ("theme_guide_txt"));
}
} // namespace dlaw
