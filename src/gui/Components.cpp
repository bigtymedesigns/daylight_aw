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

juce::String formatDb (float db)
{
    if (juce::exactlyEqual (db, 0.0f))
        return "0.0";
    return (db > 0.0f ? "+" : juce::String::fromUTF8 ("\xe2\x88\x92")) + juce::String (std::abs (db), 1);   // real minus sign
}
} // namespace

// =============================================================================================
namespace draw
{
void knob (juce::Graphics& g, const Theme& t, juce::Point<float> c, float d, float v, float ringWidth, float pointerWidth)
{
    // proportions from the design: ring r = 15/40, pointer from 5/40 to 13/40 of the knob size
    const float r = d * 15.0f / 40.0f;
    if (t.knobStyle == "round" || ! t.knobFill.isTransparent())
    {
        g.setColour (t.knobFill);
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
    }
    g.setColour (t.knobRing);
    g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), ringWidth);

    const float angle = -135.0f + 270.0f * juce::jlimit (0.0f, 1.0f, v);
    juce::Path p;
    p.startNewSubPath (polar (c, d * 5.0f / 40.0f, angle));
    p.lineTo (polar (c, d * 13.0f / 40.0f, angle));
    g.setColour (t.knobPointer);
    g.strokePath (p, juce::PathStrokeType (pointerWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void chevron (juce::Graphics& g, float x, float y, bool up, juce::Colour colour, float s)
{
    juce::Path p;
    if (up) { p.startNewSubPath (x - s, y + s * 0.45f); p.lineTo (x, y - s * 0.55f); p.lineTo (x + s, y + s * 0.45f); }
    else    { p.startNewSubPath (x - s, y - s * 0.45f); p.lineTo (x, y + s * 0.55f); p.lineTo (x + s, y - s * 0.45f); }
    g.setColour (colour);
    g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void star (juce::Graphics& g, juce::Point<float> c, float radius, bool filled, juce::Colour colour)
{
    juce::Path p;
    p.addStar (c, 5, radius * 0.42f, radius);
    g.setColour (colour);
    if (filled) g.fillPath (p);
    else        g.strokePath (p, juce::PathStrokeType (1.1f));
}
} // namespace draw

// =============================================================================================
void DaylightLookAndFeel::applyTheme()
{
    const auto& t = ctx.theme;
    setColour (juce::TextEditor::textColourId, t.panelText);
    setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::highlightColourId, t.accent.withAlpha (0.3f));
    setColour (juce::TextEditor::highlightedTextColourId, t.panelText);
    setColour (juce::CaretComponent::caretColourId, t.panelText);
    setColour (juce::TextEditor::outlineColourId, t.buttonBorder);
    setColour (juce::TextEditor::focusedOutlineColourId, t.accent);
    setColour (juce::Label::textColourId, t.panelText);
    setColour (juce::ScrollBar::thumbColourId, t.panelText.withAlpha (0.35f));
    setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::PopupMenu::backgroundColourId, t.panelFill.withAlpha (1.0f));
    setColour (juce::PopupMenu::textColourId, t.panelText);
    setColour (juce::PopupMenu::headerTextColourId, t.muted);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, t.selectFill.withAlpha (1.0f).interpolatedWith (t.panelFill.withAlpha (1.0f), 0.4f));
    setColour (juce::PopupMenu::highlightedTextColourId, t.selectText);
}

juce::Font DaylightLookAndFeel::getLabelFont (juce::Label&) { return fonts::medium (13.0f); }
juce::Font DaylightLookAndFeel::getPopupMenuFont()           { return fonts::medium (13.0f); }

void DaylightLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    const auto& t = ctx.theme;
    g.fillAll (t.panelFill.withAlpha (1.0f));
    g.setColour (t.panelBorder);
    g.drawRect (0, 0, width, height, 1);
}

void DaylightLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    g.setColour (ctx.theme.buttonFill);
    g.fillRoundedRectangle (0.0f, 0.0f, (float) w, (float) h, ctx.theme.corner);
}

void DaylightLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& e)
{
    g.setColour (e.hasKeyboardFocus (true) ? ctx.theme.accent.withAlpha (0.7f) : ctx.theme.buttonBorder);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1.0f, (float) h - 1.0f, ctx.theme.corner, 1.0f);
}

void DaylightLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                         int thumbStart, int thumbSize, bool over, bool)
{
    g.setColour (ctx.theme.panelText.withAlpha (over ? 0.40f : 0.22f));
    auto r = vertical ? juce::Rectangle<int> (x + w / 2 - 2, thumbStart, 4, thumbSize)
                      : juce::Rectangle<int> (thumbStart, y + h / 2 - 2, thumbSize, 4);
    g.fillRoundedRectangle (r.toFloat(), 2.0f);
}

