#include "PresetManager.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state,
                              juce::String name,
                              juce::String ext,
                              std::vector<FactoryPreset> factoryPresets)
    : apvts (state),
      pluginName (std::move (name)),
      extension (std::move (ext)),
      factory (std::move (factoryPresets))
{
}

juce::File PresetManager::getPresetsFolder() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Home")
                   .getChildFile (pluginName)
                   .getChildFile ("Presets");

    if (! dir.exists())
        dir.createDirectory();

    return dir;
}

juce::StringArray PresetManager::getUserPresetNames() const
{
    juce::StringArray names;

    auto files = getPresetsFolder().findChildFiles (juce::File::findFiles, false, "*." + extension);
    files.sort();

    for (const auto& f : files)
        names.add (f.getFileNameWithoutExtension());

    return names;
}

bool PresetManager::loadFactory (int index)
{
    if (index < 0 || index >= (int) factory.size())
        return false;

    const auto& preset = factory[(size_t) index];

    for (const auto& [id, value] : preset.values)
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value));

    currentName = preset.name;
    return true;
}

bool PresetManager::loadUser (const juce::String& name)
{
    return loadFromFile (getPresetsFolder().getChildFile (name + "." + extension), false);
}

bool PresetManager::loadFromFile (const juce::File& file, bool copyToLibrary)
{
    if (! file.existsAsFile())
        return false;

    auto xml = juce::parseXML (file);

    if (xml == nullptr || ! xml->hasTagName ("PluginPreset")
        || xml->getStringAttribute ("plugin") != pluginName)
        return false;                       // fichier d'un autre plugin ou illisible

    auto* stateXml = xml->getFirstChildElement();
    if (stateXml == nullptr)
        return false;

    apvts.replaceState (juce::ValueTree::fromXml (*stateXml));
    currentName = file.getFileNameWithoutExtension();

    if (copyToLibrary)
    {
        auto target = getPresetsFolder().getChildFile (file.getFileName());
        if (target != file)
            file.copyFileTo (target);
    }

    return true;
}

bool PresetManager::saveToFile (const juce::File& file)
{
    juce::XmlElement root ("PluginPreset");
    root.setAttribute ("plugin", pluginName);
    root.setAttribute ("version", 1);

    if (auto state = apvts.copyState().createXml())
        root.addChildElement (state.release());

    if (! file.replaceWithText (root.toString()))
        return false;

    currentName = file.getFileNameWithoutExtension();
    return true;
}
