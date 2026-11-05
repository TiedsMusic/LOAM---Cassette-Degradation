#include "PluginEditor.h"

namespace
{
    const juce::Colour kPaper   (0xfff6eee5);
    const juce::Colour kInk     (0xff3c2531);
    const juce::Colour kCrimson (0xffb31e07);

    constexpr float kPx = 450.0f / 148.0f;      // pixels par mm du gabarit A5

    const char* const kIds[8]   = { "wow", "flutter", "warmth", "hiss", "age", "lowCut", "highCut", "mix" };
    const char* const kNames[8] = { "WOW", "FLUTTER", "WARMTH", "HISS", "AGE", "LOW CUT", "HIGH CUT", "MIX" };
}

//==============================================================================
LoamEditor::LoamEditor (LoamProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (450, 639);

    for (int i = 0; i < 8; ++i)
    {
        auto* k = &knobs[(size_t) i];

        k->slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                       juce::MathConstants<float>::pi * 2.75f, true);
        k->slider.setColour (juce::Slider::rotarySliderFillColourId, kCrimson);
        k->slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xffd9c7b0));
        k->slider.setColour (juce::Slider::thumbColourId, kInk);
        addAndMakeVisible (k->slider);

        k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, kIds[i], k->slider);

        k->name.setText (kNames[i], juce::dontSendNotification);
        k->name.setJustificationType (juce::Justification::centred);
        k->name.setColour (juce::Label::textColourId, kInk);
        k->name.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        addAndMakeVisible (k->name);

        k->value.setJustificationType (juce::Justification::centred);
        k->value.setColour (juce::Label::textColourId, kInk);
        k->value.setFont (juce::FontOptions (12.0f));
        addAndMakeVisible (k->value);

        k->slider.onValueChange = [k]
        {
            k->value.setText (k->slider.getTextFromValue (k->slider.getValue()), juce::dontSendNotification);
        };
        k->slider.onValueChange();
    }

    // --- Presets
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xfffbf5ea));
    presetBox.setColour (juce::ComboBox::textColourId, kInk);
    presetBox.setColour (juce::ComboBox::outlineColourId, kInk);
    presetBox.setColour (juce::ComboBox::arrowColourId, kInk);
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();

        if (id >= 2000)      proc.presets.loadUser (presetBox.getText());
        else if (id >= 1000) proc.presets.loadFactory (id - 1000);
    };
    addAndMakeVisible (presetBox);

    for (auto* b : { &saveBtn, &loadBtn })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xfffbf5ea));
        b->setColour (juce::TextButton::textColourOffId, kInk);
        addAndMakeVisible (*b);
    }
    saveBtn.onClick = [this] { savePreset(); };
    loadBtn.onClick = [this] { loadPreset(); };

    refreshPresets (proc.presets.currentName);
}

//==============================================================================
void LoamEditor::refreshPresets (const juce::String& select)
{
    presetBox.clear (juce::dontSendNotification);

    int selectId = 0;

    presetBox.addSectionHeading ("Factory");
    for (int i = 0; i < proc.presets.getNumFactory(); ++i)
    {
        const auto name = proc.presets.getFactoryName (i);
        presetBox.addItem (name, 1000 + i);
        if (name == select) selectId = 1000 + i;
    }

    const auto userNames = proc.presets.getUserPresetNames();
    if (userNames.size() > 0)
    {
        presetBox.addSectionHeading ("User");
        for (int i = 0; i < userNames.size(); ++i)
        {
            presetBox.addItem (userNames[i], 2000 + i);
            if (userNames[i] == select && selectId == 0) selectId = 2000 + i;
        }
    }

    if (selectId != 0)
        presetBox.setSelectedId (selectId, juce::dontSendNotification);
}

void LoamEditor::savePreset()
{
    const auto ext = proc.presets.getExtension();
    chooser = std::make_unique<juce::FileChooser> ("Save preset",
                                                   proc.presets.getPresetsFolder().getChildFile ("My preset." + ext),
                                                   "*." + ext);

    juce::Component::SafePointer<LoamEditor> safe (this);

    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe, ext] (const juce::FileChooser& fc)
    {
        if (safe == nullptr)
            return;

        auto file = fc.getResult();
        if (file == juce::File())
            return;

        file = file.withFileExtension (ext);

        if (safe->proc.presets.saveToFile (file))
        {
            // un export hors du dossier des presets est aussi gardé dans la liste
            const auto library = safe->proc.presets.getPresetsFolder();
            if (file.getParentDirectory() != library)
                file.copyFileTo (library.getChildFile (file.getFileName()));

            safe->refreshPresets (safe->proc.presets.currentName);
        }
    });
}

void LoamEditor::loadPreset()
{
    const auto ext = proc.presets.getExtension();
    chooser = std::make_unique<juce::FileChooser> ("Load preset", proc.presets.getPresetsFolder(), "*." + ext);

    juce::Component::SafePointer<LoamEditor> safe (this);

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
    {
        if (safe == nullptr)
            return;

        const auto file = fc.getResult();
        if (file == juce::File())
            return;

        if (safe->proc.presets.loadFromFile (file, true))
            safe->refreshPresets (safe->proc.presets.currentName);
        else
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    "LOAM", "This file is not a valid LOAM preset.");
    });
}

//==============================================================================
void LoamEditor::paint (juce::Graphics& g)
{
    g.fillAll (kPaper);

    // repère de l'encart titre du gabarit (à supprimer quand le fond dessiné sera en place)
    g.setColour (kInk.withAlpha (0.15f));
    g.drawRect ((int) (12 * kPx), (int) (12 * kPx), (int) (124 * kPx), (int) (20 * kPx));

    g.setColour (kInk);
    g.setFont (34.0f);
    g.drawText ("LOAM", (int) (14 * kPx), (int) (12 * kPx), (int) (70 * kPx), (int) (20 * kPx),
                juce::Justification::centredLeft);

    g.setFont (13.0f);
    g.drawText ("Cassette degradation", (int) (12 * kPx), (int) (34 * kPx), (int) (124 * kPx), (int) (6 * kPx),
                juce::Justification::centred);
}

void LoamEditor::resized()
{
    // encart du titre : titre à gauche, presets à droite
    presetBox.setBounds ((int) (80 * kPx), (int) (14.5f * kPx), (int) (54 * kPx), 24);
    saveBtn.setBounds   ((int) (80 * kPx), (int) (14.5f * kPx) + 28, (int) (26 * kPx), 22);
    loadBtn.setBounds   ((int) (108 * kPx), (int) (14.5f * kPx) + 28, (int) (26 * kPx), 22);

    for (int i = 0; i < 8; ++i)
    {
        auto& k = knobs[(size_t) i];
        const float cx = (i % 2 == 0 ? 43.0f : 105.0f) * kPx;
        const float cy = (58.0f + (float) (i / 2) * 36.5f) * kPx;

        k.slider.setBounds (juce::Rectangle<int> (64, 64).withCentre ({ (int) cx, (int) cy }));
        k.name.setBounds ((int) (cx - 22.0f * kPx), (int) (cy + 12.0f * kPx), (int) (44.0f * kPx), 18);
        k.value.setBounds ((int) (cx - 13.0f * kPx), (int) (cy + 18.5f * kPx), (int) (26.0f * kPx), 14);
    }
}
