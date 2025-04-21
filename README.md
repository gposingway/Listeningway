![Listeningway Showcase](https://github.com/user-attachments/assets/0a770e71-7f4b-4637-9a5e-0c1b26a47e41)

**`Listeningway`** is a ReShade addon that provides  **real-time audio visualization** for shaders. It listens to your system's audio, analyzes it live, and exposes data like volume, frequency bands, and beat detection directly to your `.fx` files.

---

## For End Users: Get Started!

Let's add audio reactivity to your existing ReShade presets or try out effects designed for Listeningway! Getting started is super easy:

**What You'll Need (The Recipe):**

* **ReShade:** Version 5.2 or newer (or a recent development build from [GitHub](https://github.com/crosire/reshade/)). Make sure you're using a version with Addon support enabled if necessary (often the default download).
* **Windows:** Version 10 or 11 (required for the WASAPI audio capture magic).

**Installation:**

1.  **Download the Release ZIP:** Head over to the **Latest Release page** on GitHub. Find the main release archive file (usually named something like `Listeningway-vX.Y.Z.zip`) in the 'Assets' section and download it.
    * [**Go to Latest Listeningway Release**](https://github.com/gposingway/Listeningway/releases/latest)
2.  **Extract the ZIP:** Unzip the downloaded archive file to a temporary location on your computer using a tool like 7-Zip or Windows Explorer. Inside the extracted folder, you should find files like `Listeningway.addon`, `Listeningway.fx`, and `ListeningwayUniforms.fxh`.
3.  **Place the Extracted Files:** Now, move these extracted files to their correct final destinations. Use this table as your guide:

    | File Name                  | Where to Place It                                                                | Notes / Purpose                                  |
    | :------------------------- | :------------------------------------------------------------------------------- | :----------------------------------------------- |
    | `Listeningway.addon`       | FFXIV Game Directory (Same folder as `ffxiv_dx11.exe` & ReShade DLL `dxgi.dll`) | ReShade loads `.addon` files from this directory. |
    | `Listeningway.fx`          | Main ReShade Shaders Folder (e.g., `...\reshade-shaders\Shaders\`)               | The example shader effect file.                  |
    | `ListeningwayUniforms.fxh` | Main ReShade Shaders Folder (e.g., `...\reshade-shaders\Shaders\`)               | Include file needed by shaders using `#include`.   |

    *(**Important Reminder:** The `.addon` file goes directly into your main game folder with the ReShade DLL, **not** inside `reshade-shaders`!)*

4.  **Test Drive**
    * Launch your game! The addon should load automatically if placed correctly (you might see a message about it in the ReShade log/startup banner).
    * Open the ReShade menu, find and enable the `Listeningway.fx` effect in your shader list to see it react!

**Tuning (Optional):**

* For most users, the default settings work great! If you want to fine-tune the audio analysis (like how sensitive beat detection is), you can edit the `Listeningway.ini` file located in the same directory as the .addon file. More details on this in the section for developers below or on the [GitHub Wiki](link-to-wiki-placeholder).

---

## For Shader Creators

Want to make your *own* effects react to audio? Listeningway makes it quite simple by exposing audio data as `uniform` variables.

**How to Use the Uniforms:**

You *must* use annotation-based uniforms to access the data. This is robust and avoids potential conflicts. Simply declare a uniform in your shader with the `source` annotation pointing to the Listeningway data you want:

```hlsl
// In your .fx file: Define uniforms using the 'source' annotation

// Example: Get overall volume
uniform float MyAwesomeVolume < source = "listeningway_volume"; >;

// Example: Get the 8 frequency bands (0=bass -> 7=treble)
uniform float MyCoolFreqBands[8] < source = "listeningway_freqbands"; >;

// Example: Get the beat trigger (1.0 on beat, fades down)
uniform float MyFunkyBeat < source = "listeningway_beat"; >;

// Example: Get elapsed time
uniform float MyTime < source = "listeningway_timeseconds"; >;
````

**Tip:** For convenience, you can `#include "ListeningwayUniforms.fxh"` (make sure you placed it in `reshade-shaders\Shaders\` as per installation steps) which contains pre-defined declarations for all available Listeningway uniforms.

**Available Uniforms:**

Here's the data Listeningway provides (check `ListeningwayUniforms.fxh` for ready-to-use declarations):

  * `uniform float Listeningway_Volume < source="listeningway_volume"; >`
      * Current overall audio volume (normalized, good for intensity/brightness).
  * `uniform float Listeningway_FreqBands[8] < source="listeningway_freqbands"; >`
      * Amplitude of 8 frequency bands (Index 0 = Low Bass ... Index 7 = High Treble). Great for spectrum visualizations or driving different effects based on frequency\!
  * `uniform float Listeningway_Beat < source="listeningway_beat"; >`
      * Beat detection value. Typically pulses to 1.0 on a detected beat and then quickly falls off. Perfect for triggering flashes or movements.
  * `uniform float Listeningway_TimeSeconds < source="listeningway_timeseconds"; >`
      * Time elapsed (in seconds) since the addon started. Useful for continuous animations.
  * `uniform float Listeningway_TimePhase60Hz < source="listeningway_timephase60hz"; >`
      * Phase (0.0 to 1.0) cycling at 60Hz. Good for smooth, fast oscillations.
  * `uniform float Listeningway_TimePhase120Hz < source="listeningway_timephase120hz"; >`
      * Phase (0.0 to 1.0) cycling at 120Hz. Even faster oscillations\!
  * `uniform float Listeningway_TotalPhases60Hz < source="listeningway_totalphases60hz"; >`
      * Total number of 60Hz cycles elapsed (float).
  * `uniform float Listeningway_TotalPhases120Hz < source="listeningway_totalphases120hz"; >`
      * Total number of 120Hz cycles elapsed (float).

**Example Shader Snippet:**

Here’s a super basic example of using some uniforms in your shader's main pixel shader function:

```hlsl
// Make sure uniforms are declared above (or use the include)

float4 PS_MyAudioReactiveEffect(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    // Simple example: Use Bass, Volume, and Beat to drive Red color intensity
    float intensity = Listeningway_FreqBands[0] * 0.5 // Bass contribution
                    + Listeningway_Volume * 0.5      // Overall volume contribution
                    + Listeningway_Beat * 1.0;       // Flash red on beat!

    // Make sure intensity stays in a visible range
    intensity = saturate(intensity);

    // Get original pixel color
    float3 color = tex2D(ReShade::BackBuffer, uv).rgb;

    // Mix original color with red based on intensity
    color = lerp(color, float3(1.0, 0.0, 0.0), intensity * 0.5); // Mix 50% red at max intensity

    return float4(color, 1.0);
}

technique MyAudioReactiveEffect {
    pass {
        VertexShader = PostProcessVS;
        PixelShader = PS_MyAudioReactiveEffect;
    }
}
```

Now go make something awesome\! ✨

-----

## For Addon Developers

Interested in the nitty-gritty, building from source, or understanding the internals?

**Building the Addon:**

It's streamlined with batch scripts\!

1.  **Prepare Dependencies:**
      * Open a command prompt in the project root.
      * Run: `.\prepare.bat`
      * *(This clones the ReShade SDK & vcpkg, installs dependencies via vcpkg - grab a coffee\!)*
2.  **Build:**
      * Run: `.\build.bat`
      * *(This builds in Release mode, renames the DLL to `.addon`, and copies it to `dist\`)*

**Configuration Deep Dive (`Listeningway.ini`):**

  * All tunable parameters (FFT size, number of bands, smoothing factors, beat detection thresholds, UI layout values, etc.) live in `Listeningway.ini`.
  * This file is created/read from the same directory as the ReShade DLL / addon file.
  * Settings are loaded into the `ListeningwaySettings` struct on startup. Missing keys use defaults from `constants.h`.
  * Changes made in the overlay UI are saved back to the `.ini` atomically. Manual edits require a game/addon restart.
  * The `AudioAnalysisConfig` struct bundles settings for the analysis modules.

**Example `.ini` entries:**

```ini
[Audio]
NumBands=8
FFTSize=512
FluxAlpha=0.1
FluxThresholdMultiplier=1.5
BeatFluxMin=0.01
BeatFalloffDefault=2.0
BeatTimeScale=0.000000001
BeatTimeInitial=0.5
BeatTimeMin=0.05
BeatTimeDivisor=0.1
VolumeNorm=2.0
BandNorm=0.1

[UI]
FreqBandRowHeight=24.0
ProgressWidth=0.9
CaptureStaleTimeout=3.0

[General]
AudioAnalysisEnabled=1
DebugEnabled=0
```

**Architecture Overview:**

  * `audio_capture.*`: Handles WASAPI audio capture thread.
  * `audio_analysis.*`: Performs FFT, calculates volume, bands, beat detection.
  * `uniform_manager.*`: Manages updating shader uniforms via the ReShade API.
  * `overlay.*`: Renders the ImGui debug overlay.
  * `logging.*`: Simple thread-safe logging.
  * `listeningway_addon.cpp`: Main addon entry point, event handling, initialization.
  * `settings.*`: Loads/saves settings from `.ini`, holds the `ListeningwaySettings` struct.

**Dependencies & Credits:**

| Library / API                                                                                  | Author / Project   | Purpose                              |
| :--------------------------------------------------------------------------------------------- | :----------------- | :----------------------------------- |
| [ReShade](https://github.com/crosire/reshade)                                                  | crosire            | Core framework & SDK                 |
| [ImGui](https://github.com/ocornut/imgui)                                                      | Omar Cornut        | Debug Overlay GUI                    |
| [KissFFT](https://github.com/mborgerding/kissfft)                                              | Mark Borgerding    | Fast Fourier Transform Calculation |
| [Microsoft WASAPI](https://www.google.com/search?q=https://docs.microsoft.com/en-us/windows/win32/coreaudio/wasapi/wasapi-api) | Microsoft          | Windows Audio Capture                |

**Developer Notes:**

  * All dependencies (like KissFFT) are linked statically; no extra DLLs needed beside the `.addon` file.
  * Check the ReShade log file (`ReShade.log` or `d3d11.log` etc. in game dir) for any addon errors.
  * If you change the number of frequency bands (`NumBands` in `.ini`), you MUST update it in `settings.h` (`DEFAULT_NUM_BANDS`) AND adjust your shader code (array sizes, uniform source annotations if needed) accordingly\! Same applies if adding new uniforms.
  * Doxygen documentation can be generated using the `Doxyfile` in `tools/reshade`.

  HUGE thanks to the ReShade community and the creators of these libraries\!

-----

## Feedback & Show Us What You Make\!

Feedback, ideas, bug reports, and pull requests are very welcome over on the [GitHub Repository](https://www.google.com/search?q=https://github.com/gposingway/Listeningway)\!

And most importantly – if you use Listeningway to create some cool audio-reactive shaders, **please share them\!** Post screenshots or videos here, on GitHub Discussions, wherever you hang out\! We'd love to see your creativity in action\!

Hope you have a blast making your visuals groove\!

Happy visualizing\! =)
