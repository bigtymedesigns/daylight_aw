#pragma once

#include <juce_core/juce_core.h>
#include <vector>

#include "../Identity.h"

namespace dlaw
{
enum class ControlType { knob, stepped, toggle };

struct ControlSpec
{
    int param = 0;            // index into the effect's parameters
    ControlType type = ControlType::knob;
    int steps = 0;            // stepped / toggle: number of positions
    juce::String label;       // shown name (defaults to the effect's parameter name)
};

struct GroupSpec
{
    juce::String title;       // empty = ungrouped (frame without a title)
    std::vector<ControlSpec> controls;
};

struct RowSpec  { std::vector<GroupSpec> groups; };
struct PageSpec { juce::String name; std::vector<RowSpec> rows; };

struct EffectLayout
{
    std::vector<PageSpec> pages;
    int rows = 1;                 // window rows after packing: 1 Small, 2 Medium, 3 Large (pages > 1: always 3 + tabs)
    juce::StringArray warnings;   // problems found in a layout file (never fatal)

    int numControls() const;
    bool hasTabs() const { return pages.size() > 1; }
};

/** Parameter facts the layout engine needs (from the effect itself). */
struct ParamFacts
{
    juce::String name;
    int detectedSteps = 0;        // >1 when the effect shows named choices
};

/**
    Builds the on-screen layout.
      - layoutJson empty  -> automatic layout: 5 per row, 3 rows per page, ungrouped.
      - layoutJson given  -> hand-made layout (groups, labels, control types, pages);
                             mistakes produce warnings and are repaired, any parameter
                             the file forgets is appended so nothing is ever missing.
*/
EffectLayout buildLayout (const std::vector<ParamFacts>& params, const juce::String& layoutJson);

/** Built-in hand-made layout (layouts/<id>.json, embedded) for an effect; empty when it has none. */
juce::String builtInLayoutJson (const juce::String& effectId);
} // namespace dlaw
