#include "util.h"
#include "global.h"

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

std::string currentPath()
{
    std::string path = std::filesystem::current_path().string();
    return path;
}

void updateVertices(float* vertices, float* dens, int N)
{
    int idx = 0;
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            float x = 2.0f * i / (N - 1) - 1.0f;
            float y = 2.0f * j / (N - 1) - 1.0f;

            float d = dens[IX(i,j)];
            if (d < 0.0f) d = 0.0f;
            if (d > 1.0f) d = 1.0f;

            vertices[idx++] = x;
            vertices[idx++] = y;
            vertices[idx++] = 0.0f;

            vertices[idx++] = d;
            vertices[idx++] = d;
            vertices[idx++] = d;
        }
    }
}
