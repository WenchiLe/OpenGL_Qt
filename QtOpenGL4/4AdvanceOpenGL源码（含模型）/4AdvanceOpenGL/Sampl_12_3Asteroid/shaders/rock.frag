#version 330 core

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};

uniform Material material;
out vec4 FragColor;
in vec2 TexCoords;


uniform vec3 viewPos;

void main() {
    vec3 diffuseTexColor=vec3(texture(material.texture_diffuse1,TexCoords));
    FragColor = vec4(diffuseTexColor, 1.0);
}
