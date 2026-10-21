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
} // namespace

juce::Font medium (float h)    { return make (faces().medium, h); }
juce::Font semiBold (float h)  { return make (faces().semiBold, h); }
juce::Font extraBold (float h) { return make (faces().extraBold, h); }

float width (const juce::Font& f, const juce::String& text)
{
    return juce::GlyphArrangement::getStringWidth (f, text);
}
} // namespace dlaw::fonts
