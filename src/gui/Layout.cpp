#include "Layout.h"

namespace dlaw
{
int EffectLayout::numControls() const
{
    int n = 0;
    for (auto& p : pages)
        for (auto& r : p.rows)
            for (auto& g : r.groups)
                n += (int) g.controls.size();
    return n;
}

namespace
{
ControlSpec makeControl (int index, const ParamFacts& facts)
{
    ControlSpec c;
    c.param = index;
    c.label = facts.name;
    c.steps = facts.detectedSteps;
    c.type = facts.detectedSteps == 2 ? ControlType::toggle
           : facts.detectedSteps > 2  ? ControlType::stepped
                                      : ControlType::knob;
    return c;
}

/** Packs groups into rows of 5 and pages of 3 rows. Groups are never split unless > 5 controls. */
void packGroups (std::vector<GroupSpec> groups, const juce::String& pageName,
                 std::vector<PageSpec>& pages, juce::StringArray& warnings)
{
    // Split oversized groups first (a group must fit in one row).
    std::vector<GroupSpec> fitted;
    for (auto& g : groups)
    {
        if ((int) g.controls.size() <= kControlsPerRow)
        {
            if (! g.controls.empty())
                fitted.push_back (g);
            continue;
        }

        warnings.add ("Group \"" + g.title + "\" has more than " + juce::String (kControlsPerRow) + " controls - split over rows.");
        for (size_t i = 0; i < g.controls.size(); i += (size_t) kControlsPerRow)
        {
            GroupSpec part { g.title, {} };
            for (size_t k = i; k < juce::jmin (g.controls.size(), i + (size_t) kControlsPerRow); ++k)
                part.controls.push_back (g.controls[k]);
            fitted.push_back (part);
        }
    }

    PageSpec page { pageName, {} };
    RowSpec row;
    int used = 0;
    int continuation = 1;

    auto flushRow = [&]
    {
        if (row.groups.empty())
            return;
        if ((int) page.rows.size() == kRowsPerPage)
        {
            pages.push_back (page);
            page = PageSpec { pageName.isEmpty() ? juce::String() : pageName + " (" + juce::String (++continuation) + ")", {} };
        }
        page.rows.push_back (row);
        row = RowSpec {};
        used = 0;
    };

    for (auto& g : fitted)
    {
        const int n = (int) g.controls.size();
        if (used + n > kControlsPerRow)
            flushRow();
        row.groups.push_back (g);
        used += n;
    }
    flushRow();

    if (! page.rows.empty())
        pages.push_back (page);
}

std::vector<GroupSpec> autoGroups (const std::vector<ParamFacts>& params, const std::vector<int>& indices)
{
    std::vector<GroupSpec> groups;
    for (size_t i = 0; i < indices.size(); i += (size_t) kControlsPerRow)
    {
        GroupSpec g;
        for (size_t k = i; k < juce::jmin (indices.size(), i + (size_t) kControlsPerRow); ++k)
            g.controls.push_back (makeControl (indices[k], params[(size_t) indices[k]]));
        groups.push_back (g);
    }
    return groups;
}

bool parseGroups (const juce::var& groupsVar, const std::vector<ParamFacts>& params,
                  std::vector<GroupSpec>& out, std::vector<bool>& used, juce::StringArray& warnings)
{
    auto* arr = groupsVar.getArray();
    if (arr == nullptr)
        return false;

    for (auto& gv : *arr)
    {
        GroupSpec g;
        g.title = gv.getProperty ("title", "").toString().toUpperCase();

        if (auto* controls = gv.getProperty ("controls", {}).getArray())
        {
            for (auto& cv : *controls)
            {
                const auto name = cv.isString() ? cv.toString() : cv.getProperty ("param", "").toString();
                int index = -1;
                for (size_t i = 0; i < params.size(); ++i)
                    if (params[i].name == name)
                        index = (int) i;

                if (index < 0)
                {
                    warnings.add ("Unknown parameter \"" + name + "\" in layout - ignored.");
                    continue;
                }
                if (used[(size_t) index])
                {
                    warnings.add ("Parameter \"" + name + "\" appears twice in layout - second ignored.");
                    continue;
                }
                used[(size_t) index] = true;

                auto c = makeControl (index, params[(size_t) index]);
                if (cv.isObject())
                {
                    if (auto label = cv.getProperty ("label", {}); label.isString() && label.toString().isNotEmpty())
                        c.label = label.toString();

                    const auto type = cv.getProperty ("type", "").toString();
                    if (type == "knob")
                        c.type = ControlType::knob;
                    else if (type == "stepped" || type == "switch")
                    {
                        int steps = (int) cv.getProperty ("steps", c.steps);
                        if (steps < 2)
                            steps = type == "switch" ? 2 : juce::jmax (2, c.steps);
                        c.steps = steps;
                        c.type = (type == "switch" && steps == 2) ? ControlType::toggle : ControlType::stepped;
                    }
                    else if (type.isNotEmpty())
                        warnings.add ("Unknown control type \"" + type + "\" for \"" + name + "\".");
                }
                g.controls.push_back (c);
            }
        }
        out.push_back (g);
    }
    return true;
}
} // namespace

EffectLayout buildLayout (const std::vector<ParamFacts>& params, const juce::String& layoutJson)
{
    EffectLayout layout;
    std::vector<bool> used (params.size(), false);
    bool handMade = false;

    if (layoutJson.isNotEmpty())
    {
        auto root = juce::JSON::parse (layoutJson);
        if (! root.isObject())
        {
            layout.warnings.add ("Layout file could not be read - using the automatic layout.");
        }
        else if (auto* pages = root.getProperty ("pages", {}).getArray())
        {
            handMade = true;
            for (auto& pv : *pages)
            {
                std::vector<GroupSpec> groups;
                parseGroups (pv.getProperty ("groups", {}), params, groups, used, layout.warnings);
                packGroups (groups, pv.getProperty ("name", "").toString(), layout.pages, layout.warnings);
            }
        }
        else if (root.hasProperty ("groups"))
        {
            handMade = true;
            std::vector<GroupSpec> groups;
            parseGroups (root.getProperty ("groups", {}), params, groups, used, layout.warnings);
            packGroups (groups, {}, layout.pages, layout.warnings);
        }
        else
        {
            layout.warnings.add ("Layout file has neither \"pages\" nor \"groups\" - using the automatic layout.");
        }
    }

    // Anything the layout file forgot (or everything, for automatic layouts) is appended.
    std::vector<int> rest;
    for (size_t i = 0; i < params.size(); ++i)
        if (! used[i])
            rest.push_back ((int) i);

    if (handMade && ! rest.empty())
        layout.warnings.add (juce::String ((int) rest.size()) + " parameter(s) missing from the layout file were added at the end.");

    if (! rest.empty())
        packGroups (autoGroups (params, rest), {}, layout.pages, layout.warnings);   // on page(s) of their own

    int totalRows = 0;
    for (auto& p : layout.pages)
        totalRows += (int) p.rows.size();

    layout.small = layout.pages.size() <= 1 && totalRows <= 1;
    return layout;
}
EffectLayout buildListLayout (const std::vector<ParamFacts>& params)
{
    EffectLayout layout;
    const int n = (int) params.size();
    layout.small = n <= kControlsPerRow;

    for (int start = 0; start < n; start += kListRowsPerPage)
    {
        PageSpec page;
        const int end = juce::jmin (n, start + kListRowsPerPage);
        page.name = "Parameters " + juce::String (start + 1) + "-" + juce::String (end);
        for (int i = start; i < end; ++i)
        {
            ControlSpec c;
            c.param = i;
            c.type = ControlType::knob;   // continuous, like the original host view
            c.label = params[(size_t) i].name;
            page.rows.push_back ({ { GroupSpec { {}, { c } } } });
        }
        layout.pages.push_back (std::move (page));
    }
    return layout;
}
} // namespace dlaw
