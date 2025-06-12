#version 460 core
// SPH Depth Billboard Vertex Shader (matching our billboard approach)

struct Particle
{
  vec3 position;
  float density;
  vec3 velocity;
  float pressure;
};

layout(binding = 0, std430) restrict readonly buffer particleBuf
{
  Particle particles[];
};

uniform mat4 uMVP;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uPointRadius;
uniform uint uNumParticles;

out vec3 vCenterPos;
out vec2 vUV;
out vec3 vColor;

vec2 UVS[4] = { vec2(0,0), vec2(1,0), vec2(0,1), vec2(1,1) };
vec2 OFFSETS[4] = { vec2(-1,-1), vec2(-1,+1), vec2(+1,-1), vec2(+1,+1) };

void main() {
    uint gid = gl_VertexID / 4;
    uint lid = gl_VertexID % 4;

    // Bounds check
    if (gid >= uNumParticles) {
        gl_Position = vec4(0.0, 0.0, -1.0, 1.0);
        return;
    }

    vec3 particlePos = particles[gid].position;

    vCenterPos = (uView * vec4(particlePos, 1.0)).xyz;
    vUV = UVS[lid];
    vColor = vec3(0.1, 0.5, 0.9);

    vec3 wsCameraRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
    vec3 wsCameraUp = vec3(uView[0][1], uView[1][1], uView[2][1]);

    vec3 wsVertPos = particlePos + (wsCameraRight * OFFSETS[lid].x + wsCameraUp * OFFSETS[lid].y) * uPointRadius;

    gl_Position = uMVP * vec4(wsVertPos, 1.0);
}