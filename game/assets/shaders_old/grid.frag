#version 450

layout(location = 0) in float fragPotential;

layout(location = 0) out vec4 outColor;

void main() {
    // Base color (Cyan-ish)
    vec3 color = vec3(0.0, 0.5, 1.0);
    
    // Intensity based on potential (deep gravity well = brighter or different color)
    // Potential is usually negative.
    float intensity = 1.0 + abs(fragPotential) * 5.0;
    
    // Clamp alpha
    float alpha = 0.3;
    if (abs(fragPotential) > 0.01) {
        alpha = 0.8;
        color = vec3(0.2, 0.8, 1.0); // Brighter cyan
    }

    outColor = vec4(color * intensity, alpha);
}
