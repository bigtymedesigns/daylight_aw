#include <algorithm>
#include "PluginEditor.h"
#include "BinaryData.h"
#include "Fonts.h"

namespace dlaw
{
namespace
{
constexpr int kTopRowY = 10, kTopRowH = 40;
constexpr int kListTop = 82, kListRowH = 26;   // list view (Original): first fader row, row height
constexpr int kRowsTop = 62, kRowH = 146, kGap = 10, kMargin = 12;
constexpr float kSlotW = (float) (kWindowWidth - 2 * kMargin - (kControlsPerRow - 1) * kGap) / (float) kControlsPerRow;


juce::String menuLabel (const DaylightProcessor& p)
{
    if (auto* fx = p.getEffectHost())
    {
        juce::String label (fx->getInfo().name);
        if (p.getPresetName().isNotEmpty())
            label << juce::String::fromUTF8 (" \xc2\xb7 ") << p.getPresetName() << (p.isPresetModified() ? "*" : "");
        return label;
    }
    return p.hasUnknownEffect() ? p.getUnknownEffectId() : juce::String ("Choose effect");
}

juce::String layoutJsonFor (const juce::String& effectId)
{
    int size = 0;
    if (auto* data = BinaryData::getNamedResource ((effectId + "_json").toRawUTF8(), size))
        return juce::String::fromUTF8 (data, size);
    return {};
}

void drawGlowingText (juce::Graphics& g, const juce::String& text, const juce::Font& font, float x, float baseline,
                      juce::Colour colour, juce::Colour glow, float glowAmount)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (font, text, x, baseline);
    juce::Path p;
    ga.createPath (p);
    if (glowAmount > 0.0f)
        juce::DropShadow (glow.withAlpha (glowAmount), 10, {}).drawForPath (g, p);
    g.setColour (colour);
    g.fillPath (p);
}
} // namespace

