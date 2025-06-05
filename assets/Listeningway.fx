// Visually rich overlay for all Listeningway data
#include "ReShade.fxh"
#include "ListeningwayUniforms.fxh"

#define ORIENT_HORIZONTAL 0
#define ORIENT_VERTICAL 1
#define CORNER_TOP_LEFT 0
#define CORNER_TOP_RIGHT 1
#define CORNER_BOTTOM_LEFT 2
#define CORNER_BOTTOM_RIGHT 3

uniform int Listeningway_Orientation <
    ui_text = "Listeningway is a real-time audio analysis overlay for ReShade. It visualizes music and game audio by displaying frequency bands (spectrum), overall volume, beat detection, stereo balance, pan, time, phase, and audio format. The overlay helps you debug or create audio-reactive shaders by showing exactly what the Listeningway addon is sending to your shader in real time. Use the controls below to change the overlay's orientation and position.";
    ui_type = "combo";
    ui_items = "Horizontal\0Vertical\0";
    ui_label = "Orientation";
> = 0;
uniform int Listeningway_Corner < ui_type = "combo"; ui_items = "Top Left\0Top Right\0Bottom Left\0Bottom Right\0"; ui_label = "Corner"; > = 1;

static const float2 overlay_size = float2(0.44, 0.22); // larger for more info
static const float2 margin = float2(0.02, 0.02);

// Helper: fmod for ReShade (returns x - y * floor(x/y))
float fmod(float x, float y) {
    return x - y * floor(x / y);
}

// Vertex shader (unchanged)
void VS_Overlay(in uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD) {
    uv.x = (id == 2) ? 2.0 : 0.0;
    uv.y = (id == 1) ? 2.0 : 0.0;
    pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}

float3 band_color(float t) {
    float3 c1 = float3(0.2, 0.6, 1.0);
    float3 c2 = float3(0.0, 1.0, 1.0);
    float3 c3 = float3(0.0, 1.0, 0.2);
    float3 c4 = float3(1.0, 1.0, 0.0);
    float3 c5 = float3(1.0, 0.2, 0.2);
    if (t < 0.25) return lerp(c1, c2, t / 0.25);
    else if (t < 0.5) return lerp(c2, c3, (t - 0.25) / 0.25);
    else if (t < 0.75) return lerp(c3, c4, (t - 0.5) / 0.25);
    else return lerp(c4, c5, (t - 0.75) / 0.25);
}

float3 lerp3(float3 a, float3 b, float t) { return a + (b - a) * t; }

// Helper: draw a filled circle
float circle(float2 uv, float2 center, float radius) {
    return smoothstep(radius, radius - 0.01, length(uv - center));
}

// Helper: draw a filled rectangle
float rect(float2 uv, float2 min, float2 max) {
    return step(min.x, uv.x) * step(uv.x, max.x) * step(min.y, uv.y) * step(uv.y, max.y);
}

// Helper: draw a progress arc (for phase)
float arc(float2 uv, float2 center, float radius, float thickness, float phase) {
    float2 d = uv - center;
    float r = length(d);
    float a = atan2(d.y, d.x) / 6.2831853 + 0.5;
    float in_ring = smoothstep(thickness, 0.0, abs(r - radius));
    float in_arc = step(a, phase);
    return in_ring * in_arc;
}

// Helper: draw a line (for pan)
float line(float2 uv, float2 a, float2 b, float width) {
    float2 pa = uv - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return smoothstep(width, 0.0, length(pa - ba * h));
}

