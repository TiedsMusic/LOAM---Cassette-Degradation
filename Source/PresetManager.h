#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <utility>

// Un preset d'usine : identifiant du paramètre -> valeur normalisée (0 à 1)
struct FactoryPreset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values;
};

// Gestion des presets, réutilisable telle quelle pour LOAM, GROVE et FERN.
// - presets d'usine définis dans le code
// - presets utilisateur : fichiers dans le dossier de l'utilisateur
//   (Windows : %APPDATA%\Home\<plugin>\Presets)
// - export / import : un preset est un simple fichier XML qu'on peut partager
class PresetManager
{
public:
    PresetManager (juce::AudioProcessorValueTreeState& state,
                   juce::String pluginName,
                   juce::String fileExtension,
                   std::vector<FactoryPreset> factoryPresets);

    juce::File getPresetsFolder() const;
    const juce::String& getExtension() const { return extension; }

    int getNumFactory() const { return (int) factory.size(); }
    juce::String getFactoryName (int index) const { return factory[(size_t) index].name; }
    juce::StringArray getUserPresetNames() const;

    bool loadFactory (int index);
    bool loadUser (const juce::String& name);
    bool loadFromFile (const juce::File& file, bool copyToLibrary);
    bool saveToFile (const juce::File& file);

    juce::String currentName;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String pluginName, extension;
    std::vector<FactoryPreset> factory;
};
