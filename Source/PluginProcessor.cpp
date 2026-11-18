#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    // tempo-sync divisions: { name, beats per cycle }
    struct Div { const char* name; double beats; };
    const Div divs[] = { { "4/1", 16.0 }, { "2/1", 8.0 }, { "1/1", 4.0 }, { "1/2", 2.0 }, { "1/2T", 4.0 / 3.0 },
                         { "1/4", 1.0 }, { "1/4T", 2.0 / 3.0 }, { "1/8", 0.5 }, { "1/8T", 1.0 / 3.0 }, { "1/16", 0.25 } };
    constexpr int numDivs = (int) (sizeof (divs) / sizeof (divs[0]));
    int divIndex (float v) { return juce::jlimit (0, numDivs - 1, (int) std::lround (v * (numDivs - 1))); }

    float gainDb (float v) { return -24.0f + v * 36.0f; }
    float delayMs (float v) { return 0.5f + v * 9.5f; }

    using Attr = juce::AudioParameterFloatAttributes;
    std::unique_ptr<juce::AudioParameterFloat> knob (const char* id, const char* name, float def,
                                                     std::function<juce::String (float, int)> text)
    {
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name,
                                                            juce::NormalisableRange<float> (0.0f, 1.0f), def,
                                                            Attr().withStringFromValueFunction (std::move (text)));
    }
}

double GrimoireProcessor::rateToHz (float v, bool sync, double bpm)
{
    if (sync)
        return juce::jmax (1.0, bpm) / 60.0 / divs[divIndex (v)].beats;
    return 0.05 * std::pow (100.0, (double) v);
}

juce::String GrimoireProcessor::rateText (float v, bool sync)
{
    if (sync) return juce::String (divs[divIndex (v)].name);
    return juce::String (rateToHz (v, false, 120.0), 2) + " Hz";
}

juce::AudioProcessorValueTreeState::ParameterLayout GrimoireProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (knob ("rate", "Rate", 0.35f, [] (float v, int) { return rateText (v, false); }));
    l.add (knob ("depth", "Depth", 0.6f, [] (float v, int) { return juce::String (juce::roundToInt (v * 100)) + " %"; }));
    l.add (knob ("delay", "Delay", 0.3f, [] (float v, int) { return juce::String (delayMs (v), 1) + " ms"; }));
    l.add (knob ("feedback", "Feedback", 0.62f, [] (float v, int)
                 { const int x = juce::roundToInt ((v * 2 - 1) * 95); return (x > 0 ? "+" : "") + juce::String (x) + " %"; }));
    l.add (knob ("mix", "Mix", 1.0f, [] (float v, int) { return juce::String (juce::roundToInt (v * 100)) + " %"; }));
    l.add (knob ("gain", "Gain", 24.0f / 36.0f, [] (float v, int)
                 { const float d = gainDb (v); return (d > 0 ? "+" : "") + juce::String (d, 1) + " dB"; }));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "sync", 1 }, "Tempo Sync", false));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return l;
}

GrimoireProcessor::GrimoireProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GRIMOIRE", createLayout())
{
    for (int i = 0; i < GrimoireIDs::count; ++i)
        params[(size_t) i] = apvts.getParameter (GrimoireIDs::ids[i]);
}

bool GrimoireProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

juce::AudioProcessorParameter* GrimoireProcessor::getBypassParameter() const { return params[GrimoireIDs::bypass]; }

void GrimoireProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    bufSize = juce::nextPowerOfTwo ((int) std::ceil (sr * 0.05) + 8);
    for (auto& b : delayBuf) b.assign ((size_t) bufSize, 0.0f);
    writePos = 0;
    lfoPhase = 0.0;
    limEnv = 1.0f;

    for (int c = 0; c < 2; ++c)
    {
        hp[(size_t) c].setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 90.0, 0.707));
        hp[(size_t) c].reset();
    }

    auto init = [this] (juce::SmoothedValue<float>& s, float v, double secs) { s.reset (sr, secs); s.setCurrentAndTargetValue (v); };
    using namespace GrimoireIDs;
    init (sDepth,  params[depth]->getValue(), 0.03);
    init (sDelay,  delayMs (params[delay]->getValue()), 0.05);
    init (sFb,     (params[feedback]->getValue() * 2.0f - 1.0f) * 0.95f, 0.03);
    init (sMix,    params[mix]->getValue(), 0.03);
    init (sGain,   juce::Decibels::decibelsToGain (gainDb (params[gain]->getValue())), 0.03);
    init (sBypass, params[bypass]->getValue() > 0.5f ? 1.0f : 0.0f, 0.03);
}

