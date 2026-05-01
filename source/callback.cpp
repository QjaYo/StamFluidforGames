#include "callback.h"
#include "simulation.h"
#include <cmath>
#include <algorithm>

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

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (mouse_down)
    {
        // 직전 → 현재 사이를 일정 간격으로 쪼개 잉크 추가 (점선 → 연속선)
        double dx = xpos - mouse_x;
        double dy = ypos - mouse_y;
        double dist = std::sqrt(dx*dx + dy*dy);
        int steps = std::max(1, (int)(dist / 3.0));   // 3픽셀당 1회

        // 마우스 이동 방향대로 속도장 주입 (advect가 작동하도록)
        // 화면 픽셀 → 격자 셀 단위로 환산. y는 화면 좌표(아래 +)와 격자 좌표(위 +)가 반대.
        const double VEL_SCALE = 0.05;
        float vu = (float)( dx * VEL_SCALE);
        float vv = (float)(-dy * VEL_SCALE);

        double prev_x = mouse_x, prev_y = mouse_y;
        for (int s = 1; s <= steps; s++)
        {
            double t = (double)s / steps;
            mouse_x = prev_x + t * dx;
            mouse_y = prev_y + t * dy;
            add_source(SCR_SIZE, dens, nullptr, dt);

            // 같은 위치에 속도 주입
            int mx = (int)((mouse_x / SCR_WIDTH) * SCR_SIZE + 1);
            int my = (int)(((SCR_HEIGHT - mouse_y) / SCR_HEIGHT) * SCR_SIZE + 1);
            if (mx >= 1 && mx <= SCR_SIZE && my >= 1 && my <= SCR_SIZE)
            {
                u[IX(mx, my)] += vu;
                v[IX(mx, my)] += vv;
            }
        }
    }

    mouse_x = xpos;
    mouse_y = ypos;
}
