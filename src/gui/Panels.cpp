#include "Panels.h"
#include "UserLayouts.h"
#include "BinaryData.h"
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
    box.addRoundedRectangle (r, ctx.theme.corner);
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
            g.setColour (on ? t.selectFill : t.panelText.withAlpha (0.06f));
            g.fillRoundedRectangle (getLocalBounds().toFloat(), t.corner * 0.67f);
        }
        g.setColour (on ? t.selectText : t.panelText);
        auto textArea = getLocalBounds().reduced (8, 0);
        if (key == "Favourites")
        {
            // drawn star: the same on every system (the font has no star glyph)
            juce::Path star;
            star.addStar ({ (float) textArea.getX() + 5.0f, (float) getHeight() * 0.5f }, 5, 2.4f, 5.6f);
            g.fillPath (star);
            textArea.removeFromLeft (15);
        }
        g.setFont (on ? fonts::extraBold (13.0f) : fonts::medium (13.0f));
        g.drawText (label, textArea, juce::Justification::centredLeft, true);
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
            g.setColour (t.selectFill);
            g.fillRoundedRectangle (getLocalBounds().toFloat(), t.corner * 0.67f);
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
    categoryRows.add (new CategoryRow (*this, "Favourites", "Favourites"));
    categoryRows.add (new CategoryRow (*this, "Recent", "Recently used"));
    categoryRows.add (new CategoryRow (*this, "Latest", "Latest"));
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
        else if (r->key == "Latest")
        {
            r->count = 0;
            for (auto& e : getAllEffects())
                if (e.isLatest)
                    ++r->count;
        }
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
            if (category == "All" || category == e.category || (category == "Latest" && e.isLatest))
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
    r->showCategory = search.getText().isNotEmpty() || category == "Favourites" || category == "Recent"
                      || category == "All" || category == "Latest";
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
        row->setBounds (0, y, left.getWidth(), 20);
        y += 21;
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
                       : category == "Latest" ? juce::String ("LATEST AIRWINDOWS RELEASES")
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
SettingsPanel::SettingsPanel (UiContext& c, std::function<void()> changed, std::function<void()> showInfoFn)
    : PanelBase (c), onChanged (std::move (changed)), onShowInfo (std::move (showInfoFn)),
      openThemeFolder (c, "Open theme folder"), resetButton (c, "Reset to defaults"),
      infoButton (c, "Effect info"), savePreset (c, "Save preset"),
      nameOk (c, "Save"), nameCancel (c, "Cancel"),
      editLayoutButton (c, "Edit layout"), removeLayoutButton (c, "Remove"),
      inTrim (c, c.processor.getInputTrim(), "Input"), outTrim (c, c.processor.getOutputTrim(), "Output")
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

    const juce::StringArray names { "Light", "Dark", "Original", "User" };
    const ThemeId ids[] = { ThemeId::light, ThemeId::dark, ThemeId::original, ThemeId::user };
    for (int k = 0; k < 4; ++k)
    {
        auto* b = themeButtons.add (new PillButton (c, names[k]));
        b->joinedLeft = k > 0;
        b->joinedRight = k < 3;
        b->fontSize = 11.5f;
        b->onClick = [this, id = ids[k]]
        {
            if (id == ThemeId::user)
                ensureUserThemeExists (ctx.settings.getTheme() == ThemeId::user ? ThemeId::light : ctx.settings.getTheme());
            ctx.settings.setTheme (id);
            if (onChanged) onChanged();
        };
        addAndMakeVisible (b);
    }

    for (int k = 0; k < 2; ++k)
    {
        auto* b = tooltipButtons.add (new PillButton (c, k == 0 ? "On" : "Off"));
        b->joinedLeft = k == 1;
        b->joinedRight = k == 0;
        b->onClick = [this, k] { ctx.settings.setTooltips (k == 0); if (onChanged) onChanged(); };
        addAndMakeVisible (b);
    }

    openThemeFolder.onClick = [] { getUserThemeFolder().startAsProcess(); };
    addChildComponent (openThemeFolder);

    resetButton.onClick = [this] { ctx.processor.resetToDefaults(); };
    infoButton.onClick = [this] { if (onShowInfo) onShowInfo(); };
    addAndMakeVisible (resetButton);
    addAndMakeVisible (infoButton);

    // ---- user layouts (Documents/.../Layouts/<id>.json) ----
    editLayoutButton.setTooltip ("Make your own layout for this effect: opens its layout file. Save it in any text editor.");
    editLayoutButton.onClick = [this]
    {
        if (auto* fx = ctx.processor.getEffectHost())
        {
            if (userlayouts::create (*fx))
            {
                userlayouts::fileFor (fx->getInfo().id).revealToUser();
                layoutMessage = "Edit " + juce::String (fx->getInfo().id) + ".json in a text editor. Changes show up live.";
            }
            else
                layoutMessage = "Couldn't create the layout file. Is the Documents folder writable?";
            refresh();
        }
    };
    removeLayoutButton.setTooltip ("Remove your layout (to the trash) and use the built-in one again.");
    removeLayoutButton.onClick = [this]
    {
        if (auto* fx = ctx.processor.getEffectHost())
        {
            layoutMessage = userlayouts::remove (fx->getInfo().id) ? juce::String ("Back to the built-in layout.")
                                                                    : juce::String ("Couldn't remove the layout file.");
            refresh();
        }
    };
    addChildComponent (editLayoutButton);
    addChildComponent (removeLayoutButton);

    // ---- presets ----
    savePreset.onClick = [this] { startNaming (NameMode::saving); };
    savePreset.setEnabled (ctx.processor.hasEffect());
    addAndMakeVisible (savePreset);

    nameEditor.setFont (fonts::medium (13.0f));
    nameEditor.setIndents (6, 4);
    nameEditor.setTitle ("Preset name");
    nameEditor.setInputRestrictions (64);
    nameEditor.onReturnKey = [this] { confirmName(); };
    nameEditor.onEscapeKey = [this] { stopNaming(); };
    nameEditor.onTextChange = [this]
    {
        confirmReplace = false;
        nameOk.setButtonText (nameMode == NameMode::renaming ? "Rename" : "Save");
        message = {};
        resized();
        repaint();
    };
    addChildComponent (nameEditor);

    nameOk.primary = true;
    nameOk.onClick = [this] { confirmName(); };
    nameCancel.onClick = [this] { stopNaming(); };
    addChildComponent (nameOk);
    addChildComponent (nameCancel);

    presetList.setModel (this);
    presetList.setRowHeight (24);
    presetList.setOutlineThickness (0);
    presetList.setTitle ("Presets");
    presetList.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (presetList);

    addAndMakeVisible (inTrim);
    addAndMakeVisible (outTrim);

    rescanPresets();
    refresh();
}

