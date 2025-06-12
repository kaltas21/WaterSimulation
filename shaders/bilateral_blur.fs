#version 460 core

in vec2 texCoord;

out float smoothedDepth;

uniform sampler2D depthTexture;
uniform vec2 direction; // (1,0) for horizontal, (0,1) for vertical
uniform float sigmaSpace;
uniform float sigmaDepth;
uniform vec2 texelSize;

const int KERNEL_RADIUS = 15;

void main() {
    float centerDepth = texture(depthTexture, texCoord).r;
    
    if (centerDepth == 0.0) {
        smoothedDepth = 0.0;
        return;
    }
    
    float weightSum = 0.0;
    float depthSum = 0.0;
    
    // Bilateral filter
    for (int i = -KERNEL_RADIUS; i <= KERNEL_RADIUS; i++) {
        vec2 sampleCoord = texCoord + float(i) * direction * texelSize;
        float sampleDepth = texture(depthTexture, sampleCoord).r;
        
        if (sampleDepth == 0.0) continue;
        
        // Spatial weight (Gaussian)
        float spatialWeight = exp(-float(i * i) / (2.0 * sigmaSpace * sigmaSpace));
        
        // Depth weight (range kernel)
        float depthDiff = abs(sampleDepth - centerDepth);
        float depthWeight = exp(-depthDiff * depthDiff / (2.0 * sigmaDepth * sigmaDepth));
        
        float weight = spatialWeight * depthWeight;
        weightSum += weight;
        depthSum += sampleDepth * weight;
    }
    
    smoothedDepth = (weightSum > 0.0) ? (depthSum / weightSum) : centerDepth;
}