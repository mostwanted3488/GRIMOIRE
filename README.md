# GRIMOIRE — Voice Flanger

A flanger for vocals, set inside a sorcerer's grimoire in low-poly PS1 style.
The runes floating above the book are the knobs.

**@buyorbealoser** · VST3 + AAX (Windows x64) + Standalone

## Install (Windows)

1. Open the repo's **Releases** page → **GRIMOIRE - latest Windows build** → download `GRIMOIRE-Windows-x64.zip`
   (or **Actions** tab → latest green build → artifact **GRIMOIRE-Windows-x64**).
2. Unzip, then copy the `GRIMOIRE.vst3` folder to
   `C:\Program Files\Common Files\VST3\`
3. In FL Studio: *Options → Manage plugins → Find more plugins*, then load it on a vocal track.

The UI runs on **Microsoft Edge WebView2**, already installed on Windows 10/11.
If the plugin window stays black: <https://developer.microsoft.com/microsoft-edge/webview2/> ("Evergreen Runtime").

### Pro Tools (AAX)

Copy `GRIMOIRE.aaxplugin` to `C:\Program Files\Common Files\Avid\Audio\Plug-Ins\`.

⚠️ Pro Tools only loads AAX plugins **signed by PACE**. The automatic build produces an **unsigned** AAX:
it runs in **Pro Tools Developer** (free for AAX developers), not yet in regular Pro Tools.
To sign it:
1. Create an AAX developer account with Avid and request access (Pro Tools Developer: devauth@avid.com).
2. Once approved, Avid puts you in touch with PACE, who provide the signing tools (wraptool) and the iLok certificate.
3. Sign: `wraptool sign --account <account> --wcguid <GUID> --in GRIMOIRE.aaxplugin --out GRIMOIRE.aaxplugin`
   (this step can then be added to the GitHub workflow with the credentials stored as secrets).

## Controls

| Rune | Parameter | Range |
|---|---|---|
| ᚱ | RATE | 0.05 – 5 Hz, or a tempo division in ◈ SYNC mode (4/1 → 1/16, triplets) |
| ᛟ | DEPTH | 0 – 100 % |
| ᚦ | DELAY | 0.5 – 10 ms |
| ᛞ | FEEDBACK | −95 to +95 % |
| ᚠ | MIX | 0 – 100 % |
| ᛊ | GAIN | −24 to +12 dB (output) |

- Drag ↕ on a rune (Shift = fine), scroll wheel, double-click = default value.
- **BYPASS** closes the grimoire (linked to the host's bypass).
- 9 factory spells (presets) at the bottom left, including **TECHWIZARD**, plus **✎ engrave a spell** to save your own
  (stored in `%APPDATA%\GRIMOIRE\sorts.json`).
- ⤢ : window size (75 → 200 %).

Every parameter can be automated in the DAW.

## Build it yourself

The Windows build runs automatically on GitHub Actions on every push (`.github/workflows/build-windows.yml`).
Locally (Visual Studio 2022 + CMake):

```powershell
Register-PackageSource -ProviderName NuGet -Name nugetRepository -Location https://www.nuget.org/api/v2 -Force -Trusted
Install-Package Microsoft.Web.WebView2 -Scope CurrentUser -RequiredVersion 1.0.1901.177 -Source nugetRepository -Force
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target GRIMOIRE_VST3 GRIMOIRE_AAX
```

## Layout

- `Source/PluginProcessor.*` — audio engine: stereo flanger (Hermite-interpolated delay line, sine LFO or tempo sync, ± feedback, fixed 90° stereo offset, gain, safety limiter, crossfaded bypass).
- `Source/PluginEditor.*` — WebView2 window showing the UI and bridging parameters ↔ UI.
- `ui/index.html` — the 3D UI (hand-written WebGL renderer, PS1 style). Opened in a browser, it is also the preview with an audio test bench.
