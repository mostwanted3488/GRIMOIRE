#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

// The editor shows the grimoire 3D UI (embedded HTML page) inside a WebView2.
class GrimoireEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit GrimoireEditor (GrimoireProcessor&);
    ~GrimoireEditor() override;

    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    void resized() override { web.setBounds (getLocalBounds()); }

private:
    using Completion = juce::WebBrowserComponent::NativeFunctionCompletion;
    using Args = const juce::Array<juce::var>&;

    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
    juce::WebBrowserComponent::Options makeOptions();
    static juce::File sortsFile();
    void applyZoom (float z);

    GrimoireProcessor& proc;
    juce::WebBrowserComponent web;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrimoireEditor)
};
