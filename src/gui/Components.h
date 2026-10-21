#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "../PluginProcessor.h"
#include "Layout.h"
#include "Settings.h"
#include "Theme.h"

namespace dlaw
{
/** Shared state every GUI part needs. Owned by the editor. */
struct UiContext
{
    DaylightProcessor& processor;
    GlobalSettings& settings;
    Theme theme;
};

/** Theme-aware look for the few stock JUCE widgets we use (sliders, text editors, scrollbars). */
class DaylightLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit DaylightLookAndFeel (UiContext& c) : ctx (c) {}
    void applyTheme();

    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Font getLabelFont (juce::Label&) override;
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                        int thumbStart, int thumbSize, bool over, bool down) override;

private:
    UiContext& ctx;
};

/** Small rounded button used for A/B, segmented choices and dialog buttons. */
class PillButton : public juce::Button
{
public:
    PillButton (UiContext& c, const juce::String& text, bool filledWhenOn = true);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    float fontSize = 12.0f;
    bool primary = false;    // always filled (e.g. "Load effect")
    bool joinedLeft = false, joinedRight = false; // segmented groups

private:
    UiContext& ctx;
    bool fillOn;
};

/** The round BYPASS button (linked to the DAW's own bypass parameter). */
class BypassButton : public juce::Component
{
public:
    explicit BypassButton (UiContext& c);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    UiContext& ctx;
    juce::ParameterAttachment attachment;
};

/** One parameter: knob, stepped knob or switch, with its name above and value below. */
class ParamControl : public juce::Component,
                     private juce::TextEditor::Listener
{
public:
    ParamControl (UiContext& c, const ControlSpec& spec);
    ~ParamControl() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    static constexpr float kDragPixels = 250.0f;   // full range
    static constexpr float kFineFactor = 10.0f;    // Shift
    static constexpr float kWheelStep = 0.01f;     // 1 % per notch

private:
    juce::Rectangle<float> controlArea() const;
    juce::Rectangle<float> valueArea() const;
    float snap (float v) const;
    int stepIndex (float v) const;
    void setFromGesture (float v);
    juce::String valueText() const;

    void textEditorReturnKeyPressed (juce::TextEditor&) override;
    void textEditorEscapeKeyPressed (juce::TextEditor&) override;
    void textEditorFocusLost (juce::TextEditor&) override;
    void closeEditor (bool apply);

    UiContext& ctx;
    ControlSpec spec;
    juce::ParameterAttachment attachment;
    float value = 0.0f;
    bool hover = false, dragging = false;
    float dragStartValue = 0.0f, dragAnchorY = 0.0f;
    bool dragFine = false;
    juce::Path edge;  // hand-drawn knob outline (deterministic)
    std::unique_ptr<juce::TextEditor> editor;
};
} // namespace dlaw
