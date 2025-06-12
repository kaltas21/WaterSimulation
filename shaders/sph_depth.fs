#version 460 core

// Fragment shader for rendering particles as spheres with proper depth

in vec3 vCenterPos;  // Eye space center position
in vec2 vUV;
in vec3 vColor;

out vec4 fragColor;

uniform mat4 uProjection;
uniform float uPointRadius;

void main() {
    // Calculate sphere normal from UV coordinates
    vec3 N;
    N.xy = vUV * 2.0 - 1.0;
    
    float r2 = dot(N.xy, N.xy);
    
    // Discard fragments outside the sphere
    if (r2 > 1.0) {
        discard;
    }
    
    // Complete the sphere normal (pointing towards camera)
    N.z = sqrt(1.0 - r2);
    
    // Calculate actual position on sphere surface in eye space
    vec3 spherePos = vCenterPos + N * uPointRadius;
    
    // Project to get proper depth
    vec4 clipPos = uProjection * vec4(spherePos, 1.0);
    float ndcDepth = clipPos.z / clipPos.w;
    
    // Convert NDC depth to window coordinates [0, 1]
    gl_FragDepth = ndcDepth * 0.5 + 0.5;
    
    // Output depth as color for visualization and debug
    // In production, this would output to depth buffer only
    float depthVis = gl_FragDepth;
    fragColor = vec4(depthVis, depthVis, depthVis, 1.0);
}