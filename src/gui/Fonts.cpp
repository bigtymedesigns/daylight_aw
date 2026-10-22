#include "Fonts.h"
#include "BinaryData.h"

#include <atomic>

namespace dlaw::fonts
{
namespace
{
juce::Typeface::Ptr load (const char* resource)
{
    int size = 0;
    if (auto* data = BinaryData::getNamedResource (resource, size))
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    return {};
}

juce::Font make (const juce::Typeface::Ptr& tf, float height)
{
    if (tf != nullptr)
        return juce::Font (juce::FontOptions (tf).withHeight (height));
    return juce::Font (juce::FontOptions (height));
}

struct Faces : public juce::DeletedAtShutdown
{
    juce::Typeface::Ptr medium { load ("shantell_medium_ttf") };
    juce::Typeface::Ptr semiBold { load ("shantell_semibold_ttf") };
    juce::Typeface::Ptr extraBold { load ("shantell_extrabold_ttf") };

    ~Faces() override { instance() = nullptr; }
    static Faces*& instance() { static Faces* p = nullptr; return p; }
};

Faces& faces()
{
    auto& p = Faces::instance();
    if (p == nullptr)
        p = new Faces();
    return *p;
}

std::atomic<bool>& systemMode()
{
    static std::atomic<bool> on { false };
    return on;
}

juce::Font systemFont (float h, bool bold)
{
   #if JUCE_WINDOWS
    const juce::String family ("Segoe UI");
   #else
    const juce::String family (juce::Font::getDefaultSansSerifFontName());
   #endif
    return juce::Font (juce::FontOptions (family, h, bold ? juce::Font::bold : juce::Font::plain));
}
} // namespace

void setUseSystemFont (bool on) { systemMode() = on; }
bool usesSystemFont() { return systemMode(); }

juce::Font medium (float h)    { return systemMode() ? systemFont (h, false) : make (faces().medium, h); }
juce::Font semiBold (float h)  { return systemMode() ? systemFont (h, false) : make (faces().semiBold, h); }
juce::Font extraBold (float h) { return systemMode() ? systemFont (h, true)  : make (faces().extraBold, h); }

float width (const juce::Font& f, const juce::String& text)
{
    return juce::GlyphArrangement::getStringWidth (f, text);
}
} // namespace dlaw::fonts
