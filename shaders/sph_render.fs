#version 460 core
// SPH Billboard Fragment Shader

in vec3 vColor;
in vec3 vCenterPos;
in vec2 vUV;

out vec4 finalColor;

uniform mat4 uProjection;
uniform float uPointRadius;

#define SPHERE
#define DEPTH_REPLACEMENT

void main(void)
{
    finalColor = vec4(vColor, 1.0);

#ifdef SPHERE
    vec3 N;

    N.xy = vUV * 2.0 - 1.0;

    float r2 = dot(N.xy, N.xy);

    if (r2 > 1.0)
    {
        discard;
    }

#ifdef DEPTH_REPLACEMENT
    N.z = sqrt(1.0 - r2);

    vec4 pos_eye_space = vec4(vCenterPos + N * uPointRadius, 1.0);
    vec4 pos_clip_space = uProjection * pos_eye_space;
    float depth_ndc = pos_clip_space.z / pos_clip_space.w;
    float depth_winspace = depth_ndc * 0.5 + 0.5;
    gl_FragDepth = depth_winspace;
#endif
#endif
}