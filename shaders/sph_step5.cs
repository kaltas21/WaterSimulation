#version 460 core
// SPH Step 5: Density and pressure calculation with O(n) spatial hashing

layout(local_size_x = 64) in;

layout(r32ui, binding = 0) uniform restrict readonly uimage3D grid;

struct Particle
{
  vec3 position;
  float density;
  vec3 velocity;
  float pressure;
};

layout(binding = 0, std430) restrict buffer particleBuf
{
  Particle particles[];
};

uniform vec3 uInvCellSize;
uniform vec3 uGridOrigin;
uniform ivec3 uGridRes;

// SPH constants
const float MASS = 0.02;
const float KERNEL_RADIUS = 0.1828; // 4 * 0.0457
const float POLY6_KERNEL_WEIGHT_CONST = 315.0 / (64.0 * 3.14159265 * pow(KERNEL_RADIUS, 9));
const float STIFFNESS_K = 250.0;
const float REST_DENSITY = 998.27;
const float REST_PRESSURE = 0.0;

const ivec3 NEIGHBORHOOD_LUT[27] = {
  ivec3(-1, -1, -1), ivec3(0, -1, -1), ivec3(1, -1, -1),
  ivec3(-1, -1,  0), ivec3(0, -1,  0), ivec3(1, -1,  0),
  ivec3(-1, -1,  1), ivec3(0, -1,  1), ivec3(1, -1,  1),
  ivec3(-1,  0, -1), ivec3(0,  0, -1), ivec3(1,  0, -1),
  ivec3(-1,  0,  0), ivec3(0,  0,  0), ivec3(1,  0,  0),
  ivec3(-1,  0,  1), ivec3(0,  0,  1), ivec3(1,  0,  1),
  ivec3(-1,  1, -1), ivec3(0,  1, -1), ivec3(1,  1, -1),
  ivec3(-1,  1,  0), ivec3(0,  1,  0), ivec3(1,  1,  0),
  ivec3(-1,  1,  1), ivec3(0,  1,  1), ivec3(1,  1,  1)
};

void main()
{
  uint particleId = gl_GlobalInvocationID.x;
  
  // Bounds check
  if (particleId >= particles.length()) return;
  
  Particle particle = particles[particleId];
  
  ivec3 voxelId = ivec3(uInvCellSize * (particle.position - uGridOrigin));
  
  float density = 0.0;
  
  uint voxelCount = 0;
  uint voxelParticleCount = 0;
  uint voxelParticleOffset = 0;
  
  // O(n) spatial hashing neighbor search
  #pragma unroll 1
  while (voxelCount < 27 || voxelParticleCount > 0)
  {
    if (voxelParticleCount == 0)
    {
      ivec3 newVoxelId = voxelId + NEIGHBORHOOD_LUT[voxelCount];
      voxelCount++;

      if (any(lessThan(newVoxelId, ivec3(0))) || any(greaterThanEqual(newVoxelId, uGridRes)))
      {
        continue;
      }

      uint voxelValue = imageLoad(grid, newVoxelId).r;
      voxelParticleOffset = (voxelValue >> 8);
      voxelParticleCount = (voxelValue & 0xFF);

      if (voxelParticleCount == 0)
      {
        continue;
      }
    }

    voxelParticleCount--;

    uint otherParticleId = voxelParticleOffset + voxelParticleCount;
    vec3 otherParticlePos = particles[otherParticleId].position;

    vec3 r = particle.position - otherParticlePos;

    float rLen = length(r);

    if (rLen >= KERNEL_RADIUS)
    {
      continue;
    }

    float weight = pow(KERNEL_RADIUS * KERNEL_RADIUS - rLen * rLen, 3) * POLY6_KERNEL_WEIGHT_CONST;

    density += MASS * weight;
  }
  
  // Calculate pressure using Tait equation
  float pressure = REST_PRESSURE + STIFFNESS_K * (density - REST_DENSITY);
  
  particle.density = density;
  particle.pressure = pressure;
  
  particles[particleId] = particle;
}