#pragma once

#include "Components.h"

namespace dlaw
{
/** Base for the panels that open under the top row: draws the themed panel box. */
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
/** Effect chooser: categories, search, favourites (right-click or star), recently used, descriptions. */
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

private:
    struct Row;
    struct CategoryRow;

    int getNumRows() override;
    void paintListBoxItem (int, juce::Graphics&, int, int, bool) override {}
    juce::Component* refreshComponentForRow (int row, bool selected, juce::Component* existing) override;
    void textEditorTextChanged (juce::TextEditor&) override;

    void rebuildCategories();
    void rebuildList();
    void setHover (const EffectInfo*);

    std::function<void (const EffectInfo&)> onChoose;
    juce::TextEditor search;
    juce::ListBox list;
    juce::OwnedArray<CategoryRow> categoryRows;
    juce::Component categoryHolder;
    juce::String category = "Favourites";
    std::vector<const EffectInfo*> items;
    const EffectInfo* hovered = nullptr;
};

// =============================================================================================
/** "Load X? This can't be changed later." */
class ConfirmDialog : public juce::Component
{
public:
    ConfirmDialog (UiContext& c, const EffectInfo& info, std::function<void (bool)> onResult);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    UiContext& ctx;
    const EffectInfo& info;
    std::function<void (bool)> onResult;
    PillButton cancel, load;
    juce::Rectangle<int> box;
};

// =============================================================================================
/** Presets for this effect (left) + settings: zoom, theme, trims, reset, effect info. */
class SettingsPanel : public PanelBase,
                      private juce::ListBoxModel
{
public:
    SettingsPanel (UiContext& c, std::function<void()> onThemeOrZoomChanged, std::function<void()> onShowInfo);
    ~SettingsPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    enum class NameMode { none, saving, renaming };

    // presets
    int getNumRows() override { return presetFiles.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void rescanPresets();
    void loadPreset (const juce::File&);
    void showPresetMenu (const juce::File&);
    void startNaming (NameMode, const juce::File& file = {});
    void confirmName();
    void stopNaming();

    std::function<void()> onChanged, onShowInfo;
    juce::OwnedArray<PillButton> zoomButtons, themeButtons, tooltipButtons;
    PillButton openThemeFolder, resetButton, infoButton, savePreset, nameOk, nameCancel;
    juce::Slider inTrim, outTrim;
    std::unique_ptr<juce::SliderParameterAttachment> inAttach, outAttach;
    juce::Rectangle<int> left, right;

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
class AboutPanel : public PanelBase
{
public:
    explicit AboutPanel (UiContext& c, std::function<void()> onClose);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::function<void()> onClose;
    PillButton close;
};
} // namespace dlaw
