#pragma once

#include "Components.h"
#include "Panels.h"

namespace dlaw
{
class DaylightEditor : public juce::AudioProcessorEditor,
                       private juce::ChangeListener,
                       private juce::Timer
{
public:
    explicit DaylightEditor (DaylightProcessor&);
    ~DaylightEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

    /** Opens the presets + settings panel (used by the snapshot tests). */
    void showSettingsPanel();
    /** Opens the effect info panel (used by the snapshot tests). */
    void showInfoPanel();

private:
    class Content;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void loadTheme();
    void applyZoom();

    juce::SharedResourcePointer<GlobalSettings> settings;
    UiContext ctx;
    DaylightLookAndFeel lookAndFeel;
    std::unique_ptr<Content> content;
    juce::Time userThemeStamp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DaylightEditor)
};
} // namespace dlaw
