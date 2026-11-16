#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    struct KnobLayout { float cx, cy, nx0, ny0, nx1, ny1, vx0, vy0, vx1, vy1; };
    struct Box { float x0, y0, x1, y1; };

    const KnobLayout kKnobs[8] = {
    { 131.24f, 182.77f,  61.93f, 219.00f, 200.56f, 234.76f,  90.28f, 236.33f, 172.20f, 248.94f },
    { 321.57f, 179.42f,  252.26f, 215.66f, 390.88f, 231.42f,  280.61f, 232.99f, 362.53f, 245.60f },
    { 132.36f, 295.55f,  63.04f, 331.79f, 201.67f, 347.55f,  91.40f, 349.12f, 173.31f, 361.73f },
    { 322.13f, 293.88f,  252.81f, 330.12f, 391.44f, 345.88f,  281.17f, 347.45f, 363.08f, 360.06f },
    { 129.01f, 405.00f,  59.70f, 441.24f, 198.33f, 456.99f,  88.06f, 458.57f, 169.97f, 471.17f },
    { 321.01f, 403.89f,  251.70f, 440.12f, 390.33f, 455.88f,  280.05f, 457.45f, 361.97f, 470.06f },
    { 126.23f, 517.23f,  56.92f, 553.47f, 195.54f, 569.22f,  85.27f, 570.80f, 167.19f, 583.40f },
    { 319.90f, 515.56f,  250.58f, 551.80f, 389.21f, 567.55f,  278.94f, 569.13f, 360.86f, 581.73f },
};
const Box kTitle   { 33.57f, 31.13f, 424.25f, 94.15f };
const Box kPresets { 247.82f, 39.01f, 417.95f, 89.43f };


    constexpr float kRefW = 450.0f, kRefH = 635.0f;      // fenêtre de référence
    constexpr float kMm   = 2.2277f;                     // pixels de référence par mm de la feuille A4 dessinée

    const char* const kIds[8]   = { "wow", "flutter", "warmth", "hiss", "age", "lowCut", "highCut", "mix" };
    const char* const kNames[8] = { "WOW", "FLUTTER", "WARMTH", "HISS", "AGE", "LOW CUT", "HIGH CUT", "MIX" };

    float textWidth (const juce::Font& f, const juce::String& t)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (f, t, 0.0f, 0.0f);
        return ga.getBoundingBox (0, -1, true).getWidth();
    }
}

//==============================================================================
// Apparence
//==============================================================================
LoamLookAndFeel::LoamLookAndFeel()
{
    typeface = juce::Typeface::createSystemTypefaceFor (BinaryData::MarkerHand_ttf,
                                                        (size_t) BinaryData::MarkerHand_ttfSize);

    const void* data[8] = { BinaryData::knob1_png, BinaryData::knob2_png, BinaryData::knob3_png, BinaryData::knob4_png,
                            BinaryData::knob5_png, BinaryData::knob6_png, BinaryData::knob7_png, BinaryData::knob8_png };
    const int sizes[8]  = { BinaryData::knob1_pngSize, BinaryData::knob2_pngSize, BinaryData::knob3_pngSize, BinaryData::knob4_pngSize,
                            BinaryData::knob5_pngSize, BinaryData::knob6_pngSize, BinaryData::knob7_pngSize, BinaryData::knob8_pngSize };

    for (size_t i = 0; i < 8; ++i)
        knobImages[i] = juce::ImageCache::getFromMemory (data[i], sizes[i]);

    setColour (juce::PopupMenu::backgroundColourId, paper);
    setColour (juce::PopupMenu::textColourId, ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffe6d3b0));
    setColour (juce::PopupMenu::highlightedTextColourId, ink);
    setColour (juce::ComboBox::textColourId, ink);
}

juce::Font LoamLookAndFeel::handFont (float emPixels) const
{
    return juce::Font (juce::FontOptions (typeface).withHeight (emPixels * kHeightPerEm));
}

void LoamLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float start, float end, juce::Slider& slider)
{
    const int idx = juce::jlimit (0, 7, (int) slider.getProperties()["knob"]);
    const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
    const float s = (float) juce::jmin (w, h) / 92.0f;
    const float angle = start + pos * (end - start);
    const float rMid = 18.0f * kMm * s, rail = 13.0f * s;

    // rail de la jauge, puis partie remplie jusqu'à la valeur
    juce::Path railPath;
    railPath.addCentredArc (cx, cy, rMid, rMid, 0.0f, start, end, true);
    g.setColour (juce::Colour::fromRGBA (60, 45, 30, 38));
    g.strokePath (railPath, juce::PathStrokeType (rail, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (pos > 0.002f)
    {
        juce::Path fill;
        fill.addCentredArc (cx, cy, rMid, rMid, 0.0f, start, angle, true);
        g.setColour (accent.withAlpha (0.92f));
        g.strokePath (fill, juce::PathStrokeType (rail * 0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // potard dessiné, qui tourne avec la valeur ; son repère suit l'extrémité de la jauge
    const auto& img = knobImages[(size_t) idx];
    const float d = 30.5f * kMm * s;

    juce::Path disc;
    disc.addEllipse (cx - d * 0.5f, cy - d * 0.5f, d, d);
    juce::DropShadow (juce::Colour::fromRGBA (40, 25, 10, 110), juce::roundToInt (7.0f * s),
                      { 0, juce::roundToInt (3.0f * s) }).drawForPath (g, disc);

    if (img.isValid())
    {
        const auto t = juce::AffineTransform::scale (d / (float) img.getWidth(), d / (float) img.getHeight())
                           .translated (-d * 0.5f, -d * 0.5f)
                           .rotated (angle)
                           .translated (cx, cy);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImageTransformed (img, t, false);
    }
}

void LoamLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (1.0f);
    g.setColour (paper.withAlpha (0.86f));
    g.fillRoundedRectangle (r, (float) h * 0.3f);
    g.setColour (ink);
    g.drawRoundedRectangle (r, (float) h * 0.3f, 1.4f);

    const float ax = (float) w - (float) h * 0.55f, ay = (float) h * 0.5f, as = (float) h * 0.16f;
    juce::Path arrow;
    arrow.addTriangle (ax - as, ay - as * 0.5f, ax + as, ay - as * 0.5f, ax, ay + as * 0.8f);
    g.fillPath (arrow);
}

juce::Font LoamLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return handFont ((float) box.getHeight() * 0.55f);
}

void LoamLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (juce::roundToInt ((float) box.getHeight() * 0.35f), 1,
                     box.getWidth() - juce::roundToInt ((float) box.getHeight() * 1.1f), box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font LoamLookAndFeel::getPopupMenuFont()
{
    return handFont (15.0f);
}

void LoamLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                            bool highlighted, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (paper.withAlpha (down ? 1.0f : highlighted ? 0.95f : 0.86f));
    g.fillRoundedRectangle (r, (float) b.getHeight() * 0.3f);
    g.setColour (ink);
    g.drawRoundedRectangle (r, (float) b.getHeight() * 0.3f, 1.4f);
}

void LoamLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const float em = (float) b.getHeight() * 0.5f;
    g.setFont (handFont (em));
    g.setColour (ink);
    g.drawSingleLineText (b.getButtonText(), b.getWidth() / 2,
                          juce::roundToInt ((float) b.getHeight() * 0.5f + kBaselineShift * em),
                          juce::Justification::horizontallyCentred);
}

//==============================================================================
// Éditeur
//==============================================================================
LoamEditor::LoamEditor (LoamProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    background = juce::ImageCache::getFromMemory (BinaryData::background_jpg, BinaryData::background_jpgSize);

    for (int i = 0; i < 8; ++i)
    {
        auto& k = knobs[(size_t) i];

        k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                      juce::MathConstants<float>::pi * 2.75f, true);
        k.slider.getProperties().set ("knob", i);
        k.slider.onValueChange = [this] { repaint(); };         // met à jour les valeurs écrites
        addAndMakeVisible (k.slider);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, kIds[i], k.slider);
    }

    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();

        if (id >= 2000)      proc.presets.loadUser (presetBox.getText());
        else if (id >= 1000) proc.presets.loadFactory (id - 1000);
    };
    addAndMakeVisible (presetBox);

    saveBtn.onClick = [this] { savePreset(); };
    loadBtn.onClick = [this] { loadPreset(); };
    addAndMakeVisible (saveBtn);
    addAndMakeVisible (loadBtn);

    refreshPresets (proc.presets.currentName);

    // fenêtre redimensionnable, toujours dans les proportions du design
    setResizeLimits (300, 423, 900, 1270);
    getConstrainer()->setFixedAspectRatio ((double) (kRefW / kRefH));
    setResizable (true, true);
    setSize ((int) kRefW, (int) kRefH);
}

