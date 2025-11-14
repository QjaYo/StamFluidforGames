#include "callback.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS)  mouse_down = true;
            if (action == GLFW_RELEASE) mouse_down = false;
        }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    mouse_x = xpos;
    mouse_y = ypos;
}
