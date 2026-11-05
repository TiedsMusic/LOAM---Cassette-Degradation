#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float kTwoPi = 6.2831853f;

    constexpr float kBaseDelayMs    = 10.0f;    // retard moyen autour duquel le pitch oscille
    constexpr float kMaxDelayMs     = 30.0f;
    constexpr float kWowDepthMs     = 4.5f;     // amplitude du wow à 100 %
    constexpr float kFlutterDepthMs = 0.45f;    // amplitude du flutter à 100 %

    const char* const kIds[8] = { "wow", "flutter", "warmth", "hiss", "age", "lowCut", "highCut", "mix" };
}

//==============================================================================
LoamProcessor::LoamProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout()),
      presets (apvts, "LOAM", "loam", makeFactoryPresets())
{
    wowP     = apvts.getRawParameterValue ("wow");
    flutterP = apvts.getRawParameterValue ("flutter");
    warmthP  = apvts.getRawParameterValue ("warmth");
    hissP    = apvts.getRawParameterValue ("hiss");
    ageP     = apvts.getRawParameterValue ("age");
    lowCutP  = apvts.getRawParameterValue ("lowCut");
    highCutP = apvts.getRawParameterValue ("highCut");
    mixP     = apvts.getRawParameterValue ("mix");
}

juce::AudioProcessorValueTreeState::ParameterLayout LoamProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using Attr = juce::AudioParameterFloatAttributes;
    using Range = juce::NormalisableRange<float>;

    // Tous les potards vont de 0 à 1 en interne ; seul l'affichage change.
    auto pctText  = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };
    auto pctParse = [] (const juce::String& t) { return juce::jlimit (0.0f, 1.0f, t.getFloatValue() / 100.0f); };

    auto lowText  = [] (float v, int) { return juce::String (juce::roundToInt (20.0f * std::pow (25.0f, v))) + " Hz"; };
    auto lowParse = [] (const juce::String& t)
    {
        const float hz = juce::jmax (20.0f, t.getFloatValue());
        return juce::jlimit (0.0f, 1.0f, std::log (hz / 20.0f) / std::log (25.0f));
    };

    auto highText = [] (float v, int)
    {
        const float hz = 20000.0f * std::pow (0.1f, v);
        return hz >= 1000.0f ? juce::String (hz / 1000.0f, 1) + " kHz"
                             : juce::String (juce::roundToInt (hz)) + " Hz";
    };
    auto highParse = [] (const juce::String& t)
    {
        float hz = t.getFloatValue();
        if (t.containsIgnoreCase ("k")) hz *= 1000.0f;
        hz = juce::jlimit (2000.0f, 20000.0f, hz);
        return juce::jlimit (0.0f, 1.0f, std::log10 (20000.0f / hz));
    };

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto addPct = [&] (const char* id, const char* name, float def)
    {
        p.push_back (std::make_unique<F> (juce::ParameterID { id, 1 }, name, Range (0.0f, 1.0f, 0.001f), def,
                                          Attr().withStringFromValueFunction (pctText)
                                                .withValueFromStringFunction (pctParse)));
    };

    addPct ("wow",     "Wow",     0.25f);
    addPct ("flutter", "Flutter", 0.20f);
    addPct ("warmth",  "Warmth",  0.30f);
    addPct ("hiss",    "Hiss",    0.15f);
    addPct ("age",     "Age",     0.30f);

    p.push_back (std::make_unique<F> (juce::ParameterID { "lowCut", 1 }, "Low Cut", Range (0.0f, 1.0f, 0.001f), 0.05f,
                                      Attr().withStringFromValueFunction (lowText).withValueFromStringFunction (lowParse)));
    p.push_back (std::make_unique<F> (juce::ParameterID { "highCut", 1 }, "High Cut", Range (0.0f, 1.0f, 0.001f), 0.30f,
                                      Attr().withStringFromValueFunction (highText).withValueFromStringFunction (highParse)));

    addPct ("mix", "Mix", 1.00f);

    return { p.begin(), p.end() };
}

