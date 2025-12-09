//----------------------------------------------------------------------------
// sprite_ps.hlsl
// SpriteBatch pixel shader
//----------------------------------------------------------------------------

Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color    : COLOR0;
};

float4 PSMain(PSInput input) : SV_TARGET {
    float4 texColor = tex.Sample(samp, input.texCoord);
    return texColor * input.color;
}
