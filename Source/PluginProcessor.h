#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PresetManager.h"
#include <atomic>
#include <cmath>

class LoamProcessor : public juce::AudioProcessor
{
public:
    LoamProcessor();
    ~LoamProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LOAM"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;   // doit être déclaré avant 'presets'
    PresetManager presets;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static std::vector<FactoryPreset> makeFactoryPresets();

    using Delay = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>;
    using SVF   = juce::dsp::StateVariableTPTFilter<float>;

    std::atomic<float>* wowP     = nullptr;
    std::atomic<float>* flutterP = nullptr;
    std::atomic<float>* warmthP  = nullptr;
    std::atomic<float>* hissP    = nullptr;
    std::atomic<float>* ageP     = nullptr;
    std::atomic<float>* lowCutP  = nullptr;
    std::atomic<float>* highCutP = nullptr;
    std::atomic<float>* mixP     = nullptr;

    juce::SmoothedValue<float> wowS, flutterS, warmthS, hissS, ageS, lowCutS, highCutS, mixS;

    double sr = 44100.0;

    Delay wowDelay { 4096 }, dryDelay { 4096 };
    SVF lowCutF, hiCutF1, hiCutF2, warmLow;

    // Modulation (wow / flutter) : sinus + dérive aléatoire lissée
    double p1 = 0.0, p2 = 0.0, p3 = 0.0;
    float n1 = 0.0f, n1t = 0.0f, n2 = 0.0f, n2t = 0.0f, n3 = 0.0f, n3t = 0.0f;
    int c1 = 0, c2 = 0, c3 = 0;
    float n1k = 0.0f, n2k = 0.0f, n3k = 0.0f;

    // Souffle (hiss)
    juce::Random rng;
    float hissHp[2] { 0.0f, 0.0f }, hissLp[2] { 0.0f, 0.0f };
    float hissHpK = 0.0f, hissLpK = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoamProcessor)
};
