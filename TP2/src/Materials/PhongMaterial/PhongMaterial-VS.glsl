#version 460



uniform mat4 Model;
uniform mat4 ViewProj;
uniform vec3 PosCam;
uniform vec3 PosLum;
uniform float Time;

 out gl_PerVertex {
        vec4 gl_Position;
        float gl_PointSize;
        float gl_ClipDistance[];
    };
layout (location = 0) in vec3 Position;
layout (location = 2) in vec3 Normal;



out VTF {
vec3 vL;
vec3 vV;
vec3 vN;
vec3 v_Color;
};



void main()
{
    vec3 Pos = Position;


	gl_Position = ViewProj * Model * vec4(Pos,1.0);

 	
 	vN =normalize(Normal);

    vL = (PosLum-Pos);
    vV = (PosCam-Pos);

  

}
