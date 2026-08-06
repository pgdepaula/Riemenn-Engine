#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in float inPotential; // Matches SpacetimePass C config

layout(location = 0) out float fragPotential;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
} pc;

void main() {
    gl_Position = pc.viewProj * vec4(inPos, 1.0);
    fragPotential = inPotential;
}