SettingsPanel::~SettingsPanel()
{
    presetList.setModel (nullptr);
}

void SettingsPanel::refresh()
{
    for (int i = 0; i < zoomButtons.size(); ++i)
        zoomButtons[i]->setToggleState (GlobalSettings::zoomLevels[i] == ctx.settings.getZoom(), juce::dontSendNotification);

    const auto th = ctx.settings.getTheme();
    themeButtons[0]->setToggleState (th == ThemeId::light, juce::dontSendNotification);
    themeButtons[1]->setToggleState (th == ThemeId::dark, juce::dontSendNotification);
    themeButtons[2]->setToggleState (th == ThemeId::original, juce::dontSendNotification);
    themeButtons[3]->setToggleState (th == ThemeId::user, juce::dontSendNotification);
    resized();   // trims are knobs or faders depending on the theme
    openThemeFolder.setVisible (th == ThemeId::user);
    tooltipButtons[0]->setToggleState (ctx.settings.getTooltips(), juce::dontSendNotification);
    tooltipButtons[1]->setToggleState (! ctx.settings.getTooltips(), juce::dontSendNotification);
    nameEditor.applyColourToAllText (ctx.theme.panelText);
    presetList.repaint();
    repaint();
}

// ---- presets ------------------------------------------------------------------------------------
void SettingsPanel::rescanPresets()
{
    presetFiles.clear();
    if (auto* fx = ctx.processor.getEffectHost())
        presetFiles = store.list (fx->getInfo());
    presetList.updateContent();
    presetList.repaint();
    resized();
    repaint();
}

