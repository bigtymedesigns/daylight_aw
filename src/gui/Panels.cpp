#include "Panels.h"
#include "BinaryData.h"
#include "Fonts.h"

#include <algorithm>

namespace dlaw
{
namespace
{
const juce::String kDot = juce::String::fromUTF8 (" \xc2\xb7 ");
const juce::String kEllipsis = juce::String::fromUTF8 ("\xe2\x80\xa6");
constexpr int kFolderRowH = 26, kEffectRowH = 28, kFolderColumnW = 180;

juce::String monthYear (const char* isoDate)
{
    static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    const juce::String d (isoDate);
    const int m = d.substring (5, 7).getIntValue();
    return juce::isPositiveAndBelow (m - 1, 12) ? juce::String (months[m - 1]) + " " + d.substring (0, 4) : d;
}

void showMenu (juce::PopupMenu& m, juce::Component& owner, std::function<void (int)> done)
{
    m.showMenuAsync (juce::PopupMenu::Options().withParentComponent (owner.getTopLevelComponent()).withMousePosition(),
                     [safe = juce::Component::SafePointer<juce::Component> (&owner), done = std::move (done)] (int choice)
                     {
                         if (safe != nullptr && choice != 0)
                             done (choice);
                     });
}
} // namespace

// =============================================================================================
void PanelBase::paint (juce::Graphics& g)
{
    const auto& t = ctx.theme;
    auto r = getLocalBounds().toFloat().reduced (0.5f);

    juce::Path box;
    box.addRoundedRectangle (r, t.corner + 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 24, { 0, 10 }).drawForPath (g, box);

    g.setColour (t.panelFill);
    g.fillPath (box);
    g.setColour (t.panelBorder);
    g.strokePath (box, juce::PathStrokeType (1.0f));
}

void PanelBase::drawHeading (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area)
{
    g.setColour (ctx.theme.muted);
    g.setFont (fonts::caps (11.0f));
    g.drawText (text, area, juce::Justification::centredLeft, true);
}

// =============================================================================================
struct SelectorPanel::FolderRow : public juce::Component
{
    FolderRow (SelectorPanel& o, juce::String k, juce::String l, int ind = 0)
        : owner (o), key (std::move (k)), label (std::move (l)), indent (ind)
    {
        if (key != "-")
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = owner.ctx.theme;
        if (key == "-")
        {
            g.setColour (t.panelText.withAlpha (0.08f));
            g.fillRect (4, getHeight() / 2, getWidth() - 8, 1);
            return;
        }

        const bool on = owner.folder == key && owner.search.isEmpty();
        auto r = getLocalBounds().withTrimmedLeft (indent);
        if (on || isMouseOver())
        {
            g.setColour (on ? t.selectFill : t.panelText.withAlpha (0.05f));
            g.fillRoundedRectangle (r.toFloat(), t.corner - 1.0f);
        }

        auto text = r.reduced (10, 0);
        if (key == "+new")
        {
            g.setColour (t.accent);
            g.setFont (fonts::regular (12.5f));
            g.drawText ("+ New collection", text, juce::Justification::centredLeft, true);
            return;
        }

        g.setColour (on ? t.selectText : t.panelText.withAlpha (0.82f));
        if (key == "Favourites")
        {
            draw::star (g, { (float) text.getX() + 5.0f, (float) getHeight() * 0.5f }, 5.6f, true, on ? t.selectText : t.panelText.withAlpha (0.82f));
            text.removeFromLeft (16);
        }
        g.setFont (on ? fonts::semiBold (12.5f) : fonts::regular (12.5f));
        g.drawText (label, text.withTrimmedRight (26), juce::Justification::centredLeft, true);
        g.setColour (on ? t.selectText : t.muted);
        g.setFont (fonts::regular (11.0f));
        g.drawText (juce::String (count), text, juce::Justification::centredRight, false);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (key == "-" || ! getLocalBounds().contains (e.getPosition()))
            return;
        if (key == "+new")
        {
            owner.startNaming (-1, {});
            return;
        }
        if (e.mods.isPopupMenu() && key.startsWith ("col:"))
        {
            owner.showCollectionMenu (owner.ctx.settings.findCollection (key.substring (4)));
            return;
        }
        owner.showFolder (key);
    }

    SelectorPanel& owner;
    juce::String key, label;
    int indent = 0;
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
            g.setColour (t.panelText.withAlpha (0.04f));
            g.fillRect (getLocalBounds());
        }
        g.setColour (t.panelText.withAlpha (0.06f));
        g.fillRect (0, getHeight() - 1, getWidth(), 1);

        auto r = getLocalBounds().reduced (10, 0);
        auto starArea = r.removeFromRight (18);

        const auto nameFont = fonts::regular (12.5f);
        g.setColour (t.panelText);
        g.setFont (nameFont);
        const juce::String name (info->name);
        g.drawText (name, r, juce::Justification::centredLeft, true);

