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
/** Presets (left, coming in the next test build) + settings (right). */
class SettingsPanel : public PanelBase
{
public:
    SettingsPanel (UiContext& c, std::function<void()> onThemeOrZoomChanged);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    std::function<void()> onChanged;
    juce::OwnedArray<PillButton> zoomButtons, themeButtons;
    PillButton openThemeFolder, resetButton, infoButton, savePreset;
    juce::Slider inTrim, outTrim;
    std::unique_ptr<juce::SliderParameterAttachment> inAttach, outAttach;
    bool showInfo = false;
    juce::Rectangle<int> left, right;
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
