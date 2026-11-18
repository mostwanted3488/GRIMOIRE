#include "PluginEditor.h"
#include "BinaryData.h"

#if JUCE_WINDOWS && ! JUCE_USE_WIN_WEBVIEW2
 #error "WebView2 doit etre active (NEEDS_WEBVIEW2) pour afficher l'interface"
#endif

namespace { constexpr int baseW = 800, baseH = 600; }

juce::File GrimoireEditor::sortsFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("GRIMOIRE").getChildFile ("sorts.json");
}

juce::WebBrowserComponent::Options GrimoireEditor::makeOptions()
{
    using Opt = juce::WebBrowserComponent::Options;
    return Opt{}
        .withBackend (Opt::Backend::webview2)
        .withWinWebView2Options (Opt::WinWebView2{}
                                     .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                                                              .getChildFile ("GRIMOIRE_WebView2"))
                                     .withBackgroundColour (juce::Colours::black))
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const juce::String& url) { return getResource (url); })

        // l'interface envoie toutes les valeurs (0..1) dans l'ordre de GrimoireIDs
        .withNativeFunction ("setParams", [this] (Args args, Completion done)
        {
            if (args.size() > 0)
                if (auto* arr = args[0].getArray())
                    for (int i = 0; i < juce::jmin (arr->size(), GrimoireIDs::count); ++i)
                    {
                        auto* p = proc.param (i);
                        const float v = juce::jlimit (0.0f, 1.0f, (float) (double) (*arr)[i]);
                        if (std::abs (p->getValue() - v) > 1.0e-5f)
                            p->setValueNotifyingHost (v);
                    }
            done ({});
        })
        .withNativeFunction ("gesture", [this] (Args args, Completion done)
        {
            if (args.size() >= 2)
            {
                const int i = (int) args[0];
                if (i >= 0 && i < GrimoireIDs::count)
                {
                    if ((bool) args[1]) proc.param (i)->beginChangeGesture();
                    else                proc.param (i)->endChangeGesture();
                }
            }
            done ({});
        })
        .withNativeFunction ("setPreset", [this] (Args args, Completion done)
        {
            if (args.size() >= 2) { proc.presetName = args[0].toString(); proc.presetDirty = (bool) args[1]; }
            done ({});
        })
        .withNativeFunction ("saveSorts", [] (Args args, Completion done)
        {
            if (args.size() >= 1)
            {
                auto f = sortsFile();
                f.getParentDirectory().createDirectory();
                f.replaceWithText (args[0].toString());
            }
            done ({});
        })
        .withNativeFunction ("setZoom", [this] (Args args, Completion done)
        {
            if (args.size() >= 1) applyZoom ((float) (double) args[0]);
            done ({});
        })
        .withNativeFunction ("getInit", [this] (Args, Completion done)
        {
            juce::Array<juce::var> v;
            for (int i = 0; i < GrimoireIDs::count; ++i) v.add (proc.param (i)->getValue());
            auto* o = new juce::DynamicObject();
            o->setProperty ("v", v);
            o->setProperty ("preset", proc.presetName);
            o->setProperty ("dirty", proc.presetDirty);
            o->setProperty ("zoom", proc.zoom);
            o->setProperty ("introSeen", proc.introSeen);
            o->setProperty ("sorts", sortsFile().existsAsFile() ? sortsFile().loadFileAsString() : juce::String ("[]"));
            proc.introSeen = true; // l'intro ne rejoue pas à chaque ouverture de la fenêtre
            done (juce::var (o));
        });
}

GrimoireEditor::GrimoireEditor (GrimoireProcessor& p)
    : AudioProcessorEditor (p), proc (p), web (makeOptions())
{
    addAndMakeVisible (web);
    setSize (juce::roundToInt (baseW * proc.zoom), juce::roundToInt (baseH * proc.zoom));
    web.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    startTimerHz (30);
}

GrimoireEditor::~GrimoireEditor() { stopTimer(); }

void GrimoireEditor::applyZoom (float z)
{
    proc.zoom = juce::jlimit (0.5f, 2.0f, z);
    setSize (juce::roundToInt (baseW * proc.zoom), juce::roundToInt (baseH * proc.zoom));
}

void GrimoireEditor::timerCallback()
{
    // état envoyé à l'interface : valeurs (automation de l'hôte), tempo, niveau de sortie
    juce::Array<juce::var> v;
    for (int i = 0; i < GrimoireIDs::count; ++i) v.add (proc.param (i)->getValue());
    auto* o = new juce::DynamicObject();
    o->setProperty ("v", v);
    o->setProperty ("bpm", proc.hostBpm.load());
    o->setProperty ("lvl", proc.outputLevel.exchange (0.0f));
    web.emitEventIfBrowserIsVisible ("state", juce::var (o));
}

std::optional<juce::WebBrowserComponent::Resource> GrimoireEditor::getResource (const juce::String& url)
{
    const auto path = url.fromFirstOccurrenceOf ("/", false, false).upToFirstOccurrenceOf ("?", false, false);
    if (path.isEmpty() || path == "index.html")
    {
        std::vector<std::byte> data ((size_t) BinaryData::index_htmlSize);
        std::memcpy (data.data(), BinaryData::index_html, data.size());
        return juce::WebBrowserComponent::Resource { std::move (data), "text/html" };
    }
    return std::nullopt;
}
