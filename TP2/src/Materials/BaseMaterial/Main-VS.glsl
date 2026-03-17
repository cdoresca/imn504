#version 460

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Proj;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

layout(location = 0) in vec3 Position;

void main() {

/******** TP 2 - A completer *******************
Au minimum : Remplir la variable gl_Position   
    
  ****************************************/


}