#version 460 core
// SPH Step 4: Velocity field calculation with O(n) spatial hashing

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(r32ui, binding = 0) uniform restrict readonly uimage3D grid;
layout(rgba32f, binding = 1) uniform restrict writeonly image3D velocityField;

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

uniform vec3 uGridOrigin;
uniform vec3 uGridSize;
uniform ivec3 uGridRes;

// SPH constants
const float MASS = 0.02;
const float KERNEL_RADIUS = 0.1828;
const float POLY6_KERNEL_WEIGHT_CONST = 315.0 / (64.0 * 3.14159265 * pow(KERNEL_RADIUS, 9));

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
  ivec3 voxelId = ivec3(gl_GlobalInvocationID);
  
  if (any(greaterThanEqual(voxelId, uGridRes))) return;
  
  // Calculate world position of this voxel center
  vec3 voxelWorldPos = uGridOrigin + (vec3(voxelId) + 0.5) * (uGridSize / vec3(uGridRes));
  
  vec3 velocitySum = vec3(0.0);
  float weightSum = 0.0;
  
  uint voxelCount = 0;
  uint voxelParticleCount = 0;
  uint voxelParticleOffset = 0;
  
  // O(n) spatial hashing neighbor search
  #pragma unroll 1
  while (voxelCount < 27 || voxelParticleCount > 0)
  {
    if (voxelParticleCount == 0)
    {
      ivec3 neighborVoxel = voxelId + NEIGHBORHOOD_LUT[voxelCount];
      voxelCount++;
      
      if (any(lessThan(neighborVoxel, ivec3(0))) || any(greaterThanEqual(neighborVoxel, uGridRes)))
      {
        continue;
      }
      
      uint voxelValue = imageLoad(grid, neighborVoxel).r;
      voxelParticleOffset = (voxelValue >> 8);
      voxelParticleCount = (voxelValue & 0xFF);
      
      if (voxelParticleCount == 0)
      {
        continue;
      }
    }
    
    voxelParticleCount--;
    
    uint particleId = voxelParticleOffset + voxelParticleCount;
    vec3 particlePos = particles[particleId].position;
    
    vec3 r = voxelWorldPos - particlePos;
    float rLen = length(r);
    
    if (rLen >= KERNEL_RADIUS) continue;
    
    // Poly6 kernel weight
    float weight = POLY6_KERNEL_WEIGHT_CONST * pow(KERNEL_RADIUS * KERNEL_RADIUS - rLen * rLen, 3);
    
    velocitySum += particles[particleId].velocity * weight;
    weightSum += weight;
  }
  
  vec3 filteredVelocity = vec3(0.0);
  if (weightSum > 0.0) {
    filteredVelocity = velocitySum / weightSum;
  }
  
  imageStore(velocityField, voxelId, vec4(filteredVelocity, 1.0));
}