        if (! info->isLatest)
        {
            // older version of an effect (there is a newer one in its family)
            const float x = (float) r.getX() + fonts::width (nameFont, name) + 7.0f;
            g.setColour (t.muted);
            g.setFont (fonts::regular (11.0f));
            g.drawText ("older version", juce::Rectangle<float> (x, 0.0f, 90.0f, (float) getHeight()), juce::Justification::centredLeft, false);
        }

        if (detail.isNotEmpty())
        {
            g.setColour (t.muted);
            g.setFont (fonts::regular (11.0f));
            g.drawText (detail, r.withTrimmedRight (8), juce::Justification::centredRight, true);
        }

        if (fav || hov)
            draw::star (g, starArea.toFloat().getCentre(), 6.0f, fav, fav ? t.panelText.withAlpha (0.75f) : t.muted);
    }

    bool onStar (juce::Point<int> p) const { return p.x > getWidth() - 32; }

    void mouseEnter (const juce::MouseEvent&) override { owner.setHover (info); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (info == nullptr || ! getLocalBounds().contains (e.getPosition()))
            return;

        if (e.mods.isPopupMenu())
        {
            owner.showEffectMenu (*info);
            return;
        }
        if (onStar (e.getPosition()))
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
    juce::String detail;   // category or release month, right-aligned
};

SelectorPanel::SelectorPanel (UiContext& c, std::function<void (const EffectInfo&)> choose)
    : PanelBase (c), onChoose (std::move (choose))
{
    search.setTextToShowWhenEmpty ("Search names and descriptions", c.theme.muted);
    search.setFont (fonts::regular (12.5f));
    search.setIndents (10, 6);
    search.addListener (this);
    search.setTitle ("Search effects");
    addAndMakeVisible (search);

    list.setModel (this);
    list.setRowHeight (kEffectRowH);
    list.setOutlineThickness (0);
    list.setTitle ("Effects");
    addAndMakeVisible (list);

    folderView.setViewedComponent (&folderHolder, false);
    folderView.setScrollBarsShown (true, false);
    folderView.setScrollBarThickness (8);
    folderView.setTitle ("Folders");
    addAndMakeVisible (folderView);

    nameEditor.setFont (fonts::regular (12.5f));
    nameEditor.setIndents (8, 4);
    nameEditor.setInputRestrictions (40);
    nameEditor.setTitle ("Collection name");
    nameEditor.onReturnKey = [this] { finishNaming (true); };
    nameEditor.onEscapeKey = [this] { finishNaming (false); };
    nameEditor.onFocusLost = [this] { finishNaming (true); };
    folderHolder.addChildComponent (nameEditor);

    // Start in Favourites when there are any, else in All effects.
    if (ctx.settings.getFavourites().isEmpty() && ctx.settings.getCollections().empty())
        folder = "All";
    rebuildFolders();
}

SelectorPanel::~SelectorPanel()
{
    list.setModel (nullptr);
}

juce::StringArray SelectorPanel::getFolderKeys() const
{
    juce::StringArray keys;
    for (auto* r : folderRows)
        if (r->key != "-" && r->key != "+new")
            keys.add (r->key);
    return keys;
}

void SelectorPanel::rebuildFolders()
{
    folderRows.clear();
    folderRows.add (new FolderRow (*this, "Favourites", "Favourites"));
    for (auto& col : ctx.settings.getCollections())
        folderRows.add (new FolderRow (*this, "col:" + col.name, col.name, 14));
    folderRows.add (new FolderRow (*this, "+new", {}, 14));
    folderRows.add (new FolderRow (*this, "Recent", "Recently used"));
    if (countFor ("New") > 0 || folder == "New")
        folderRows.add (new FolderRow (*this, "New", "New"));
    folderRows.add (new FolderRow (*this, "Last6", "Last 6 months"));
    folderRows.add (new FolderRow (*this, "All", "All effects"));
    folderRows.add (new FolderRow (*this, "-", {}));

    juce::StringArray cats;
    for (auto& e : getAllEffects())
        cats.addIfNotAlreadyThere (e.category);
    cats.sortNatural();
    for (auto& cat : cats)
        folderRows.add (new FolderRow (*this, cat, cat));

    // a folder that no longer exists (deleted collection, empty New) -> All effects
    if (! getFolderKeys().contains (folder))
        folder = "All";

    for (auto* r : folderRows)
        folderHolder.addAndMakeVisible (r);
    nameEditor.toFront (false);
    layoutFolders();
    refresh();
}

bool SelectorPanel::inFolder (const EffectInfo& e, const juce::String& key) const
{
    if (key == "New")   return e.isNew;
    if (key == "Last6") return e.isRecent;
    if (key == "All")   return true;
    return key == e.category;
}

int SelectorPanel::countFor (const juce::String& key) const
{
    auto& s = ctx.settings;
    if (key == "Favourites") return s.getFavourites().size();
    if (key == "Recent")     return s.getRecent().size();
    if (key.startsWith ("col:"))
    {
        const int i = s.findCollection (key.substring (4));
        return i >= 0 ? s.getCollections()[(size_t) i].ids.size() : 0;
    }
    int n = 0;
    for (auto& e : getAllEffects())
        if (inFolder (e, key) && s.isListed (e))
            ++n;
    return n;
}

