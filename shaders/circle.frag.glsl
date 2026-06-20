/* SPDX-License-Identifier: MIT */
#version 450

/* circle.frag.glsl — SDF circle and ring fragment shader.
   Reuses rect.vert.glsl. 
   Draws a perfect circle or ring with optional angular clipping (for progress). */

layout(location = 0) in vec2  fragLocalPos;
layout(location = 1) in vec2  fragSize;
layout(location = 2) in vec4  fragCornerRadius; // unused here
layout(location = 3) in vec4  fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform CircleUniforms {
    float thickness;  // 0 for solid, > 0 for ring thickness
    float startAngle; // in radians (e.g. -PI/2 for top)
    float endAngle;   // in radians
    float padding;
} circle;

#define PI 3.14159265359

void main() {
    vec2 halfSize = fragSize * 0.5;
    vec2 centered = fragLocalPos - halfSize;
    
    float maxRadius = min(halfSize.x, halfSize.y);
    float dist = 0.0;
    
    if (circle.thickness > 0.0) {
        // Ring
        float ringRadius = maxRadius - circle.thickness * 0.5;
        dist = abs(length(centered) - ringRadius) - circle.thickness * 0.5;
    } else {
        // Solid circle
        dist = length(centered) - maxRadius;
    }
    
    float aa = 1.0;
    float alpha = 1.0 - smoothstep(-aa, aa, dist);
    
    // Angular clipping for progress rings
    // Check if startAngle and endAngle are different
    if (circle.startAngle != circle.endAngle) {
        float angle = atan(centered.y, centered.x);
        // Normalize angle to [0, 2PI] range roughly
        // We want to handle standard progression
        // This is a simplified angle check, assumes start < end and both in reasonable ranges.
        // A robust check maps angle into [startAngle, startAngle + 2PI].
        
        float normalizedAngle = mod(angle - circle.startAngle, 2.0 * PI);
        if (normalizedAngle < 0.0) normalizedAngle += 2.0 * PI;
        
        float range = mod(circle.endAngle - circle.startAngle, 2.0 * PI);
        if (range < 0.0) range += 2.0 * PI;
        
        if (normalizedAngle > range) {
            alpha = 0.0;
        } else {
            // Optional: add rounded caps at the ends of the progress arc
            // This is complex for a simple shader, we just hard clip the angle for now.
            // Using smoothstep on angle to avoid aliased cut-off
            float angleWidth = aa / length(centered); // angular width of 1 pixel
            float distToEnd = min(normalizedAngle, range - normalizedAngle);
            if (distToEnd < angleWidth) {
                alpha *= smoothstep(0.0, angleWidth, distToEnd);
            }
        }
    }
    
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
