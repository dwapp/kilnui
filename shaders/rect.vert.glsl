#version 450

/* rect.vert — Rounded-rectangle vertex shader.
   Inputs per vertex: position, local-space xy, rect size, corner radii, RGBA color.
   Passes all data to the fragment shader for SDF corner clipping. */

layout(location = 0) in vec2  inPosition;       // screen-space position (pixels)
layout(location = 1) in vec2  inLocalPos;        // local-space position inside rect
layout(location = 2) in vec2  inSize;            // rect width, height
layout(location = 3) in vec4  inCornerRadius;    // tl, tr, bl, br
layout(location = 4) in vec4  inColor;           // premultiplied-alpha RGBA [0..1]

layout(location = 0) out vec2  fragLocalPos;
layout(location = 1) out vec2  fragSize;
layout(location = 2) out vec4  fragCornerRadius;
layout(location = 3) out vec4  fragColor;

layout(set = 1, binding = 0) uniform UBO {
    mat4 projection;
} ubo;

void main() {
    gl_Position      = ubo.projection * vec4(inPosition, 0.0, 1.0);
    fragLocalPos     = inLocalPos;
    fragSize         = inSize;
    fragCornerRadius = inCornerRadius;
    fragColor        = inColor;
}
