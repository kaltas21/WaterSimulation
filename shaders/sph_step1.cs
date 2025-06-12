#version 460 core
// SPH Step 1: Position integration and grid population

layout(local_size_x = 32) in;

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

layout(r32ui, binding = 0) uniform restrict uimage3D grid;

uniform float uDT;
uniform vec3 uGravity;
uniform vec3 uGridOrigin;
uniform vec3 uGridSize;
uniform vec3 uInvCellSize;

// Sphere collision uniforms
uniform vec3 uSpherePosition;
uniform vec3 uSphereImpulse;
uniform float uSphereRadius;
uniform int uSphereActive;

const float SAFE_BOUNDS = 0.5;

void main()
{
  uint particleId = gl_GlobalInvocationID.x;
  
  // Bounds check
  if (particleId >= particles.length()) return;
  
  Particle particle = particles[particleId];
  
  // Apply gravity
  vec3 newVelo = particle.velocity + uGravity * uDT;
  
  // Apply sphere impulse if active
  if (uSphereActive != 0) {
    vec3 toSphere = uSpherePosition - particle.position;
    float distToSphere = length(toSphere);
    
    // Check if particle is within sphere interaction radius
    if (distToSphere <= uSphereRadius && distToSphere > 0.001) {
      vec3 sphereDir = toSphere / distToSphere;
      
      // Apply impulse based on distance (closer = stronger)
      float impulseStrength = 1.0 - (distToSphere / uSphereRadius);
      impulseStrength = impulseStrength * impulseStrength; // Quadratic falloff
      
      newVelo += uSphereImpulse * impulseStrength * uDT;
    }
  }
  
  // Integrate position
  vec3 newPos = particle.position + newVelo * uDT;
  
  // Boundary handling with damping
  float wallDamping = 0.5;
  vec3 boundsL = uGridOrigin + SAFE_BOUNDS;
  vec3 boundsH = uGridOrigin + uGridSize - SAFE_BOUNDS;
  
  if (newPos.x < boundsL.x) { newVelo.x *= -wallDamping; newPos.x = boundsL.x; }
  if (newPos.x > boundsH.x) { newVelo.x *= -wallDamping; newPos.x = boundsH.x; }
  if (newPos.y < boundsL.y) { newVelo.y *= -wallDamping; newPos.y = boundsL.y; }
  if (newPos.y > boundsH.y) { newVelo.y *= -wallDamping; newPos.y = boundsH.y; }
  if (newPos.z < boundsL.z) { newVelo.z *= -wallDamping; newPos.z = boundsL.z; }
  if (newPos.z > boundsH.z) { newVelo.z *= -wallDamping; newPos.z = boundsH.z; }
  
  // Update particle
  particle.velocity = newVelo;
  particle.position = newPos;
  particles[particleId] = particle;
  
  // Add to spatial grid - count particles per cell
  ivec3 voxelCoord = ivec3(uInvCellSize * (newPos - uGridOrigin));
  
  // Make sure particle is within grid bounds
  if (all(greaterThanEqual(voxelCoord, ivec3(0))) && all(lessThan(voxelCoord, imageSize(grid)))) {
    imageAtomicAdd(grid, voxelCoord, 1);
  }
}