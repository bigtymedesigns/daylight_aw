#include "Components.h"
#include "Fonts.h"

namespace dlaw
{
namespace
{
juce::Point<float> polar (juce::Point<float> c, float radius, float degrees)
{
    const auto a = juce::degreesToRadians (degrees);
    return { c.x + radius * std::sin (a), c.y - radius * std::cos (a) };
}

/** Knob outline for the theme's knob.style (deterministic per control).
    rough: the original cut, hand-cut edge.
    soft:  round, slightly uneven pencil circle (low, smooth wobble).
    grain: practically round with a fine charcoal grain on the edge. */
juce::Path knobOutline (const juce::String& style, float radius, int seed)
{
    juce::Random rnd ((juce::int64) seed * 7919 + 17);
    const float p1 = rnd.nextFloat() * 6.28f, p2 = rnd.nextFloat() * 6.28f, p3 = rnd.nextFloat() * 6.28f;

    const bool soft = style == "soft", grain = style == "grain";
    const int n = soft || grain ? 240 : 72;

    juce::Path p;
    for (int i = 0; i <= n; ++i)
    {
        const float t = juce::MathConstants<float>::twoPi * (float) i / (float) n;
        float r = radius;
        if (soft)
            r += 0.42f * std::sin (2.0f * t + p1) + 0.22f * std::sin (3.0f * t + p2) + 0.06f * std::sin (9.0f * t + p3);
        else if (grain)
            r += 0.12f * std::sin (2.0f * t + p1) + 0.09f * std::sin (19.0f * t + p2) + 0.07f * std::sin (37.0f * t + p3);
        else
            r += 0.45f * std::sin (3.0f * t + p1) + 0.3f * std::sin (7.0f * t + p2) + 0.2f * std::sin (13.0f * t + p3);
        const juce::Point<float> pt { r * std::sin (t), -r * std::cos (t) };
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    p.closeSubPath();
    return p;
}

/** Extra edge detail drawn over the knob for the soft and grain styles. */
void drawKnobEdgeDetail (juce::Graphics& g, const juce::String& style, juce::Point<float> centre, float radius,
                         float scale, int seed, juce::Colour edgeColour, juce::Colour fillColour)
{
    juce::Random rnd ((juce::int64) seed * 104729 + 3);
    if (style == "soft")
    {
        // a second, lighter pencil pass, slightly off the first one and not quite closed
        const float start = rnd.nextFloat() * 6.28f;
        const float p1 = rnd.nextFloat() * 6.28f;
        juce::Path retrace;
        constexpr int n = 160;
        for (int i = 0; i <= n; ++i)
        {
            const float t = start + juce::MathConstants<float>::twoPi * 0.82f * (float) i / (float) n;
            const float r = (radius + 0.55f + 0.35f * std::sin (2.0f * t + p1)) * scale;
            const juce::Point<float> pt { centre.x + r * std::sin (t), centre.y - r * std::cos (t) };
            if (i == 0) retrace.startNewSubPath (pt); else retrace.lineTo (pt);
        }
        g.setColour (edgeColour.withMultipliedAlpha (0.45f));
        g.strokePath (retrace, juce::PathStrokeType (0.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else if (style == "grain")
    {
        // tiny charcoal specks just inside and outside the edge
        for (int i = 0; i < 70; ++i)
        {
            const float t = rnd.nextFloat() * juce::MathConstants<float>::twoPi;
            const float d = (radius + (rnd.nextFloat() - 0.5f) * 1.6f) * scale;
            const float sz = (0.35f + rnd.nextFloat() * 0.55f) * scale;
            const bool outside = d > radius * scale;
            g.setColour ((outside ? edgeColour : fillColour.contrasting (0.25f)).withMultipliedAlpha (outside ? 0.35f : 0.18f));
            g.fillEllipse (centre.x + d * std::sin (t) - sz * 0.5f, centre.y - d * std::cos (t) - sz * 0.5f, sz, sz);
        }
    }
}

/** "0.5000" -> "0.500" (or "0.5000" while active); keeps text values and units as they are. */
juce::String formatValue (const juce::String& raw, bool active)
{
    auto t = raw.trim();
    auto numberPart = t.upToFirstOccurrenceOf (" ", false, false);
    auto rest = t.fromFirstOccurrenceOf (" ", true, false);

    if (numberPart.isEmpty() || ! numberPart.containsOnly ("0123456789.-+") || ! numberPart.containsAnyOf ("0123456789"))
        return t;

    const auto decimals = active ? 4 : 3;
    return juce::String (numberPart.getDoubleValue(), decimals) + rest;
}
} // namespace

// =============================================================================================
void DaylightLookAndFeel::applyTheme()
{
    const auto& t = ctx.theme;
    setColour (juce::TextEditor::textColourId, t.panelText);
    setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::highlightColourId, t.panelText.withAlpha (0.2f));
    setColour (juce::TextEditor::highlightedTextColourId, t.panelText);
    setColour (juce::CaretComponent::caretColourId, t.panelText);
    setColour (juce::TextEditor::outlineColourId, t.panelBorder);
    setColour (juce::TextEditor::focusedOutlineColourId, t.panelText);
    setColour (juce::Label::textColourId, t.panelText);
    setColour (juce::ScrollBar::thumbColourId, t.panelText.withAlpha (0.35f));
    setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TooltipWindow::backgroundColourId, t.panelFill.withAlpha (1.0f));
    setColour (juce::TooltipWindow::textColourId, t.panelText);
    setColour (juce::TooltipWindow::outlineColourId, t.panelBorder);
}

juce::Rectangle<int> DaylightLookAndFeel::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parentArea)
{
    const auto font = fonts::medium (12.0f);
    int w = 0, lines = 0;
    for (auto& line : juce::StringArray::fromLines (text))
    {
        w = juce::jmax (w, (int) fonts::width (font, line));
        ++lines;
    }
    w = juce::jmin (w + 18, 320);
    const int h = lines * 17 + 10;
    return juce::Rectangle<int> (pos.x > parentArea.getCentreX() ? pos.x - (w + 12) : pos.x + 12,
                                 pos.y > parentArea.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h)
        .constrainedWithin (parentArea);
}

void DaylightLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    const auto& t = ctx.theme;
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (findColour (juce::TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (findColour (juce::TooltipWindow::outlineColourId));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (t.panelText);
    g.setFont (fonts::medium (12.0f));
    g.drawFittedText (text, juce::Rectangle<int> (9, 5, width - 18, height - 10), juce::Justification::topLeft, 8);
}

void DaylightLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                            juce::Slider::SliderStyle, juce::Slider&)
{
    const auto& t = ctx.theme;
    const float cy = (float) y + (float) h * 0.5f;
    g.setColour (t.panelText.withAlpha (0.35f));
    g.drawLine ((float) x, cy, (float) (x + w), cy, 1.6f);

    // centre mark (0 dB)
    const float mid = (float) x + (float) w * 0.5f;
    g.drawLine (mid, cy - 4.0f, mid, cy + 4.0f, 1.2f);

    g.setColour (t.buttonActiveFill);
    g.fillEllipse (pos - 6.0f, cy - 6.0f, 12.0f, 12.0f);
}

juce::Font DaylightLookAndFeel::getLabelFont (juce::Label&) { return fonts::semiBold (13.0f); }

void DaylightLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    g.setColour (ctx.theme.panelFill.withMultipliedAlpha (0.6f));
    g.fillRoundedRectangle (0.0f, 0.0f, (float) w, (float) h, 4.0f);
}

void DaylightLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& e)
{
    g.setColour (e.hasKeyboardFocus (true) ? ctx.theme.panelText : ctx.theme.panelBorder);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1.0f, (float) h - 1.0f, 4.0f, 1.0f);
}

void DaylightLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                         int thumbStart, int thumbSize, bool over, bool)
{
    g.setColour (ctx.theme.panelText.withAlpha (over ? 0.45f : 0.25f));
    auto r = vertical ? juce::Rectangle<int> (x + w / 2 - 2, thumbStart, 4, thumbSize)
                      : juce::Rectangle<int> (thumbStart, y + h / 2 - 2, thumbSize, 4);
    g.fillRoundedRectangle (r.toFloat(), 2.0f);
}

// =============================================================================================
PillButton::PillButton (UiContext& c, const juce::String& text, bool filledWhenOn)
    : juce::Button (text), ctx (c), fillOn (filledWhenOn)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void PillButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto& t = ctx.theme;
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const bool filled = primary || (fillOn && getToggleState());
    const float radius = 4.0f;

    juce::Path shape;
    shape.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radius, radius,
                               ! joinedLeft, ! joinedRight, ! joinedLeft, ! joinedRight);

    if (filled)
    {
        g.setColour (t.buttonActiveFill);
        g.fillPath (shape);
    }
    else if (highlighted || down)
    {
        g.setColour (t.buttonOutline.withAlpha (down ? 0.16f : 0.08f));
        g.fillPath (shape);
    }

    if (! isEnabled())
        g.setOpacity (0.4f);

    g.setColour (t.buttonOutline.withAlpha (0.6f));
    g.strokePath (shape, juce::PathStrokeType (1.0f));

    g.setColour (filled ? t.buttonActiveText : t.panelText);
    g.setFont (fonts::semiBold (fontSize));
    g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);
}