void SelectorPanel::refresh()
{
    // Collections may have changed in another instance (settings are shared live): rebuild the folders.
    juce::StringArray want, have;
    for (auto& col : ctx.settings.getCollections())
        want.add ("col:" + col.name);
    for (auto* r : folderRows)
        if (r->key.startsWith ("col:"))
            have.add (r->key);
    if (want != have)
    {
        rebuildFolders();   // calls refresh() again with matching rows
        return;
    }

    for (auto* r : folderRows)
        r->count = countFor (r->key);
    rebuildList();
}

void SelectorPanel::showFolder (const juce::String& key)
{
    folder = key;
    search.clear();
    rebuildList();
}

void SelectorPanel::rebuildList()
{
    items.clear();
    hiddenMatches = 0;
    const auto q = search.getText().trim().toLowerCase();
    auto& s = ctx.settings;

    auto fromIds = [this] (const juce::StringArray& ids)
    {
        for (auto& id : ids)
            if (auto* e = findEffect (id.toStdString()))
                items.push_back (e);
    };
    auto byName = [] (const EffectInfo* a, const EffectInfo* b) { return juce::String (a->name).compareNatural (b->name) < 0; };

    if (q.isNotEmpty())
    {
        for (auto& e : getAllEffects())
            if (juce::String (e.name).toLowerCase().contains (q) || juce::String (e.description).toLowerCase().contains (q))
            {
                if (s.isListed (e)) items.push_back (&e);
                else                ++hiddenMatches;
            }
        std::sort (items.begin(), items.end(), byName);
    }
    else if (folder == "Favourites") fromIds (s.getFavourites());
    else if (folder == "Recent")     fromIds (s.getRecent());
    else if (folder.startsWith ("col:"))
    {
        const int i = s.findCollection (folder.substring (4));
        if (i >= 0)
            fromIds (s.getCollections()[(size_t) i].ids);
    }
    else
    {
        for (auto& e : getAllEffects())
            if (inFolder (e, folder) && s.isListed (e))
                items.push_back (&e);

        if (folder == "Last6")
            std::sort (items.begin(), items.end(), [&] (auto* a, auto* b)
                       {
                           const int c = juce::String (a->released).compare (b->released);
                           return c != 0 ? c > 0 : byName (a, b);   // newest first
                       });
        else
            std::sort (items.begin(), items.end(), byName);
    }

    list.updateContent();
    list.repaint();
    for (auto* r : folderRows)
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
    r->detail = {};
    if (r->info != nullptr)
    {
        // every row: the Airwindows release date; in mixed lists the category before it
        const bool oneCategory = search.isEmpty() && ! (folder == "Favourites" || folder == "Recent" || folder == "All"
                                                        || folder == "New" || folder == "Last6" || folder.startsWith ("col:"));
        r->detail = monthYear (r->info->released);
        if (! oneCategory && ! (search.isEmpty() && folder == "Last6"))
            r->detail = juce::String (r->info->category) + kDot + r->detail;
    }
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
        repaint (getLocalBounds().removeFromBottom (46));
    }
}

// ---- menus + naming ------------------------------------------------------------------------
void SelectorPanel::showEffectMenu (const EffectInfo& info)
{
    auto& s = ctx.settings;
    const juce::String id (info.id);
    juce::PopupMenu m;
    m.addItem (1, s.isFavourite (id) ? "Remove from Favourites" : "Add to Favourites");

    juce::PopupMenu cols;
    const auto& all = s.getCollections();
    for (int i = 0; i < (int) all.size(); ++i)
        cols.addItem (100 + i, all[(size_t) i].name, true, s.isInCollection (i, id));
    if (! all.empty())
        cols.addSeparator();
    cols.addItem (2, "New collection" + kEllipsis);
    m.addSubMenu ("Add to collection", cols);

    if (folder.startsWith ("col:") && search.isEmpty())
        m.addItem (3, "Remove from " + folder.substring (4));

    juce::Component::SafePointer<SelectorPanel> safe (this);
    showMenu (m, *this, [safe, id] (int choice)
    {
        if (safe == nullptr) return;
        auto& self = *safe;
        auto& st = self.ctx.settings;
        if (choice == 1) st.toggleFavourite (id);
        else if (choice == 2) { self.startNaming (-1, id); return; }
        else if (choice == 3) st.toggleInCollection (st.findCollection (self.folder.substring (4)), id);
        else if (choice >= 100) st.toggleInCollection (choice - 100, id);
        self.refresh();
    });
}

