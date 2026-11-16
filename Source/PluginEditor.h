#pragma once

#include "PluginProcessor.h"
#include <array>
#include <memory>

// Apparence dessinée à la main : potards en images qui tournent, jauge colorée autour,
// boutons et listes dans le style du design. Toutes les tailles sont relatives à une
// fenêtre de référence de 450 x 635 px (la fenêtre peut être agrandie en gardant ses proportions).
class LoamLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LoamLookAndFeel();

    // police manuscrite ; la taille donnée est la taille de la lettre (em) en pixels
    juce::Font handFont (float emPixels) const;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;

    std::array<juce::Image, 8> knobImages;
    juce::Typeface::Ptr typeface;

    static constexpr float kHeightPerEm = 1.229f;   // hauteur JUCE = 1,229 x taille de lettre pour cette police
    static constexpr float kBaselineShift = 0.33f;  // centre des majuscules au-dessus de la ligne de base

    const juce::Colour ink   { 0xff261c16 };
    const juce::Colour paper { 0xfffcf6ec };
    const juce::Colour accent { 0xff2c7054 };       // jauge : vert pin (modifiable)
};

class LoamEditor : public juce::AudioProcessorEditor
{
public:
    explicit LoamEditor (LoamProcessor&);
    ~LoamEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void refreshPresets (const juce::String& select);
    void savePreset();
    void loadPreset();

    LoamProcessor& proc;
    LoamLookAndFeel laf;
    juce::Image background;
    std::array<Knob, 8> knobs;

    juce::ComboBox presetBox;
    juce::TextButton saveBtn { "SAVE" }, loadBtn { "LOAD" };
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoamEditor)
};