// =============================================================================================
class DaylightEditor::Content : public juce::Component,
                                private juce::Timer
{
public:
    explicit Content (UiContext& c)
        : ctx (c), backdrop (*this), menuBar (*this), bypass (c),
          abA (c, "A"), abB (c, "B")
    {
        addAndMakeVisible (backdrop);
        addAndMakeVisible (logo);
        addAndMakeVisible (menuBar);
        addAndMakeVisible (bypass);

        logo.onClick = [this] { toggleAbout(); };
        logo.setTooltip ("About Daylight_aw");
        abA.setTooltip ("Setting A. Switch between A and B to compare two settings.");
        abB.setTooltip ("Setting B. Switch between A and B to compare two settings.");
        logo.ctx = &ctx;

        abA.joinedRight = true;
        abB.joinedLeft = true;
        abA.onClick = [this] { ctx.processor.switchAB (0); refreshTopRow(); };
        abB.onClick = [this] { ctx.processor.switchAB (1); refreshTopRow(); };
        menuBar.addAndMakeVisible (abA);
        menuBar.addAndMakeVisible (abB);

        rebuild();

        if (! ctx.processor.hasEffect() && ! ctx.processor.hasUnknownEffect())
            openSelector();

        startTimerHz (5);   // keeps the "*" (preset edited) marker current
        updateTooltips();
    }

    /** Processor sent a change: rebuild only when the effect itself changed, else just refresh. */
    void processorChanged()
    {
        if (ctx.processor.getEffectHost() != builtHost || ctx.processor.hasUnknownEffect() != builtUnknown)
        {
            rebuild();
            return;
        }
        refreshTopRow();
        if (settingsPanel != nullptr)
            settingsPanel->refresh();
    }

    ~Content() override { stopTimer(); }

    int contentHeight() const
    {
        if (! ctx.processor.hasEffect())
            return ctx.processor.hasUnknownEffect() ? kSmallHeight : kLargeHeight;
        return layout.small ? kSmallHeight : kLargeHeight;
    }

    // Called on effect change, theme change, page change.
    void rebuild()
    {
        builtHost = ctx.processor.getEffectHost();
        builtUnknown = ctx.processor.hasUnknownEffect();
        builtListView = ctx.theme.listView;
        controls.clear();
        tabs.clear();
        frames.clear();

        if (auto* fx = ctx.processor.getEffectHost())
        {
            std::vector<ParamFacts> facts;
            for (int i = 0; i < fx->getNumParams(); ++i)
                facts.push_back ({ fx->getParamNames()[i], builtListView ? 0 : fx->detectSteps (i) });

            layout = builtListView ? buildListLayout (facts) : buildLayout (facts, layoutJsonFor (fx->getInfo().id));
            for ([[maybe_unused]] auto& w : layout.warnings)
                DBG ("Layout warning (" << fx->getInfo().id << "): " << w);

            page = juce::jlimit (0, juce::jmax (0, (int) layout.pages.size() - 1), ctx.processor.lastTab.load());
            buildPage();

            if (layout.pages.size() > 1)
                for (int i = 0; i < (int) layout.pages.size(); ++i)
                {
                    auto* t = tabs.add (new TabButton (*this, i));
                    t->setTooltip (layout.pages[(size_t) i].name.isNotEmpty() ? layout.pages[(size_t) i].name
                                                                              : "Page " + juce::String (i + 1));
                    addAndMakeVisible (t);
                }
        }
        else
        {
            layout = {};
        }

        refreshTopRow();
        setSize (kWindowWidth, contentHeight());
        resized();
        backdrop.repaint();
    }

    void refreshTopRow()
    {
        const bool has = ctx.processor.hasEffect();
        menuBar.setTooltip (has ? "Presets and settings" : ctx.processor.hasUnknownEffect() ? "" : "Choose an effect");
        abA.setEnabled (has);
        abB.setEnabled (has);
        abA.setToggleState (has && ctx.processor.getActiveAB() == 0, juce::dontSendNotification);
        abB.setToggleState (has && ctx.processor.getActiveAB() == 1, juce::dontSendNotification);
        menuBar.repaint();
        logo.repaint();
        bypass.repaint();
    }

    void themeChanged()
    {
        if (ctx.theme.listView != builtListView)
        {
            // a different kind of view (designed <-> list): build the controls again
            rebuild();
        }
        // Same geometry, new colours: just repaint everything (cached layers are re-rendered).
        backdrop.repaint();
        if (selector != nullptr) selector->refresh();
        if (settingsPanel != nullptr) settingsPanel->refresh();
        if (info != nullptr) info->refresh();
        updateTooltips();
        repaintAll (*this);
    }

    static void repaintAll (juce::Component& c)
    {
        c.repaint();
        for (auto* child : c.getChildren())
            repaintAll (*child);
    }

    void resized() override
    {
        backdrop.setBounds (getLocalBounds());
        logo.setBounds (kMargin, kTopRowY, 160, kTopRowH);
        menuBar.setBounds ((getWidth() - 340) / 2, kTopRowY + 5, 340, 30);
        bypass.setBounds (getWidth() - kMargin - 150, kTopRowY + 5, 150, 30);

        auto ab = menuBar.getLocalBounds().reduced (4, 4).removeFromRight (48);
        abA.setBounds (ab.removeFromLeft (24));
        abB.setBounds (ab);

        layoutPage();

        const int n = tabs.size();
        for (int i = 0; i < n; ++i)
            tabs[i]->setBounds (getWidth() / 2 - (n * 24 + (n - 1) * 10) / 2 + i * 34, getHeight() - 47, 24, 24);

        auto panelArea = juce::Rectangle<int> (kMargin, 56, getWidth() - 2 * kMargin, getHeight() - 56 - 8);
        if (selector != nullptr)
            selector->setBounds (panelArea.withTrimmedBottom (getHeight() >= kLargeHeight ? 48 : 0));
        if (settingsPanel != nullptr)
            settingsPanel->setBounds (panelArea.withHeight (juce::jmin (panelArea.getHeight(), 300)));
        if (about != nullptr)
            about->setBounds (kMargin, 50, 300, 230);
        if (info != nullptr)
            info->setBounds (panelArea.withTrimmedBottom (getHeight() >= kLargeHeight ? 48 : 0));
        if (confirm != nullptr)
            confirm->setBounds (getLocalBounds());
    }

    // ---- overlays -------------------------------------------------------------------------
    void toggleMenu()
    {
        if (selector != nullptr || settingsPanel != nullptr || info != nullptr)
        {
            closePanels();
            return;
        }
        if (ctx.processor.hasEffect())
            openSettings();
        else if (! ctx.processor.hasUnknownEffect())
            openSelector();
    }

    void closePanels()
    {
        selector.reset();
        settingsPanel.reset();
        about.reset();
        info.reset();
        menuBar.repaint();
        backdrop.repaint();
    }

    void openSelector()
    {
        closePanels();
        selector = std::make_unique<SelectorPanel> (ctx, [this] (const EffectInfo& e) { askToLoad (e); });
        addAndMakeVisible (*selector);
        resized();
        menuBar.repaint();
        backdrop.repaint();
    }

    void openSettings()
    {
        closePanels();
        settingsPanel = std::make_unique<SettingsPanel> (ctx, [this] { if (onSettingsChanged) onSettingsChanged(); },
                                                         [this] { juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<Content> (this)]
                                                                  { if (safe != nullptr) safe->openInfo(); }); });
        addAndMakeVisible (*settingsPanel);
        resized();
        menuBar.repaint();
    }

    void openInfo()
    {
        if (! ctx.processor.hasEffect())
            return;
        closePanels();
        info = std::make_unique<InfoPanel> (ctx, [this] { info.reset(); menuBar.repaint(); });
        addAndMakeVisible (*info);
        resized();
        menuBar.repaint();
    }

    void toggleAbout()
    {
        const bool wasOpen = about != nullptr;
        closePanels();
        if (wasOpen)
            return;
        about = std::make_unique<AboutPanel> (ctx, [this] { about.reset(); });
        addAndMakeVisible (*about);
        resized();
    }

    void askToLoad (const EffectInfo& e)
    {
        confirm = std::make_unique<ConfirmDialog> (ctx, e, [this, id = std::string (e.id)] (bool ok)
        {
            juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<Content> (this), id, ok]
            {
                if (safe == nullptr) return;
                safe->confirm.reset();
                if (! ok) return;
                if (safe->ctx.processor.selectEffect (id))
                {
                    safe->ctx.settings.addRecent (id);
                    safe->closePanels();
                    // the processor's change message triggers rebuild + resize
                }
            });
        });
        addAndMakeVisible (*confirm);
        resized();
        confirm->grabKeyboardFocus();
    }

    std::function<void()> onSettingsChanged;

    // ---- pieces ------------------------------------------------------------------------------
    struct Frame { juce::Rectangle<float> bounds; juce::String title; };

    struct Logo : public juce::Button
    {
        Logo() : juce::Button ("Daylight_aw") { setMouseCursor (juce::MouseCursor::PointingHandCursor); setTitle ("About Daylight_aw"); }
        void paintButton (juce::Graphics& g, bool, bool) override
        {
            if (ctx == nullptr) return;
            const auto& t = ctx->theme;
            if (t.systemFont)
            {
                g.setColour (t.logoColor);
                g.setFont (fonts::extraBold (15.0f));
                g.drawText (kPluginName, getLocalBounds(), juce::Justification::centredLeft, false);
                return;
            }
            drawGlowingText (g, kPluginName, fonts::extraBold (21.0f), 0.0f, (float) getHeight() * 0.5f + 7.0f,
                             t.logoColor, t.frameGlowColor, t.logoGlow);
        }
        UiContext* ctx = nullptr;
    };

    struct MenuBar : public juce::Component,
                     public juce::SettableTooltipClient
    {
        explicit MenuBar (Content& o)
            : owner (o),
              inAttach (o.ctx.processor.getInputTrim(), [this] (float) { repaint(); }, nullptr),
              outAttach (o.ctx.processor.getOutputTrim(), [this] (float) { repaint(); }, nullptr)
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setTitle ("Menu");
        }

        juce::String trimText() const
        {
            auto& p = owner.ctx.processor;
            juce::StringArray parts;
            auto fmt = [] (float db) { return (db > 0 ? "+" : "-") + juce::String (std::abs (db), 1); };
            if (! juce::exactlyEqual (p.getInputTrim().get(), 0.0f))  parts.add ("IN " + fmt (p.getInputTrim().get()));
            if (! juce::exactlyEqual (p.getOutputTrim().get(), 0.0f)) parts.add ("OUT " + fmt (p.getOutputTrim().get()));
            return parts.joinIntoString (" ");
        }

        juce::Rectangle<int> triggerArea() const
        {
            auto r = getLocalBounds().reduced (4, 3).withTrimmedRight (52);
            const auto trim = trimText();
            if (trim.isNotEmpty())
                r.removeFromRight ((int) fonts::width (fonts::semiBold (11.0f), trim) + 16);
            return r;
        }

        void paint (juce::Graphics& g) override
        {
            const auto& t = owner.ctx.theme;
            auto b = getLocalBounds().toFloat().reduced (0.5f);
            juce::Path p;
            p.addRoundedRectangle (b, t.corner);
            if (t.barGlow > 0.0f)
                juce::DropShadow (t.frameGlowColor.withAlpha (t.barGlow), 12, {}).drawForPath (g, p);
            g.setColour (t.barFill);
            g.fillPath (p);
            g.setColour (t.barBorder);
            g.strokePath (p, juce::PathStrokeType (1.0f));

            auto& proc = owner.ctx.processor;
            const auto label = menuLabel (proc);

            auto trig = triggerArea();
            const bool open = owner.selector != nullptr || owner.settingsPanel != nullptr || owner.info != nullptr;
            if (open)
            {
                g.setColour (t.text.withAlpha (0.08f));
                g.fillRoundedRectangle (trig.toFloat(), t.corner * 0.67f);
                g.setColour (t.text.withAlpha (0.5f));
                g.drawRoundedRectangle (trig.toFloat().reduced (0.5f), t.corner * 0.67f, 1.0f);
            }

            const auto font = fonts::semiBold (13.0f);
            g.setFont (font);
            g.setColour (t.text);
            const float textW = juce::jmin ((float) trig.getWidth() - 20.0f, fonts::width (font, label));
            const float startX = (float) trig.getCentreX() - (textW + 16.0f) * 0.5f;
            g.drawText (label, juce::Rectangle<float> (startX, (float) trig.getY(), textW, (float) trig.getHeight()),
                        juce::Justification::centredLeft, true);

            juce::Path chevron;
            const float cx = startX + textW + 10.0f, cy = (float) trig.getCentreY();
            if (open) { chevron.startNewSubPath (cx - 3.5f, cy + 1.5f); chevron.lineTo (cx, cy - 2.0f); chevron.lineTo (cx + 3.5f, cy + 1.5f); }
            else      { chevron.startNewSubPath (cx - 3.5f, cy - 1.5f); chevron.lineTo (cx, cy + 2.0f); chevron.lineTo (cx + 3.5f, cy - 1.5f); }
            g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            const auto trim = trimText();
            if (trim.isNotEmpty())
            {
                auto tr = juce::Rectangle<int> (trig.getRight() + 4, 6, (int) fonts::width (fonts::semiBold (11.0f), trim) + 10, getHeight() - 12);
                g.setColour (t.text.withAlpha (0.6f));
                g.drawRoundedRectangle (tr.toFloat(), 3.0f, 1.0f);
                g.setFont (fonts::semiBold (11.0f));
                g.setColour (t.text);
                g.drawText (trim, tr, juce::Justification::centred);
            }
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (! getLocalBounds().contains (e.getPosition()))
                return;
            if (triggerArea().contains (e.getPosition()))
                owner.toggleMenu();
            else if (e.getPosition().x < getWidth() - 52) // trim label: open settings
                if (owner.ctx.processor.hasEffect() && owner.settingsPanel == nullptr)
                    owner.openSettings();
        }

        Content& owner;
        juce::ParameterAttachment inAttach, outAttach;
    };

    struct TabButton : public juce::Component,
                       public juce::SettableTooltipClient
    {
        TabButton (Content& o, int i) : owner (o), index (i)
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setTitle ("Page " + juce::String (i + 1));
        }
        void paint (juce::Graphics& g) override
        {
            const auto& t = owner.ctx.theme;
            const bool on = owner.page == index;
            auto r = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (on ? t.buttonActiveFill : t.barFill);
            if (t.listView)
            {
                g.fillRoundedRectangle (r, t.corner);
                g.setColour (t.buttonOutline);
                g.drawRoundedRectangle (r, t.corner, 1.0f);
            }
            else
            {
                g.fillEllipse (r);
                g.setColour (t.buttonOutline);
                g.drawEllipse (r, 1.6f);
            }
            g.setColour (on ? t.buttonActiveText : t.text);
            g.setFont (fonts::extraBold (12.0f));
            g.drawText (juce::String (index + 1), getLocalBounds(), juce::Justification::centred);
        }
        void mouseEnter (const juce::MouseEvent&) override { owner.hoverTab = index; owner.repaintTabName(); }
        void mouseExit (const juce::MouseEvent&) override  { owner.hoverTab = -1; owner.repaintTabName(); }
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.getPosition()))
                owner.setPage (index);
        }
        Content& owner;
        int index;
    };

    /** Static layer: paper, figure, horizon, group frames + titles, messages. Rendered once and cached. */
    struct Backdrop : public juce::Component
    {
        explicit Backdrop (Content& o) : owner (o)
        {
            setBufferedToImage (true);
            setInterceptsMouseClicks (false, false);
        }

        void paint (juce::Graphics& g) override
        {
            const auto& t = owner.ctx.theme;
            const auto w = (float) getWidth(), h = (float) getHeight();
            const bool small = getHeight() < kLargeHeight;

            g.fillAll (t.paperColor);
            if (! t.topBarFill.isTransparent())
            {
                g.setColour (t.topBarFill);
                g.fillRect (0.0f, 0.0f, w, 56.0f);
                g.setColour (t.barBorder);
                g.fillRect (0.0f, 56.0f, w, 1.0f);
            }
            if (t.listView)
                if (auto* fx = owner.ctx.processor.getEffectHost(); fx != nullptr && fx->getNumParams() > 0)
                {
                    g.setColour (t.groupTitle);
                    g.setFont (fonts::medium (12.5f));
                    juce::String sub = "Airwindows: " + juce::String (fx->getInfo().name)
                                       + juce::String::fromUTF8 ("  \xc2\xb7  ") + fx->getInfo().description;
                    g.drawText (sub, juce::Rectangle<float> (14.0f, 60.0f, w - 28.0f, 18.0f), juce::Justification::centredLeft, true);
                }
            if (t.paper.isValid() && t.textureStrength > 0.0f)
            {
                juce::FillType ft (t.paper, juce::AffineTransform::scale (0.5f)); // 2x texture
                ft.setOpacity (t.textureStrength);
                g.setFillType (ft);
                g.fillAll();
                g.setOpacity (1.0f);
            }

            const float figW = small ? 69.0f : 121.0f, figH = small ? 68.0f : 119.0f;
            if (t.horizonOpacity > 0.0f)
            {
                // The horizon never runs through a group frame.
                juce::Graphics::ScopedSaveState save (g);
                for (auto& f : owner.frames)
                    g.excludeClipRegion (f.bounds.expanded (4.0f).getSmallestIntegerContainer());
                g.setColour (t.horizonColor.withMultipliedAlpha (t.horizonOpacity));
                g.drawLine (20.0f, h - 63.0f, w - figW - 30.0f, h - 63.5f, 1.0f);
            }
            // The figure sits in the bottom-right corner, but only when no group frame is there
            // (a full last row would run through it).
            const juce::Rectangle<float> figArea (w - figW, h - figH, figW, figH);
            const bool figureFree = std::none_of (owner.frames.begin(), owner.frames.end(),
                                                  [&] (const auto& f) { return f.bounds.intersects (figArea.reduced (8.0f)); });
            if (t.figure.isValid() && figureFree)
            {
                g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
                g.drawImage (t.figure, figArea,
                             juce::RectanglePlacement::xRight | juce::RectanglePlacement::yBottom | juce::RectanglePlacement::onlyReduceInSize);
            }

            // group frames
            for (auto& f : owner.frames)
            {
                juce::Path p;
                p.addRoundedRectangle (f.bounds, 10.0f);
                if (t.frameGlowStrength > 0.0f && t.frameGlowRadius > 0.0f)
                {
                    juce::Path stroke;
                    juce::PathStrokeType (2.0f).createStrokedPath (stroke, p);
                    juce::DropShadow (t.frameGlowColor.withAlpha (t.frameGlowStrength), (int) t.frameGlowRadius, {}).drawForPath (g, stroke);
                }
                g.setColour (t.frameColor.withMultipliedAlpha (t.frameOpacity));
                g.strokePath (p, juce::PathStrokeType (1.6f));

                if (f.title.isNotEmpty())
                {
                    g.setColour (t.groupTitle);
                    g.setFont (fonts::extraBold (12.0f).withExtraKerningFactor (0.12f));
                    g.drawText (f.title, f.bounds.withTrimmedLeft (12.0f).withTrimmedTop (6.0f).withHeight (18.0f),
                                juce::Justification::centredLeft, true);
                }
            }

            auto& proc = owner.ctx.processor;
            g.setColour (t.text);

            if (proc.hasUnknownEffect())
            {
                g.setFont (fonts::extraBold (22.0f));
                g.drawText ("This effect needs a newer Daylight_aw", juce::Rectangle<float> (36.0f, 74.0f, w - 140.0f, 30.0f), juce::Justification::centredLeft, true);
                g.setFont (fonts::semiBold (14.0f));
                g.drawFittedText ("The session uses \"" + proc.getUnknownEffectId() + "\". Audio passes through untouched and the "
                                  "saved settings are kept. Update Daylight_aw to use it.",
                                  juce::Rectangle<int> (36, 108, getWidth() - 160, 60), juce::Justification::topLeft, 3);
            }
            else if (auto* fx = proc.getEffectHost(); fx != nullptr && fx->getNumParams() == 0)
            {
                g.setFont (fonts::extraBold (44.0f));
                g.drawText (fx->getInfo().name, juce::Rectangle<float> (36.0f, 74.0f, w - 160.0f, 50.0f), juce::Justification::centredLeft, true);
                g.setFont (fonts::semiBold (15.0f));
                g.drawText ("this effect has no controls - it just works", juce::Rectangle<float> (36.0f, 128.0f, w - 160.0f, 22.0f), juce::Justification::centredLeft, true);
                g.setColour (t.muted);
                g.setFont (fonts::medium (13.0f));
                g.drawFittedText (fx->getInfo().description, juce::Rectangle<int> (36, 152, getWidth() - 160, 36), juce::Justification::topLeft, 2);
            }
            else if (! proc.hasEffect() && owner.selector == nullptr && owner.confirm == nullptr)
            {
                // empty state hint pointing at the menu bar
                juce::Path arrow;
                arrow.startNewSubPath (330.0f, 132.0f);
                arrow.cubicTo (320.0f, 107.0f, 330.0f, 80.0f, 352.0f, 64.0f);
                arrow.startNewSubPath (340.0f, 62.0f);
                arrow.lineTo (353.0f, 63.0f);
                arrow.lineTo (349.0f, 76.0f);
                g.strokePath (arrow, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setFont (fonts::semiBold (17.0f));
                g.drawText ("choose an effect to begin", juce::Rectangle<float> (230.0f, 140.0f, 260.0f, 24.0f), juce::Justification::centredLeft);
            }

            if (owner.tabs.size() > 1)
            {
                const int shown = owner.hoverTab >= 0 ? owner.hoverTab : owner.page;
                if (juce::isPositiveAndBelow (shown, (int) owner.layout.pages.size()))
                {
                    auto name = owner.layout.pages[(size_t) shown].name;
                    if (name.isEmpty()) name = "Page " + juce::String (shown + 1);
                    g.setColour (t.muted);
                    g.setFont (fonts::semiBold (11.0f));
                    g.drawText (name, juce::Rectangle<float> (0.0f, h - 22.0f, w, 16.0f), juce::Justification::centred);
                }
            }
        }

        Content& owner;
    };

    void repaintTabName() { backdrop.repaint (0, getHeight() - 24, getWidth(), 20); }

    void setPage (int p)
    {
        if (p == page) return;
        page = p;
        ctx.processor.lastTab = p;
        buildPage();
        layoutPage();
        for (auto* t : tabs) t->repaint();
        backdrop.repaint();
    }

private:
    void buildPage()
    {
        controls.clear();
        if (! juce::isPositiveAndBelow (page, (int) layout.pages.size()))
            return;

        for (auto& row : layout.pages[(size_t) page].rows)
            for (auto& group : row.groups)
                for (auto& c : group.controls)
                {
                    auto* pc = controls.add (new ParamControl (ctx, c));
                    addAndMakeVisible (pc);
                }

        // keep overlays on top
        for (juce::Component* c : { (juce::Component*) selector.get(), (juce::Component*) settingsPanel.get(),
                                    (juce::Component*) about.get(), (juce::Component*) info.get(),
                                    (juce::Component*) confirm.get() })
            if (c != nullptr) c->toFront (false);
    }

    void layoutPage()
    {
        frames.clear();
        if (! juce::isPositiveAndBelow (page, (int) layout.pages.size()))
            return;

        if (builtListView)
        {
            for (int i = 0; i < controls.size(); ++i)
                controls[i]->setBounds (0, kListTop + i * kListRowH, getWidth(), kListRowH);
            return;
        }

        int ci = 0;
        float y = (float) kRowsTop;
        for (auto& row : layout.pages[(size_t) page].rows)
        {
            float x = (float) kMargin;
            for (auto& group : row.groups)
            {
                const int n = (int) group.controls.size();
                const float w = (float) n * kSlotW + (float) (n - 1) * (float) kGap;
                frames.push_back ({ { x, y, w, (float) kRowH }, group.title });

                const float cellW = w / (float) n;
                for (int i = 0; i < n && ci < controls.size(); ++i, ++ci)
                    controls[ci]->setBounds (juce::Rectangle<float> (x + (float) i * cellW, y + 22.0f, cellW, (float) kRowH - 26.0f).toNearestInt());

                x += w + (float) kGap;
            }
            y += (float) (kRowH + kGap);
        }
    }

    void timerCallback() override
    {
        const auto label = menuLabel (ctx.processor);
        if (label != shownMenuLabel)
        {
            shownMenuLabel = label;
            menuBar.repaint();
        }
    }

    EffectHost* builtHost = nullptr;
    bool builtListView = false;
    bool builtUnknown = false;
    juce::String shownMenuLabel;

    UiContext& ctx;
    EffectLayout layout;
    int page = 0, hoverTab = -1;
    std::vector<Frame> frames;

    Backdrop backdrop;
    Logo logo;
    MenuBar menuBar;
    BypassButton bypass;
    PillButton abA, abB;
    juce::OwnedArray<ParamControl> controls;
    juce::OwnedArray<TabButton> tabs;

    std::unique_ptr<SelectorPanel> selector;
    std::unique_ptr<SettingsPanel> settingsPanel;
    std::unique_ptr<AboutPanel> about;
    std::unique_ptr<InfoPanel> info;
    std::unique_ptr<ConfirmDialog> confirm;

    std::unique_ptr<juce::TooltipWindow> tooltipWindow;   // inside the content so it follows the zoom

    void updateTooltips()
    {
        if (ctx.settings.getTooltips() && tooltipWindow == nullptr)
            tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 900);
        else if (! ctx.settings.getTooltips())
            tooltipWindow.reset();
    }
};

