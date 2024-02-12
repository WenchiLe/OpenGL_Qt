#version 330 core

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};

struct Light {
    vec3 pos;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;


uniform Material material;
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
uniform bool blinn;
uniform vec3 viewPos;

void main() {
    vec3 diffuseTexColor=vec3(texture(material.texture_diffuse1,TexCoords));
    vec3 specularTexColor=vec3(texture(material.texture_specular1,TexCoords));

    // ambient
    vec3 ambient = diffuseTexColor*light.ambient;
    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.pos-FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff *diffuseTexColor*light.diffuse;
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular =  spec*specularTexColor*light.specular;

    if(blinn)
     {
         vec3 halfwayDir = normalize(lightDir + viewDir);
         spec = pow(max(dot(norm, halfwayDir), 0.0), 8.0);
     }
     else
     {
         vec3 reflectDir = reflect(-lightDir, norm);
         spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
     }
    specular=vec3(0.8)*spec;
    vec3 result = (ambient + diffuse + specular);
    FragColor = vec4(result, 1.0);
}
