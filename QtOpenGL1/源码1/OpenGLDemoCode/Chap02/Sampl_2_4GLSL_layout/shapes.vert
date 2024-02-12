#version 330 core
in vec3 aPos;
out vec4 vertexColor;
void main(){
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);
    vertexColor=vec4(0.8, 0.0, 0.0, 1.0);
}
