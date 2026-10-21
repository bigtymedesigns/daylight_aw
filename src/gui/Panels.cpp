#include "Panels.h"
#include "Fonts.h"

namespace dlaw
{
namespace
{
const juce::String kDot = juce::String::fromUTF8 (" \xc2\xb7 ");
}

// =============================================================================================
void PanelBase::paint (juce::Graphics& g)
{
    const auto& t = ctx.theme;
    auto r = getLocalBounds().toFloat().reduced (1.0f);

    juce::Path box;
    box.addRoundedRectangle (r, 6.0f);
    if (t.panelGlow > 0.0f)
        juce::DropShadow (t.frameGlowColor.withAlpha (t.panelGlow), 16, {}).drawForPath (g, box);
    else
        juce::DropShadow (juce::Colours::black.withAlpha (0.14f), 16, { 0, 5 }).drawForPath (g, box);

    g.setColour (t.panelFill);
    g.fillPath (box);
    g.setColour (t.panelBorder);
    g.strokePath (box, juce::PathStrokeType (1.0f));
}

void PanelBase::drawHeading (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area)
{
    g.setColour (ctx.theme.panelText.withAlpha (0.75f));
    g.setFont (fonts::extraBold (11.5f).withExtraKerningFactor (0.1f));
    g.drawText (text, area, juce::Justification::centredLeft, true);
}

// =============================================================================================
struct SelectorPanel::CategoryRow : public juce::Component
{
    CategoryRow (SelectorPanel& o, juce::String k, juce::String l) : owner (o), key (std::move (k)), label (std::move (l))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = owner.ctx.theme;
        const bool on = owner.category == key && owner.search.isEmpty();
        if (on || isMouseOver())
        {
            g.setColour (t.panelText.withAlpha (on ? 0.13f : 0.06f));
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
        }
        g.setColour (t.panelText);
        g.setFont (on ? fonts::extraBold (13.0f) : fonts::medium (13.0f));
        g.drawText (label, getLocalBounds().reduced (8, 0), juce::Justification::centredLeft, true);
        g.setColour (t.panelText.withAlpha (0.7f));
        g.setFont (fonts::medium (11.0f));
        g.drawText (juce::String (count), getLocalBounds().reduced (8, 0), juce::Justification::centredRight, false);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }
    void mouseUp (const juce::MouseEvent&) override
    {
        owner.category = key;
        owner.search.clear();
        owner.rebuildList();
    }

    SelectorPanel& owner;
    juce::String key, label;
    int count = 0;
};

struct SelectorPanel::Row : public juce::Component
{
    explicit Row (SelectorPanel& o) : owner (o) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void paint (juce::Graphics& g) override
    {
        if (info == nullptr)
            return;

        const auto& t = owner.ctx.theme;
        const bool hov = owner.hovered == info;
        const bool fav = owner.ctx.settings.isFavourite (info->id);

        if (hov)
        {
            g.setColour (t.panelText.withAlpha (0.08f));
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
        }

        auto r = getLocalBounds().reduced (8, 0);
        auto starArea = r.removeFromRight (22);

        g.setColour (t.panelText);
        g.setFont (fonts::semiBold (13.0f));
        g.drawText (info->name, r, juce::Justification::centredLeft, true);

        if (showCategory)
        {
            g.setColour (t.panelText.withAlpha (0.7f));
            g.setFont (fonts::medium (11.0f));
            g.drawText (info->category, r, juce::Justification::centredRight, true);
        }

        if (fav || hov)
        {
            juce::Path star;
            star.addStar (starArea.toFloat().getCentre(), 5, 3.0f, 7.0f);
            g.setColour (t.panelText);
            if (fav) g.fillPath (star);
            else     g.strokePath (star, juce::PathStrokeType (1.1f));
        }
    }

    bool onStar (juce::Point<int> p) const { return p.x > getWidth() - 34; }

