#version 460 core
// SPH Step 6: Force calculation with O(n) spatial hashing

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

uniform float uDT;
uniform vec3 uGravity;
uniform vec3 uInvCellSize;
uniform vec3 uGridOrigin;
uniform ivec3 uGridRes;

// SPH constants
const float MASS = 0.02;
const float KERNEL_RADIUS = 0.1828; // 4 * 0.0457
const float VIS_COEFF = 0.035;
const float SPIKY_KERNEL_WEIGHT_CONST = 15.0 / (3.14159265 * pow(KERNEL_RADIUS, 6));
const float VIS_KERNEL_WEIGHT_CONST = 45.0 / (3.14159265 * pow(KERNEL_RADIUS, 6));

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
  
  vec3 forcePressure = vec3(0.0);
  vec3 forceViscosity = vec3(0.0);
  
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
    
    if (otherParticleId == particleId) continue;
    
    Particle otherParticle = particles[otherParticleId];
    vec3 r = particle.position - otherParticle.position;
    float rLen = length(r);
    
    if (rLen >= KERNEL_RADIUS || rLen <= 0.0001) continue;
    
    // Pressure force (using spiky kernel gradient)
    vec3 weightPressure = SPIKY_KERNEL_WEIGHT_CONST * pow(KERNEL_RADIUS - rLen, 2) * (r / rLen);
    float pressure = particle.pressure + otherParticle.pressure;
    forcePressure -= (MASS * pressure * weightPressure) / (2.0 * otherParticle.density);
    
    // Viscosity force (using viscosity kernel laplacian)
    float weightVis = VIS_KERNEL_WEIGHT_CONST * (KERNEL_RADIUS - rLen);
    vec3 velocityDiff = otherParticle.velocity - particle.velocity;
    forceViscosity += (MASS * velocityDiff * weightVis) / otherParticle.density;
  }
  
  // Apply gravity force
  vec3 forceGravity = uGravity * particle.density;
  
  // Total force
  vec3 totalForce = (forceViscosity * VIS_COEFF) + forcePressure + forceGravity;
  
  // Calculate acceleration (F = ma, so a = F/m)
  vec3 acceleration = totalForce / particle.density;
  
  // Update velocity using forward Euler integration
  particles[particleId].velocity += acceleration * uDT;
  
  // Clamp velocity to prevent instability
  float maxVel = 50.0;
  if (length(particles[particleId].velocity) > maxVel) {
    particles[particleId].velocity = normalize(particles[particleId].velocity) * maxVel;
  }
}