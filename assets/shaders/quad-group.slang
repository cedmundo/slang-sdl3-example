
struct UniformData {
  float delta_time;
  uint32_t span_start;
  uint32_t span_end;
};

struct QuadState {
  float2 origin;
  float angle;
  float speed;
  float radius;
};

// Layout here: https://wiki.libsdl.org/SDL3/SDL_CreateGPUComputePipeline
layout(set = 2) ParameterBlock<UniformData> uniforms;
layout(set = 1, binding = 0) RWStructuredBuffer<QuadState> io_states;
layout(set = 1, binding = 1) RWStructuredBuffer<float2> o_positions;

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 threadId: SV_DispatchThreadID) {
  uint index = threadId.x;
  if (index >= uniforms.span_end) {
    return;
  }

  QuadState state = io_states[index];
  float angle = state.angle;
  state.angle = angle + uniforms.delta_time * state.speed;
  io_states[index] = state;
  o_positions[index] = float2(state.origin.x + cos(angle) * state.radius,
                              state.origin.y + sin(angle) * state.radius);
}
