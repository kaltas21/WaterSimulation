#version 460 core
// SPH Billboard Vertex Shader

const float FLOAT_MIN = 1.175494351e-38;
const float GRID_EPS = 0.000001;
const float MAX_DENSITY = 30.0;
const vec3 PARTICLE_COLOR = vec3(0.2, 0.5, 0.8);

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

uniform mat4 uVP;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uGridSize;
uniform vec3 uGridOrigin;
uniform ivec3 uGridRes;
uniform uint uParticleCount;
uniform float uPointRadius;
uniform int uColorMode;

out vec3 vColor;
out vec3 vCenterPos;
out vec2 vUV;

vec2 UVS[4] = { vec2(0,0), vec2(1,0), vec2(0,1), vec2(1,1) };
vec2 OFFSETS[4] = { vec2(-1,-1), vec2(-1,+1), vec2(+1,-1), vec2(+1,+1) };

void main()
{
  uint gid = gl_VertexID / 4;
  uint lid = gl_VertexID % 4;

  // Bounds check to avoid out-of-bounds access
  if (gid >= uParticleCount) {
    gl_Position = vec4(0.0, 0.0, -1.0, 1.0); // Clip this vertex
    vColor = vec3(1.0, 0.0, 0.0); // Red for debugging
    vCenterPos = vec3(0.0);
    vUV = vec2(0.0);
    return;
  }

  vec3 particlePos = particles[gid].position;

  // Force bright color for debugging black particle issue
  vColor = vec3(1.0, 0.0, 1.0); // Bright magenta for maximum visibility
  
  if (uColorMode == 1)
  {
    vec3 velocity = particles[gid].velocity;
    float speed = length(velocity);
    vColor = mix(vec3(0.0, 0.2, 0.8), vec3(1.0, 0.5, 0.0), clamp(speed / 5.0, 0.0, 1.0));
  }
  else if (uColorMode == 2)
  {
    float density = particles[gid].density;
    float norm = clamp(density / 1200.0, 0.0, 1.0);
    vColor = mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), norm);
  }

  vCenterPos = (uView * vec4(particlePos, 1.0)).xyz;
  vUV = UVS[lid];

  vec3 wsCameraRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
  vec3 wsCameraUp = vec3(uView[0][1], uView[1][1], uView[2][1]);

  vec3 wsVertPos = particlePos + (wsCameraRight * OFFSETS[lid].x + wsCameraUp * OFFSETS[lid].y) * uPointRadius;

  gl_Position = uVP * vec4(wsVertPos, 1.0);
}