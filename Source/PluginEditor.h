#pragma once

#include "PluginProcessor.h"
#include <array>
#include <memory>

// Interface fonctionnelle au format A5 portrait (450 x 639 px), calée sur le gabarit :
// titre + presets en haut, 8 potards en 2 colonnes de 4.
// Les dessins viendront remplacer le fond et les potards plus tard.
class LoamEditor : public juce::AudioProcessorEditor
{
public:
    explicit LoamEditor (LoamProcessor&);
    ~LoamEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label name, value;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void refreshPresets (const juce::String& select);
    void savePreset();
    void loadPreset();

    LoamProcessor& proc;
    std::array<Knob, 8> knobs;

    juce::ComboBox presetBox;
    juce::TextButton saveBtn { "Save" }, loadBtn { "Load" };
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoamEditor)
};
