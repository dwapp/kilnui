/* SPDX-License-Identifier: MIT */
#version 450

/* line.frag.glsl — SDF Line fragment shader.
   Uses precise SDF for lines with optional dashing and 1px anti-aliasing. */

layout(location = 0) in vec2  fragLocalPos;
layout(location = 1) in vec2  fragPointA;
layout(location = 2) in vec2  fragPointB;
layout(location = 3) in float fragThickness;
layout(location = 4) in vec4  fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform LineUniforms {
    float dashLength;
    float dashGap;
    float padding1;
    float padding2;
} line;

// SDF for a line segment
float lineSDF(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    // Distance from current pixel to the line segment
    float dist = lineSDF(fragLocalPos, fragPointA, fragPointB) - fragThickness * 0.5;
    
    float aa = 1.0;
    float alpha = 1.0 - smoothstep(-aa, aa, dist);
    
    // Dashing
    if (line.dashLength > 0.0) {
        // Find projection of current point onto the line segment to get linear distance
        vec2 pa = fragLocalPos - fragPointA;
        vec2 ba = fragPointB - fragPointA;
        float h = dot(pa, ba) / length(ba);
        
        // h is the actual distance along the line segment
        float cycle = line.dashLength + line.dashGap;
        if (mod(h, cycle) > line.dashLength) {
            alpha = 0.0;
        }
    }
    
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
