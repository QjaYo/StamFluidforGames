#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <filesystem>
#include "global.h"

void processInput(GLFWwindow *window);
std::string currentPath();
void updateVertices(float* vertices, float* dens, int N);
