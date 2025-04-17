// Simple debug shader for Listeningway addon
#define NUM_BANDS 8

uniform float fListeningwayFreqBands[NUM_BANDS] < ui_visible = false; >;

// Simple passthrough vertex shader
void VS_DebugBars(in uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD)
{
    uv.x = (id == 2) ? 2.0 : 0.0;
    uv.y = (id == 1) ? 2.0 : 0.0;
    pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}

float4 DebugPS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    float v = clamp(fListeningwayFreqBands[0], 0.0, 1.0); // Read and clamp the first band's value
    return float4(v, v, v, 1.0); // Output as grayscale color
}

technique DebugBars
{
    pass
    {
        VertexShader = VS_DebugBars;
        PixelShader = DebugPS;
    }
}