// =============================================================================================
BypassButton::BypassButton (UiContext& c)
    : ctx (c),
      attachment (c.processor.getBypass(), [this] (float) { repaint(); }, nullptr)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle ("Bypass");
    setTooltip ("Bypass (the same switch as your DAW's bypass)");
    attachment.sendInitialUpdate();
}

void BypassButton::paint (juce::Graphics& g)
{
    const auto& t = ctx.theme;
    const bool on = ctx.processor.getBypass().get();
    auto b = getLocalBounds().toFloat();

    // label
    g.setFont (fonts::extraBold (11.0f).withExtraKerningFactor (0.1f));
    g.setColour (t.text);
    g.drawText ("BYPASS", b.withTrimmedRight (34.0f), juce::Justification::centredRight, false);

    auto circle = juce::Rectangle<float> (b.getRight() - 27.0f, b.getCentreY() - 13.0f, 26.0f, 26.0f);
    if (t.barGlow > 0.0f)
    {
        juce::Path p; p.addEllipse (circle);
        juce::DropShadow (t.frameGlowColor.withAlpha (t.barGlow), 10, {}).drawForPath (g, p);
    }
    g.setColour (on ? t.buttonActiveFill : t.barFill);
    g.fillEllipse (circle);
    g.setColour (t.buttonOutline);
    g.drawEllipse (circle.reduced (0.8f), 1.6f);

    // power icon
    auto c = circle.getCentre();
    juce::Path icon;
    icon.addCentredArc (c.x, c.y + 0.5f, 4.2f, 4.2f, 0.0f, juce::degreesToRadians (35.0f), juce::degreesToRadians (325.0f), true);
    icon.startNewSubPath (c.x, c.y - 5.0f);
    icon.lineTo (c.x, c.y - 0.5f);
    g.setColour (on ? t.buttonActiveText : t.text);
    g.strokePath (icon, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void BypassButton::mouseUp (const juce::MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()))
        attachment.setValueAsCompleteGesture (ctx.processor.getBypass().get() ? 0.0f : 1.0f);
}

// =============================================================================================
ParamControl::ParamControl (UiContext& c, const ControlSpec& s)
    : ctx (c), spec (s),
      attachment (c.processor.getSlot (s.param), [this] (float v) { value = v; repaint(); }, nullptr)
{
    edge = knobOutline (ctx.theme.knobStyle, 22.0f, spec.param + 1);
    edgeStyle = ctx.theme.knobStyle;
    attachment.sendInitialUpdate();
    setTitle (spec.label);
    setRepaintsOnMouseActivity (false);

    juce::String tip;
    if (auto* fx = ctx.processor.getEffectHost())
    {
        const auto original = fx->getParamNames()[spec.param];
        if (original != spec.label)
            tip << "Airwindows name: " << original << "\n";
    }
    tip << (spec.type == ControlType::knob ? "Drag (Shift = fine) or scroll. Double-click: default.\nDouble-click the value to type it."
                                           : "Click or drag to choose. Double-click: default.");
    setTooltip (tip);
}

ParamControl::~ParamControl() = default;

juce::Rectangle<float> ParamControl::controlArea() const
{
    auto b = getLocalBounds().toFloat();
    const float total = 18.0f + 2.0f + 64.0f + 2.0f + 18.0f;
    const float top = b.getCentreY() - total * 0.5f;
    return { b.getCentreX() - 32.0f, top + 20.0f, 64.0f, 64.0f };
}

juce::Rectangle<float> ParamControl::valueArea() const
{
    auto k = controlArea();
    return { (float) getLocalBounds().getX(), k.getBottom() + 2.0f, (float) getWidth(), 18.0f };
}

int ParamControl::stepIndex (float v) const
{
    const int n = juce::jmax (2, spec.steps);
    return juce::jlimit (0, n - 1, (int) std::floor (v * (float) n));
}

float ParamControl::snap (float v) const
{
    if (spec.type == ControlType::knob)
        return juce::jlimit (0.0f, 1.0f, v);
    const int n = juce::jmax (2, spec.steps);
    return ((float) stepIndex (juce::jlimit (0.0f, 1.0f, v)) + 0.5f) / (float) n; // centre of each choice
}

juce::String ParamControl::valueText() const
{
    if (auto* fx = ctx.processor.getEffectHost())
        return formatValue (fx->getDisplayText (spec.param, value), hover || dragging);
    return {};
}

