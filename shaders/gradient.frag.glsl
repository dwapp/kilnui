#version 450

/* gradient.frag.glsl — Gradient rounded-rectangle fragment shader.
   Reuses rect.vert.glsl. 
   Supports linear and radial gradients with up to 4 color stops. */

layout(location = 0) in vec2  fragLocalPos;
layout(location = 1) in vec2  fragSize;
layout(location = 2) in vec4  fragCornerRadius;
layout(location = 3) in vec4  fragColor; // used as global tint

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform GradientUniforms {
    vec2  startPoint; // Linear: start pos (local). Radial: center pos.
    vec2  endPoint;   // Linear: end pos (local).
    vec4  colors[4];  // Stop colors
    vec4  stops;      // Stop positions [0..1]
    int   type;       // 0 for linear, 1 for radial
    int   numStops;
    float radialRadius;
    float padding;
} grad;

float roundedBoxSDF(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + vec2(r);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main() {
    // Calculate gradient parameter t [0, 1]
    float t = 0.0;
    if (grad.type == 0) { // Linear
        vec2 dir = grad.endPoint - grad.startPoint;
        float lenSq = dot(dir, dir);
        if (lenSq > 0.0) {
            vec2 p = fragLocalPos - grad.startPoint;
            t = dot(p, dir) / lenSq;
        }
    } else { // Radial
        float d = distance(fragLocalPos, grad.startPoint);
        if (grad.radialRadius > 0.0) {
            t = d / grad.radialRadius;
        }
    }
    t = clamp(t, 0.0, 1.0);

    // Evaluate gradient colors
    vec4 gradColor = grad.colors[0];
    float stopValues[4] = float[](grad.stops.x, grad.stops.y, grad.stops.z, grad.stops.w);
    
    for (int i = 1; i < 4; i++) {
        if (i >= grad.numStops) break;
        float prevStop = stopValues[i - 1];
        float currStop = stopValues[i];
        
        if (t >= prevStop && t <= currStop) {
            float f = (currStop > prevStop) ? (t - prevStop) / (currStop - prevStop) : 0.0;
            gradColor = mix(gradColor, grad.colors[i], f);
        } else if (t > currStop) {
            gradColor = grad.colors[i];
        }
    }

    // SDF Rounded Rect Clipping
    vec2 halfSize = fragSize * 0.5;
    vec2 centered = fragLocalPos - halfSize;
    
    float r;
    if (centered.x < 0.0 && centered.y < 0.0) r = fragCornerRadius.x;
    else if (centered.x >= 0.0 && centered.y < 0.0) r = fragCornerRadius.y;
    else if (centered.x < 0.0 && centered.y >= 0.0) r = fragCornerRadius.z;
    else r = fragCornerRadius.w;
    
    float dist = roundedBoxSDF(centered, halfSize, r);
    float aa = 1.0;
    float alpha = 1.0 - smoothstep(-aa, aa, dist);
    
    outColor = gradColor * fragColor * alpha;
}
