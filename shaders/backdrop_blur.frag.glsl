#version 450

/* backdrop_blur.frag.glsl — Background blur fragment shader.
   Reuses tex.vert.glsl. 
   Applies a 1D Gaussian blur. Run in two passes (horizontal then vertical) 
   for an efficient 2D Glassmorphism blur effect. */

layout(location = 0) in vec2  fragTexCoord;
layout(location = 1) in vec4  fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D bgTexture;

layout(set = 2, binding = 1) uniform BlurUniforms {
    vec2  direction; // (1.0, 0.0) for horizontal pass, (0.0, 1.0) for vertical pass
    float radius;    // blur amount in pixels
    float texelSize; // 1.0 / texture_resolution
} blur;

void main() {
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;
    
    // 9-tap Gaussian blur (could be scaled up or down based on needs)
    const int taps = 4;
    
    for (int i = -taps; i <= taps; i++) {
        // Calculate Gaussian weight
        float sigma = float(taps) * 0.5;
        float weight = exp(-0.5 * pow(float(i) / sigma, 2.0));
        
        vec2 offset = blur.direction * (float(i) * blur.radius * blur.texelSize);
        color += texture(bgTexture, fragTexCoord + offset) * weight;
        
        totalWeight += weight;
    }
    
    color /= totalWeight;
    
    // Multiply by the tint color (fragColor). 
    // Useful for adding a subtle white/dark tint to the frosted glass.
    outColor = color * fragColor;
}
