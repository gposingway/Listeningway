// Simple debug shader for Listeningway addon
#include "ListeningwayUniforms.fxh"

#define ORIENT_HORIZONTAL 0
#define ORIENT_VERTICAL 1
#define CORNER_TOP_LEFT 0
#define CORNER_TOP_RIGHT 1
#define CORNER_BOTTOM_LEFT 2
#define CORNER_BOTTOM_RIGHT 3

uniform int Listeningway_Orientation < ui_type = "combo"; ui_items = "Horizontal\0Vertical\0"; ui_label = "Orientation"; > = 0;
uniform int Listeningway_Corner < ui_type = "combo"; ui_items = "Top Left\0Top Right\0Bottom Left\0Bottom Right\0"; ui_label = "Corner"; > = 0;

uniform float4 backbufferTex : BACKBUFFER;

// Overlay size and margin
static const float2 overlay_size = float2(0.38, 0.12); // width, height in UV space (wider for more bands)
static const float2 margin = float2(0.02, 0.02);

// Simple passthrough vertex shader
void VS_DebugBars(in uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD)
{
    uv.x = (id == 2) ? 2.0 : 0.0;
    uv.y = (id == 1) ? 2.0 : 0.0;
    pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}

float3 band_color(float t) {
    // Gradient: blue -> cyan -> green -> yellow -> red
    float3 c1 = float3(0.2, 0.6, 1.0); // blue
    float3 c2 = float3(0.0, 1.0, 1.0); // cyan
    float3 c3 = float3(0.0, 1.0, 0.2); // green
    float3 c4 = float3(1.0, 1.0, 0.0); // yellow
    float3 c5 = float3(1.0, 0.2, 0.2); // red
    if (t < 0.25) return lerp(c1, c2, t / 0.25);
    else if (t < 0.5) return lerp(c2, c3, (t - 0.25) / 0.25);
    else if (t < 0.75) return lerp(c3, c4, (t - 0.5) / 0.25);
    else return lerp(c4, c5, (t - 0.75) / 0.25);
}

float4 DebugPS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    // Determine overlay position
    float2 base = margin;
    if (Listeningway_Corner == CORNER_TOP_RIGHT) base.x = 1.0 - overlay_size.x - margin.x;
    else if (Listeningway_Corner == CORNER_BOTTOM_LEFT) base.y = 1.0 - overlay_size.y - margin.y;
    else if (Listeningway_Corner == CORNER_BOTTOM_RIGHT) base = 1.0 - overlay_size - margin;

    float2 overlay_uv = (uv - base) / overlay_size;
    // Sample the backbuffer for the base color
    if (any(overlay_uv < 0.0) || any(overlay_uv > 1.0))
        return backbufferTex; // Show the game for non-overlay pixels

    float3 color = float3(0.08, 0.08, 0.10); // background

    // Draw bars and volume
    if (Listeningway_Orientation == ORIENT_HORIZONTAL) {
        // Horizontal bars
        int band = int(overlay_uv.x * LISTENINGWAY_NUM_BANDS);
        band = clamp(band, 0, LISTENINGWAY_NUM_BANDS - 1);
        float bandValue = clamp(Listeningway_FreqBands[band], 0.0, 1.0);
        float bar_top = 1.0 - bandValue;
        float bar_width = 1.0 / LISTENINGWAY_NUM_BANDS * 0.8;
        float bar_center = (band + 0.5) / LISTENINGWAY_NUM_BANDS;
        float xdist = abs(overlay_uv.x - bar_center);
        if (xdist < bar_width * 0.5 && overlay_uv.y > bar_top) {
            color = band_color(band / (float)(LISTENINGWAY_NUM_BANDS - 1));
        }
        // Volume meter as a thin bar below
        float vol_height = 0.08;
        if (overlay_uv.y > 1.0 - vol_height && overlay_uv.x < clamp(Listeningway_Volume, 0.0, 1.0))
            color = float3(1.0, 0.5, 0.1);
    } else {
        // Vertical bars
        int band = int((1.0 - overlay_uv.y) * LISTENINGWAY_NUM_BANDS);
        band = clamp(band, 0, LISTENINGWAY_NUM_BANDS - 1);
        float bandValue = clamp(Listeningway_FreqBands[band], 0.0, 1.0);
        float bar_right = bandValue;
        float bar_height = 1.0 / LISTENINGWAY_NUM_BANDS * 0.8;
        float bar_center = 1.0 - (band + 0.5) / LISTENINGWAY_NUM_BANDS;
        float ydist = abs(overlay_uv.y - bar_center);
        if (ydist < bar_height * 0.5 && overlay_uv.x < bar_right) {
            color = band_color(band / (float)(LISTENINGWAY_NUM_BANDS - 1));
        }
        // Volume meter as a thin bar at left
        float vol_width = 0.08;
        if (overlay_uv.x < vol_width && overlay_uv.y > 1.0 - clamp(Listeningway_Volume, 0.0, 1.0))
            color = float3(1.0, 0.5, 0.1);
    }
    return float4(color, 1.0); // Draw overlay fully opaque
}

technique DebugBars
{
    pass
    {
        VertexShader = VS_DebugBars;
        PixelShader = DebugPS;
    }
}
