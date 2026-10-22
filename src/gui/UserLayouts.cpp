#include "UserLayouts.h"
#include "Theme.h"
#include "BinaryData.h"
#include "../dsp/EffectHost.h"

#include <map>

namespace dlaw::userlayouts
{
namespace
{
juce::File& testFolder()
{
    static juce::File f;
    return f;
}
} // namespace

void setFolderForTesting (const juce::File& f) { testFolder() = f; }

juce::File folder()
{
    if (testFolder() != juce::File())
        return testFolder();
    return getUserDataFolder().getChildFile ("Layouts");
}

juce::File fileFor (const juce::String& effectId)
{
    return folder().getChildFile (effectId + ".json");
}

juce::String builtInJson (const juce::String& effectId)
{
    int size = 0;
    if (auto* data = BinaryData::getNamedResource ((effectId + "_json").toRawUTF8(), size))
        return juce::String::fromUTF8 (data, size);
    return {};
}

juce::String effectiveJson (const juce::String& effectId, bool& isUser)
{
    isUser = false;
    const auto f = fileFor (effectId);
    if (f.existsAsFile() && f.getSize() < 512 * 1024)
    {
        isUser = true;
        return f.loadFileAsString();
    }
    return builtInJson (effectId);
}

juce::String toJson (const EffectLayout& layout, const std::vector<ParamFacts>& params)
{
    juce::Array<juce::var> pages;
    for (const auto& page : layout.pages)
    {
        juce::Array<juce::var> groups;
        for (const auto& row : page.rows)
            for (const auto& g : row.groups)
            {
                juce::Array<juce::var> controls;
                for (const auto& c : g.controls)
                {
                    const auto& name = params[(size_t) c.param].name;
                    const bool plain = c.label == name && c.type == ControlType::knob;
                    if (plain)
                    {
                        controls.add (name);
                        continue;
                    }
                    auto* o = new juce::DynamicObject();
                    o->setProperty ("param", name);
                    if (c.label != name)
                        o->setProperty ("label", c.label);
                    if (c.type == ControlType::stepped) { o->setProperty ("type", "stepped"); o->setProperty ("steps", c.steps); }
                    if (c.type == ControlType::toggle)  o->setProperty ("type", "switch");
                    controls.add (juce::var (o));
                }
                auto* go = new juce::DynamicObject();
                go->setProperty ("title", g.title.substring (0, 1) + g.title.substring (1).toLowerCase());
                go->setProperty ("controls", controls);
                groups.add (juce::var (go));
            }
        auto* po = new juce::DynamicObject();
        po->setProperty ("name", page.name);
        po->setProperty ("groups", groups);
        pages.add (juce::var (po));
    }

    juce::Array<juce::var> names;
    for (auto& p : params)
        names.add (p.name);

    auto* root = new juce::DynamicObject();
    root->setProperty ("formatVersion", 1);
    root->setProperty ("_help", "Your layout for this effect. Save in any text editor and the plugin updates. "
                                "Groups hold 1-5 controls; a control is a parameter name, or "
                                "{\"param\": name, \"label\": shown name, \"type\": \"knob\" | \"stepped\" | \"switch\"}. "
                                "Delete this file (or use Settings > Remove) to go back to the built-in layout. "
                                "Full guide: README.md in this folder.");
    root->setProperty ("_parameters", names);
    root->setProperty ("pages", pages);
    return juce::JSON::toString (juce::var (root));
}

bool create (EffectHost& fx)
{
    const juce::String id (fx.getInfo().id);
    const auto f = fileFor (id);
    if (f.existsAsFile())
        return true;
    if (! folder().createDirectory())
        return false;

    // a guide next to the files
    const auto readme = folder().getChildFile ("README.md");
    if (! readme.existsAsFile())
    {
        int size = 0;
        if (auto* data = BinaryData::getNamedResource ("layouts_readme_md", size))
            readme.replaceWithData (data, (size_t) size);
    }

    std::vector<ParamFacts> facts;
    for (int i = 0; i < fx.getNumParams(); ++i)
        facts.push_back ({ fx.getParamNames()[i], fx.detectSteps (i) });

    const auto layout = buildLayout (facts, builtInJson (id));
    return f.replaceWithText (toJson (layout, facts));
}

bool remove (const juce::String& effectId)
{
    const auto f = fileFor (effectId);
    if (! f.existsAsFile())
        return true;
    return f.moveToTrash() || f.deleteFile();
}

Status& status (const juce::String& effectId)
{
    static std::map<juce::String, Status> map;
    return map[effectId];
}
} // namespace dlaw::userlayouts
