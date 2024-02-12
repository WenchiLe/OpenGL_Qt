#version 330 core

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};
uniform Material material;

struct Light {
    vec3 pos;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;


out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform vec3 viewPos;

void main() {
    vec3 diffuseTexColor=vec3(texture(material.texture_diffuse1,TexCoords));

    // ambient
    vec3 ambient = diffuseTexColor*light.ambient;
    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.pos-FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff *diffuseTexColor*light.diffuse;

    vec3 result = (ambient + diffuse );
    FragColor = vec4(result, 1.0);
}
