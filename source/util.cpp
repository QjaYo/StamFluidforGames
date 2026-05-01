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

void updateDensityBuffer(float* densBuffer, const float* dens, int N)
{
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            float d = dens[IX(i + 1, j + 1)];   // boundary 건너뛰고 inner 시작 (1,1)부터
            if (d < 0.0f) d = 0.0f;
            if (d > 1.0f) d = 1.0f;
            densBuffer[j * N + i] = d;
        }
    }
}
