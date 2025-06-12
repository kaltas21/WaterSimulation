#version 460 core
// SPH Step 2: Grid offset calculation

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(binding = 0, std430) buffer counters
{
  uint globalParticleCount;
};

layout(r32ui, binding = 0) uniform restrict uimage3D grid;

shared uint localParticleCount;
shared uint globalParticleBaseOffset;

void main()
{
  ivec3 voxelId = ivec3(gl_GlobalInvocationID);
  
  if (gl_LocalInvocationIndex == 0)
  {
    localParticleCount = 0;
  }
  
  barrier();
  
  uint voxelParticleCount = imageLoad(grid, voxelId).x;
  
  uint localParticleOffset = atomicAdd(localParticleCount, voxelParticleCount);
  
  barrier();
  
  if (gl_LocalInvocationIndex == 0)
  {
    globalParticleBaseOffset = atomicAdd(globalParticleCount, localParticleCount);
  }
  
  barrier();
  
  uint globalParticleOffset = globalParticleBaseOffset + localParticleOffset;
  
  imageStore(grid, voxelId, uvec4(globalParticleOffset << 8));
}