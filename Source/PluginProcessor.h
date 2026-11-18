#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>

// Parameter order = order used by the UI (pluginVals() in JS)
namespace GrimoireIDs
{
    inline constexpr int count = 8;
    inline const char* const ids[count] = { "rate", "depth", "delay", "feedback", "mix", "gain", "sync", "bypass" };
    enum { rate, depth, delay, feedback, mix, gain, sync, bypass };
}

class GrimoireProcessor : public juce::AudioProcessor
{
public:
    GrimoireProcessor();
    ~GrimoireProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GRIMOIRE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;

    juce::RangedAudioParameter* param (int i) const { return params[(size_t) i]; }

    // values read by the UI
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<double> hostBpm { 120.0 };

    // non-audio state (message thread only)
    juce::String presetName { "GRIMOIRE" };
    bool presetDirty = false;
    float zoom = 1.0f;
    bool introSeen = false;

    static double rateToHz (float v, bool sync, double bpm);
    static juce::String rateText (float v, bool sync);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float readDelay (int ch, float delaySamples) const;

    juce::AudioProcessorValueTreeState apvts;
    std::array<juce::RangedAudioParameter*, GrimoireIDs::count> params {};

    double sr = 44100.0;
    std::array<std::vector<float>, 2> delayBuf;
    int bufSize = 1, writePos = 0;
    double lfoPhase = 0.0;

    std::array<juce::IIRFilter, 2> hp;

    juce::SmoothedValue<float> sDepth, sDelay, sFb, sMix, sGain, sBypass;
    float limEnv = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrimoireProcessor)
};
