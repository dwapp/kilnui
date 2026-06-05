#version 450

/* rect.frag — SDF rounded-rectangle fragment shader.
   Supports independent corner radii from Clay_CornerRadius.
   Uses a standard roundedBoxSDF with 1px smoothstep anti-aliasing. */

layout(location = 0) in vec2  fragLocalPos;
layout(location = 1) in vec2  fragSize;
layout(location = 2) in vec4  fragCornerRadius;   // tl, tr, bl, br
layout(location = 3) in vec4  fragColor;

layout(location = 0) out vec4 outColor;

/* Signed distance from point p to a rounded box centered at the origin.
   halfSize = rect half-extents, r = corner radius for the quadrant containing p. */
float roundedBoxSDF(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + vec2(r);
    vec2 q_out = max(q, 0.0);
    vec2 q2 = q_out * q_out;
    float d_out = sqrt(sqrt(q2.x * q2.x + q2.y * q2.y)) - r;
    return min(max(q.x, q.y), 0.0) + d_out;
}

void main() {
    vec2 halfSize = fragSize * 0.5;
    vec2 centered = fragLocalPos - halfSize;   // shift origin to rect center

    /* Pick the radius for the quadrant this fragment falls in */
    float r;
    if (centered.x < 0.0 && centered.y < 0.0) r = fragCornerRadius.x;  // top-left
    else if (centered.x >= 0.0 && centered.y < 0.0) r = fragCornerRadius.y;  // top-right
    else if (centered.x < 0.0 && centered.y >= 0.0) r = fragCornerRadius.z;  // bottom-left
    else r = fragCornerRadius.w;  // bottom-right

    float dist = roundedBoxSDF(centered, halfSize, r);

    /* 1px anti-aliasing band */
    float aa = 1.0;
    float alpha = 1.0 - smoothstep(-aa, aa, dist);

    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
