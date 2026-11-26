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

/** Theme-aware look for the few stock JUCE widgets we use (text editors, scrollbars, menus). */
class DaylightLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit DaylightLookAndFeel (UiContext& c) : ctx (c) {}
    void applyTheme();

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    int getPopupMenuBorderSize() override { return 5; }
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                        int thumbStart, int thumbSize, bool over, bool down) override;

private:
    UiContext& ctx;
};

/** Rounded button: A/B, panel buttons, dropdowns, links. */
class PillButton : public juce::Button
{
public:
    enum class Icon { none, reset, info, change };

    PillButton (UiContext& c, const juce::String& text, bool filledWhenOn = true);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    float fontSize = 12.0f;
    bool primary = false;    // always filled (e.g. "Load effect")
    bool joinedLeft = false, joinedRight = false; // segmented groups
    bool dropdown = false;   // text on the left, chevron on the right
    bool link = false;       // accent-coloured text, no frame ("Open theme folder...")
    Icon icon = Icon::none;

private:
    UiContext& ctx;
    bool fillOn;
};

/** "BYPASS" label + a small round button (the DAW's own bypass parameter).
    Only the round button reacts to the mouse: clicks on the label or around it do nothing. */
class BypassButton : public juce::Component
{
public:
    explicit BypassButton (UiContext& c);
    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    void mouseUp (const juce::MouseEvent&) override;
    int preferredWidth() const;

    static constexpr float kButtonSize = 22.0f;

private:
    juce::Rectangle<float> circle() const;
    UiContext& ctx;
    juce::ParameterAttachment attachment;
};

/** Input/output trim in the top bar: "IN" + mini knob + value (dB; teal when not 0 dB).
    Drag (Shift or right-drag = fine), wheel, double-click = 0 dB. */
class TrimKnob : public juce::Component
{
public:
    TrimKnob (UiContext& c, juce::AudioParameterFloat& param, const juce::String& label);

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    int preferredWidth() const;

private:
    juce::Rectangle<float> knobArea() const;
    void set (float db, bool asGesture);

    UiContext& ctx;
    juce::AudioParameterFloat& param;
    juce::String label;
    juce::ParameterAttachment attachment;
    float db = 0.0f;
    bool hover = false, dragging = false, gestureOpen = false, dragFine = false, rightDrag = false;
    float dragStartDb = 0.0f, dragAnchorY = 0.0f;
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
    static constexpr float kFineFactor = 10.0f;    // Shift or right-drag
    static constexpr float kWheelStep = 0.01f;     // 1 % per notch
    static constexpr float kKnobSize = 46.0f;
    /** Right-button moves smaller than this are a click (left free for a later per-knob menu). */
    static constexpr float kRightDragThreshold = 3.0f;

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
    bool hover = false, dragging = false, gestureOpen = false, rightDrag = false, dragFine = false;
    float dragStartValue = 0.0f, dragAnchorY = 0.0f;
    std::unique_ptr<juce::TextEditor> editor;
};

namespace draw
{
/** Ring knob (Default look): outline ring + pointer; "round" style fills the disc. */
void knob (juce::Graphics&, const Theme&, juce::Point<float> centre, float diameter, float value01,
           float ringWidth, float pointerWidth);
/** Down/up chevron centred at (x, y). */
void chevron (juce::Graphics&, float x, float y, bool up, juce::Colour, float size = 3.5f);
/** Filled or outlined five-point star. */
void star (juce::Graphics&, juce::Point<float> centre, float radius, bool filled, juce::Colour);
} // namespace draw
} // namespace dlaw
