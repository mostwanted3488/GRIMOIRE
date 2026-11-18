# GRIMOIRE — Voice Flanger

Flanger pour voix dans un univers de grimoire de sorcier en low poly façon PS1.
Les runes qui lévitent au-dessus du livre sont les knobs.

**Underwater Palace** · VST3 (Windows x64) + version Standalone

## Installer (Windows)

1. Page **Releases** du repo → **GRIMOIRE - dernier build Windows** → télécharger `GRIMOIRE-Windows-x64.zip`
   (ou onglet **Actions** → dernier build vert → artefact **GRIMOIRE-Windows-x64**).
2. Dézipper, puis copier le dossier `GRIMOIRE.vst3` dans
   `C:\Program Files\Common Files\VST3\`
3. Dans FL Studio : *Options → Manage plugins → Find more plugins*, puis l'ajouter sur une piste de voix.

L'interface utilise **Microsoft Edge WebView2**, déjà présent sur Windows 10/11.
Sinon : <https://developer.microsoft.com/microsoft-edge/webview2/> (« Evergreen Runtime »).

## Contrôles

| Rune | Paramètre | Plage |
|---|---|---|
| ᚱ | RATE | 0,05 – 5 Hz, ou division du tempo en mode ◈ SYNC (4/1 → 1/16, triolets) |
| ᛟ | DEPTH | 0 – 100 % |
| ᚦ | DELAY | 0,5 – 10 ms |
| ᛞ | FEEDBACK | −95 à +95 % |
| ᛉ | SPREAD | décalage gauche/droite 0 – 180° |
| ᚠ | MIX | 0 – 100 % |
| ᛚ | TONE | sombre ↔ clair (signal traité) |
| ᛊ | GAIN | −24 à +12 dB (sortie) |

- Glisser ↕ sur une rune (Shift = fin), molette, double-clic = valeur par défaut.
- **BYPASS** referme le grimoire (bypass de l'hôte relié).
- 9 sorts d'usine (presets) en bas à gauche, dont **TECHWIZARD**, + **✎ graver un sort** pour sauvegarder les tiens
  (enregistrés dans `%APPDATA%\GRIMOIRE\sorts.json`).
- ⤢ : taille de la fenêtre (75 → 200 %).

Tous les paramètres sont automatisables dans le DAW.

## Construire soi-même

Le build Windows tourne automatiquement sur GitHub Actions à chaque push (`.github/workflows/build-windows.yml`).
En local (Visual Studio 2022 + CMake) :

```powershell
Register-PackageSource -ProviderName NuGet -Name nugetRepository -Location https://www.nuget.org/api/v2 -Force -Trusted
Install-Package Microsoft.Web.WebView2 -Scope CurrentUser -RequiredVersion 1.0.1901.177 -Source nugetRepository -Force
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target GRIMOIRE_VST3
```

## Structure

- `Source/PluginProcessor.*` — moteur audio : flanger stéréo (ligne à retard interpolée Hermite, LFO sinus ou synchro tempo, feedback ±, filtre TONE, gain, limiteur de sécurité, bypass en fondu).
- `Source/PluginEditor.*` — fenêtre WebView2 qui affiche l'interface et fait le lien paramètres ↔ interface.
- `ui/index.html` — l'interface 3D (rendu WebGL fait main façon PS1). Ouverte dans un navigateur, c'est aussi la preview avec banc d'essai audio.