// Helper: draw text-like blocks (for numbers, format)
float text_digit(float2 uv, int digit, float2 pos, float2 size) {
    // Standard 7-segment layout:
    //  ---a---
    // |       |
    // f       b
    // |       |
    //  ---g---
    // |       |
    // e       c
    // |       |
    //  ---d---
    float2 rel = (uv - pos) / size;
    float seg_w = 0.15, seg_l = 0.7, seg_o = 0.05;
    float v = 0.0;
    // Define each segment as a rectangle
    float a = rect(rel, float2(seg_o, 0.0), float2(1.0 - seg_o, seg_w)); // top
    float b = rect(rel, float2(1.0 - seg_w, seg_o), float2(1.0, 0.5 - seg_o)); // top right
    float c = rect(rel, float2(1.0 - seg_w, 0.5 + seg_o), float2(1.0, 1.0 - seg_o)); // bottom right
    float d = rect(rel, float2(seg_o, 1.0 - seg_w), float2(1.0 - seg_o, 1.0)); // bottom
    float e = rect(rel, float2(0.0, 0.5 + seg_o), float2(seg_w, 1.0 - seg_o)); // bottom left
    float f = rect(rel, float2(0.0, seg_o), float2(seg_w, 0.5 - seg_o)); // top left
    float g = rect(rel, float2(seg_o, 0.5 - seg_w * 0.5), float2(1.0 - seg_o, 0.5 + seg_w * 0.5)); // middle
    // Segment patterns for 0-9 (gfedcba order, 1=on, 0=off)
    static const int patterns[10] = {
        0x3F, // 0: 0b0111111
        0x06, // 1: 0b0000110
        0x5B, // 2: 0b1011011
        0x4F, // 3: 0b1001111
        0x66, // 4: 0b1100110
        0x6D, // 5: 0b1101101
        0x7D, // 6: 0b1111101
        0x07, // 7: 0b0000111
        0x7F, // 8: 0b1111111
        0x6F  // 9: 0b1101111
    };
    int segs = patterns[clamp(digit, 0, 9)];
    if (segs & 0x01) v += a;
    if (segs & 0x02) v += b;
    if (segs & 0x04) v += c;
    if (segs & 0x08) v += d;
    if (segs & 0x10) v += e;
    if (segs & 0x20) v += f;
    if (segs & 0x40) v += g;
    return saturate(v);
}

