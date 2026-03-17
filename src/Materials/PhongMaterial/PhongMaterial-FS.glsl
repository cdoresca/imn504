#version 460



uniform vec4 Phong;
uniform vec3 diffuseAlbedo;
uniform vec3 specularColor;

in VTF {
vec3 vL;
vec3 vV;
vec3 vN;
vec3 v_Color;
};


layout (location = 0) out vec4 Color;


void main()
{


	vec3 L = normalize(vL);
	vec3 V = normalize(vV);
	vec3 N = normalize(vN);

	if(!gl_FrontFacing )
		L = -L;

	float Id = max(0,dot(L,N));
	float Is = pow(max(0,dot(reflect(-L,N),V)),Phong.w);


	Color.xyz = Phong.x*diffuseAlbedo + Phong.y*diffuseAlbedo*Id + Phong.z*Is*specularColor;

	


	//Color.xyz = v_Color;
	Color.w = 1.0;

	
}