// =============================================================================================
PillButton::PillButton (UiContext& c, const juce::String& buttonText, bool filledWhenOn)
    : juce::Button (buttonText), ctx (c), fillOn (filledWhenOn)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void PillButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto& t = ctx.theme;
    if (! isEnabled())
        g.beginTransparencyLayer (0.35f);

    if (link)
    {
        g.setColour (highlighted ? t.accent.brighter (0.25f) : t.accent);
        g.setFont (fonts::medium (fontSize));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centredLeft, true);
        if (! isEnabled()) g.endTransparencyLayer();
        return;
    }

    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const bool filled = primary || (fillOn && getToggleState());
    const float radius = t.corner;

    juce::Path shape;
    shape.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radius, radius,
                               ! joinedLeft, ! joinedRight, ! joinedLeft, ! joinedRight);

    if (filled)
    {
        g.setColour (t.buttonActiveFill);
        g.fillPath (shape);
    }
    else
    {
        g.setColour (t.buttonFill);
        g.fillPath (shape);
        if (highlighted || down)
        {
            g.setColour (t.buttonText.withAlpha (down ? 0.10f : 0.05f));
            g.fillPath (shape);
        }
        if (! joinedLeft && ! joinedRight)
        {
            g.setColour (t.buttonBorder);
            g.strokePath (shape, juce::PathStrokeType (1.0f));
        }
    }

    const auto textColour = filled ? t.buttonActiveText : t.buttonText;
    g.setColour (textColour);
    g.setFont (filled || joinedLeft || joinedRight ? fonts::semiBold (fontSize) : fonts::medium (fontSize));
    auto area = getLocalBounds().reduced (10, 0);

    if (icon != Icon::none)
    {
        const auto ic = area.removeFromLeft (14).toFloat().withSizeKeepingCentre (12.0f, 12.0f);
        area.removeFromLeft (6);
        juce::Path p;
        if (icon == Icon::reset)
        {
            // circular arrow
            p.addCentredArc (ic.getCentreX(), ic.getCentreY(), 4.8f, 4.8f, 0.0f, juce::degreesToRadians (-40.0f), juce::degreesToRadians (250.0f), true);
            g.strokePath (p, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            const auto tip = polar (ic.getCentre(), 4.8f, -40.0f);
            juce::Path head;
            head.addTriangle (tip.translated (-3.2f, -1.6f), tip.translated (0.6f, -3.4f), tip.translated (0.9f, 0.9f));
            g.fillPath (head);
        }
        else if (icon == Icon::change)
        {
            // two arrows: swap
            const auto c = ic.getCentre();
            juce::Path a;
            a.startNewSubPath (c.x - 5.0f, c.y - 2.5f); a.lineTo (c.x + 4.5f, c.y - 2.5f);
            a.startNewSubPath (c.x + 5.0f, c.y + 2.5f); a.lineTo (c.x - 4.5f, c.y + 2.5f);
            a.startNewSubPath (c.x + 2.0f, c.y - 5.0f); a.lineTo (c.x + 4.8f, c.y - 2.5f); a.lineTo (c.x + 2.0f, c.y);
            a.startNewSubPath (c.x - 2.0f, c.y);        a.lineTo (c.x - 4.8f, c.y + 2.5f); a.lineTo (c.x - 2.0f, c.y + 5.0f);
            g.strokePath (a, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            // "i" in a circle
            g.drawEllipse (ic.reduced (0.6f), 1.2f);
            g.fillRect (juce::Rectangle<float> (1.4f, 4.2f).withCentre (ic.getCentre().translated (0.0f, 1.2f)));
            g.fillEllipse (juce::Rectangle<float> (1.8f, 1.8f).withCentre (ic.getCentre().translated (0.0f, -2.6f)));
        }
        g.drawText (getButtonText(), area, juce::Justification::centredLeft, true);
    }
    else if (dropdown)
    {
        g.drawText (getButtonText(), area.withTrimmedRight (14), juce::Justification::centredLeft, true);
        draw::chevron (g, (float) area.getRight() - 4.0f, (float) getHeight() * 0.5f, false, t.buttonText.withAlpha (0.8f), 3.0f);
    }
    else
    {
        g.drawText (getButtonText(), getLocalBounds().reduced (4, 0), juce::Justification::centred, true);
    }

    if (! isEnabled())
        g.endTransparencyLayer();
}

// =============================================================================================
BypassButton::BypassButton (UiContext& c)
    : ctx (c),
      attachment (c.processor.getBypass(), [this] (float) { repaint(); }, nullptr)
{
    setTitle ("Bypass");
    attachment.sendInitialUpdate();
}

int BypassButton::preferredWidth() const
{
    return (int) std::ceil (fonts::width (fonts::caps (11.0f, 0.06f), "BYPASS") + 7.0f + kButtonSize) + 2;
}

juce::Rectangle<float> BypassButton::circle() const
{
    auto b = getLocalBounds().toFloat();
    return { b.getRight() - kButtonSize - 1.0f, b.getCentreY() - kButtonSize * 0.5f, kButtonSize, kButtonSize };
}

bool BypassButton::hitTest (int x, int y)
{
    const auto c = circle();
    return c.getCentre().getDistanceFrom ({ (float) x, (float) y }) <= c.getWidth() * 0.5f + 1.5f;
}

void BypassButton::paint (juce::Graphics& g)
{
    const auto& t = ctx.theme;
    const bool on = ctx.processor.getBypass().get();
    const auto c = circle();

    g.setFont (fonts::caps (11.0f, 0.06f));
    g.setColour (on ? t.accent : t.muted);
    g.drawText ("BYPASS", getLocalBounds().toFloat().withRight (c.getX() - 7.0f), juce::Justification::centredRight, false);

    if (on)
    {
        g.setColour (t.buttonActiveFill);
        g.fillEllipse (c);
    }
    g.setColour (on ? t.buttonActiveFill : t.buttonText.withAlpha (0.28f));
    g.drawEllipse (c.reduced (0.75f), 1.5f);

    // power icon
    const auto m = c.getCentre();
    juce::Path icon;
    icon.addCentredArc (m.x, m.y + 0.4f, 3.6f, 3.6f, 0.0f, juce::degreesToRadians (40.0f), juce::degreesToRadians (320.0f), true);
    icon.startNewSubPath (m.x, m.y - 4.6f);
    icon.lineTo (m.x, m.y - 0.6f);
    g.setColour (on ? t.buttonActiveText : t.buttonText.withAlpha (0.85f));
    g.strokePath (icon, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void BypassButton::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu() && hitTest (e.getPosition().x, e.getPosition().y))
        attachment.setValueAsCompleteGesture (ctx.processor.getBypass().get() ? 0.0f : 1.0f);
}

// =============================================================================================
TrimKnob::TrimKnob (UiContext& c, juce::AudioParameterFloat& p, const juce::String& l)
    : ctx (c), param (p), label (l),
      attachment (p, [this] (float v) { db = v; repaint(); }, nullptr)
{
    attachment.sendInitialUpdate();
    setTitle (label == "IN" ? "Input trim" : "Output trim");
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

int TrimKnob::preferredWidth() const
{
    return (int) std::ceil (fonts::width (fonts::caps (11.0f, 0.06f), label)) + 6 + 22 + 6 + 32;
}

juce::Rectangle<float> TrimKnob::knobArea() const
{
    const float x = std::ceil (fonts::width (fonts::caps (11.0f, 0.06f), label)) + 6.0f;
    return { x, (float) getHeight() * 0.5f - 11.0f, 22.0f, 22.0f };
}

void TrimKnob::set (float newDb, bool asGesture)
{
    newDb = juce::jlimit (param.range.start, param.range.end, std::round (newDb * 10.0f) / 10.0f);
    if (juce::exactlyEqual (newDb, db))
        return;
    if (asGesture)
        attachment.setValueAsPartOfGesture (newDb);
    else
        attachment.setValueAsCompleteGesture (newDb);
}

void TrimKnob::paint (juce::Graphics& g)
{
    const auto& t = ctx.theme;
    const auto k = knobArea();
    const bool active = hover || dragging;

    g.setFont (fonts::caps (11.0f, 0.06f));
    g.setColour (t.muted);
    g.drawText (label, juce::Rectangle<float> (0.0f, 0.0f, k.getX() - 6.0f, (float) getHeight()), juce::Justification::centredLeft, false);

    // the mini knob: a slightly thicker ring + pointer, like the design (2.5 / 3.5 at 40 px)
    draw::knob (g, t, k.getCentre(), k.getWidth(), param.convertTo0to1 (db), 1.4f, 1.9f);

    const bool zero = juce::exactlyEqual (db, 0.0f);
    g.setFont (active ? fonts::semiBold (12.0f) : fonts::regular (12.0f));
    g.setColour (zero ? t.text : t.accent);
    g.drawText (formatDb (db), juce::Rectangle<float> (k.getRight() + 6.0f, 0.0f, (float) getWidth() - k.getRight() - 6.0f, (float) getHeight()),
                juce::Justification::centredLeft, false);
}

void TrimKnob::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;
    rightDrag = e.mods.isPopupMenu();
    gestureOpen = false;
    dragFine = e.mods.isShiftDown() || rightDrag;
    dragStartDb = db;
    dragAnchorY = e.position.y;
    if (! rightDrag)
    {
        attachment.beginGesture();
        gestureOpen = true;
    }
    repaint();
}

void TrimKnob::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;
    if (! gestureOpen)
    {
        // a right click only becomes a fine drag once the mouse really moves
        if (std::abs (e.position.y - dragAnchorY) < ParamControl::kRightDragThreshold)
            return;
        attachment.beginGesture();
        gestureOpen = true;
    }
    const bool fine = e.mods.isShiftDown() || rightDrag;
    if (fine != dragFine)
    {
        dragFine = fine;          // re-anchor: switching precision never jumps the value
        dragStartDb = db;
        dragAnchorY = e.position.y;
    }
    const float range = param.range.end - param.range.start;
    const float pixels = ParamControl::kDragPixels * (fine ? ParamControl::kFineFactor : 1.0f);
    set (dragStartDb + (dragAnchorY - e.position.y) / pixels * range, true);
}