float4 OverlayPS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target {
    float4 originalColor = tex2D(ReShade::BackBuffer, uv);
    float2 base = margin;
    if (Listeningway_Corner == CORNER_TOP_RIGHT) base.x = 1.0 - overlay_size.x - margin.x;
    else if (Listeningway_Corner == CORNER_BOTTOM_LEFT) base.y = 1.0 - overlay_size.y - margin.y;
    else if (Listeningway_Corner == CORNER_BOTTOM_RIGHT) base = 1.0 - overlay_size - margin;
    float2 overlay_uv = (uv - base) / overlay_size;
    if (any(overlay_uv < 0.0) || any(overlay_uv > 1.0))
        return originalColor;
    float3 color = originalColor.rgb;
    float alpha = 0.0; // Start fully transparent
    // Draw 50% transparent black background under overlay
    float3 bg = lerp(originalColor.rgb, float3(0,0,0), 0.5);
    float bg_alpha = 0.5;
    color = bg;
    alpha = bg_alpha;
    // Draw spectrum bars
    int band = int(overlay_uv.x * LISTENINGWAY_NUM_BANDS);
    band = clamp(band, 0, LISTENINGWAY_NUM_BANDS - 1);
    float bandValue = clamp(Listeningway_FreqBands[band], 0.0, 1.0);
    float bar_top = 0.60 - bandValue * 0.45;
    float bar_width = 1.0 / LISTENINGWAY_NUM_BANDS * 0.8;
    float bar_center = (band + 0.5) / LISTENINGWAY_NUM_BANDS;
    float xdist = abs(overlay_uv.x - bar_center);
    if (xdist < bar_width * 0.5 && overlay_uv.y > bar_top && overlay_uv.y < 0.60) {
        color = band_color(band / (float)(LISTENINGWAY_NUM_BANDS - 1));
        alpha = 1.0;
    }
    // Draw overall volume as a wide bar below spectrum
    float vol = clamp(Listeningway_Volume, 0.0, 1.0);
    if (overlay_uv.y > 0.62 && overlay_uv.y < 0.68 && overlay_uv.x < vol) {
        color = lerp3(float3(0.2,0.2,0.2), float3(1.0,0.7,0.2), vol);
        alpha = 1.0;
    }
    // Draw stereo meters (left/right)
    float vleft = clamp(Listeningway_VolumeLeft, 0.0, 1.0);
    float vright = clamp(Listeningway_VolumeRight, 0.0, 1.0);
    if (overlay_uv.x < 0.04 && overlay_uv.y > 0.10 && overlay_uv.y < 0.10 + vleft * 0.40) {
        color = float3(0.2, 0.8, 0.2);
        alpha = 1.0;
    }
    if (overlay_uv.x > 0.96 && overlay_uv.y > 0.10 && overlay_uv.y < 0.10 + vright * 0.40) {
        color = float3(0.2, 0.6, 1.0);
        alpha = 1.0;
    }
    // Draw pan indicator (slider)
    float pan = clamp(Listeningway_AudioPan, -1.0, 1.0);
    float pan_x = 0.5 + pan * 0.45;
    if (overlay_uv.y > 0.72 && overlay_uv.y < 0.76 && abs(overlay_uv.x - pan_x) < 0.02) {
        color = float3(1.0, 0.3, 0.7);
        alpha = 1.0;
    }
    // Draw beat as a pulsing circle
    float beat = clamp(Listeningway_Beat, 0.0, 1.0);
    float2 beat_center = float2(0.08, 0.90);
    float beat_r = 0.04 + 0.03 * beat;
    if (circle(overlay_uv, beat_center, beat_r) > 0.5) {
        color = lerp3(float3(0.7,0.2,0.2), float3(1.0,1.0,1.0), beat);
        alpha = 1.0;
    }
    // Draw time as a digital clock (seconds)
    float t = Listeningway_TimeSeconds;
    int sec = int(fmod(t, 60.0));
    int min = int(fmod(t / 60.0, 60.0));
    float2 clock_pos = float2(0.80, 0.90);
    float2 digit_size = float2(0.04, 0.08);
    if (text_digit(overlay_uv, min / 10, clock_pos, digit_size) > 0.5) { color = float3(1,1,1); alpha = 1.0; }
    if (text_digit(overlay_uv, min % 10, clock_pos + float2(0.045,0), digit_size) > 0.5) { color = float3(1,1,1); alpha = 1.0; }
    if (text_digit(overlay_uv, sec / 10, clock_pos + float2(0.10,0), digit_size) > 0.5) { color = float3(1,1,1); alpha = 1.0; }
    if (text_digit(overlay_uv, sec % 10, clock_pos + float2(0.145,0), digit_size) > 0.5) { color = float3(1,1,1); alpha = 1.0; }
    // Draw phase wheels (60Hz, 120Hz)
    float phase60 = frac(Listeningway_TimePhase60Hz);
    float phase120 = frac(Listeningway_TimePhase120Hz);
    float2 phase60_center = float2(0.20, 0.90);
    float2 phase120_center = float2(0.30, 0.90);
    if (arc(overlay_uv, phase60_center, 0.035, 0.012, phase60) > 0.5) { color = float3(0.8,0.8,0.2); alpha = 1.0; }
    if (arc(overlay_uv, phase120_center, 0.035, 0.012, phase120) > 0.5) { color = float3(0.2,0.8,0.8); alpha = 1.0; }
    // Draw total phase progress bars
    float total60 = frac(Listeningway_TotalPhases60Hz / 100.0);
    float total120 = frac(Listeningway_TotalPhases120Hz / 100.0);
    if (overlay_uv.y > 0.80 && overlay_uv.y < 0.82 && overlay_uv.x > 0.20 && overlay_uv.x < 0.20 + total60 * 0.10) { color = float3(0.8,0.8,0.2); alpha = 1.0; }
    if (overlay_uv.y > 0.84 && overlay_uv.y < 0.86 && overlay_uv.x > 0.30 && overlay_uv.x < 0.30 + total120 * 0.10) { color = float3(0.2,0.8,0.8); alpha = 1.0; }
    // Draw audio format as colored block
    int fmt = int(Listeningway_AudioFormat + 0.5);
    float2 fmt_pos = float2(0.94, 0.10);
    float2 fmt_size = float2(0.04, 0.08);
    if (rect(overlay_uv, fmt_pos, fmt_pos + fmt_size) > 0.5) {
        if (fmt == 1) color = float3(0.7,0.7,0.7); // mono
        else if (fmt == 2) color = float3(0.2,0.8,0.2); // stereo
        else if (fmt == 6) color = float3(0.2,0.6,1.0); // 5.1
        else if (fmt == 8) color = float3(1.0,0.5,0.2); // 7.1
        else color = float3(0.2,0.2,0.2); // none/unknown
        alpha = 1.0;
    }
    // Only draw overlay elements, never a background
    return float4(color, alpha);
}

technique ListeningwayData <
    ui_label = "Listeningway Data";
    ui_tooltip = "Displays all Listeningway audio analysis data: spectrum, volume, beat, stereo, pan, time, phase, and format. Use this overlay to debug or visualize real-time audio features in your shader.";
>{
    pass {
        VertexShader = VS_Overlay;
        PixelShader = OverlayPS;
    }
}
