#include "Fonts.h"
#include "BinaryData.h"

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
    juce::Typeface::Ptr inter400 { load ("inter_400_ttf") };
    juce::Typeface::Ptr inter500 { load ("inter_500_ttf") };
    juce::Typeface::Ptr inter600 { load ("inter_600_ttf") };
    juce::Typeface::Ptr inter700 { load ("inter_700_ttf") };
    juce::Typeface::Ptr jost300 { load ("jost_300_ttf") };
    juce::Typeface::Ptr jost400 { load ("jost_400_ttf") };

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
} // namespace

// JUCE's font height is the full line height (ascent + descent); CSS sizes are the em size.
// Inter's line height is ~1.21 em and Jost's ~1.45 em, so the em sizes from the design match.
juce::Font regular (float h)     { return make (faces().inter400, h * 1.21f); }
juce::Font medium (float h)      { return make (faces().inter500, h * 1.21f); }
juce::Font semiBold (float h)    { return make (faces().inter600, h * 1.21f); }
juce::Font bold (float h)        { return make (faces().inter700, h * 1.21f); }
juce::Font logo (float h)        { return make (faces().jost300, h * 1.445f); }
juce::Font logoRegular (float h) { return make (faces().jost400, h * 1.445f); }

juce::Font caps (float h, float tracking) { return bold (h).withExtraKerningFactor (tracking); }

float width (const juce::Font& f, const juce::String& text)
{
    return juce::GlyphArrangement::getStringWidth (f, text);
}
} // namespace dlaw::fonts