    void mouseEnter (const juce::MouseEvent&) override { owner.setHover (info); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (info == nullptr || ! getLocalBounds().contains (e.getPosition()))
            return;

        if (e.mods.isPopupMenu() || onStar (e.getPosition()))
        {
            owner.ctx.settings.toggleFavourite (info->id);
            owner.refresh();
            return;
        }
        if (owner.onChoose)
            owner.onChoose (*info);
    }

    SelectorPanel& owner;
    const EffectInfo* info = nullptr;
    bool showCategory = false;
};

SelectorPanel::SelectorPanel (UiContext& c, std::function<void (const EffectInfo&)> choose)
    : PanelBase (c), onChoose (std::move (choose))
{
    search.setTextToShowWhenEmpty ("Search " + juce::String ((int) getAllEffects().size()) + " effects...", c.theme.panelText.withAlpha (0.55f));
    search.setFont (fonts::medium (13.0f));
    search.setIndents (8, 5);
    search.addListener (this);
    search.setTitle ("Search effects");
    addAndMakeVisible (search);

    list.setModel (this);
    list.setRowHeight (26);
    list.setOutlineThickness (0);
    list.setTitle ("Effects");
    addAndMakeVisible (list);

    addAndMakeVisible (categoryHolder);
    rebuildCategories();
    rebuildList();
}

SelectorPanel::~SelectorPanel()
{
    list.setModel (nullptr);
}

void SelectorPanel::rebuildCategories()
{
    categoryRows.clear();
    categoryRows.add (new CategoryRow (*this, "Favourites", juce::String::fromUTF8 ("\xe2\x98\x85 Favourites")));
    categoryRows.add (new CategoryRow (*this, "Recent", "Recently used"));
    categoryRows.add (new CategoryRow (*this, "All", "All effects"));

    juce::StringArray cats;
    for (auto& e : getAllEffects())
        cats.addIfNotAlreadyThere (e.category);
    cats.sortNatural();
    for (auto& cat : cats)
        categoryRows.add (new CategoryRow (*this, cat, cat));

    for (auto* r : categoryRows)
        categoryHolder.addAndMakeVisible (r);
    refresh();
}

void SelectorPanel::refresh()
{
    for (auto* r : categoryRows)
    {
        if (r->key == "Favourites")      r->count = ctx.settings.getFavourites().size();
        else if (r->key == "Recent")     r->count = ctx.settings.getRecent().size();
        else if (r->key == "All")        r->count = (int) getAllEffects().size();
        else
        {
            r->count = 0;
            for (auto& e : getAllEffects())
                if (r->key == e.category)
                    ++r->count;
        }
    }
    rebuildList();
}

void SelectorPanel::rebuildList()
{
    items.clear();
    const auto q = search.getText().trim().toLowerCase();

    auto fromIds = [this] (const juce::StringArray& ids)
    {
        for (auto& id : ids)
            if (auto* e = findEffect (id.toStdString()))
                items.push_back (e);
    };

    if (q.isNotEmpty())
    {
        for (auto& e : getAllEffects())
            if (juce::String (e.name).toLowerCase().contains (q) || juce::String (e.description).toLowerCase().contains (q))
                items.push_back (&e);
    }
    else if (category == "Favourites") fromIds (ctx.settings.getFavourites());
    else if (category == "Recent")     fromIds (ctx.settings.getRecent());
    else
    {
        for (auto& e : getAllEffects())
            if (category == "All" || category == e.category)
                items.push_back (&e);
    }

    if (q.isNotEmpty() || category == "All" || (category != "Favourites" && category != "Recent"))
        std::sort (items.begin(), items.end(), [] (auto* a, auto* b) { return juce::String (a->name).compareNatural (b->name) < 0; });

    list.updateContent();
    list.repaint();
    for (auto* r : categoryRows)
        r->repaint();
    repaint();
}

int SelectorPanel::getNumRows() { return (int) items.size(); }