std::vector<FactoryPreset> LoamProcessor::makeFactoryPresets()
{
    // wow, flutter, warmth, hiss, age, lowCut, highCut, mix
    auto mk = [] (const char* name, float w, float f, float wm, float h, float a, float lc, float hc, float mx)
    {
        FactoryPreset fp;
        fp.name = name;
        const float v[8] = { w, f, wm, h, a, lc, hc, mx };
        for (int i = 0; i < 8; ++i)
            fp.values.emplace_back (kIds[i], v[i]);
        return fp;
    };

    return {
        mk ("Init",           0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 1.00f),
        mk ("Fresh Cassette", 0.12f, 0.10f, 0.25f, 0.12f, 0.10f, 0.05f, 0.20f, 1.00f),
        mk ("Warm Mixtape",   0.25f, 0.15f, 0.50f, 0.20f, 0.30f, 0.10f, 0.35f, 1.00f),
        mk ("Subtle Glue",    0.08f, 0.05f, 0.35f, 0.05f, 0.15f, 0.00f, 0.12f, 0.70f),
        mk ("Old Walkman",    0.45f, 0.35f, 0.45f, 0.35f, 0.55f, 0.25f, 0.55f, 1.00f),
        mk ("Worn Out",       0.70f, 0.55f, 0.60f, 0.55f, 0.80f, 0.35f, 0.70f, 1.00f)
    };
}

//==============================================================================
bool LoamProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void LoamProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const float fs = (float) sr;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    const int maxDelay = (int) std::ceil (kMaxDelayMs * 0.001 * sampleRate) + 8;

    wowDelay.setMaximumDelayInSamples (maxDelay);
    wowDelay.prepare (spec);
    wowDelay.reset();

    dryDelay.setMaximumDelayInSamples (maxDelay);
    dryDelay.prepare (spec);
    dryDelay.reset();

    for (auto* f : { &lowCutF, &hiCutF1, &hiCutF2, &warmLow })
    {
        f->prepare (spec);
        f->reset();
    }

    lowCutF.setType (juce::dsp::StateVariableTPTFilterType::highpass);   // 12 dB/oct
    lowCutF.setResonance (0.707f);
    hiCutF1.setType (juce::dsp::StateVariableTPTFilterType::lowpass);    // 24 dB/oct (Butterworth, 2 étages)
    hiCutF1.setResonance (0.541f);
    hiCutF2.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    hiCutF2.setResonance (1.307f);
    warmLow.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    warmLow.setCutoffFrequency (200.0f);

    juce::SmoothedValue<float>* smoothers[8] = { &wowS, &flutterS, &warmthS, &hissS, &ageS, &lowCutS, &highCutS, &mixS };
    std::atomic<float>* params[8] = { wowP, flutterP, warmthP, hissP, ageP, lowCutP, highCutP, mixP };
    for (int i = 0; i < 8; ++i)
    {
        smoothers[i]->reset (sampleRate, 0.03);
        smoothers[i]->setCurrentAndTargetValue (params[i]->load());
    }

    p1 = p2 = p3 = 0.0;
    n1 = n1t = n2 = n2t = n3 = n3t = 0.0f;
    c1 = c2 = c3 = 0;
    n1k = 1.0f - std::exp (-1.0f / (0.35f * fs));
    n2k = 1.0f - std::exp (-1.0f / (0.50f * fs));
    n3k = 1.0f - std::exp (-1.0f / (0.03f * fs));

    hissHp[0] = hissHp[1] = hissLp[0] = hissLp[1] = 0.0f;
    hissHpK = 1.0f - std::exp (-kTwoPi * 700.0f / fs);
    hissLpK = 1.0f - std::exp (-kTwoPi * juce::jmin (9000.0f, 0.45f * fs) / fs);

    // Le wow/flutter demande un retard moyen : on le déclare à l'hôte (compensation de latence)
    setLatencySamples ((int) std::round (kBaseDelayMs * 0.001 * sampleRate));
}