void ParamControl::paint (juce::Graphics& g)
{
    const auto& t = ctx.theme;
    const bool active = hover || dragging;
    auto k = controlArea();
    const float s = k.getWidth() / 64.0f;
    auto c = k.getCentre();
    auto at = [&] (float x, float y) { return juce::Point<float> (k.getX() + x * s, k.getY() + y * s); };

    // name
    g.setColour (t.text);
    auto nameFont = fonts::semiBold (13.0f);
    g.setFont (nameFont);
    auto nameArea = juce::Rectangle<float> (0.0f, k.getY() - 20.0f, (float) getWidth(), 18.0f);
    g.drawText (spec.label, nameArea, juce::Justification::centred, true);
    if (active)
    {
        const float w = juce::jmin ((float) getWidth() - 8.0f, fonts::width (nameFont, spec.label));
        g.fillRect (juce::Rectangle<float> (nameArea.getCentreX() - w * 0.5f, nameArea.getBottom() - 1.0f, w, 1.6f));
    }

    const float angle = -135.0f + 270.0f * value;

    if (spec.type == ControlType::toggle)
    {
        const bool on = value >= 0.5f;
        juce::Path lane;
        lane.addRoundedRectangle (juce::Rectangle<float> (at (25, 8), at (39, 56)), 7.0f * s);
        g.setColour (t.knobTicks.withAlpha (0.06f));
        g.fillPath (lane);
        g.setColour (t.knobTicks);
        g.strokePath (lane, juce::PathStrokeType (1.6f));
        g.drawLine ({ at (21, 10), at (24, 10) }, 1.6f);
        g.drawLine ({ at (21, 54), at (24, 54) }, 1.6f);

        const float cy = on ? 19.0f : 45.0f;
        g.setColour (t.knobShadow);
        g.fillEllipse (juce::Rectangle<float> (21.5f * s, 21.0f * s).withCentre (at (32, cy + 2.5f)));
        auto cap = edge;
        cap.applyTransform (juce::AffineTransform::scale (10.5f / 22.0f * s).translated (at (32, cy)));
        g.setColour (t.knobFill);
        g.fillPath (cap);
        g.setColour (t.knobEdge);
        g.strokePath (cap, juce::PathStrokeType (1.3f));
    }
    else
    {
        if (spec.type == ControlType::stepped)
        {
            const int n = juce::jmax (2, spec.steps);
            g.setColour (t.knobTicks);
            for (int i = 0; i < n; ++i)
            {
                const float a = -135.0f + 270.0f * (float) i / (float) (n - 1);
                if (n <= 12)
                    g.fillEllipse (juce::Rectangle<float> (4.4f * s, 4.4f * s).withCentre (polar (c, 29.0f * s, a)));
                else
                    g.drawLine ({ polar (c, 26.5f * s, a), polar (c, 30.0f * s, a) }, 1.4f);
            }
        }
        else
        {
            g.setColour (t.knobTicks);
            g.drawLine ({ polar (c, 25.0f * s, -135.0f), polar (c, 29.0f * s, -135.0f) }, 1.8f);
            g.drawLine ({ polar (c, 25.0f * s, 135.0f), polar (c, 29.0f * s, 135.0f) }, 1.8f);
        }

        g.setColour (t.knobShadow);
        g.fillEllipse (juce::Rectangle<float> (44.0f * s, 44.0f * s).withCentre (c.translated (0.0f, 2.5f * s)));

        if (edgeStyle != t.knobStyle)   // theme changed while open
        {
            edge = knobOutline (t.knobStyle, 22.0f, spec.param + 1);
            edgeStyle = t.knobStyle;
        }
        auto disc = edge;
        disc.applyTransform (juce::AffineTransform::scale (s).translated (c));
        g.setColour (t.knobFill);
        g.fillPath (disc);
        g.setColour (t.knobEdge);
        g.strokePath (disc, juce::PathStrokeType (t.knobStyle == "soft" ? 1.3f : 1.5f));
        drawKnobEdgeDetail (g, t.knobStyle, c, 22.0f, s, spec.param + 1, t.knobEdge, t.knobFill);

        g.setColour (t.knobPointer);
        if (spec.type == ControlType::stepped)
        {
            const float a = -135.0f + 270.0f * (float) stepIndex (value) / (float) (juce::jmax (2, spec.steps) - 1);
            juce::Path tri;
            tri.addTriangle (polar (c, 20.0f * s, a), polar (c, 12.5f * s, a - 14.0f), polar (c, 12.5f * s, a + 14.0f));
            g.fillPath (tri);
        }
        else
        {
            g.drawLine ({ polar (c, 3.0f * s, angle), polar (c, 18.0f * s, angle) }, 2.6f);
        }
    }

    // value
    g.setColour (t.text);
    g.setFont (active ? fonts::extraBold (13.0f) : fonts::semiBold (13.0f));
    g.drawText (valueText(), valueArea(), juce::Justification::centred, true);
}

