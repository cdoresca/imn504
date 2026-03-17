#version 460

layout(local_size_x = 64) in;

layout(std430,binding = 0) readonly buffer rPosition{
	vec4 rPos[];
};
layout(std430, binding = 1) buffer Start{
    uint start[];
};
layout(std430, binding = 2) buffer ParticuleId{ 
	uint pid[];    
};

uniform uint PARTICULENUMBER;

const float rayon = 0.3;
const float  sizeCell = 2 * rayon; 
const ivec3 gridDim = ivec3(34);

uint cellIndex(vec3 pos) {
    ivec3 c = ivec3(floor((pos + 10) / sizeCell));
    c = clamp(c, ivec3(0), gridDim - 1);
   
    return uint((c.x * gridDim.y + c.y) * gridDim.z + c.z);
}

void main() {

    uint id = gl_GlobalInvocationID.x;
    if (id >= PARTICULENUMBER) return;

    vec3 pos = rPos[id].xyz;
    uint cell = cellIndex(pos);
    uint slot = atomicAdd(start[cell], 1);  
    pid[slot] = id;
}