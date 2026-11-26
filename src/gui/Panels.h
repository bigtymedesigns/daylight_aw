#pragma once

#include "Components.h"
#include "../presets/PresetStore.h"

namespace dlaw
{
/** Base for the panels: draws the themed panel box (rounded, faint border, soft shadow). */
class PanelBase : public juce::Component
{
public:
    explicit PanelBase (UiContext& c) : ctx (c) {}
    void paint (juce::Graphics&) override;

protected:
    void drawHeading (juce::Graphics&, const juce::String& text, juce::Rectangle<int> area);
    UiContext& ctx;
};

// =============================================================================================
/** Effect browser: Favourites + collections, Recently used, New, Last 6 months, All effects,
    categories, search, "Latest versions only" view, descriptions. */
class SelectorPanel : public PanelBase,
                      private juce::ListBoxModel,
                      private juce::TextEditor::Listener
{
public:
    SelectorPanel (UiContext& c, std::function<void (const EffectInfo&)> onChoose);
    ~SelectorPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

    /** Folder keys (tests): "Favourites", "col:<name>", "Recent", "New", "Last6", "All", or a category. */
    void showFolder (const juce::String& key);
    const std::vector<const EffectInfo*>& getItems() const { return items; }
    juce::StringArray getFolderKeys() const;

private:
    struct Row;
    struct FolderRow;

    int getNumRows() override;
    void paintListBoxItem (int, juce::Graphics&, int, int, bool) override {}
    juce::Component* refreshComponentForRow (int row, bool selected, juce::Component* existing) override;
    void textEditorTextChanged (juce::TextEditor&) override;

    void rebuildFolders();
    void rebuildList();
    void layoutFolders();
    void setHover (const EffectInfo*);
    void showEffectMenu (const EffectInfo&);
    void showCollectionMenu (int index);
    void startNaming (int renameIndex, const juce::String& effectToAdd);
    void finishNaming (bool apply);
    int countFor (const juce::String& key) const;
    bool inFolder (const EffectInfo&, const juce::String& key) const;
    juce::Rectangle<int> viewToggleArea() const;
    void mouseUp (const juce::MouseEvent&) override;

    std::function<void (const EffectInfo&)> onChoose;
    juce::TextEditor search;
    juce::ListBox list;
    juce::OwnedArray<FolderRow> folderRows;
    juce::Component folderHolder;
    juce::Viewport folderView;   // the folder column scrolls when there are more folders than fit
    juce::String folder = "Favourites";
    std::vector<const EffectInfo*> items;
    const EffectInfo* hovered = nullptr;
    int hiddenMatches = 0;       // search results hidden by "Latest versions only"

    juce::TextEditor nameEditor; // new / renamed collection, inline in the folder column
    int renamingIndex = -1;      // -1 = new collection
    juce::String addAfterNaming; // effect to put into a new collection
    bool naming = false;
};

// =============================================================================================
/** "Load X? This can't be changed later." */
class ConfirmDialog : public juce::Component
{
public:
    /** replacing = the effect that is in use now (nullptr when choosing the first effect). */
    ConfirmDialog (UiContext& c, const EffectInfo& info, const EffectInfo* replacing, std::function<void (bool)> onResult);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    UiContext& ctx;
    const EffectInfo& info;
    const EffectInfo* replacing;
    std::function<void (bool)> onResult;
    PillButton cancel, load;
    juce::Rectangle<int> box;
};

// =============================================================================================
/** Dropdown from the effect name: presets for this effect (left); zoom, theme, reset, effect info (right). */
class SettingsPanel : public PanelBase,
                      private juce::ListBoxModel
{
public:
    SettingsPanel (UiContext& c, std::function<void()> onShowInfo, std::function<void()> onChangeEffect);
    ~SettingsPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

    static constexpr int kWidth = 460, kHeight = 300, kRightWidth = 180;

private:
    enum class NameMode { none, saving, renaming };

    int getNumRows() override { return presetFiles.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void rescanPresets();
    void loadPreset (const juce::File&);
    void showPresetMenu (const juce::File&);
    void startNaming (NameMode, const juce::File& file = {});
    void confirmName();
    void stopNaming();
    void showZoomMenu();
    void showThemeMenu();

    juce::Rectangle<int> leftArea() const;
    juce::Rectangle<int> rightArea() const;
    bool isCompact() const { return getHeight() < 260; }   // in the Small window
    static constexpr int kCompactLabelW = 50;

    std::function<void()> onShowInfo, onChangeEffect;
    PillButton zoomButton, themeButton, openThemeFolder, resetButton, infoButton, changeButton, savePreset, nameOk, nameCancel;

    PresetStore store { PresetStore::defaultRoot() };
    juce::Array<juce::File> presetFiles;
    juce::ListBox presetList;
    juce::TextEditor nameEditor;
    NameMode nameMode = NameMode::none;
    juce::File renamingFile;
    bool confirmReplace = false;
    juce::String message;
};

// =============================================================================================
/** Full Airwindopedia text for the chosen effect (scrollable). */
class InfoPanel : public PanelBase
{
public:
    InfoPanel (UiContext& c, std::function<void()> onClose);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

    /** Airwindopedia text embedded for this effect id (empty if none). */
    static juce::String docFor (const juce::String& effectId);

private:
    std::function<void()> onClose;
    PillButton close;
    juce::TextEditor text;
    juce::String headline;
};

// =============================================================================================
/** Compact About popup under the logo. Closes on a click outside or Esc (handled by the editor). */
class AboutPanel : public PanelBase
{
public:
    explicit AboutPanel (UiContext& c);
    void paint (juce::Graphics&) override;
    int preferredHeight() const;

    static constexpr int kWidth = 180;

private:
    juce::TextLayout paragraph() const;
    juce::TextLayout licences() const;
};
} // namespace dlaw
