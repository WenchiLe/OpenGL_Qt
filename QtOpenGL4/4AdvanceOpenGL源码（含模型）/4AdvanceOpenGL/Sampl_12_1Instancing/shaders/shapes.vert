#version 330 core 
layout (location = 0) in vec3 aPos; 
layout (location = 1) in vec3 aNormal;
uniform vec2 offsets[100];

out vec3 Color;
void main() { 
    Color=aNormal;
    vec2 offset = offsets[gl_InstanceID];
    gl_Position = vec4(aPos, 1.0)+vec4(offset,0.0,0.0);
} 
