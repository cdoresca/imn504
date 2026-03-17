#version 460

layout(local_size_x = 64) in;

layout(std430,binding = 0) readonly buffer rPosition{
	vec4 rPos[];
};
layout(std430,binding = 1) writeonly buffer wPosition{
	vec4 wPos[];
};
layout(std430,binding = 2) readonly buffer rVelocity{
	vec4 rSpeed[];
};
layout(std430,binding = 3) writeonly buffer wVelocity{
	vec4 wSpeed[];
};
layout(std140,binding = 4) uniform physic{
   float deltaTime;
   float mass;
   float gravity;
};
layout(std430, binding = 5) readonly  buffer Count{
	uint count[]; 
};
layout(std430, binding = 6) buffer Start{
	uint start[];  
};
layout(std430, binding = 7) buffer ParticuleId{ 
	uint pid[];    
};


uniform vec3 gravityDir;
uniform uint PARTICULENUMBER;

struct Box {
	vec3 min;
	vec3 max;
};


const float rayon = 0.3;
const float  sizeCell = 2 * rayon; 
const ivec3 gridDim = ivec3(34);


void collideBox(Box b,inout vec3 pos, inout vec3 velocity){
	
	vec3 closesthit = clamp(pos,b.min,b.max);
	float longueur = length(pos - closesthit);
	if (longueur < rayon) return;

	vec3 n = normalize(pos - closesthit);
	
	pos += (rayon - longueur) * n;
	if (dot(velocity, n) < 0.0) return;

	float alpha = 1;
	float beta = 0.6;

	vec3 v_perp = dot(velocity,n) * n;
	vec3 v_parallel = velocity - v_perp;
	velocity = alpha * v_parallel - beta * v_perp;
}

void collideSphere(inout vec3 pos, inout vec3 velocity, uint other){
	float alpha = 1.0;
	float beta = 0.6;
	float gamma = 0.1;
	
	
	vec3 posOther  = rPos[other].xyz;
	vec3 speedOther = rSpeed[other].xyz;

	float distance = length(pos - posOther);

	if(distance > rayon) return;

	vec3 n = normalize(pos - posOther);

	vec3  rel_vel = speedOther - velocity;
    

    float overlap =  rayon - distance;
	pos += n * overlap * 0.5;

	if(dot(rel_vel, n) < 0.0) return;
	
	velocity = alpha * velocity + beta *  dot(rel_vel, n) * n;
		
	
	
	
}


void collideGrid(inout vec3 pos,inout vec3 velocity){
	uint id = gl_GlobalInvocationID.x;
	ivec3 c = clamp(ivec3(floor((pos + 10.0) / sizeCell)), ivec3(0), gridDim - 1);
	for (int dz = -1; dz <= 1; dz++){
		for (int dy = -1; dy <= 1; dy++){
			for (int dx = -1; dx <= 1; dx++) {

				ivec3 voisin_cell = c + ivec3(dx,dy,dz);
		
				if (any(lessThan(voisin_cell, ivec3(0))) ||
				any(greaterThanEqual(voisin_cell, gridDim))) continue;

				uint vcell = uint((voisin_cell.x * gridDim.y + voisin_cell.y) * gridDim.z + voisin_cell.z);
				uint begin = start[vcell]; 
				uint end   = start[vcell] + count[vcell];

				for(uint i = begin; i < end; i++)
				{
					uint j = pid[i];
					if (j == id) continue;		
					collideSphere(pos,velocity,j);
				}
			}
		}
	}
}



void main(){

	uint id = gl_GlobalInvocationID.x;
	if (id >= PARTICULENUMBER) return;

	vec3 velocity = rSpeed[id].xyz;
	vec3 pos = rPos[id].xyz;

	Box box;
	box.min = vec3(-10,-10,-10);
	box.max = vec3(10,10,10);

	collideGrid(pos,velocity);

	velocity += gravity * gravityDir * deltaTime;
	pos += velocity * deltaTime;

	collideBox(box,pos,velocity);

	wPos[id] = vec4(pos, 1.0f);
	wSpeed[id] = vec4(velocity, 0.0);
}