void SelectorPanel::showCollectionMenu (int index)
{
    auto& s = ctx.settings;
    if (! juce::isPositiveAndBelow (index, (int) s.getCollections().size()))
        return;
    juce::PopupMenu m;
    m.addItem (1, "Rename" + kEllipsis);
    m.addItem (2, "Move up", index > 0);
    m.addItem (3, "Move down", index < (int) s.getCollections().size() - 1);
    m.addSeparator();
    m.addItem (4, "Delete collection");

    juce::Component::SafePointer<SelectorPanel> safe (this);
    showMenu (m, *this, [safe, index] (int choice)
    {
        if (safe == nullptr) return;
        auto& self = *safe;
        auto& st = self.ctx.settings;
        if (! juce::isPositiveAndBelow (index, (int) st.getCollections().size()))
            return;   // changed in another window meanwhile
        const auto key = "col:" + st.getCollections()[(size_t) index].name;
        if (choice == 1) { self.startNaming (index, {}); return; }
        if (choice == 2 || choice == 3)
        {
            st.moveCollection (index, choice == 2 ? -1 : 1);
        }
        else if (choice == 4)
        {
            st.removeCollection (index);   // the effects and favourites stay as they are
            if (self.folder == key)
                self.folder = "Favourites";
        }
        self.rebuildFolders();
    });
}

void SelectorPanel::startNaming (int renameIndex, const juce::String& effectToAdd)
{
    renamingIndex = renameIndex;
    addAfterNaming = effectToAdd;
    naming = true;

    // the editor covers the collection's row, or the "+ New collection" row
    const juce::String key = renameIndex >= 0 ? "col:" + ctx.settings.getCollections()[(size_t) renameIndex].name : juce::String ("+new");
    for (auto* r : folderRows)
        if (r->key == key)
            nameEditor.setBounds (r->getBounds().withTrimmedLeft (r->indent));

    nameEditor.setText (renameIndex >= 0 ? ctx.settings.getCollections()[(size_t) renameIndex].name : juce::String(), false);
    nameEditor.setTextToShowWhenEmpty ("Collection name", ctx.theme.muted);
    nameEditor.applyColourToAllText (ctx.theme.panelText);
    nameEditor.selectAll();
    nameEditor.setVisible (true);
    nameEditor.toFront (true);
    nameEditor.grabKeyboardFocus();
}

void SelectorPanel::finishNaming (bool apply)
{
    if (! naming)
        return;
    naming = false;
    const auto text = nameEditor.getText().trim();
    // hide after this callback (we may be inside the editor's own key/focus handling)
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<SelectorPanel> (this)]
                                     { if (safe != nullptr) safe->nameEditor.setVisible (false); });

    if (! apply || text.isEmpty())
        return;

    auto& s = ctx.settings;
    if (renamingIndex >= 0)
    {
        const bool wasShown = folder == "col:" + s.getCollections()[(size_t) renamingIndex].name;
        s.renameCollection (renamingIndex, text);
        if (wasShown)
            folder = "col:" + s.getCollections()[(size_t) renamingIndex].name;
    }
    else
    {
        const int i = s.addCollection (text);
        if (addAfterNaming.isNotEmpty())
            s.toggleInCollection (i, addAfterNaming);
        else
            folder = "col:" + s.getCollections()[(size_t) i].name;
    }
    rebuildFolders();
}

// ---- layout + drawing -------------------------------------------------------------------------
void SelectorPanel::layoutFolders()
{
    int y = 0;
    for (auto* row : folderRows)
    {
        const int h = row->key == "-" ? 13 : kFolderRowH;
        row->setBounds (0, y, folderHolder.getWidth(), h);
        y += h + (row->key == "-" ? 0 : 1);
    }
    folderHolder.setSize (folderHolder.getWidth(), y);
}

juce::Rectangle<int> SelectorPanel::viewToggleArea() const
{
    const auto b = list.getBounds();
    return { b.getRight() - 150, b.getY() - 24, 150, 20 };
}

void SelectorPanel::resized()
{
    auto r = getLocalBounds().reduced (1);
    r.removeFromBottom (44); // description strip

    auto left = r.removeFromLeft (kFolderColumnW).reduced (8, 10);
    folderView.setBounds (left);
    folderHolder.setSize (left.getWidth(), 10);
    layoutFolders();
    const bool scrolls = folderHolder.getHeight() > left.getHeight();
    folderHolder.setSize (left.getWidth() - (scrolls ? 10 : 0), folderHolder.getHeight());
    layoutFolders();

    r.removeFromLeft (1);
    r = r.reduced (10, 10);
    search.setBounds (r.removeFromTop (28));
    r.removeFromTop (28);   // heading + view switch
    list.setBounds (r);
}

void SelectorPanel::mouseUp (const juce::MouseEvent& e)
{
    if (viewToggleArea().contains (e.getPosition()))
    {
        ctx.settings.setLatestOnly (! ctx.settings.getLatestOnly());
        refresh();
    }
}