juce::Component* SelectorPanel::refreshComponentForRow (int row, bool, juce::Component* existing)
{
    auto* r = dynamic_cast<Row*> (existing);
    if (r == nullptr)
    {
        delete existing;
        r = new Row (*this);
    }

    r->info = juce::isPositiveAndBelow (row, (int) items.size()) ? items[(size_t) row] : nullptr;
    r->showCategory = search.getText().isNotEmpty() || category == "Favourites" || category == "Recent" || category == "All";
    r->repaint();
    return r;
}

void SelectorPanel::textEditorTextChanged (juce::TextEditor&) { rebuildList(); }

void SelectorPanel::setHover (const EffectInfo* e)
{
    if (hovered != e)
    {
        hovered = e;
        list.repaint();
        repaint (getLocalBounds().removeFromBottom (44));
    }
}

void SelectorPanel::resized()
{
    auto r = getLocalBounds().reduced (1);
    r.removeFromBottom (42); // description strip

    auto left = r.removeFromLeft (180).reduced (8, 10);
    int y = 0;
    for (auto* row : categoryRows)
    {
        row->setBounds (0, y, left.getWidth(), 22);
        y += 23;
    }
    categoryHolder.setBounds (left.withHeight (juce::jmin (left.getHeight(), y)));

    r.removeFromLeft (1);
    r = r.reduced (12, 10);
    search.setBounds (r.removeFromTop (28));
    r.removeFromTop (24);
    list.setBounds (r);
}

void SelectorPanel::paint (juce::Graphics& g)
{
    PanelBase::paint (g);
    const auto& t = ctx.theme;
    auto r = getLocalBounds().reduced (1);
    auto strip = r.removeFromBottom (42);

    g.setColour (t.panelText.withAlpha (0.2f));
    g.drawHorizontalLine (strip.getY(), (float) strip.getX(), (float) strip.getRight());
    g.drawVerticalLine (r.getX() + 180, (float) r.getY() + 8.0f, (float) r.getBottom() - 8.0f);

    // list heading
    auto heading = r.withTrimmedLeft (193).withTrimmedRight (12).withTrimmedTop (44).withHeight (20);
    const auto q = search.getText().trim();
    juce::String title = q.isNotEmpty() ? juce::String ("SEARCH") + kDot + juce::String ((int) items.size()) + " RESULTS"
                       : category == "Favourites" ? juce::String ("FAVOURITES")
                       : category == "Recent" ? juce::String ("RECENTLY USED")
                       : category == "All" ? juce::String ("ALL EFFECTS")
                       : category.toUpperCase();
    drawHeading (g, title, heading);

    if (items.empty())
    {
        g.setColour (t.panelText.withAlpha (0.75f));
        g.setFont (fonts::medium (13.0f));
        const auto msg = (category == "Favourites" && q.isEmpty())
                             ? "No favourites yet - right-click an effect or click its star."
                             : "Nothing found.";
        g.drawFittedText (msg, list.getBounds().reduced (6, 4), juce::Justification::topLeft, 2);
    }

    g.setColour (t.panelText.withAlpha (0.9f));
    g.setFont (fonts::medium (13.0f));
    const juce::String desc = hovered != nullptr ? juce::String (hovered->description)
                                                 : "Hover an effect to see what it does. Right-click or click the star to add it to your favourites.";
    g.drawFittedText (desc, strip.reduced (14, 4), juce::Justification::centredLeft, 2);
}

// =============================================================================================
ConfirmDialog::ConfirmDialog (UiContext& c, const EffectInfo& i, std::function<void (bool)> result)
    : ctx (c), info (i), onResult (std::move (result)),
      cancel (c, "Cancel"), load (c, "Load effect")
{
    load.primary = true;
    load.fontSize = cancel.fontSize = 13.0f;
    cancel.onClick = [this] { if (onResult) onResult (false); };
    load.onClick = [this] { if (onResult) onResult (true); };
    addAndMakeVisible (cancel);
    addAndMakeVisible (load);
    setWantsKeyboardFocus (true);
}

