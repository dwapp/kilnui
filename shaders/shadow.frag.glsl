#version 450

/* shadow.frag.glsl — SDF drop shadow fragment shader.
   Reuses rect.vert.glsl. 
   Approximates a Gaussian shadow analytically using SDF. */

layout(location = 0) in vec2  fragLocalPos;
layout(location = 1) in vec2  fragSize;
layout(location = 2) in vec4  fragCornerRadius;
layout(location = 3) in vec4  fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 3, binding = 0) uniform ShadowUniforms {
    vec2  offset;
    float blurRadius;
    float spread;
} shadow;

float roundedBoxSDF(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + vec2(r);
    vec2 q_out = max(q, 0.0);
    vec2 q2 = q_out * q_out;
    float d_out = sqrt(sqrt(q2.x * q2.x + q2.y * q2.y)) - r;
    return min(max(q.x, q.y), 0.0) + d_out;
}

void main() {
    vec2 halfSize = fragSize * 0.5;
    vec2 centered = fragLocalPos - halfSize;
    
    // Apply offset (shadow offset moves the shadow shape)
    centered -= shadow.offset;
    
    // Apply spread (increases the caster size)
    halfSize += shadow.spread;
    
    // Select correct corner radius for the current quadrant
    float r;
    if (centered.x < 0.0 && centered.y < 0.0) r = fragCornerRadius.x;
    else if (centered.x >= 0.0 && centered.y < 0.0) r = fragCornerRadius.y;
    else if (centered.x < 0.0 && centered.y >= 0.0) r = fragCornerRadius.z;
    else r = fragCornerRadius.w;
    
    // Increase radius by spread so corners don't get sharp
    r = max(r + shadow.spread, 0.0);
    
    float dist = roundedBoxSDF(centered, halfSize, r);
    
    // Analytical shadow falloff
    float alpha = 1.0;
    if (shadow.blurRadius > 0.0) {
        // We approximate the blur using smoothstep over the blur region.
        // A standard UI shadow typically spans from -blurRadius to +blurRadius
        alpha = 1.0 - smoothstep(-shadow.blurRadius, shadow.blurRadius, dist);
        
        // To make it look more like a Gaussian drop shadow, we can apply an exponential falloff outside
        if (dist > 0.0) {
            float x = dist / shadow.blurRadius;
            alpha = exp(-x * x * 2.0); // Gaussian-like falloff
        }
    } else {
        // 1px anti-aliasing if no blur
        float aa = 1.0;
        alpha = 1.0 - smoothstep(-aa, aa, dist);
    }
    
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