//==============================================================================
void LoamProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n     = buffer.getNumSamples();

    for (int c = numCh; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);

    if (numCh == 0 || n == 0)
        return;

    wowS.setTargetValue (wowP->load());
    flutterS.setTargetValue (flutterP->load());
    warmthS.setTargetValue (warmthP->load());
    hissS.setTargetValue (hissP->load());
    ageS.setTargetValue (ageP->load());
    lowCutS.setTargetValue (lowCutP->load());
    highCutS.setTargetValue (highCutP->load());
    mixS.setTargetValue (mixP->load());

    const float fs         = (float) sr;
    const float baseDelay  = kBaseDelayMs * 0.001f * fs;
    const float wowDepth   = kWowDepthMs * 0.001f * fs;
    const float flutDepth  = kFlutterDepthMs * 0.001f * fs;

    float* ch[2] { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    for (int i = 0; i < n; ++i)
    {
        const float wow = wowS.getNextValue(),   flutter = flutterS.getNextValue();
        const float warm = warmthS.getNextValue(), hiss = hissS.getNextValue();
        const float age = ageS.getNextValue();
        const float lowCut = lowCutS.getNextValue(), highCut = highCutS.getNextValue();
        const float mix = mixS.getNextValue();

        // --- AGE : modifie plusieurs réglages à la fois (jamais un simple "volume" des autres effets)
        const float wowE    = wow + 0.50f * age * (1.0f - wow);          // plus d'instabilité lente
        const float flutE   = flutter + 0.40f * age * (1.0f - flutter);  // plus de tremblement rapide
        const float hcE     = highCut + 0.70f * age * (1.0f - highCut);  // bande passante réduite (aigus perdus)
        const float hissE   = hiss * (1.0f + 0.60f * age);               // souffle un peu plus présent (0 reste 0)

        // --- Modulations : sinus + dérive aléatoire lissée (pas parfaitement périodique)
        p1 += (double) kTwoPi * 0.55 / sr; if (p1 > kTwoPi) p1 -= kTwoPi;
        p2 += (double) kTwoPi * 1.15 / sr; if (p2 > kTwoPi) p2 -= kTwoPi;

        if (--c1 <= 0) { n1t = rng.nextFloat() * 2.0f - 1.0f; c1 = (int) (0.5f * fs); }
        n1 += n1k * (n1t - n1);
        if (--c2 <= 0) { n2t = rng.nextFloat() * 2.0f - 1.0f; c2 = (int) (0.8f * fs); }
        n2 += n2k * (n2t - n2);
        if (--c3 <= 0) { n3t = rng.nextFloat() * 2.0f - 1.0f; c3 = (int) (0.04f * fs); }
        n3 += n3k * (n3t - n3);

        p3 += (double) kTwoPi * (7.0 * (1.0 + 0.25 * (double) n2)) / sr;    // flutter ~5 à 9 Hz
        if (p3 > kTwoPi) p3 -= kTwoPi;

        const float wowMod  = (0.55f * (float) std::sin (p1) + 0.30f * (float) std::sin (p2) + 0.40f * n1) / 1.25f;
        const float flutMod = 0.70f * (float) std::sin (p3) + 0.30f * n3;

        const float delay = baseDelay + wowE * wowDepth * wowMod + flutE * flutDepth * flutMod;

        // --- Filtres : coefficients mis à jour toutes les 16 échantillons
        if ((i & 15) == 0)
        {
            lowCutF.setCutoffFrequency (20.0f * std::pow (25.0f, lowCut));
            const float hcF = juce::jmin (20000.0f * std::pow (0.1f, hcE), 0.45f * fs);
            hiCutF1.setCutoffFrequency (hcF);
            hiCutF2.setCutoffFrequency (hcF);
        }

        // --- Warmth : saturation douce (gain unitaire en petit signal) + rondeur dans les graves
        const float drive = 1.0f + 7.0f * warm;
        const float bias  = 0.12f * warm;
        const float tb    = std::tanh (bias);
        const float norm  = 1.0f / (drive * (1.0f - tb * tb));
        const float satAmt = juce::jmin (1.0f, warm * 3.0f);              // 0 = aucun traitement
        const float hissAmp = std::pow (juce::jlimit (0.0f, 1.6f, hissE), 1.5f) * 0.05f;

        for (int c = 0; c < numCh; ++c)
        {
            const float in = ch[c][i];

            // signal sec retardé du retard moyen, pour rester aligné avec le signal traité
            dryDelay.pushSample (c, in);
            const float dry = dryDelay.popSample (c, baseDelay);

            wowDelay.pushSample (c, in);
            float y = wowDelay.popSample (c, delay);

            // warmth
            const float sat = (std::tanh (drive * y + bias) - tb) * norm;
            y = y * (1.0f - satAmt) + sat * satAmt;
            y += warm * 0.35f * warmLow.processSample (c, y);

            // hiss : bruit indépendant du signal, différent à gauche et à droite, riche en médiums/aigus
            const float nz = rng.nextFloat() * 2.0f - 1.0f;
            hissHp[c] += hissHpK * (nz - hissHp[c]);
            const float hp = nz - hissHp[c];
            hissLp[c] += hissLpK * (hp - hissLp[c]);
            y += hissLp[c] * hissAmp * 2.0f;

            // bande passante
            y = lowCutF.processSample (c, y);
            y = hiCutF2.processSample (c, hiCutF1.processSample (c, y));

            ch[c][i] = dry * (1.0f - mix) + y * mix;
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* LoamProcessor::createEditor()
{
    return new LoamEditor (*this);
}

void LoamProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    apvts.state.setProperty ("preset", presets.currentName, nullptr);

    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LoamProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            presets.currentName = apvts.state.getProperty ("preset").toString();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LoamProcessor();
}