void ConfirmDialog::resized()
{
    box = juce::Rectangle<int> (0, 0, 340, 150).withCentre (getLocalBounds().getCentre());
    auto buttons = box.reduced (20, 18).removeFromBottom (28);
    load.setBounds (buttons.removeFromRight (110));
    buttons.removeFromRight (8);
    cancel.setBounds (buttons.removeFromRight (80));
}

void ConfirmDialog::paint (juce::Graphics& g)
{
    const auto& t = ctx.theme;
    g.fillAll (juce::Colours::black.withAlpha (0.18f));

    juce::Path p;
    p.addRoundedRectangle (box.toFloat(), 10.0f);
    juce::DropShadow (t.panelGlow > 0.0f ? t.frameGlowColor.withAlpha (t.panelGlow) : juce::Colours::black.withAlpha (0.2f), 22, { 0, 6 }).drawForPath (g, p);
    g.setColour (t.panelFill.withAlpha (1.0f));
    g.fillPath (p);
    g.setColour (t.panelText);
    g.strokePath (p, juce::PathStrokeType (1.6f));

    auto r = box.reduced (20, 16);
    g.setFont (fonts::extraBold (18.0f));
    g.drawText ("Load " + juce::String (info.name) + "?", r.removeFromTop (26), juce::Justification::centredLeft, true);
    g.setFont (fonts::medium (13.0f));
    g.drawFittedText ("This can't be changed later. To use a different effect, add a new Daylight_aw instance.",
                      r.removeFromTop (44), juce::Justification::topLeft, 3);
}

bool ConfirmDialog::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { if (onResult) onResult (false); return true; }
    if (k == juce::KeyPress::returnKey) { if (onResult) onResult (true); return true; }
    return false;
}

// =============================================================================================
SettingsPanel::SettingsPanel (UiContext& c, std::function<void()> changed)
    : PanelBase (c), onChanged (std::move (changed)),
      openThemeFolder (c, "Open theme folder"), resetButton (c, "Reset to defaults"),
      infoButton (c, "Effect info"), savePreset (c, "Save preset")
{
    int i = 0;
    for (auto level : GlobalSettings::zoomLevels)
    {
        auto* b = zoomButtons.add (new PillButton (c, juce::String (level) + "%"));
        b->joinedLeft = i > 0;
        b->joinedRight = i < 3;
        b->onClick = [this, level] { ctx.settings.setZoom (level); if (onChanged) onChanged(); };
        addAndMakeVisible (b);
        ++i;
    }

    const juce::StringArray names { "Light", "Dark", "User" };
    for (int k = 0; k < 3; ++k)
    {
        auto* b = themeButtons.add (new PillButton (c, names[k]));
        b->joinedLeft = k > 0;
        b->joinedRight = k < 2;
        b->onClick = [this, k]
        {
            const auto id = k == 0 ? ThemeId::light : k == 1 ? ThemeId::dark : ThemeId::user;
            if (id == ThemeId::user)
                ensureUserThemeExists (ctx.settings.getTheme() == ThemeId::dark ? ThemeId::dark : ThemeId::light);
            ctx.settings.setTheme (id);
            if (onChanged) onChanged();
        };
        addAndMakeVisible (b);
    }

    openThemeFolder.onClick = [] { getUserThemeFolder().startAsProcess(); };
    addChildComponent (openThemeFolder);

    resetButton.onClick = [this] { ctx.processor.resetToDefaults(); };
    infoButton.onClick = [this] { showInfo = ! showInfo; infoButton.setToggleState (showInfo, juce::dontSendNotification); repaint(); };
    infoButton.setClickingTogglesState (false);
    addAndMakeVisible (resetButton);
    addAndMakeVisible (infoButton);

    savePreset.setEnabled (false);
    addAndMakeVisible (savePreset);

    for (auto* s : { &inTrim, &outTrim })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        s->setDoubleClickReturnValue (true, 0.0);
        s->setMouseDragSensitivity (250);
        s->setVelocityModeParameters (1.0, 1, 0.0, false, juce::ModifierKeys::shiftModifier);
        s->onValueChange = [this] { repaint(); };
        addAndMakeVisible (*s);
    }
    inTrim.setTitle ("Input trim");
    outTrim.setTitle ("Output trim");
    inAttach = std::make_unique<juce::SliderParameterAttachment> (c.processor.getInputTrim(), inTrim, nullptr);
    outAttach = std::make_unique<juce::SliderParameterAttachment> (c.processor.getOutputTrim(), outTrim, nullptr);

    refresh();
}

