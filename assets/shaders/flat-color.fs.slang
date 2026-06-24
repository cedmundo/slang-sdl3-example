struct UniformData {
  float4 from_color;
  float4 to_color;
  float time;
};

struct PSInput {
  float4 position : SV_Position;
};

struct PSOutput {
  float4 color : SV_Target;
};

// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks
layout(set = 3, binding = 0) ParameterBlock<UniformData> uniforms;

[shader("pixel")]
PSOutput main(PSInput input) {
  PSOutput output;
  float factor = (sin(uniforms.time) + 1) / 2;
  output.color = lerp(uniforms.from_color, uniforms.to_color, factor);
  return output;
}