void TrimKnob::mouseUp (const juce::MouseEvent&)
{
    if (gestureOpen)
        attachment.endGesture();
    gestureOpen = dragging = rightDrag = false;
    repaint();
}

void TrimKnob::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        set (0.0f, false);
}

void TrimKnob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const float step = e.mods.isShiftDown() ? 0.1f : 0.5f;
    const float dir = (! juce::exactlyEqual (w.deltaY, 0.0f) ? w.deltaY : w.deltaX) * (w.isReversed ? -1.0f : 1.0f);
    if (! juce::exactlyEqual (dir, 0.0f))
        set (db + (dir > 0.0f ? step : -step), false);
}

// =============================================================================================
ParamControl::ParamControl (UiContext& c, const ControlSpec& s)
    : ctx (c), spec (s),
      attachment (c.processor.getSlot (s.param), [this] (float v) { value = v; repaint(); }, nullptr)
{
    attachment.sendInitialUpdate();
    setTitle (spec.label);
    setRepaintsOnMouseActivity (false);
}

ParamControl::~ParamControl() = default;

juce::Rectangle<float> ParamControl::controlArea() const
{
    // name 16 + 6 + knob 46 + 6 + value 15
    auto b = getLocalBounds().toFloat();
    const float total = 16.0f + 6.0f + kKnobSize + 6.0f + 15.0f;
    const float top = b.getCentreY() - total * 0.5f;
    return { b.getCentreX() - kKnobSize * 0.5f, top + 22.0f, kKnobSize, kKnobSize };
}

