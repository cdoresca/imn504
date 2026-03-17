#version 460

layout(local_size_x = 64) in;

layout(std430, binding = 0) writeonly buffer CellIds{ 
    uint count[]; 
};
layout(std430, binding = 1) buffer Start{
    uint start[];
};

uniform uint CELL_COUNT;

void main() {

   
}