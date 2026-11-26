#include "PluginEditor.h"
#include "BinaryData.h"
#include "Fonts.h"

namespace dlaw
{
namespace
{
// Geometry at 100 % zoom (see the 1.1 design, board 8).
constexpr int kTopH = 56, kSide = 16, kTopItemH = 30;
constexpr int kRowsTop = 70, kRowH = 142, kGap = 12;
constexpr float kColW = (float) (kWindowWidth - 2 * kSide - (kControlsPerRow - 1) * kGap) / (float) kControlsPerRow;   // 124

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
} // namespace

// =============================================================================================
class DaylightEditor::Content : public juce::Component,
                                private juce::Timer
{
public:
    explicit Content (UiContext& c)
        : ctx (c), backdrop (*this), nameButton (*this), bypass (c),
          abA (c, "A"), abB (c, "B"),
          inTrim (c, c.processor.getInputTrim(), "IN"), outTrim (c, c.processor.getOutputTrim(), "OUT")
    {
        setWantsKeyboardFocus (true);
        addAndMakeVisible (backdrop);
        addAndMakeVisible (logo);
        addAndMakeVisible (nameButton);
        addAndMakeVisible (abA);
        addAndMakeVisible (abB);
        addChildComponent (inTrim);
        addChildComponent (outTrim);
        addAndMakeVisible (bypass);

        logo.ctx = &ctx;
        logo.onClick = [this] { toggleAbout(); };

        abA.joinedRight = true;
        abB.joinedLeft = true;
        abA.setTitle ("Setting A");
        abB.setTitle ("Setting B");
        abA.onClick = [this] { ctx.processor.switchAB (0); refreshTopRow(); };
        abB.onClick = [this] { ctx.processor.switchAB (1); refreshTopRow(); };

        dismisser.onClick = [this] { closePopups(); };
        addChildComponent (dismisser);

        rebuild();

        if (! ctx.processor.hasEffect() && ! ctx.processor.hasUnknownEffect())
            openSelector();

        startTimerHz (5);   // keeps the "*" (preset edited) marker current
    }

    ~Content() override { stopTimer(); }

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

    int contentHeight() const
    {
        if (selector != nullptr && ! ctx.processor.hasUnknownEffect())
            return kLargeHeight;   // the effect browser always gets the Large window
        if (! ctx.processor.hasEffect())
            return ctx.processor.hasUnknownEffect() ? kSmallHeight : kLargeHeight;   // the effect browser needs room
        return windowHeightForRows (layout.rows);
    }

    // Called on effect change and page change.
    void rebuild()
    {
        builtHost = ctx.processor.getEffectHost();
        builtUnknown = ctx.processor.hasUnknownEffect();
        controls.clear();
        tabs.clear();
        frames.clear();

        if (auto* fx = ctx.processor.getEffectHost())
        {
            std::vector<ParamFacts> facts;
            for (int i = 0; i < fx->getNumParams(); ++i)
                facts.push_back ({ fx->getParamNames()[i], fx->detectSteps (i) });

            layout = buildLayout (facts, builtInLayoutJson (fx->getInfo().id));
            for ([[maybe_unused]] auto& w : layout.warnings)
                DBG ("Layout warning (" << fx->getInfo().id << "): " << w);

            page = juce::jlimit (0, juce::jmax (0, (int) layout.pages.size() - 1), ctx.processor.lastTab.load());
            buildPage();

            if (layout.hasTabs())
                for (int i = 0; i < (int) layout.pages.size(); ++i)
                    addAndMakeVisible (tabs.add (new TabButton (*this, i)));
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
        abA.setEnabled (has);
        abB.setEnabled (has);
        abA.setToggleState (has && ctx.processor.getActiveAB() == 0, juce::dontSendNotification);
        abB.setToggleState (has && ctx.processor.getActiveAB() == 1, juce::dontSendNotification);
        inTrim.setVisible (has);
        outTrim.setVisible (has);
        nameButton.repaint();
        logo.repaint();
        bypass.repaint();
    }

    void themeChanged()
    {
        // Same geometry, new colours: repaint everything (the cached backdrop is re-rendered).
        backdrop.repaint();
        if (selector != nullptr) selector->refresh();
        if (settingsPanel != nullptr) settingsPanel->refresh();
        if (info != nullptr) info->refresh();
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
        dismisser.setBounds (getLocalBounds());

        const int itemY = (kTopH - kTopItemH) / 2;
        logo.setBounds (kSide, 0, 124, kTopH);
        nameButton.setBounds (kSide + 126, itemY, 170, kTopItemH);
        abA.setBounds (nameButton.getRight() + 10, itemY, 28, kTopItemH);
        abB.setBounds (abA.getRight(), itemY, 28, kTopItemH);
        inTrim.setBounds (abB.getRight() + 16, itemY, inTrim.preferredWidth(), kTopItemH);
        outTrim.setBounds (inTrim.getRight() + 10, itemY, outTrim.preferredWidth(), kTopItemH);
        const int bw = bypass.preferredWidth();
        bypass.setBounds (getWidth() - kSide - bw, itemY, bw, kTopItemH);

        layoutPage();

        const int n = tabs.size();
        for (int i = 0; i < n; ++i)
            tabs[i]->setBounds (getWidth() / 2 - (n * 24 + (n - 1) * 10) / 2 + i * 34, getHeight() - 46, 24, 24);

        const auto panelArea = juce::Rectangle<int> (12, kTopH + 6, getWidth() - 24, getHeight() - kTopH - 6 - 12);
        if (selector != nullptr)
            selector->setBounds (panelArea);
        if (info != nullptr)
            info->setBounds (panelArea);
        if (settingsPanel != nullptr)
            settingsPanel->setBounds (juce::jmax (12, nameButton.getX() - 22), kTopH - 6, SettingsPanel::kWidth,
                                      juce::jmin (SettingsPanel::kHeight, getHeight() - kTopH - 2));
        if (about != nullptr)
            about->setBounds (12, kTopH - 6, AboutPanel::kWidth, about->preferredHeight());
        if (confirm != nullptr)
            confirm->setBounds (getLocalBounds());
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey && (about != nullptr || settingsPanel != nullptr))
        {
            closePopups();
            return true;
        }
        return false;
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

    /** Dropdown + About: they close on any click outside them (the click does nothing else). */
    void closePopups()
    {
        settingsPanel.reset();
        about.reset();
        dismisser.setVisible (false);
        nameButton.repaint();
    }

    void closePanels()
    {
        const bool hadSelector = selector != nullptr;
        selector.reset();
        info.reset();
        closePopups();
        backdrop.repaint();
        if (hadSelector)
            updateSize();   // back from the browser's Large window to the effect's own size
    }

    /** Window size follows the content (effect rows, or Large while the browser is open). */
    void updateSize()
    {
        if (getHeight() != contentHeight())
        {
            setSize (kWindowWidth, contentHeight());
            resized();
            backdrop.repaint();
            if (onSizeChanged) onSizeChanged();
        }
    }
    std::function<void()> onSizeChanged;

    void showDismisser()
    {
        dismisser.setVisible (true);
        dismisser.toFront (false);
    }

    void openSelector()
    {
        closePanels();
        selector = std::make_unique<SelectorPanel> (ctx, [this] (const EffectInfo& e) { askToLoad (e); });
        addAndMakeVisible (*selector);
        updateSize();
        resized();
        nameButton.repaint();
        backdrop.repaint();
    }

    void openSettings()
    {
        closePanels();
        showDismisser();
        settingsPanel = std::make_unique<SettingsPanel> (ctx,
            [this] { juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<Content> (this)]
                                                      { if (safe != nullptr) safe->openInfo(); }); },
            [this] { juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<Content> (this)]
                                                      { if (safe != nullptr) safe->openSelector(); }); });
        settingsPanel->setWantsKeyboardFocus (true);
        addAndMakeVisible (*settingsPanel);
        resized();
        settingsPanel->grabKeyboardFocus();
        nameButton.repaint();
    }

    void openInfo()
    {
        if (! ctx.processor.hasEffect())
            return;
        closePanels();
        info = std::make_unique<InfoPanel> (ctx, [this] { info.reset(); nameButton.repaint(); });
        addAndMakeVisible (*info);
        resized();
        nameButton.repaint();
    }

    void toggleAbout()
    {
        const bool wasOpen = about != nullptr;
        closePopups();
        if (wasOpen)
            return;
        showDismisser();
        about = std::make_unique<AboutPanel> (ctx);
        addAndMakeVisible (*about);
        resized();
        about->grabKeyboardFocus();
    }

    void askToLoad (const EffectInfo& e)
    {
        const EffectInfo* current = ctx.processor.hasEffect() ? &ctx.processor.getEffectHost()->getInfo() : nullptr;
        if (current != nullptr && juce::String (current->id) == e.id)
        {
            closePanels();   // the same effect: nothing to change
            return;
        }
        confirm = std::make_unique<ConfirmDialog> (ctx, e, current, [this, id = std::string (e.id)] (bool ok)
        {
            juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<Content> (this), id, ok]
            {
                if (safe == nullptr) return;
                safe->confirm.reset();
                if (! ok) return;
                if (safe->ctx.processor.changeEffect (id))   // first choice or a change
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

    // ---- pieces ------------------------------------------------------------------------------
    struct Frame { juce::Rectangle<float> bounds; juce::String title; };

    /** Invisible layer under an open dropdown/About: a click anywhere else closes it. */
    struct Dismisser : public juce::Component
    {
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
        std::function<void()> onClick;
    };

    struct Logo : public juce::Button
    {
        Logo() : juce::Button (kPluginName)
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setTitle ("About Daylight_aw");
        }
        void paintButton (juce::Graphics& g, bool highlighted, bool) override
        {
            if (ctx == nullptr) return;
            g.setColour (highlighted ? ctx->theme.logo : ctx->theme.logo.withMultipliedAlpha (0.94f));
            g.setFont (fonts::logo (20.0f).withExtraKerningFactor (0.06f));
            g.drawFittedText (kPluginName, getLocalBounds(), juce::Justification::centredLeft, 1, 0.8f);   // never cut off
        }
        UiContext* ctx = nullptr;
    };

    /** The effect-name button: opens the presets + settings dropdown (or the browser before choosing). */
    struct NameButton : public juce::Component
    {
        explicit NameButton (Content& o) : owner (o)
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setTitle ("Presets and settings");
        }

        bool isOpen() const
        {
            return owner.selector != nullptr || owner.settingsPanel != nullptr || owner.info != nullptr;
        }

        void paint (juce::Graphics& g) override
        {
            const auto& t = owner.ctx.theme;
            auto b = getLocalBounds().toFloat().reduced (0.5f);
            const bool open = isOpen();
            g.setColour (t.buttonFill);
            g.fillRoundedRectangle (b, t.corner);
            if (open || isMouseOver())
            {
                g.setColour (t.buttonText.withAlpha (open ? 0.07f : 0.04f));
                g.fillRoundedRectangle (b, t.corner);
            }
            g.setColour (open ? t.buttonText.withAlpha (0.3f) : t.buttonBorder);
            g.drawRoundedRectangle (b, t.corner, 1.0f);

            const auto label = menuLabel (owner.ctx.processor);
            const auto font = fonts::medium (13.0f);
            const float textW = juce::jmin ((float) getWidth() - 36.0f, fonts::width (font, label));
            const float startX = (float) getWidth() * 0.5f - (textW + 16.0f) * 0.5f;
            g.setColour (t.buttonText);
            g.setFont (font);
            g.drawText (label, juce::Rectangle<float> (startX, 0.0f, textW, (float) getHeight()), juce::Justification::centredLeft, true);
            draw::chevron (g, startX + textW + 11.0f, (float) getHeight() * 0.5f, open, t.buttonText, 3.2f);
        }

        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override  { repaint(); }
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.getPosition()) && ! owner.ctx.processor.hasUnknownEffect())
                owner.toggleMenu();
        }

        Content& owner;
    };

    struct TabButton : public juce::Component
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
            g.setColour (on ? t.buttonActiveFill : t.buttonFill);
            g.fillEllipse (r);
            g.setColour (on ? t.buttonActiveFill : t.buttonBorder.withMultipliedAlpha (1.6f));
            g.drawEllipse (r, 1.2f);
            g.setColour (on ? t.buttonActiveText : t.buttonText);
            g.setFont (fonts::semiBold (12.0f));
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

    /** Static layer: background, halos, top-bar line, group panels + titles, messages. Rendered once and cached. */
    struct Backdrop : public juce::Component
    {
        explicit Backdrop (Content& o) : owner (o)
        {
            setBufferedToImage (true);
            setInterceptsMouseClicks (false, false);
        }

        static void drawHalo (juce::Graphics& g, const Theme::Halo& h, float w, float hgt)
        {
            if (h.opacity <= 0.0f || h.radius <= 0.0f)
                return;
            const juce::Point<float> c (h.x * w, h.y * hgt);
            float far = 0.0f;
            const juce::Point<float> corners[] = { { 0.0f, 0.0f }, { w, 0.0f }, { 0.0f, hgt }, { w, hgt } };
            for (auto corner : corners)
                far = juce::jmax (far, c.getDistanceFrom (corner));
            const float r = juce::jmax (1.0f, h.radius * far);
            juce::ColourGradient grad (h.colour.withAlpha (h.opacity), c, h.colour.withAlpha (0.0f), c.translated (r, 0.0f), true);
            g.setGradientFill (grad);
            g.fillAll();
        }

        void paint (juce::Graphics& g) override
        {
            const auto& t = owner.ctx.theme;
            const auto w = (float) getWidth(), h = (float) getHeight();

            g.fillAll (t.background);
            drawHalo (g, t.halo1, w, h);
            drawHalo (g, t.halo2, w, h);
            g.setColour (t.topBarLine);
            g.fillRect (0.0f, (float) kTopH - 1.0f, w, 1.0f);

            // group panels
            for (auto& f : owner.frames)
            {
                g.setColour (t.groupFill);
                g.fillRoundedRectangle (f.bounds, t.corner + 2.0f);
                g.setColour (t.groupBorder);
                g.drawRoundedRectangle (f.bounds.reduced (0.5f), t.corner + 2.0f, 1.0f);

                if (f.title.isNotEmpty())
                {
                    g.setColour (t.groupTitle);
                    g.setFont (fonts::caps (11.0f, 0.1f));
                    g.drawText (f.title.toUpperCase(), f.bounds.withTrimmedLeft (10.0f).withTrimmedRight (6.0f).withTrimmedTop (9.0f).withHeight (16.0f),
                                juce::Justification::centredLeft, true);
                }
            }

            auto& proc = owner.ctx.processor;
            g.setColour (t.text);

            if (proc.hasUnknownEffect())
            {
                g.setFont (fonts::semiBold (17.0f));
                g.drawText ("This effect needs a newer Daylight_aw", juce::Rectangle<float> (28.0f, 82.0f, w - 56.0f, 26.0f), juce::Justification::centredLeft, true);
                g.setColour (t.text.withAlpha (0.8f));
                g.setFont (fonts::regular (13.0f));
                g.drawFittedText ("The session uses \"" + proc.getUnknownEffectId() + "\". Audio passes through untouched and the "
                                  "saved settings are kept. Update Daylight_aw to use it.",
                                  juce::Rectangle<int> (28, 114, getWidth() - 56, 60), juce::Justification::topLeft, 3);
            }
            else if (auto* fx = proc.getEffectHost(); fx != nullptr && fx->getNumParams() == 0)
            {
                g.setFont (fonts::logo (34.0f).withExtraKerningFactor (0.03f));
                g.drawText (fx->getInfo().name, juce::Rectangle<float> (28.0f, 76.0f, w - 56.0f, 44.0f), juce::Justification::centredLeft, true);
                g.setColour (t.groupTitle);
                g.setFont (fonts::caps (11.0f, 0.1f));
                g.drawText ("THIS EFFECT HAS NO CONTROLS - IT JUST WORKS", juce::Rectangle<float> (28.0f, 124.0f, w - 56.0f, 18.0f), juce::Justification::centredLeft, true);
                g.setColour (t.muted);
                g.setFont (fonts::regular (13.0f));
                g.drawFittedText (fx->getInfo().description, juce::Rectangle<int> (28, 150, getWidth() - 56, 40), juce::Justification::topLeft, 2);
            }
            else if (! proc.hasEffect() && owner.selector == nullptr && owner.confirm == nullptr)
            {
                g.setColour (t.muted);
                g.setFont (fonts::regular (14.0f));
                g.drawText ("Choose an effect to begin", juce::Rectangle<float> (0.0f, h * 0.5f - 10.0f, w, 20.0f), juce::Justification::centred);
            }

            if (owner.tabs.size() > 1)
            {
                const int shown = owner.hoverTab >= 0 ? owner.hoverTab : owner.page;
                if (juce::isPositiveAndBelow (shown, (int) owner.layout.pages.size()))
                {
                    auto name = owner.layout.pages[(size_t) shown].name;
                    if (name.isEmpty()) name = "Page " + juce::String (shown + 1);
                    g.setColour (t.muted);
                    g.setFont (fonts::regular (11.0f));
                    g.drawText (name, juce::Rectangle<float> (0.0f, h - 20.0f, w, 14.0f), juce::Justification::centred);
                }
            }
        }

        Content& owner;
    };

    void repaintTabName() { backdrop.repaint (0, getHeight() - 22, getWidth(), 18); }

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

    juce::Component* currentAbout() { return about.get(); }
    juce::Component* currentSettings() { return settingsPanel.get(); }

