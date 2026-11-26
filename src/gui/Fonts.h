#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace dlaw::fonts
{
/** Embedded fonts (SIL Open Font Licence): Inter for all interface text, Jost for the logo.
    Type scale: 13 px buttons and names, 12 px values, 11 px caps section titles. */
juce::Font regular (float height);     // Inter 400
juce::Font medium (float height);      // Inter 500
juce::Font semiBold (float height);    // Inter 600
juce::Font bold (float height);        // Inter 700
juce::Font logo (float height);        // Jost 300 (Light)
juce::Font logoRegular (float height); // Jost 400

/** Caps titles ("PRESETS", group titles): letter-spaced. */
juce::Font caps (float height, float tracking = 0.08f);

float width (const juce::Font& f, const juce::String& text);
} // namespace dlaw::fonts
