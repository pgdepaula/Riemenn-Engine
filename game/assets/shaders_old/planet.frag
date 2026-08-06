#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec4 modelParams;
    vec4 rotParams;
    vec4 lightAndStar; /* xyz = lightPos, w = isStar */
    vec4 colorParams;  /* xyz = color, w = pad */
} pc;

void main() {
    /* Texture Color */
    vec4 texColor = texture(texSampler, fragUV);
    
    /* Mix with Body Color (tint) */
    vec3 baseBase = texColor.rgb * pc.colorParams.rgb;
    
    /* If it is a Star, ignore lighting */
    if (pc.lightAndStar.w > 0.5) {
        outColor = vec4(baseBase, 1.0);
        return;
    }
    
    /* Lighting Calculation */
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(pc.lightAndStar.xyz - fragPos);
    
    /* Diffuse (Lambert) */
    float diff = max(dot(norm, lightDir), 0.0);
    /* Ambient (weak) */
    float ambient = 0.05;
    
    vec3 finalColor = baseBase * (diff + ambient);
    
    outColor = vec4(finalColor, 1.0);
}
