
struct VSInput {
  float3 position : POSITION;
  float3 color : COLOR;
  float2 uvs : TEXCOORD0;
  uint instanceID : SV_InstanceID;
};

struct VSOutput {
  float4 position : SV_Position;
  float3 color;
  float2 uvs;
};

struct InstanceData {
  float2 position;
}

layout(set = 0, binding = 0) StructuredBuffer<InstanceData> instances;

[shader("vertex")]
VSOutput main(VSInput input) {
  VSOutput output;
  float3 instance_position = float3(instances[input.instanceID].position, 0.0f);
  output.position = float4(input.position + instance_position, 1.0f);
  output.color = input.color;
  output.uvs = input.uvs;
  return output;
}