void SelectorPanel::paint (juce::Graphics& g)
{
    PanelBase::paint (g);
    const auto& t = ctx.theme;
    auto r = getLocalBounds().reduced (1);
    auto strip = r.removeFromBottom (44);

    g.setColour (t.panelText.withAlpha (0.08f));
    g.fillRect (strip.getX() + kFolderColumnW + 1, strip.getY(), strip.getWidth() - kFolderColumnW - 1, 1);
    g.fillRect (r.getX() + kFolderColumnW, r.getY() + 1, 1, r.getHeight() + strip.getHeight() - 2);

    // list heading
    const auto lb = list.getBounds();
    auto heading = juce::Rectangle<int> (lb.getX(), lb.getY() - 24, lb.getWidth() - 160, 20);
    const auto q = search.getText().trim();
    juce::String title;
    if (q.isNotEmpty())                  title = "SEARCH";
    else if (folder == "Favourites")     title = "FAVOURITES";
    else if (folder == "Recent")         title = "RECENTLY USED";
    else if (folder == "All")            title = "ALL EFFECTS";
    else if (folder == "New")            title = "NEW IN THIS UPDATE";
    else if (folder == "Last6")          title = "LAST 6 MONTHS";
    else if (folder.startsWith ("col:")) title = folder.substring (4).toUpperCase();
    else                                 title = folder.toUpperCase();
    drawHeading (g, title + kDot + juce::String ((int) items.size()), heading);

    // "Latest versions only" switch (a small check box)
    {
        const auto a = viewToggleArea();
        const bool on = ctx.settings.getLatestOnly();
        auto box = juce::Rectangle<float> ((float) a.getRight() - 13.0f, (float) a.getCentreY() - 6.5f, 13.0f, 13.0f);
        g.setColour (on ? t.accent : t.buttonBorder.withMultipliedAlpha (2.0f));
        g.drawRoundedRectangle (box.reduced (0.5f), 3.0f, 1.2f);
        if (on)
        {
            juce::Path tick;
            tick.startNewSubPath (box.getX() + 3.0f, box.getCentreY() + 0.2f);
            tick.lineTo (box.getX() + 5.6f, box.getBottom() - 3.4f);
            tick.lineTo (box.getRight() - 2.8f, box.getY() + 3.4f);
            g.strokePath (tick, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (on ? t.panelText : t.muted);
        g.setFont (fonts::regular (11.5f));
        g.drawText ("Latest versions only", a.withTrimmedRight (20), juce::Justification::centredRight, false);
    }

    if (items.empty())
    {
        g.setColour (t.muted);
        g.setFont (fonts::regular (12.5f));
        juce::String msg = "Nothing found.";
        if (q.isEmpty() && folder == "Favourites")
            msg = "No favourites yet - click an effect's star, or right-click it.";
        else if (q.isEmpty() && folder.startsWith ("col:"))
            msg = "Empty collection - right-click an effect > Add to collection.";
        else if (q.isEmpty() && folder == "Recent")
            msg = "Effects you load appear here.";
        if (hiddenMatches > 0)
            msg = juce::String (hiddenMatches) + (hiddenMatches == 1 ? " older version matches" : " older versions match")
                  + " - untick \"Latest versions only\" to see " + (hiddenMatches == 1 ? "it." : "them.");
        g.drawFittedText (msg, lb.reduced (8, 6), juce::Justification::topLeft, 3);
    }

    g.setColour (hovered != nullptr ? t.panelText.withAlpha (0.85f) : t.muted);
    g.setFont (fonts::regular (12.0f));
    const juce::String desc = hovered != nullptr ? juce::String (hovered->description)
                                                 : "Hover an effect to see what it does. Star it, or right-click it for collections.";
    g.drawFittedText (desc, strip.withTrimmedLeft (kFolderColumnW + 1).reduced (12, 4), juce::Justification::centredLeft, 2);
}

// =============================================================================================
ConfirmDialog::ConfirmDialog (UiContext& c, const EffectInfo& i, const EffectInfo* rep, std::function<void (bool)> result)
    : ctx (c), info (i), replacing (rep), onResult (std::move (result)),
      cancel (c, "Cancel"), load (c, rep != nullptr ? "Change effect" : "Load effect")
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
    box = juce::Rectangle<int> (0, 0, 360, replacing != nullptr ? 166 : 148).withCentre (getLocalBounds().getCentre());
    auto buttons = box.reduced (18, 16).removeFromBottom (30);
    load.setBounds (buttons.removeFromRight (replacing != nullptr ? 124 : 110));
    buttons.removeFromRight (8);
    cancel.setBounds (buttons.removeFromRight (80));
}

void ConfirmDialog::paint (juce::Graphics& g)
{
    const auto& t = ctx.theme;
    g.fillAll (juce::Colours::black.withAlpha (0.45f));

    juce::Path p;
    p.addRoundedRectangle (box.toFloat(), t.corner + 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 26, { 0, 10 }).drawForPath (g, p);
    g.setColour (t.panelFill.withAlpha (1.0f));
    g.fillPath (p);
    g.setColour (t.panelBorder);
    g.strokePath (p, juce::PathStrokeType (1.0f));

    auto r = box.reduced (18, 16);
    g.setColour (t.panelText);
    g.setFont (fonts::semiBold (15.0f));
    const juce::String name (info.name);
    g.drawText ((replacing != nullptr ? "Change to " : "Load ") + name + "?", r.removeFromTop (24), juce::Justification::centredLeft, true);
    r.removeFromTop (4);
    g.setColour (t.panelText.withAlpha (0.8f));
    g.setFont (fonts::regular (12.5f));
    const auto text = replacing != nullptr
        ? juce::String (replacing->name) + "'s settings, A/B and preset name are replaced by " + name
              + "'s defaults. Automation on this track will move " + name + "'s controls instead."
        : juce::String ("You can change it later: click the effect name > Change effect.");
    g.drawFittedText (text, r.removeFromTop (replacing != nullptr ? 58 : 40), juce::Justification::topLeft, 4);
}

bool ConfirmDialog::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { if (onResult) onResult (false); return true; }
    if (k == juce::KeyPress::returnKey) { if (onResult) onResult (true); return true; }
    return false;
}

