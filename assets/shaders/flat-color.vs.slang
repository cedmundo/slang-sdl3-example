
struct VSInput {
  uint vertexID : SV_VertexID;
};

struct VSOutput {
  float4 position : SV_Position;
};

static const float2[] quadXYVertices = {
  float2(-0.5f, +0.5f),  // bottom left
  float2(-0.5f, -0.5f),  // top left
  float2(+0.5f, -0.5f),  // top right
  float2(+0.5f, -0.5f),  // top right
  float2(+0.5f, +0.5f),  // bottom right
  float2(-0.5f, +0.5f),  // bottom left
};

[shader("vertex")]
VSOutput vertexMain(VSInput input) {
  VSOutput output;
  output.position = float4(quadXYVertices[input.vertexID], 0.0f, 1.0f);
  return output;
}
