#version 450

/* line.vert.glsl — SDF Line vertex shader.
   Inputs per vertex: screen position, local position, line points (A and B), thickness, and color. */

layout(location = 0) in vec2  inPosition;       // Screen-space position (pixels)
layout(location = 1) in vec2  inLocalPos;       // Local-space position inside bounding quad
layout(location = 2) in vec2  inPointA;         // Start point of the line (in local space)
layout(location = 3) in vec2  inPointB;         // End point of the line (in local space)
layout(location = 4) in float inThickness;      // Line thickness
layout(location = 5) in vec4  inColor;          // RGBA color

layout(location = 0) out vec2  fragLocalPos;
layout(location = 1) out vec2  fragPointA;
layout(location = 2) out vec2  fragPointB;
layout(location = 3) out float fragThickness;
layout(location = 4) out vec4  fragColor;

layout(set = 1, binding = 0) uniform UBO {
    mat4 projection;
} ubo;

void main() {
    gl_Position   = ubo.projection * vec4(inPosition, 0.0, 1.0);
    fragLocalPos  = inLocalPos;
    fragPointA    = inPointA;
    fragPointB    = inPointB;
    fragThickness = inThickness;
    fragColor     = inColor;
}
