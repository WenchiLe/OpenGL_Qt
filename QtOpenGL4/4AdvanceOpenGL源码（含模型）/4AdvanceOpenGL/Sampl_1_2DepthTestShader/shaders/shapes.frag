#version 330 core

out vec4 FragColor;
float near = 0.1;
float far = 100.0;
float LinearizeDepth(float depth) {
    float z_ndc = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z_ndc  * (far - near));
}

void main() {
    //FragColor = vec4(vec3(gl_FragCoord.z), 1.0);
    float depth = (LinearizeDepth(gl_FragCoord.z)-near) / (far-near); // divide by far for demonstration
    FragColor = vec4(vec3(depth), 1.0);

}