private:
    void buildPage()
    {
        controls.clear();
        if (! juce::isPositiveAndBelow (page, (int) layout.pages.size()))
            return;

        for (auto& row : layout.pages[(size_t) page].rows)
            for (auto& group : row.groups)
                for (auto& c : group.controls)
                    addAndMakeVisible (controls.add (new ParamControl (ctx, c)));

        // keep overlays on top
        for (juce::Component* c : { (juce::Component*) selector.get(), (juce::Component*) info.get(),
                                    (juce::Component*) &dismisser, (juce::Component*) settingsPanel.get(),
                                    (juce::Component*) about.get(), (juce::Component*) confirm.get() })
            if (c != nullptr && (c != &dismisser || dismisser.isVisible())) c->toFront (false);
    }

    void layoutPage()
    {
        frames.clear();
        if (! juce::isPositiveAndBelow (page, (int) layout.pages.size()))
            return;

        int ci = 0;
        float y = (float) kRowsTop;
        for (auto& row : layout.pages[(size_t) page].rows)
        {
            float x = (float) kSide;
            for (auto& group : row.groups)
            {
                const int n = (int) group.controls.size();
                const float w = (float) n * kColW + (float) (n - 1) * (float) kGap;
                frames.push_back ({ { x, y, w, (float) kRowH }, group.title });

                // controls share the panel below the title (padding 10 at the sides and bottom)
                const float cellW = (w - 20.0f) / (float) n;
                for (int i = 0; i < n && ci < controls.size(); ++i, ++ci)
                    controls[ci]->setBounds (juce::Rectangle<float> (x + 10.0f + (float) i * cellW, y + 26.0f, cellW, (float) kRowH - 36.0f).toNearestInt());

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
            nameButton.repaint();
        }
    }

    EffectHost* builtHost = nullptr;
    bool builtUnknown = false;
    juce::String shownMenuLabel;

    UiContext& ctx;
    EffectLayout layout;
    int page = 0, hoverTab = -1;
    std::vector<Frame> frames;

    Backdrop backdrop;
    Logo logo;
    NameButton nameButton;
    BypassButton bypass;
    PillButton abA, abB;
    TrimKnob inTrim, outTrim;
    juce::OwnedArray<ParamControl> controls;
    juce::OwnedArray<TabButton> tabs;
    Dismisser dismisser;

    std::unique_ptr<SelectorPanel> selector;
    std::unique_ptr<SettingsPanel> settingsPanel;
    std::unique_ptr<AboutPanel> about;
    std::unique_ptr<InfoPanel> info;
    std::unique_ptr<ConfirmDialog> confirm;
};

// =============================================================================================
DaylightEditor::DaylightEditor (DaylightProcessor& p)
    : juce::AudioProcessorEditor (p),
      ctx { p, *settings, Theme::builtInDefault() },
      lookAndFeel (ctx)
{
    loadTheme();
    setLookAndFeel (&lookAndFeel);

    content = std::make_unique<Content> (ctx);
    content->onSizeChanged = [this] { applyZoom(); };
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
    if (settings->getTheme() == ThemeId::user)
    {
        ensureUserThemeExists();
        ctx.theme = Theme::fromFolder (getUserThemeFolder(), Theme::builtInDefault());
        userThemeStamp = userThemeTime();
    }
    else
    {
        ctx.theme = Theme::builtInDefault();
    }
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

void DaylightEditor::showAbout()
{
    if (content != nullptr)
        content->toggleAbout();
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
    return getUserThemeFolder().getChildFile ("theme.json").getLastModificationTime();
}

void DaylightEditor::timerCallback()
{
    settings->reloadIfChanged();

    if (settings->getTheme() == ThemeId::user && userThemeTime() != userThemeStamp)
    {
        // live reload while someone edits the user theme
        loadTheme();
        content->themeChanged();
    }
}
} // namespace dlaw
