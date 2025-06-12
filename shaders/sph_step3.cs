#version 460 core
// SPH Step 3: Particle reordering

layout(local_size_x = 32) in;

struct Particle
{
  vec3 position;
  float density;
  vec3 velocity;
  float pressure;
};

layout(binding = 0, std430) restrict readonly buffer particleBuf1
{
  Particle inParticles[];
};

layout(binding = 1, std430) restrict writeonly buffer particleBuf2
{
  Particle outParticles[];
};

layout(r32ui, binding = 0) uniform restrict uimage3D grid;

uniform vec3 uInvCellSize;
uniform vec3 uGridOrigin;

void main()
{
  uint inParticleId = gl_GlobalInvocationID.x;
  
  // Bounds check
  if (inParticleId >= inParticles.length()) return;
  
  Particle particle = inParticles[inParticleId];
  
  // Calculate voxel coordinate for this particle
  ivec3 voxelCoord = ivec3(uInvCellSize * (particle.position - uGridOrigin));
  
  // Atomically increment count and get the previous value
  // The grid contains: (offset << 8) | 0, and we're incrementing the count
  uint voxelValue = imageAtomicAdd(grid, voxelCoord, 1);
  
  // Extract offset and count
  uint offset = (voxelValue >> 8);
  uint count = (voxelValue & 0xFF);
  
  // Calculate output position: offset + count gives us the slot for this particle
  uint outParticleId = offset + count;
  
  // Write particle to its new sorted position
  if (outParticleId < outParticles.length()) {
    outParticles[outParticleId] = particle;
  }
}