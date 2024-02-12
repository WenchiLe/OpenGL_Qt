#ifndef VERTICES_H
#define VERTICES_H

float planeVertices[] = {
    // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
    25.0f, 0.0f,  25.0f,  1.0f, 0.0f,
    -25.0f, 0.0f,  25.0f,  0.0f, 0.0f,
    -25.0f, 0.0f, -25.0f,  0.0f, 1.0f,

    25.0f, 0.0f,  25.0f,  1.0f, 0.0f,
    -25.0f, 0.0f, -25.0f,  0.0f, 1.0f,
    25.0f, 0.0f, -25.0f,  1.0f, 1.0f
};

float points[] = {
    -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, // top-left
    0.5f, 0.5f, 0.0f, 1.0f, 0.0f, // top-right
    0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // bottom-right
    -0.5f, -0.5f, 1.0f, 1.0f, 0.0f // bottom-left
};

float quadVertices[] = {
    // positions  // colors
    -0.05f, 0.05f, 1.0f, 0.0f, 0.0f,
    0.05f, -0.05f, 0.0f, 1.0f, 0.0f,
    -0.05f, -0.05f, 0.0f, 0.0f, 1.0f,
    -0.05f, 0.05f, 1.0f, 0.0f, 0.0f,
    0.05f, -0.05f, 0.0f, 1.0f, 0.0f,
    0.05f, 0.05f, 0.0f, 1.0f, 1.0f
};

#endif // VERTICES_H
