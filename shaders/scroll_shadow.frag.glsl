#version 450

/* scroll_shadow.frag.glsl — Scroll indicator shadow fragment shader.
   Reuses rect.vert.glsl. 
   Creates a gradient shadow indicating scroll direction (e.g. at edges of a list). */

layout(location = 0) in vec2  fragLocalPos;
layout(location = 1) in vec2  fragSize;
layout(location = 2) in vec4  fragCornerRadius;
layout(location = 3) in vec4  fragColor; // tint color for the shadow

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform ScrollShadowUniforms {
    vec2  direction; // (0,1) for top edge, (0,-1) for bottom, (1,0) left, (-1,0) right
    float intensity; // opacity at the edge
    float padding;
} scroll;

float roundedBoxSDF(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + vec2(r);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main() {
    // Determine the gradient progress based on direction
    float t = 0.0;
    
    if (abs(scroll.direction.y) > 0.0) {
        // Vertical shadow
        if (scroll.direction.y > 0.0) {
            // Top edge (fades out as y increases)
            t = 1.0 - (fragLocalPos.y / fragSize.y);
        } else {
            // Bottom edge (fades out as y decreases)
            t = fragLocalPos.y / fragSize.y;
        }
    } else {
        // Horizontal shadow
        if (scroll.direction.x > 0.0) {
            // Left edge
            t = 1.0 - (fragLocalPos.x / fragSize.x);
        } else {
            // Right edge
            t = fragLocalPos.x / fragSize.x;
        }
    }
    
    // Non-linear falloff for a more natural shadow curve
    float shadowAlpha = pow(clamp(t, 0.0, 1.0), 2.0) * scroll.intensity;
    
    // Optional: still respect the corner radii if this shadow sits inside a rounded container
    vec2 halfSize = fragSize * 0.5;
    vec2 centered = fragLocalPos - halfSize;
    
    float r;
    if (centered.x < 0.0 && centered.y < 0.0) r = fragCornerRadius.x;
    else if (centered.x >= 0.0 && centered.y < 0.0) r = fragCornerRadius.y;
    else if (centered.x < 0.0 && centered.y >= 0.0) r = fragCornerRadius.z;
    else r = fragCornerRadius.w;
    
    float dist = roundedBoxSDF(centered, halfSize, r);
    float aa = 1.0;
    float clipAlpha = 1.0 - smoothstep(-aa, aa, dist);
    
    outColor = vec4(fragColor.rgb, fragColor.a * shadowAlpha * clipAlpha);
}
