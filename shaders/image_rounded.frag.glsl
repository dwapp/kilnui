#version 450

/* image_rounded.frag.glsl — Rounded image fragment shader.
   Reuses tex.vert.glsl. 
   Samples a texture and clips it to a rounded rectangle using SDF. */

layout(location = 0) in vec2  fragTexCoord;
layout(location = 1) in vec4  fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

// Passed via uniform since tex.vert doesn't include size and radius
layout(set = 2, binding = 1) uniform ImageRoundedUniforms {
    vec2 size;
    vec4 cornerRadius; // tl, tr, bl, br
} params;

float roundedBoxSDF(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + vec2(r);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main() {
    // Reconstruct local pixel coordinates from UV and size
    vec2 fragLocalPos = fragTexCoord * params.size;
    
    vec2 halfSize = params.size * 0.5;
    vec2 centered = fragLocalPos - halfSize;
    
    // Pick the radius for the quadrant
    float r;
    if (centered.x < 0.0 && centered.y < 0.0) r = params.cornerRadius.x;
    else if (centered.x >= 0.0 && centered.y < 0.0) r = params.cornerRadius.y;
    else if (centered.x < 0.0 && centered.y >= 0.0) r = params.cornerRadius.z;
    else r = params.cornerRadius.w;
    
    float dist = roundedBoxSDF(centered, halfSize, r);
    
    float aa = 1.0;
    float alpha = 1.0 - smoothstep(-aa, aa, dist);
    
    vec4 texel = texture(texSampler, fragTexCoord);
    
    // Premultiplied alpha expected for tint
    outColor = texel * fragColor * alpha;
}