void SettingsPanel::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool)
{
    if (! juce::isPositiveAndBelow (row, presetFiles.size()))
        return;

    const auto& t = ctx.theme;
    const auto name = presetFiles[row].getFileNameWithoutExtension();
    const bool current = name == ctx.processor.getPresetName();

    if (current)
    {
        g.setColour (t.selectFill);
        g.fillRoundedRectangle (juce::Rectangle<float> (0.0f, 1.0f, (float) width, (float) height - 2.0f), t.corner * 0.67f);
    }

    g.setColour (current ? t.selectText : t.panelText);
    g.setFont (current ? fonts::extraBold (12.5f) : fonts::medium (12.5f));
    g.drawText (name, juce::Rectangle<int> (6, 0, width - 12, height), juce::Justification::centredLeft, true);
}

void SettingsPanel::listBoxItemClicked (int row, const juce::MouseEvent& e)
{
    if (! juce::isPositiveAndBelow (row, presetFiles.size()))
        return;

    if (e.mods.isPopupMenu())
        showPresetMenu (presetFiles[row]);
    else
        loadPreset (presetFiles[row]);
}

void SettingsPanel::loadPreset (const juce::File& file)
{
    PresetData data;
    if (! PresetStore::load (file, data))
        message = "Couldn't read this preset.";
    else if (! ctx.processor.applyPreset (data))
        message = "This preset is for a different effect.";
    else
        message = {};
    rescanPresets();
}

void SettingsPanel::showPresetMenu (const juce::File& file)
{
    juce::PopupMenu m;
    m.addItem (1, "Load");
    m.addItem (2, "Rename...");
    m.addItem (3, "Move to trash");
    m.addSeparator();
    m.addItem (4, "Show in folder");

    juce::Component::SafePointer<SettingsPanel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withParentComponent (getTopLevelComponent()).withMousePosition(),
                     [safe, file] (int choice)
                     {
                         if (safe == nullptr)
                             return;
                         auto& self = *safe;
                         switch (choice)
                         {
                             case 1: self.loadPreset (file); break;
                             case 2: self.startNaming (NameMode::renaming, file); break;
                             case 3:
                                 if (PresetStore::remove (file))
                                 {
                                     if (file.getFileNameWithoutExtension() == self.ctx.processor.getPresetName())
                                         self.ctx.processor.setPresetName ({});
                                     self.message = {};
                                 }
                                 else
                                     self.message = "Couldn't remove this preset.";
                                 self.rescanPresets();
                                 break;
                             case 4: file.revealToUser(); break;
                             default: break;
                         }
                     });
}

void SettingsPanel::startNaming (NameMode mode, const juce::File& file)
{
    auto* fx = ctx.processor.getEffectHost();
    if (fx == nullptr)
        return;

    nameMode = mode;
    renamingFile = file;
    confirmReplace = false;
    message = {};

    juce::String initial;
    if (mode == NameMode::renaming)
        initial = file.getFileNameWithoutExtension();
    else if (ctx.processor.getPresetName().isNotEmpty())
        initial = ctx.processor.getPresetName();
    else
    {
        int n = presetFiles.size() + 1;
        while (store.fileFor (fx->getInfo(), "Preset " + juce::String (n)).exists())
            ++n;
        initial = "Preset " + juce::String (n);
    }

    nameEditor.setText (initial, false);
    nameEditor.selectAll();
    nameOk.setButtonText (mode == NameMode::renaming ? "Rename" : "Save");
    savePreset.setVisible (false);
    nameEditor.setVisible (true);
    nameOk.setVisible (true);
    nameCancel.setVisible (true);
    resized();
    nameEditor.grabKeyboardFocus();
    repaint();
}

