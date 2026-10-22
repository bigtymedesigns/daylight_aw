#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace dlaw::fonts
{
/** Shantell Sans (SIL Open Font Licence), embedded in the plugin - or, when a theme asks for it
    ("style.font": "system"), the computer's own UI font (Segoe UI on Windows, the system font on
    macOS). System fonts are only requested from the OS, never embedded. */
void setUseSystemFont (bool);

juce::Font medium (float height);
juce::Font semiBold (float height);
juce::Font extraBold (float height);

float width (const juce::Font& f, const juce::String& text);
} // namespace dlaw::fonts
