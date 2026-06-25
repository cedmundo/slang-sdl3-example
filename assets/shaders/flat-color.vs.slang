
struct VSInput {
  float3 position : POSITION;
  float3 color : COLOR;
  float2 uvs : TEXCOORD0;
};

struct VSOutput {
  float4 position : SV_Position;
  float3 color;
  float2 uvs;
};

[shader("vertex")]
VSOutput main(VSInput input) {
  VSOutput output;
  output.position = float4(input.position, 1.0f);
  output.color = input.color;
  output.uvs = input.uvs;
  return output;
}