// =============================================================================================
DaylightEditor::DaylightEditor (DaylightProcessor& p)
    : juce::AudioProcessorEditor (p),
      ctx { p, *settings, Theme::builtInLight() },
      lookAndFeel (ctx)
{
    loadTheme();
    setLookAndFeel (&lookAndFeel);

    content = std::make_unique<Content> (ctx);
    // Settings changes arrive once, through GlobalSettings' change message (for every instance).
    addAndMakeVisible (*content);

    p.addChangeListener (this);
    settings->addChangeListener (this);

    setResizable (false, false);
    applyZoom();
    startTimer (1000);
}

DaylightEditor::~DaylightEditor()
{
    stopTimer();
    ctx.processor.removeChangeListener (this);
    settings->removeChangeListener (this);
    content = nullptr;
    setLookAndFeel (nullptr);
}

void DaylightEditor::loadTheme()
{
    switch (settings->getTheme())
    {
        case ThemeId::dark: ctx.theme = Theme::builtInDark(); break;
        case ThemeId::user:
            ensureUserThemeExists (ThemeId::light);
            ctx.theme = Theme::fromFolder (getUserThemeFolder(), Theme::builtInLight());
            userThemeStamp = userThemeTime();
            break;
        case ThemeId::light: ctx.theme = Theme::builtInLight(); break;
        case ThemeId::original: ctx.theme = Theme::builtInOriginal(); break;
    }
    if (Theme::knobStyleOverrideForTesting().isNotEmpty())
        ctx.theme.knobStyle = Theme::knobStyleOverrideForTesting();
    fonts::setUseSystemFont (ctx.theme.systemFont);
    lookAndFeel.applyTheme();
}

