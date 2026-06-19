#version 450

/* border.frag.glsl — SDF rounded-rectangle border fragment shader.
   Reuses rect.vert.glsl. 
   Draws an outline with independent side widths and optional dashing. */

layout(location = 0) in vec2  fragLocalPos;
layout(location = 1) in vec2  fragSize;
layout(location = 2) in vec4  fragCornerRadius;
layout(location = 3) in vec4  fragColor; // used as border color

layout(location = 0) out vec4 outColor;

layout(set = 3, binding = 0) uniform BorderUniforms {
    float top;
    float right;
    float bottom;
    float left;
    float dashLength;
    float dashGap;
} border;

float roundedBoxSDF(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + vec2(r);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main() {
    vec2 halfSize = fragSize * 0.5;
    vec2 centered = fragLocalPos - halfSize;
    
    // Determine corner radius for current quadrant
    float r;
    if (centered.x < 0.0 && centered.y < 0.0) r = fragCornerRadius.x;
    else if (centered.x >= 0.0 && centered.y < 0.0) r = fragCornerRadius.y;
    else if (centered.x < 0.0 && centered.y >= 0.0) r = fragCornerRadius.z;
    else r = fragCornerRadius.w;
    
    // Outer SDF
    float distOuter = roundedBoxSDF(centered, halfSize, r);
    
    // Inner SDF
    // To support independent widths, we compute the inner box parameters
    float top = border.top;
    float right = border.right;
    float bottom = border.bottom;
    float left = border.left;
    
    vec2 innerHalfSize = halfSize - vec2(left + right, top + bottom) * 0.5;
    vec2 innerCenterOffset = vec2(left - right, top - bottom) * 0.5;
    vec2 innerCentered = centered - innerCenterOffset;
    
    // Inner radius is reduced by the average border width roughly
    // For exactness we would pick the specific corner, but this approximation is okay.
    float currentBorder = 0.0;
    if (centered.x < 0.0 && centered.y < 0.0) currentBorder = max(left, top);
    else if (centered.x >= 0.0 && centered.y < 0.0) currentBorder = max(right, top);
    else if (centered.x < 0.0 && centered.y >= 0.0) currentBorder = max(left, bottom);
    else currentBorder = max(right, bottom);
    
    float rInner = max(r - currentBorder, 0.0);
    
    float distInner = roundedBoxSDF(innerCentered, innerHalfSize, rInner);
    
    float aa = 1.0;
    // Outer alpha (1 inside the outer box)
    float alphaOuter = 1.0 - smoothstep(-aa, aa, distOuter);
    // Inner alpha (0 inside the inner box)
    float alphaInner = smoothstep(-aa, aa, distInner);
    
    float alpha = alphaOuter * alphaInner;
    
    // Dashing
    if (border.dashLength > 0.0) {
        // Approximate perimeter position using max of absolute coordinates
        // This is a simplified dash mapping for rects
        float p;
        if (abs(centered.x) > abs(centered.y)) {
            p = centered.y + halfSize.y; 
        } else {
            p = centered.x + halfSize.x;
        }
        
        float dashCycle = border.dashLength + border.dashGap;
        if (mod(p, dashCycle) > border.dashLength) {
            alpha = 0.0;
        }
    }
    
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