void ParamControl::resized()
{
    if (editor != nullptr)
        editor->setBounds (valueArea().toNearestInt().reduced (6, 0));
}

void ParamControl::mouseEnter (const juce::MouseEvent&) { hover = true; repaint(); }
void ParamControl::mouseExit (const juce::MouseEvent&)  { hover = false; repaint(); }

void ParamControl::setFromGesture (float v)
{
    v = snap (v);
    if (v != value)
        attachment.setValueAsPartOfGesture (v);
}

void ParamControl::mouseDown (const juce::MouseEvent& e)
{
    if (! controlArea().expanded (6.0f).contains (e.position))
        return;

    if (spec.type == ControlType::toggle)
    {
        attachment.setValueAsCompleteGesture (value >= 0.5f ? 0.25f : 0.75f);
        return;
    }

    dragging = true;
    dragFine = e.mods.isShiftDown();
    dragStartValue = value;
    dragAnchorY = e.position.y;
    attachment.beginGesture();
    repaint();
}

void ParamControl::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    const bool fine = e.mods.isShiftDown();
    if (fine != dragFine)
    {
        // re-anchor so switching precision never jumps the value
        dragFine = fine;
        dragStartValue = value;
        dragAnchorY = e.position.y;
    }

    const float pixels = kDragPixels * (fine ? kFineFactor : 1.0f);
    float v = dragStartValue + (dragAnchorY - e.position.y) / pixels;

    if (spec.type == ControlType::stepped)
    {
        // one step per 30 px
        const int n = juce::jmax (2, spec.steps);
        const int delta = (int) std::round ((dragAnchorY - e.position.y) / 30.0f);
        v = ((float) juce::jlimit (0, n - 1, stepIndex (dragStartValue) + delta) + 0.5f) / (float) n;
    }

    setFromGesture (juce::jlimit (0.0f, 1.0f, v));
}

void ParamControl::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
    {
        attachment.endGesture();
        dragging = false;
        repaint();
    }
}

void ParamControl::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (valueArea().contains (e.position))
    {
        // type an exact value
        editor = std::make_unique<juce::TextEditor>();
        editor->setFont (fonts::semiBold (13.0f));
        editor->setJustification (juce::Justification::centred);
        editor->setText (formatValue (valueText(), true), false);
        editor->selectAll();
        editor->addListener (this);
        addAndMakeVisible (*editor);
        resized();
        editor->grabKeyboardFocus();
        return;
    }

    if (controlArea().expanded (6.0f).contains (e.position) && spec.type != ControlType::toggle)
        if (auto* fx = ctx.processor.getEffectHost())
            attachment.setValueAsCompleteGesture (snap (fx->getDefault (spec.param)));
}

void ParamControl::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const float dir = (w.deltaY != 0.0f ? w.deltaY : w.deltaX) > 0.0f ? 1.0f : -1.0f;
    if (w.deltaY == 0.0f && w.deltaX == 0.0f)
        return;

    float v = value;
    if (spec.type == ControlType::knob)
        v += dir * kWheelStep * (e.mods.isShiftDown() ? 0.1f : 1.0f);
    else
    {
        const int n = juce::jmax (2, spec.steps);
        v = ((float) juce::jlimit (0, n - 1, stepIndex (value) + (int) dir) + 0.5f) / (float) n;
    }
    attachment.setValueAsCompleteGesture (snap (juce::jlimit (0.0f, 1.0f, v)));
}

void ParamControl::textEditorReturnKeyPressed (juce::TextEditor&) { closeEditor (true); }
void ParamControl::textEditorEscapeKeyPressed (juce::TextEditor&) { closeEditor (false); }
void ParamControl::textEditorFocusLost (juce::TextEditor&)        { closeEditor (false); }

void ParamControl::closeEditor (bool apply)
{
    if (editor == nullptr)
        return;

    const auto text = editor->getText();
    // Delete asynchronously: we may be inside one of the editor's own callbacks.
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<ParamControl> (this), ed = editor.release()]
    {
        delete ed;
        if (safe != nullptr) safe->repaint();
    });

    if (! apply)
        return;

    float v = 0.0f;
    if (auto* fx = ctx.processor.getEffectHost())
        if (fx->textToValue (spec.param, text, v))
            attachment.setValueAsCompleteGesture (snap (v));
}
} // namespace dlaw