void SettingsPanel::stopNaming()
{
    nameMode = NameMode::none;
    confirmReplace = false;
    nameEditor.setVisible (false);
    nameOk.setVisible (false);
    nameCancel.setVisible (false);
    savePreset.setVisible (true);
    resized();
    repaint();
}

void SettingsPanel::confirmName()
{
    auto* fx = ctx.processor.getEffectHost();
    if (fx == nullptr || nameMode == NameMode::none)
        return;

    const auto clean = PresetStore::sanitiseName (nameEditor.getText());
    if (clean.isEmpty())
    {
        message = "Please type a name.";
        resized();
        repaint();
        return;
    }

    if (nameMode == NameMode::renaming)
    {
        const auto oldName = renamingFile.getFileNameWithoutExtension();
        juce::File renamed;
        if (! store.rename (renamingFile, clean, renamed))
        {
            message = "\"" + clean + "\" already exists.";
            resized();
            repaint();
            return;
        }
        if (oldName == ctx.processor.getPresetName())
            ctx.processor.setPresetName (renamed.getFileNameWithoutExtension());
    }
    else
    {
        const auto target = store.fileFor (fx->getInfo(), clean);
        if (target.exists() && ! confirmReplace)
        {
            confirmReplace = true;
            nameOk.setButtonText ("Replace");
            message = "\"" + clean + "\" exists. Replace it?";
            resized();
            repaint();
            return;
        }

        std::vector<float> values;
        for (int i = 0; i < fx->getNumParams(); ++i)
            values.push_back (ctx.processor.getSlot (i).get());

        if (! store.save (fx->getInfo(), clean, fx->getParamNames(), values))
        {
            message = "Couldn't save. Is the Documents folder writable?";
            resized();
            repaint();
            return;
        }
        ctx.processor.markPresetSaved (clean);
    }

    message = {};
    stopNaming();
    rescanPresets();
}

// ---- layout + drawing -------------------------------------------------------------------------
SettingsPanel::Columns SettingsPanel::columns() const
{
    auto r = getLocalBounds().reduced (14, 12);
    Columns c;
    c.presets = r.removeFromLeft (138);
    r.removeFromLeft (14);
    c.look = r.removeFromLeft (200);
    r.removeFromLeft (14);
    c.trims = r.removeFromLeft (130);
    r.removeFromLeft (14);
    c.effect = r;
    return c;
}

