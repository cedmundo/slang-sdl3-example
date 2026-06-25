struct UniformData {
  float time;
};

struct PSInput {
  float4 position : SV_Position;
  float3 color;
  float2 uvs;
};

struct PSOutput {
  float4 color : SV_Target;
};

struct TextureData {
  Sampler2D tex0;
}

// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks
layout(set = 2, binding = 0) ConstantBuffer<TextureData> samplers;
layout(set = 3, binding = 0) ParameterBlock<UniformData> uniforms;

[shader("pixel")]
PSOutput main(PSInput input) {
  PSOutput output;
  float factor = (sin(uniforms.time) + 1) / 2;
  float3 uv_color = samplers.tex0.Sample(input.uvs).xyz;
  float3 va_color = input.color;
  output.color = float4(lerp(uv_color, va_color, factor), 1.0);
  return output;
}