juce::Rectangle<float> ParamControl::valueArea() const
{
    auto k = controlArea();
    return { 0.0f, k.getBottom() + 5.0f, (float) getWidth(), 16.0f };
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
    const auto k = controlArea();
    const auto c = k.getCentre();

    // name
    g.setColour (active ? t.text : t.nameText);
    g.setFont (fonts::regular (13.0f));
    g.drawText (spec.label, juce::Rectangle<float> (0.0f, k.getY() - 22.0f, (float) getWidth(), 16.0f), juce::Justification::centred, true);

    if (spec.type == ControlType::toggle)
    {
        // a small vertical switch in the same outline style as the knobs
        const bool on = value >= 0.5f;
        auto lane = juce::Rectangle<float> (16.0f, 38.0f).withCentre (c);
        g.setColour (on ? t.knobPointer.withAlpha (0.18f) : t.knobFill);
        g.fillRoundedRectangle (lane, 8.0f);
        g.setColour (t.knobRing);
        g.drawRoundedRectangle (lane, 8.0f, 1.6f);
        auto cap = juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ c.x, on ? lane.getY() + 9.0f : lane.getBottom() - 9.0f });
        g.setColour (on ? t.knobPointer : t.knobRing.withMultipliedAlpha (1.6f));
        g.fillEllipse (cap);
    }
    else
    {
        if (spec.type == ControlType::stepped)
        {
            const int n = juce::jmax (2, spec.steps);
            const int current = stepIndex (value);
            for (int i = 0; i < n; ++i)
            {
                const float a = -135.0f + 270.0f * (float) i / (float) (n - 1);
                g.setColour (i == current ? t.knobPointer : t.knobTicks);
                if (n <= 12)
                    g.fillEllipse (juce::Rectangle<float> (3.4f, 3.4f).withCentre (polar (c, kKnobSize * 0.5f + 1.0f, a)));
                else
                    g.drawLine ({ polar (c, kKnobSize * 0.5f - 1.0f, a), polar (c, kKnobSize * 0.5f + 2.0f, a) }, 1.2f);
            }
            const float pos = (float) current / (float) (n - 1);
            draw::knob (g, t, c, kKnobSize, pos, 1.6f, 2.4f);
        }
        else
        {
            draw::knob (g, t, c, kKnobSize, value, 1.6f, 2.4f);
        }
    }

    // value
    g.setColour (active ? t.text : t.value);
    g.setFont (fonts::regular (12.0f));
    g.drawText (valueText(), valueArea(), juce::Justification::centred, true);
}