void DaylightEditor::applyZoom()
{
    if (content == nullptr)
        return;

    const float z = (float) settings->getZoom() / 100.0f;
    content->setTransform (juce::AffineTransform::scale (z));
    setSize (juce::roundToInt ((float) content->getWidth() * z), juce::roundToInt ((float) content->getHeight() * z));
}

void DaylightEditor::showSettingsPanel()
{
    if (content != nullptr && ctx.processor.hasEffect())
        content->openSettings();
}

void DaylightEditor::showInfoPanel()
{
    if (content != nullptr)
        content->openInfo();
}

void DaylightEditor::resized()
{
    if (content != nullptr)
        content->setTopLeftPosition (0, 0);
}

void DaylightEditor::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == static_cast<juce::ChangeBroadcaster*> (&ctx.processor))
    {
        content->processorChanged();
        applyZoom();
        return;
    }

    // global settings changed (this or another instance)
    loadTheme();
    content->themeChanged();
    applyZoom();
}

juce::Time DaylightEditor::userThemeTime()
{
    auto folder = getUserThemeFolder();
    juce::Time newest;
    for (auto name : { "theme.json", "paper.png", "figure.png" })
        newest = juce::jmax (newest, folder.getChildFile (name).getLastModificationTime());
    return newest;
}

void DaylightEditor::timerCallback()
{
    settings->reloadIfChanged();

    if (settings->getTheme() == ThemeId::user)
    {
        // live reload while someone edits the user theme
        if (userThemeTime() != userThemeStamp)
        {
            loadTheme();
            content->themeChanged();
        }
    }
}
} // namespace dlaw
