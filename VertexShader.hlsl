struct VSOut
{
    float3 c : COLOR0;
    float4 pos : SV_Position;
    
};

cbuffer CBuf
{
    matrix transformation;
};

VSOut main(float2 pos : Position, float3 c : Color)
{
    VSOut v;
    v.pos = mul(float4(pos.x, pos.y, 0.0f, 1.0f), transformation);
    v.c = c;
    return v;
}