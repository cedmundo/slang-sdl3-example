struct UniformData {
  float time;
};

struct PSInput {
  float4 position : SV_Position;
  float3 color;
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
  output.color = float4(input.color * factor, 1.0);
  return output;
}