void SettingsPanel::refresh()
{
    for (int i = 0; i < zoomButtons.size(); ++i)
        zoomButtons[i]->setToggleState (GlobalSettings::zoomLevels[i] == ctx.settings.getZoom(), juce::dontSendNotification);

    const auto th = ctx.settings.getTheme();
    themeButtons[0]->setToggleState (th == ThemeId::light, juce::dontSendNotification);
    themeButtons[1]->setToggleState (th == ThemeId::dark, juce::dontSendNotification);
    themeButtons[2]->setToggleState (th == ThemeId::user, juce::dontSendNotification);
    openThemeFolder.setVisible (th == ThemeId::user);
    repaint();
}

void SettingsPanel::resized()
{
    auto r = getLocalBounds().reduced (14, 12);
    const int colW = (r.getWidth() - 3 * 14) / 4;

    left = r.removeFromLeft (colW);
    r.removeFromLeft (14);
    auto look = r.removeFromLeft (colW);
    r.removeFromLeft (14);
    auto trims = r.removeFromLeft (colW);
    r.removeFromLeft (14);
    right = r;

    savePreset.setBounds (left.withTrimmedTop (22).withHeight (24).withWidth (juce::jmin (left.getWidth(), 110)));

    auto z = look.withTrimmedTop (38).withHeight (24);
    const int zw = z.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        zoomButtons[i]->setBounds (z.getX() + i * zw, z.getY(), i == 3 ? z.getWidth() - 3 * zw : zw, z.getHeight());

    auto th = look.withTrimmedTop (86).withHeight (24);
    const int tw = th.getWidth() / 3;
    for (int i = 0; i < 3; ++i)
        themeButtons[i]->setBounds (th.getX() + i * tw, th.getY(), i == 2 ? th.getWidth() - 2 * tw : tw, th.getHeight());
    openThemeFolder.setBounds (look.withTrimmedTop (116).withHeight (22));

    inTrim.setBounds (trims.withTrimmedTop (38).withHeight (20));
    outTrim.setBounds (trims.withTrimmedTop (86).withHeight (20));

    resetButton.setBounds (right.withTrimmedTop (22).withHeight (24));
    infoButton.setBounds (right.withTrimmedTop (52).withHeight (24));
}