void ParamControl::resized()
{
    if (editor != nullptr)
        editor->setBounds (valueArea().toNearestInt().reduced (6, -2));
}

void ParamControl::mouseEnter (const juce::MouseEvent&) { hover = true; repaint(); }
void ParamControl::mouseExit (const juce::MouseEvent&)  { hover = false; repaint(); }

void ParamControl::setFromGesture (float v)
{
    v = snap (v);
    if (! juce::exactlyEqual (v, value))
        attachment.setValueAsPartOfGesture (v);
}

void ParamControl::mouseDown (const juce::MouseEvent& e)
{
    if (! controlArea().expanded (8.0f).contains (e.position))
        return;

    rightDrag = e.mods.isPopupMenu();
    if (spec.type == ControlType::toggle && ! rightDrag)
    {
        attachment.setValueAsCompleteGesture (value >= 0.5f ? 0.25f : 0.75f);
        return;
    }

    dragging = true;
    gestureOpen = false;
    dragFine = e.mods.isShiftDown() || rightDrag;
    dragStartValue = value;
    dragAnchorY = e.position.y;
    if (! rightDrag)
    {
        attachment.beginGesture();
        gestureOpen = true;
    }
    repaint();
}

void ParamControl::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    if (! gestureOpen)
    {
        // Right button: only a real movement starts a (fine) drag; a plain right-click does nothing.
        if (std::abs (e.position.y - dragAnchorY) < kRightDragThreshold)
            return;
        attachment.beginGesture();
        gestureOpen = true;
    }

    const bool fine = e.mods.isShiftDown() || rightDrag;
    if (fine != dragFine)
    {
        // re-anchor so switching precision never jumps the value
        dragFine = fine;
        dragStartValue = value;
        dragAnchorY = e.position.y;
    }

    const float pixels = kDragPixels * (fine ? kFineFactor : 1.0f);
    float v = dragStartValue + (dragAnchorY - e.position.y) / pixels;

    if (spec.type != ControlType::knob)
    {
        // one step per 30 px (fine: per 90 px)
        const int n = juce::jmax (2, spec.steps);
        const int delta = (int) std::round ((dragAnchorY - e.position.y) / (fine ? 90.0f : 30.0f));
        v = ((float) juce::jlimit (0, n - 1, stepIndex (dragStartValue) + delta) + 0.5f) / (float) n;
    }

    setFromGesture (juce::jlimit (0.0f, 1.0f, v));
}

void ParamControl::mouseUp (const juce::MouseEvent&)
{
    if (gestureOpen)
        attachment.endGesture();
    if (dragging)
        repaint();
    gestureOpen = dragging = rightDrag = false;
}

void ParamControl::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
        return;

    if (valueArea().contains (e.position))
    {
        // type an exact value
        editor = std::make_unique<juce::TextEditor>();
        editor->setFont (fonts::regular (12.0f));
        editor->setJustification (juce::Justification::centred);
        editor->setText (formatValue (valueText(), true), false);
        editor->selectAll();
        editor->addListener (this);
        addAndMakeVisible (*editor);
        resized();
        editor->grabKeyboardFocus();
        return;
    }

    if (controlArea().expanded (8.0f).contains (e.position) && spec.type != ControlType::toggle)
        if (auto* fx = ctx.processor.getEffectHost())
            attachment.setValueAsCompleteGesture (snap (fx->getDefault (spec.param)));
}

void ParamControl::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const float dir = (! juce::exactlyEqual (w.deltaY, 0.0f) ? w.deltaY : w.deltaX) > 0.0f ? 1.0f : -1.0f;
    if (juce::exactlyEqual (w.deltaY, 0.0f) && juce::exactlyEqual (w.deltaX, 0.0f))
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