// =============================================================================================
SettingsPanel::SettingsPanel (UiContext& c, std::function<void()> showInfoFn, std::function<void()> changeFn)
    : PanelBase (c), onShowInfo (std::move (showInfoFn)), onChangeEffect (std::move (changeFn)),
      zoomButton (c, {}), themeButton (c, {}), openThemeFolder (c, "Open theme folder" + kEllipsis),
      resetButton (c, "Reset to defaults"), infoButton (c, "Effect info"), changeButton (c, "Change effect" + kEllipsis),
      savePreset (c, "Save preset"),
      nameOk (c, "Save"), nameCancel (c, "Cancel")
{
    zoomButton.dropdown = themeButton.dropdown = true;
    zoomButton.setTitle ("Zoom");
    themeButton.setTitle ("Theme");
    zoomButton.onClick = [this] { showZoomMenu(); };
    themeButton.onClick = [this] { showThemeMenu(); };
    addAndMakeVisible (zoomButton);
    addAndMakeVisible (themeButton);

    openThemeFolder.link = true;
    openThemeFolder.fontSize = 11.5f;
    openThemeFolder.onClick = []
    {
        ensureUserThemeExists();
        getUserThemeFolder().startAsProcess();
    };
    addAndMakeVisible (openThemeFolder);

    resetButton.icon = PillButton::Icon::reset;
    infoButton.icon = PillButton::Icon::info;
    changeButton.icon = PillButton::Icon::change;
    resetButton.onClick = [this] { ctx.processor.resetToDefaults(); };
    infoButton.onClick = [this] { if (onShowInfo) onShowInfo(); };
    changeButton.onClick = [this] { if (onChangeEffect) onChangeEffect(); };
    addAndMakeVisible (resetButton);
    addAndMakeVisible (infoButton);
    addAndMakeVisible (changeButton);

    // ---- presets ----
    savePreset.onClick = [this] { startNaming (NameMode::saving); };
    savePreset.setEnabled (ctx.processor.hasEffect());
    savePreset.fontSize = 12.0f;
    addAndMakeVisible (savePreset);

    nameEditor.setFont (fonts::regular (12.5f));
    nameEditor.setIndents (8, 5);
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
    presetList.setRowHeight (28);
    presetList.setOutlineThickness (0);
    presetList.setTitle ("Presets");
    presetList.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (presetList);

    rescanPresets();
    refresh();
}

SettingsPanel::~SettingsPanel()
{
    presetList.setModel (nullptr);
}

void SettingsPanel::refresh()
{
    zoomButton.setButtonText (juce::String (ctx.settings.getZoom()) + " %");
    themeButton.setButtonText (ctx.settings.getTheme() == ThemeId::user ? "User" : "Default");
    nameEditor.applyColourToAllText (ctx.theme.panelText);
    presetList.repaint();
    repaint();
}

void SettingsPanel::showZoomMenu()
{
    juce::PopupMenu m;
    for (auto level : GlobalSettings::zoomLevels)
        m.addItem (level, juce::String (level) + " %", true, level == ctx.settings.getZoom());
    juce::Component::SafePointer<SettingsPanel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withParentComponent (getTopLevelComponent()).withTargetComponent (&zoomButton),
                     [safe] (int choice) { if (safe != nullptr && choice > 0) safe->ctx.settings.setZoom (choice); });
}

