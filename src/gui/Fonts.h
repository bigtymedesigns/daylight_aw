#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace dlaw::fonts
{
/** Shantell Sans (SIL Open Font Licence), embedded in the plugin. */
juce::Font medium (float height);
juce::Font semiBold (float height);
juce::Font extraBold (float height);

float width (const juce::Font& f, const juce::String& text);
} // namespace dlaw::fonts