LoamEditor::~LoamEditor()
{
    setLookAndFeel (nullptr);
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
    const float s = (float) getWidth() / kRefW;

    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (background, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);

    // noms sur les rubans, valeurs dans les petits cadres
    for (int i = 0; i < 8; ++i)
    {
        const auto& k = kKnobs[i];

        float em = (k.ny1 - k.ny0) * 1.15f * s;
        auto f = laf.handFont (em);
        const float maxName = (k.nx1 - k.nx0) * s * 0.9f;
        while (textWidth (f, kNames[i]) > maxName) { em *= 0.95f; f = laf.handFont (em); }

        g.setFont (f);
        g.setColour (laf.ink);
        g.drawSingleLineText (kNames[i], juce::roundToInt ((k.nx0 + k.nx1) * 0.5f * s),
                              juce::roundToInt ((k.ny0 + k.ny1) * 0.5f * s + LoamLookAndFeel::kBaselineShift * em),
                              juce::Justification::horizontallyCentred);

        const auto text = knobs[(size_t) i].slider.getTextFromValue (knobs[(size_t) i].slider.getValue());
        float vem = (k.vy1 - k.vy0) * 1.05f * s;
        auto vf = laf.handFont (vem);
        const float maxVal = (k.vx1 - k.vx0) * s * 0.9f;
        while (textWidth (vf, text) > maxVal) { vem *= 0.95f; vf = laf.handFont (vem); }

        g.setFont (vf);
        g.setColour (juce::Colour (0xff3c3430));
        g.drawSingleLineText (text, juce::roundToInt ((k.vx0 + k.vx1) * 0.5f * s),
                              juce::roundToInt ((k.vy0 + k.vy1) * 0.5f * s + LoamLookAndFeel::kBaselineShift * vem),
                              juce::Justification::horizontallyCentred);
    }

    // titre en grand, sous-titre en dessous plus petit et en gris foncé, alignés à gauche
    const float lx = (kTitle.x0 + 14.0f) * s;
    const float avail = (kPresets.x0 - kTitle.x0 - 30.0f) * s;
    float em = (kTitle.y1 - kTitle.y0) * 1.1f * s;
    auto tf = laf.handFont (em);
    while (textWidth (tf, "LOAM") > avail) { em *= 0.95f; tf = laf.handFont (em); }

    const float top = kTitle.y0 * s + 3.0f * s;
    g.setFont (tf);
    g.setColour (juce::Colour (0xff18130f));
    g.drawSingleLineText ("LOAM", juce::roundToInt (lx), juce::roundToInt (top + 0.836f * em), juce::Justification::left);

    const float sub = em * 0.36f;
    g.setFont (laf.handFont (sub));
    g.setColour (juce::Colour (0xff4e4a48));
    g.drawSingleLineText ("Cassette degradation", juce::roundToInt (lx + 4.0f * s),
                          juce::roundToInt (top + 0.98f * em + 0.836f * sub), juce::Justification::left);
}

void LoamEditor::resized()
{
    const float s = (float) getWidth() / kRefW;

    for (int i = 0; i < 8; ++i)
    {
        const int sz = juce::roundToInt (92.0f * s);
        knobs[(size_t) i].slider.setBounds (juce::Rectangle<int> (sz, sz)
                                                .withCentre ({ juce::roundToInt (kKnobs[i].cx * s),
                                                               juce::roundToInt (kKnobs[i].cy * s) }));
    }

    // presets dans l'encart du haut : liste au-dessus, SAVE et LOAD dessous
    const float px = kPresets.x0 + 3.0f, pw = kPresets.x1 - kPresets.x0 - 6.0f, ph = kPresets.y1 - kPresets.y0;
    presetBox.setBounds (juce::roundToInt (px * s), juce::roundToInt ((kPresets.y0 + 4.0f) * s),
                         juce::roundToInt (pw * s), juce::roundToInt (ph * 0.46f * s));

    const float bw = (pw - 6.0f) * 0.5f, by = kPresets.y0 + ph * 0.56f;
    saveBtn.setBounds (juce::roundToInt (px * s), juce::roundToInt (by * s),
                       juce::roundToInt (bw * s), juce::roundToInt (ph * 0.40f * s));
    loadBtn.setBounds (juce::roundToInt ((px + bw + 6.0f) * s), juce::roundToInt (by * s),
                       juce::roundToInt (bw * s), juce::roundToInt (ph * 0.40f * s));
}
