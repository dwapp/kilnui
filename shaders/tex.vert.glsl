/* SPDX-License-Identifier: MIT */
#version 450

/* tex.vert — Textured quad vertex shader.
   Used for text glyphs and images.  Color is premultiplied alpha tint. */

layout(location = 0) in vec2  inPosition;
layout(location = 1) in vec2  inTexCoord;
layout(location = 2) in vec4  inColor;

layout(location = 0) out vec2  fragTexCoord;
layout(location = 1) out vec4  fragColor;

layout(set = 1, binding = 0) uniform UBO {
    mat4 projection;
} ubo;

void main() {
    gl_Position  = ubo.projection * vec4(inPosition, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragColor    = inColor;
}