void SettingsPanel::resized()
{
    const auto cols = columns();
    left = cols.presets;
    auto look = cols.look;
    auto trims = cols.trims;
    right = cols.effect;

    // presets column
    auto pc = left.withTrimmedTop (22);
    if (nameMode == NameMode::none)
    {
        savePreset.setBounds (pc.removeFromTop (24).withWidth (juce::jmin (left.getWidth(), 110)));
    }
    else
    {
        nameEditor.setBounds (pc.removeFromTop (26));
        pc.removeFromTop (4);
        auto buttons = pc.removeFromTop (24);
        nameOk.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2 - 3));
        buttons.removeFromLeft (6);
        nameCancel.setBounds (buttons);
    }
    pc.removeFromTop (message.isNotEmpty() ? 32 : 6);
    presetList.setBounds (pc);

    auto z = look.withTrimmedTop (38).withHeight (24);
    const int zw = z.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        zoomButtons[i]->setBounds (z.getX() + i * zw, z.getY(), i == 3 ? z.getWidth() - 3 * zw : zw, z.getHeight());

    auto th = look.withTrimmedTop (86).withHeight (24);
    // "Original" (3rd) gets a little more room
    const int tw = (th.getWidth() - 62) / 3;
    int x = th.getX();
    for (int i = 0; i < 4; ++i)
    {
        const int w = i == 2 ? th.getWidth() - 3 * tw : tw;
        themeButtons[i]->setBounds (x, th.getY(), w, th.getHeight());
        x += w;
    }
    openThemeFolder.setBounds (look.withTrimmedTop (116).withHeight (22));

    if (ctx.theme.listView)
    {
        // faders, one under the other
        inTrim.setBounds (trims.withTrimmedTop (22).withHeight (40));
        outTrim.setBounds (trims.withTrimmedTop (66).withHeight (40));
    }
    else
    {
        auto knobs = trims.withTrimmedTop (22).withHeight (84);
        inTrim.setBounds (knobs.removeFromLeft (knobs.getWidth() / 2));
        outTrim.setBounds (knobs);
    }

    auto tt = trims.withTrimmedTop (130).withHeight (22).withWidth (juce::jmin (trims.getWidth(), 110));
    tooltipButtons[0]->setBounds (tt.removeFromLeft (tt.getWidth() / 2));
    tooltipButtons[1]->setBounds (tt);

    resetButton.setBounds (right.withTrimmedTop (22).withHeight (24));
    infoButton.setBounds (right.withTrimmedTop (52).withHeight (24));

    auto* fx = ctx.processor.getEffectHost();
    const bool layouts = fx != nullptr && ! ctx.theme.listView && fx->getNumParams() > 0;
    const bool custom = layouts && userlayouts::fileFor (fx->getInfo().id).existsAsFile();
    auto lr = right.withTrimmedTop (82).withHeight (24);
    removeLayoutButton.setVisible (custom);
    if (custom)
        removeLayoutButton.setBounds (lr.removeFromRight (58).withTrimmedLeft (4));
    editLayoutButton.setVisible (layouts);
    editLayoutButton.setBounds (lr);
}

void SettingsPanel::paint (juce::Graphics& g)
{
    PanelBase::paint (g);
    const auto& t = ctx.theme;
    const auto cols = columns();
    auto look = cols.look;
    auto trims = cols.trims;

    // Presets column
    auto* fx = ctx.processor.getEffectHost();
    drawHeading (g, "PRESETS", left.withHeight (16));

    if (message.isNotEmpty())
    {
        g.setColour (t.panelText);
        g.setFont (fonts::semiBold (11.5f));
        g.drawFittedText (message, presetList.getBounds().withY (presetList.getY() - 30).withHeight (28),
                          juce::Justification::centredLeft, 2);
    }

    if (presetFiles.isEmpty() && fx != nullptr)
    {
        g.setColour (t.panelText.withAlpha (0.7f));
        g.setFont (fonts::medium (12.0f));
        g.drawFittedText ("No presets for " + juce::String (fx->getInfo().name) + " yet.",
                          presetList.getBounds().reduced (6, 4).withHeight (34), juce::Justification::topLeft, 2);
    }

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

    drawHeading (g, "TRIMS", trims.withHeight (16));
    g.setColour (t.panelText);
    g.setFont (fonts::semiBold (12.5f));
    g.drawText ("Tooltips", trims.withTrimmedTop (112).withHeight (16), juce::Justification::centredLeft);

    drawHeading (g, "EFFECT" + (fx != nullptr ? kDot + juce::String (fx->getInfo().name).toUpperCase() : juce::String()),
                 right.withHeight (16));
    if (fx != nullptr)
    {
        const auto st = userlayouts::status (fx->getInfo().id);
        juce::String text;
        if (layoutMessage.isNotEmpty())
            text = layoutMessage;
        else if (st.custom && ! ctx.theme.listView)
            text = st.warnings.isEmpty() ? juce::String ("Your own layout is in use.")
                                         : "Your layout: " + juce::String (st.warnings.size()) + " problem(s) fixed: " + st.warnings[0];
        else
        {
            text = fx->getInfo().description;
            if (fx->getInfo().isDither)
                text << "\n\nThis is a dither: keep the output trim at 0 dB.";
        }
        g.setColour (t.panelText.withAlpha (0.85f));
        g.setFont (fonts::medium (12.0f));
        const int top = ctx.theme.listView ? 86 : 114;
        g.drawFittedText (text, right.withTrimmedTop (top), juce::Justification::topLeft, 8);
    }
}