void SettingsPanel::showThemeMenu()
{
    juce::PopupMenu m;
    const bool user = ctx.settings.getTheme() == ThemeId::user;
    m.addItem (1, "Default", true, ! user);
    m.addItem (2, "User", true, user);
    juce::Component::SafePointer<SettingsPanel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withParentComponent (getTopLevelComponent()).withTargetComponent (&themeButton),
                     [safe] (int choice)
                     {
                         if (safe == nullptr || choice == 0) return;
                         if (choice == 2)
                             ensureUserThemeExists();
                         safe->ctx.settings.setTheme (choice == 2 ? ThemeId::user : ThemeId::def);
                     });
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
        g.fillRoundedRectangle (juce::Rectangle<float> (0.0f, 1.0f, (float) width, (float) height - 2.0f), t.corner - 1.0f);
    }

    g.setColour (current ? t.selectText : t.panelText);
    g.setFont (current ? fonts::semiBold (12.5f) : fonts::regular (12.5f));
    g.drawText (name, juce::Rectangle<int> (10, 0, width - 20, height), juce::Justification::centredLeft, true);
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
    m.addItem (2, "Rename" + kEllipsis);
    m.addItem (3, "Move to trash");
    m.addSeparator();
    m.addItem (4, "Show in folder");

    juce::Component::SafePointer<SettingsPanel> safe (this);
    showMenu (m, *this, [safe, file] (int choice)
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
juce::Rectangle<int> SettingsPanel::leftArea() const
{
    return getLocalBounds().withTrimmedRight (kRightWidth).reduced (14);
}

juce::Rectangle<int> SettingsPanel::rightArea() const
{
    return getLocalBounds().removeFromRight (kRightWidth).reduced (14);
}

void SettingsPanel::resized()
{
    auto l = leftArea();
    l.removeFromTop (14 + 10);   // heading
    if (nameMode == NameMode::none)
    {
        savePreset.setBounds (l.removeFromTop (28));
    }
    else
    {
        nameEditor.setBounds (l.removeFromTop (28));
        l.removeFromTop (6);
        auto buttons = l.removeFromTop (26);
        nameOk.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2 - 3));
        buttons.removeFromLeft (6);
        nameCancel.setBounds (buttons);
    }
    l.removeFromTop (message.isNotEmpty() ? 32 : 8);
    l.removeFromBottom (22);     // footer hint
    presetList.setBounds (l);

    auto r = rightArea();
    if (isCompact())
    {
        // Small window (1 row): labels next to the dropdowns, Reset + Info side by side
        auto row = [&] (PillButton& b) { b.setBounds (r.removeFromTop (24).withTrimmedLeft (kCompactLabelW)); };
        row (zoomButton);
        r.removeFromTop (6);
        row (themeButton);
        r.removeFromTop (2);
        openThemeFolder.setBounds (r.removeFromTop (18));
        r.removeFromTop (5 + 1 + 5);
        auto pair = r.removeFromTop (24);
        resetButton.setButtonText ("Reset");
        infoButton.setButtonText ("Info");
        resetButton.setBounds (pair.removeFromLeft (pair.getWidth() / 2 - 3));
        pair.removeFromLeft (6);
        infoButton.setBounds (pair);
        r.removeFromTop (6);
        changeButton.setBounds (r.removeFromTop (24));
        return;
    }
    resetButton.setButtonText ("Reset to defaults");
    infoButton.setButtonText ("Effect info");
    r.removeFromTop (14 + 8);
    zoomButton.setBounds (r.removeFromTop (28));
    r.removeFromTop (14 + 14 + 8);
    themeButton.setBounds (r.removeFromTop (28));
    r.removeFromTop (6);
    openThemeFolder.setBounds (r.removeFromTop (18));
    r.removeFromTop (8 + 1 + 8);
    resetButton.setBounds (r.removeFromTop (28));
    r.removeFromTop (8);
    infoButton.setBounds (r.removeFromTop (28));
    r.removeFromTop (8);
    changeButton.setBounds (r.removeFromTop (28));
}

void SettingsPanel::paint (juce::Graphics& g)
{
    PanelBase::paint (g);
    const auto& t = ctx.theme;
    auto* fx = ctx.processor.getEffectHost();

    g.setColour (t.panelText.withAlpha (0.08f));
    g.fillRect (getWidth() - kRightWidth, 1, 1, getHeight() - 2);

    // presets (left)
    const auto l = leftArea();
    drawHeading (g, "PRESETS", l.withHeight (14));

    if (message.isNotEmpty())
    {
        g.setColour (t.panelText);
        g.setFont (fonts::medium (11.5f));
        g.drawFittedText (message, presetList.getBounds().withY (presetList.getY() - 30).withHeight (28),
                          juce::Justification::centredLeft, 2);
    }

    if (presetFiles.isEmpty() && fx != nullptr)
    {
        g.setColour (t.muted);
        g.setFont (fonts::regular (12.0f));
        g.drawFittedText ("No presets for " + juce::String (fx->getInfo().name) + " yet.",
                          presetList.getBounds().reduced (10, 6).withHeight (34), juce::Justification::topLeft, 2);
    }

    g.setColour (t.muted);
    g.setFont (fonts::regular (11.0f));
    const bool themeProblem = ctx.settings.getTheme() == ThemeId::user && ! ctx.theme.warnings.isEmpty();
    g.drawText (themeProblem ? "User theme: " + ctx.theme.warnings[0]
                             : "Loads into slot " + juce::String (ctx.processor.getActiveAB() == 0 ? "A" : "B") + kDot + "right-click for more",
                l.withTop (l.getBottom() - 16), juce::Justification::centredLeft, true);

    // settings (right)
    const auto r = rightArea();
    if (isCompact())
    {
        drawHeading (g, "ZOOM", zoomButton.getBounds().withX (r.getX()).withWidth (kCompactLabelW));
        drawHeading (g, "THEME", themeButton.getBounds().withX (r.getX()).withWidth (kCompactLabelW));
        g.setColour (t.panelText.withAlpha (0.08f));
        g.fillRect (r.getX(), openThemeFolder.getBottom() + 5, r.getWidth(), 1);
        return;
    }
    drawHeading (g, "ZOOM", r.withHeight (14));
    drawHeading (g, "THEME", juce::Rectangle<int> (r.getX(), zoomButton.getBottom() + 14, r.getWidth(), 14));
    g.setColour (t.panelText.withAlpha (0.08f));
    g.fillRect (r.getX(), openThemeFolder.getBottom() + 8, r.getWidth(), 1);

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
    text.setFont (fonts::regular (12.5f));
    text.setIndents (4, 2);
    text.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    text.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    text.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
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
    text.applyColourToAllText (ctx.theme.panelText.withAlpha (0.88f));
    text.moveCaretToTop (false);
    repaint();
}

