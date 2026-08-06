#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;      /* 64 bytes */
    vec4 modelParams;   /* 16 bytes: xyz = worldPos, w = radius */
    vec4 rotParams;     /* 16 bytes: xyz = axis, w = angle */
    vec4 lightAndStar;  /* 16 bytes */
    vec4 colorParams;   /* 16 bytes */
    /* Total: 128 bytes */
} pc;

mat4 rotationMatrix(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

void main() {
    /* Reconstruct Model Transform */
    /* 1. Scale */
    vec3 scaledPos = inPosition * pc.modelParams.w;
    
    /* 2. Rotate */
    /* If angle is 0 or axis is 0, skip? Shader logic simpler to just do it or identity */
    mat4 rot = rotationMatrix(pc.rotParams.xyz, pc.rotParams.w);
    /* Manually multiply vec3 by mat4 rotation part */
    vec3 rotatedPos = (rot * vec4(scaledPos, 1.0)).xyz;
    
    /* 3. Translate */
    vec3 worldPos = rotatedPos + pc.modelParams.xyz;
    
    gl_Position = pc.viewProj * vec4(worldPos, 1.0);
    
    fragUV = inUV;
    
    /* Normal */
    fragNormal = normalize(mat3(rot) * inNormal);
    
    fragPos = worldPos;
}