float GrimoireProcessor::readDelay (int ch, float d) const
{
    // 4-point Hermite interpolated read from the circular delay line
    const auto& b = delayBuf[(size_t) ch];
    const int mask = bufSize - 1;
    float rp = (float) writePos - d;
    while (rp < 0.0f) rp += (float) bufSize;
    const int i1 = (int) rp;
    const float f = rp - (float) i1;
    const float x0 = b[(size_t) ((i1 - 1) & mask)], x1 = b[(size_t) (i1 & mask)];
    const float x2 = b[(size_t) ((i1 + 1) & mask)], x3 = b[(size_t) ((i1 + 2) & mask)];
    const float c1 = 0.5f * (x2 - x0);
    const float c2 = x0 - 2.5f * x1 + 2.0f * x2 - 0.5f * x3;
    const float c3 = 0.5f * (x3 - x0) + 1.5f * (x1 - x2);
    return ((c3 * f + c2) * f + c1) * f + x1;
}

void GrimoireProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    for (int c = numCh; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    if (numCh == 0 || n == 0) return;

    // host tempo (sync)
    using namespace GrimoireIDs;
    const bool syncOn = params[sync]->getValue() > 0.5f;
    double bpm = hostBpm.load();
    bool playing = false;
    double ppq = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) ppq = *p;
        }
    hostBpm.store (bpm);

    const float rateV = params[rate]->getValue();
    const double hz = rateToHz (rateV, syncOn, bpm);
    if (syncOn && playing)
    {
        const double beats = divs[divIndex (rateV)].beats;
        lfoPhase = std::fmod (ppq / beats, 1.0);
        if (lfoPhase < 0.0) lfoPhase += 1.0;
    }
    const double inc = hz / sr;

    sDepth.setTargetValue (params[depth]->getValue());
    sDelay.setTargetValue (delayMs (params[delay]->getValue()));
    sFb.setTargetValue ((params[feedback]->getValue() * 2.0f - 1.0f) * 0.95f);
    sMix.setTargetValue (params[mix]->getValue());
    sGain.setTargetValue (juce::Decibels::decibelsToGain (gainDb (params[gain]->getValue())));
    sBypass.setTargetValue (params[bypass]->getValue() > 0.5f ? 1.0f : 0.0f);
    const float spread = juce::MathConstants<float>::halfPi; // stereo offset fixed at 90°

    const float twoPi = juce::MathConstants<float>::twoPi;
    const float minDelay = (float) (0.00008 * sr);
    const float maxDelay = (float) (bufSize - 4);
    double sumSq = 0.0;

    for (int i = 0; i < n; ++i)
    {
        const float dep = sDepth.getNextValue();
        const float baseMs = sDelay.getNextValue();
        const float fb = sFb.getNextValue();
        const float mx = sMix.getNextValue();
        const float g = sGain.getNextValue();
        const float byp = sBypass.getNextValue();

        const float base = baseMs * 0.001f * (float) sr;
        const float amp = dep * juce::jmax (0.0f, base - minDelay);
        const float ph = (float) lfoPhase * twoPi;

        float out[2] {};
        for (int c = 0; c < numCh; ++c)
        {
            const float x = buffer.getSample (c, i);
            const float lfo = std::sin (ph + (c == 1 ? spread : 0.0f));
            const float d = juce::jlimit (1.5f, maxDelay, base + amp * lfo);

            float y = readDelay (c, d);
            float w = hp[(size_t) c].processSingleSampleRaw (x) + fb * y;
            if (! std::isfinite (w)) w = 0.0f;
            delayBuf[(size_t) c][(size_t) writePos] = w;

            const float fx = x * (1.0f - mx * 0.85f) + y * mx;
            out[c] = (fx * (1.0f - byp) + x * byp) * g;
        }

        // safety limiter (ceiling ~ -0.4 dBFS)
        float peak = 0.0f;
        for (int c = 0; c < numCh; ++c) peak = juce::jmax (peak, std::abs (out[c]));
        const float target = peak * limEnv > 0.95f ? 0.95f / peak : 1.0f;
        limEnv = target < limEnv ? target : limEnv + (1.0f - limEnv) * 0.0002f;
        for (int c = 0; c < numCh; ++c)
        {
            const float o = out[c] * limEnv;
            buffer.setSample (c, i, o);
            sumSq += (double) o * o;
        }

        writePos = (writePos + 1) & (bufSize - 1);
        lfoPhase += inc;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;
    }

    const float rms = (float) std::sqrt (sumSq / (double) (n * numCh));
    outputLevel.store (juce::jmax (rms, outputLevel.load() * 0.85f));
}

void GrimoireProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("presetDirty", presetDirty, nullptr);
    state.setProperty ("zoom", zoom, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void GrimoireProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            presetName = state.getProperty ("presetName", "GRIMOIRE").toString();
            presetDirty = (bool) state.getProperty ("presetDirty", false);
            zoom = (float) state.getProperty ("zoom", 1.0f);
            apvts.replaceState (state);
        }
}

juce::AudioProcessorEditor* GrimoireProcessor::createEditor() { return new GrimoireEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GrimoireProcessor(); }
