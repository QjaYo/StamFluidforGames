#include "util.h"
#include "global.h"

#include <iostream>

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    static bool key1_was_down = false;
    static bool key2_was_down = false;
    static bool key3_was_down = false;
    static bool key4_was_down = false;
    static bool obstacle_key_was_down = false;
    static bool static_obstacle_key_was_down = false;
    static bool force_key_was_down = false;
    static bool vorticity_key_was_down = false;
    static bool buoyancy_key_was_down = false;
    static bool breeze_key_was_down = false;

    auto pressed_once = [&](int key, bool &was_down)
    {
        bool is_down = glfwGetKey(window, key) == GLFW_PRESS;
        bool pressed = is_down && !was_down;
        was_down = is_down;
        return pressed;
    };

    if (pressed_once(GLFW_KEY_1, key1_was_down))
    {
        boundary_mode = BOUNDARY_SOLID;
        std::cout << "[input] 1: boundary = solid" << std::endl;
    }
    if (pressed_once(GLFW_KEY_2, key2_was_down))
    {
        boundary_mode = BOUNDARY_WRAP;
        std::cout << "[input] 2: boundary = wrap" << std::endl;
    }
    if (pressed_once(GLFW_KEY_3, key3_was_down))
    {
        boundary_mode = BOUNDARY_INFLOW;
        std::cout << "[input] 3: boundary = inflow" << std::endl;
    }
    if (pressed_once(GLFW_KEY_4, key4_was_down))
    {
        chimney_enabled = !chimney_enabled;
        std::cout << "[input] 4: chimney = " << (chimney_enabled ? "on" : "off") << std::endl;
    }

    if (pressed_once(GLFW_KEY_O, obstacle_key_was_down))
    {
        obstacles_enabled = !obstacles_enabled;
        std::cout << "[input] O: obstacles = " << (obstacles_enabled ? "on" : "off") << std::endl;
    }
    if (pressed_once(GLFW_KEY_C, static_obstacle_key_was_down))
    {
        static_obstacle_enabled = !static_obstacle_enabled;
        std::cout << "[input] C: center obstacle = " << (static_obstacle_enabled ? "on" : "off") << std::endl;
    }
    if (pressed_once(GLFW_KEY_F, force_key_was_down))
    {
        mouse_force_enabled = !mouse_force_enabled;
        std::cout << "[input] F: mouse force = " << (mouse_force_enabled ? "on" : "off") << std::endl;
    }
    if (pressed_once(GLFW_KEY_V, vorticity_key_was_down))
    {
        vorticity_enabled = !vorticity_enabled;
        std::cout << "[input] V: vorticity confinement = " << (vorticity_enabled ? "on" : "off") << std::endl;
    }
    if (pressed_once(GLFW_KEY_B, buoyancy_key_was_down))
    {
        buoyancy_enabled = !buoyancy_enabled;
        std::cout << "[input] B: buoyancy = " << (buoyancy_enabled ? "on" : "off") << std::endl;
    }
    if (pressed_once(GLFW_KEY_W, breeze_key_was_down))
    {
        breeze_enabled = !breeze_enabled;
        std::cout << "[input] W: breeze = " << (breeze_enabled ? "on" : "off") << std::endl;
    }
}

std::string currentPath()
{
    std::string path = std::filesystem::current_path().string();
    return path;
}

void updateDensityBuffer(float* densBuffer, const float* dens, int N)
{
    N = SCR_SIZE;

    for (int j = 0; j < N; j++)
    {
        for (int i = 0; i < N; i++)
        {
            int cell = IX(i + 1, j + 1);
            if (obstacles_enabled && solid[cell])
            {
                densBuffer[j * N + i] = 0.18f;
                continue;
            }

            float d = dens[cell];
            if (d < 0.0f) d = 0.0f;
            float white_density = render_white_density > 0.0f ? render_white_density : 1.0f;
            d /= white_density;
            if (d > 1.0f) d = 1.0f;
            densBuffer[j * N + i] = d;
        }
    }
}