void InfoPanel::resized()
{
    auto r = getLocalBounds().reduced (14, 12);
    auto bottom = r.removeFromBottom (28);
    close.setBounds (bottom.removeFromRight (76));
    r.removeFromTop (44);
    r.removeFromBottom (6);
    text.setBounds (r);
}

void InfoPanel::paint (juce::Graphics& g)
{
    // drawn without the base class's own fill: the editor's text editor is transparent
    PanelBase::paint (g);
    const auto& t = ctx.theme;
    auto r = getLocalBounds().reduced (14, 12);
    auto* fx = ctx.processor.getEffectHost();

    drawHeading (g, "EFFECT INFO" + (fx != nullptr ? kDot + juce::String (fx->getInfo().name).toUpperCase() : juce::String()),
                 r.removeFromTop (14));
    r.removeFromTop (4);
    g.setColour (t.panelText);
    g.setFont (fonts::semiBold (13.0f));
    g.drawFittedText (headline, r.removeFromTop (24), juce::Justification::centredLeft, 1);

    g.setColour (t.muted);
    g.setFont (fonts::regular (11.0f));
    g.drawText ("Text from the Airwindopedia by Chris Johnson (Airwindows).",
                r.removeFromBottom (28).withTrimmedRight (86), juce::Justification::centredLeft, true);
}

// =============================================================================================
AboutPanel::AboutPanel (UiContext& c) : PanelBase (c)
{
    setWantsKeyboardFocus (true);
    setTitle ("About Daylight_aw");
}

juce::TextLayout AboutPanel::paragraph() const
{
    juce::AttributedString s;
    s.setLineSpacing (2.0f);
    s.append ("Airwindows code used under the MIT licence. ", fonts::regular (10.0f), ctx.theme.panelText);
    s.append ("Not an official Airwindows release.", fonts::semiBold (10.0f), ctx.theme.panelText);
    juce::TextLayout layout;
    layout.createLayout (s, (float) (kWidth - 20));
    return layout;
}

juce::TextLayout AboutPanel::licences() const
{
    juce::AttributedString s;
    s.setLineSpacing (1.0f);
    s.append (juce::String::fromUTF8 ("Airwindows (MIT) \xc2\xb7 Airwindows Consolidated (MIT) \xc2\xb7 JUCE (AGPLv3) \xc2\xb7 "
                                      "Inter, Jost (SIL OFL 1.1) \xc2\xb7 Daylight_aw: AGPLv3"),
              fonts::regular (9.0f), ctx.theme.muted);
    juce::TextLayout layout;
    layout.createLayout (s, (float) (kWidth - 20));
    return layout;
}

int AboutPanel::preferredHeight() const
{
    return (int) std::ceil (8.0f + 16.0f + 3.0f + paragraph().getHeight() + 4.0f + licences().getHeight() + 9.0f);
}

void AboutPanel::paint (juce::Graphics& g)
{
    PanelBase::paint (g);
    const auto& t = ctx.theme;
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);

    auto top = r.removeFromTop (16.0f);
    const auto nameFont = fonts::logoRegular (12.0f).withExtraKerningFactor (0.05f);
    g.setColour (t.panelText);
    g.setFont (nameFont);
    g.drawText (kPluginName, top, juce::Justification::centredLeft, false);
    g.setColour (t.muted);
    g.setFont (fonts::regular (9.0f));
    g.drawText ("v" + juce::String (kVersion), top.withTrimmedLeft (fonts::width (nameFont, kPluginName) + 6.0f),
                juce::Justification::centredLeft, false);

    r.removeFromTop (3.0f);
    auto p = paragraph();
    p.draw (g, r.removeFromTop (p.getHeight()));
    r.removeFromTop (4.0f);
    auto l = licences();
    l.draw (g, r.removeFromTop (l.getHeight()));
}
} // namespace dlaw
