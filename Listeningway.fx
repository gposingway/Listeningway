// Listeningway.fx - ReShade FX Shader for Audio Visualization

// Define the number of frequency bands - MUST match NUM_BANDS in C++ code!
#define LW_NUM_BANDS 32

// --- Uniforms (Updated by Addon) ---
uniform float fListeningwayVolume <
    ui_visible = false;
> = 0.0;

uniform float fListeningwayFreqBands[LW_NUM_BANDS] <
    ui_visible = false;
>;

// --- UI / Debug Uniforms ---
uniform bool bListeningwayDebugView <
    ui_label = "Show Debug Visualization";
    ui_tooltip = "Displays raw volume and frequency band values as bars.";
> = false;

uniform float fBarGraphHeight <
    ui_type = "slider";
    ui_label = "Bar Graph Height";
    ui_min = 0.01; ui_max = 0.5;
> = 0.1;

uniform float fBarGraphWidth <
    ui_type = "slider";
    ui_label = "Bar Graph Width";
    ui_min = 0.1; ui_max = 1.0;
> = 0.8;

uniform float fBarGraphYPos <
    ui_type = "slider";
    ui_label = "Bar Graph Y Position";
    ui_min = 0.0; ui_max = 1.0;
> = 0.9;

uniform float4 cBarColor <
    ui_type = "color";
    ui_label = "Bar Color";
> = float4(0.1, 0.7, 1.0, 0.8);

uniform float4 cBarBgColor <
    ui_type = "color";
    ui_label = "Bar Background Color";
> = float4(0.1, 0.1, 0.1, 0.5);

// --- Textures --- (ReShade provides these)
texture BackBufferTex : COLOR;
sampler BackBuffer { Texture = BackBufferTex; };

// --- Helper Functions ---
// Simple box drawing: returns 1.0 inside, 0.0 outside
float Box(float2 uv, float2 center, float2 size)
{
    float2 halfSize = size * 0.5;
    float2 delta = abs(uv - center);
    return step(delta.x, halfSize.x) * step(delta.y, halfSize.y);
}

// --- Vertex Shader ---
// Standard pass-through vertex shader required by ReShade
struct VS_OUTPUT {
    float4 position : SV_Position; // Clip space position
    float2 texcoord : TEXCOORD0;   // Texture coordinates
};

VS_OUTPUT ListeningwayVS(uint id : SV_VertexID)
{
    VS_OUTPUT output;
    // Generate texture coordinates for a full-screen triangle
    output.texcoord.x = (id == 2) ? 2.0 : 0.0;
    output.texcoord.y = (id == 1) ? 2.0 : 0.0;
    // Generate clip space position for the full-screen triangle
    output.position = float4(output.texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return output;
}

// --- Pixel Shader Input Struct ---
// Mirrors the VS_OUTPUT struct to ensure signature matching
struct PS_INPUT {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

// --- Pixel Shader ---
// Use the PS_INPUT struct for input
float4 ListeningwayPS(PS_INPUT input) : SV_Target
{
    // Use input.texcoord instead of texcoord
    float4 color = tex2D(BackBuffer, input.texcoord);

    // --- Bar Graph Visualization ---
    // Calculate dimensions and position in screen space (0.0 to 1.0)
    float graphWidth = fBarGraphWidth;
    float graphHeight = fBarGraphHeight;
    float graphBottom = fBarGraphYPos;
    float graphTop = graphBottom - graphHeight;
    float graphLeft = (1.0 - graphWidth) * 0.5;

    float barTotalWidth = graphWidth / LW_NUM_BANDS;
    float barSpacing = barTotalWidth * 0.1; // 10% spacing
    float barWidth = barTotalWidth - barSpacing;

    // Check if the current pixel is within the graph background area
    // Use input.texcoord instead of texcoord
    if (input.texcoord.x >= graphLeft && input.texcoord.x <= graphLeft + graphWidth && input.texcoord.y >= graphTop && input.texcoord.y <= graphBottom)
    {
        // Draw background for the graph area
        color.rgb = lerp(color.rgb, cBarBgColor.rgb, cBarBgColor.a);

        // Determine which band this pixel falls into
        // Use input.texcoord instead of texcoord
        int bandIndex = floor((input.texcoord.x - graphLeft) / barTotalWidth);

        if (bandIndex >= 0 && bandIndex < LW_NUM_BANDS)
        {
            // Calculate the horizontal center of the current band's bar
            float bandCenterX = graphLeft + (bandIndex + 0.5) * barTotalWidth;

            // Get the normalized height for this band
            float bandHeight = fListeningwayFreqBands[bandIndex];

            // Calculate the bar's dimensions and position
            float barActualHeight = bandHeight * graphHeight;
            float barTop = graphBottom - barActualHeight;
            float barCenterY = graphBottom - barActualHeight * 0.5;

            // Check if the pixel is within the visible part of the bar
            float2 barCenter = float2(bandCenterX, barCenterY);
            float2 barSize = float2(barWidth, barActualHeight);

            // Use input.texcoord instead of texcoord
            float barMask = Box(input.texcoord, barCenter, barSize);

            // If inside the bar, blend with bar color
            if (barMask > 0.0)
            {
                color.rgb = lerp(color.rgb, cBarColor.rgb, cBarColor.a);
            }
        }
    }

    // --- Debug View (Overlay) ---
    if (bListeningwayDebugView)
    {
        // Draw a simple bar for the overall volume near the top left
        float volWidth = 0.1;
        float volHeight = 0.02;
        float volPosX = 0.01;
        float volPosY = 0.01;
        float volFill = fListeningwayVolume * volWidth;

        // Background
        float2 volBgCenter = float2(volPosX + volWidth * 0.5, volPosY + volHeight * 0.5);
        float2 volBgSize = float2(volWidth, volHeight);
        // Use input.texcoord instead of texcoord
        color.rgb = lerp(color.rgb, float3(0.1, 0.1, 0.1), Box(input.texcoord, volBgCenter, volBgSize) * 0.7);

        // Foreground (Filled part)
        float2 volFgCenter = float2(volPosX + volFill * 0.5, volPosY + volHeight * 0.5);
        float2 volFgSize = float2(volFill, volHeight);
        // Use input.texcoord instead of texcoord
        color.rgb = lerp(color.rgb, float3(0.9, 0.9, 0.9), Box(input.texcoord, volFgCenter, volFgSize));
    }

    return color;
}

// --- Technique ---
technique Listeningway
{
    pass
    {
        VertexShader = ListeningwayVS; // Assign the vertex shader
        PixelShader = ListeningwayPS;
    }
}