void SettingsPanel::paint (juce::Graphics& g)
{
    PanelBase::paint (g);
    const auto& t = ctx.theme;
    auto r = getLocalBounds().reduced (14, 12);
    const int colW = (r.getWidth() - 3 * 14) / 4;
    auto look = r.withTrimmedLeft (colW + 14).withWidth (colW);
    auto trims = r.withTrimmedLeft (2 * (colW + 14)).withWidth (colW);

    // Presets column
    auto* fx = ctx.processor.getEffectHost();
    drawHeading (g, "PRESETS" + (fx != nullptr ? kDot + juce::String (fx->getInfo().name).toUpperCase() : juce::String()), left.withHeight (16));
    g.setColour (t.panelText.withAlpha (0.8f));
    g.setFont (fonts::medium (12.5f));
    if (showInfo && fx != nullptr)
    {
        juce::String text = fx->getInfo().description;
        if (fx->getInfo().isDither)
            text << "\n\nThis is a dither: keep the output trim at 0 dB.";
        g.drawFittedText (text, left.withTrimmedTop (52), juce::Justification::topLeft, 8);
    }
    else
        g.drawFittedText ("User presets arrive in the next test build.", left.withTrimmedTop (52), juce::Justification::topLeft, 3);

    drawHeading (g, "SETTINGS", look.withHeight (16));
    g.setColour (t.panelText);
    g.setFont (fonts::semiBold (12.5f));
    g.drawText ("Zoom", look.withTrimmedTop (20).withHeight (16), juce::Justification::centredLeft);
    g.drawText ("Theme", look.withTrimmedTop (68).withHeight (16), juce::Justification::centredLeft);

    if (ctx.settings.getTheme() == ThemeId::user && ! ctx.theme.warnings.isEmpty())
    {
        g.setColour (t.panelText.withAlpha (0.8f));
        g.setFont (fonts::medium (11.0f));
        g.drawFittedText (ctx.theme.warnings[0], look.withTrimmedTop (140).withHeight (30), juce::Justification::topLeft, 2);
    }

    auto fmt = [] (double db) { return (db > 0 ? "+" : db < 0 ? "-" : "") + juce::String (std::abs (db), 1) + " dB"; };
    drawHeading (g, "TRIMS", trims.withHeight (16));
    g.setColour (t.panelText);
    g.setFont (fonts::semiBold (12.5f));
    g.drawText ("Input trim", trims.withTrimmedTop (20).withHeight (16), juce::Justification::centredLeft);
    g.drawText (fmt (inTrim.getValue()), trims.withTrimmedTop (20).withHeight (16), juce::Justification::centredRight);
    g.drawText ("Output trim", trims.withTrimmedTop (68).withHeight (16), juce::Justification::centredLeft);
    g.drawText (fmt (outTrim.getValue()), trims.withTrimmedTop (68).withHeight (16), juce::Justification::centredRight);

    drawHeading (g, "EFFECT", right.withHeight (16));
}

// =============================================================================================
AboutPanel::AboutPanel (UiContext& c, std::function<void()> closeFn)
    : PanelBase (c), onClose (std::move (closeFn)), close (c, "Close")
{
    close.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (close);
}

void AboutPanel::resized()
{
    close.setBounds (getLocalBounds().reduced (14).removeFromBottom (24).removeFromRight (70));
}

void AboutPanel::paint (juce::Graphics& g)
{
    PanelBase::paint (g);
    const auto& t = ctx.theme;
    auto r = getLocalBounds().reduced (16, 14);

    g.setColour (t.panelText);
    g.setFont (fonts::extraBold (18.0f));
    g.drawText (kPluginName, r.removeFromTop (24), juce::Justification::centredLeft);
    g.setFont (fonts::semiBold (13.0f));
    g.drawText (juce::String ("by ") + kVendorName, r.removeFromTop (20), juce::Justification::centredLeft);
    g.setColour (t.panelText.withAlpha (0.8f));
    g.setFont (fonts::medium (12.5f));
    g.drawText (juce::String ("Version ") + kVersion, r.removeFromTop (20), juce::Justification::centredLeft);
    r.removeFromTop (4);
    g.setColour (t.panelText);
    g.drawFittedText ("Based on the work of Chris Johnson (Airwindows). Airwindows code used under the MIT licence.",
                      r.removeFromTop (36), juce::Justification::topLeft, 2);
    g.setFont (fonts::extraBold (12.5f));
    g.drawText ("Not an official Airwindows release.", r.removeFromTop (20), juce::Justification::centredLeft);
    g.setFont (fonts::medium (11.0f));
    g.setColour (t.panelText.withAlpha (0.8f));
    g.drawFittedText ("Licences: Airwindows (MIT), Airwindows Consolidated (MIT), JUCE (AGPLv3), Shantell Sans (SIL OFL 1.1). "
                      "Daylight_aw is free software under the AGPLv3.",
                      r.removeFromTop (44), juce::Justification::topLeft, 3);
}
} // namespace dlaw
