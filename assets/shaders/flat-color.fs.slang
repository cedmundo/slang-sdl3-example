struct PSInput {
  float4 position : SV_Position;
};

struct PSOutput {
  float4 color : SV_Target;
};

[shader("pixel")]
PSOutput pixelMain(PSInput input) {
  PSOutput output;
  output.color = float4(1.0f);
  return output;
}