// =============================================================================================
juce::String InfoPanel::docFor (const juce::String& effectId)
{
    int size = 0;
    if (auto* data = BinaryData::getNamedResource (("doc_" + effectId + "_txt").toRawUTF8(), size))
        return juce::String::fromUTF8 (data, size);
    return {};
}

InfoPanel::InfoPanel (UiContext& c, std::function<void()> closeFn)
    : PanelBase (c), onClose (std::move (closeFn)), close (c, "Close")
{
    close.onClick = [this]
    {
        // close after this click has finished (the panel deletes itself)
        juce::Component::SafePointer<InfoPanel> safe (this);
        juce::MessageManager::callAsync ([safe] { if (safe != nullptr && safe->onClose) safe->onClose(); });
    };
    addAndMakeVisible (close);

    text.setMultiLine (true, true);
    text.setReadOnly (true);
    text.setCaretVisible (false);
    text.setScrollbarsShown (true);
    text.setFont (fonts::medium (13.0f));
    text.setIndents (4, 2);
    text.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    text.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    text.setTitle ("Effect info");
    addAndMakeVisible (text);
    refresh();
}

void InfoPanel::refresh()
{
    auto* fx = ctx.processor.getEffectHost();
    if (fx == nullptr)
        return;

    auto doc = docFor (fx->getInfo().id).trim();
    juce::String body;
    if (doc.startsWith ("#"))
    {
        headline = doc.upToFirstOccurrenceOf ("\n", false, false).fromFirstOccurrenceOf ("#", false, false).trim();
        body = doc.fromFirstOccurrenceOf ("\n", false, false).trim();
    }
    else
    {
        headline = fx->getInfo().description;
        body = doc;
    }

    if (body.isEmpty())
        body = fx->getInfo().description;
    if (fx->getInfo().isDither)
        body << "\n\nNote: this is a dither. Keep Daylight_aw's output trim at 0 dB.";

    text.setText (body, false);
    text.applyColourToAllText (ctx.theme.panelText);
    text.moveCaretToTop (false);
    repaint();
}

void InfoPanel::resized()
{
    auto r = getLocalBounds().reduced (14, 12);
    auto bottom = r.removeFromBottom (24);
    close.setBounds (bottom.removeFromRight (70));
    r.removeFromTop (44);
    r.removeFromBottom (6);
    text.setBounds (r);
}

void InfoPanel::paint (juce::Graphics& g)
{
    PanelBase::paint (g);
    const auto& t = ctx.theme;
    auto r = getLocalBounds().reduced (14, 12);
    auto* fx = ctx.processor.getEffectHost();

    drawHeading (g, "EFFECT INFO" + (fx != nullptr ? kDot + juce::String (fx->getInfo().name).toUpperCase() : juce::String()),
                 r.removeFromTop (16));
    g.setColour (t.panelText);
    g.setFont (fonts::extraBold (13.5f));
    g.drawFittedText (headline, r.removeFromTop (26), juce::Justification::centredLeft, 1);

    g.setColour (t.panelText.withAlpha (0.65f));
    g.setFont (fonts::medium (11.0f));
    g.drawText ("Text from the Airwindopedia by Chris Johnson (Airwindows).",
                r.removeFromBottom (24).withTrimmedRight (80), juce::Justification::centredLeft, true);
}

// =============================================================================================
AboutPanel::AboutPanel (UiContext& c, std::function<void()> closeFn)
    : PanelBase (c), onClose (std::move (closeFn)), close (c, "Close")
{
    close.onClick = [this]
    {
        // close after this click has finished (the panel deletes itself)
        juce::Component::SafePointer<AboutPanel> safe (this);
        juce::MessageManager::callAsync ([safe] { if (safe != nullptr && safe->onClose) safe->onClose(); });
    };
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
