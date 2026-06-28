#define activator
#ifdef activator

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "callback.h"
#include "shader.h"
#include "util.h"
#include "global.h"
#include "simulation.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static bool is_house_background_pixel(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned char max_c = std::max({r, g, b});
    unsigned char min_c = std::min({r, g, b});
    return max_c > 215 && max_c - min_c < 42;
}

static unsigned int load_house_texture(const std::filesystem::path &path, int &width, int &height, int &contentBottom)
{
    int channels = 0;
    unsigned char *pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (!pixels)
    {
        std::cout << "Failed to load house texture: " << path << std::endl;
        return 0;
    }

    std::vector<unsigned char> rgba(pixels, pixels + (size_t)width * (size_t)height * 4);
    stbi_image_free(pixels);

    std::vector<unsigned char> transparent((size_t)width * (size_t)height, 0);
    std::vector<int> stack;

    auto try_push = [&](int x, int y)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return;
        int idx = y * width + x;
        if (transparent[idx])
            return;
        int off = idx * 4;
        if (!is_house_background_pixel(rgba[off], rgba[off + 1], rgba[off + 2]))
            return;
        transparent[idx] = 1;
        stack.push_back(idx);
    };

    for (int x = 0; x < width; x++)
    {
        try_push(x, 0);
        try_push(x, height - 1);
    }
    for (int y = 0; y < height; y++)
    {
        try_push(0, y);
        try_push(width - 1, y);
    }

    while (!stack.empty())
    {
        int idx = stack.back();
        stack.pop_back();
        int x = idx % width;
        int y = idx / width;
        try_push(x + 1, y);
        try_push(x - 1, y);
        try_push(x, y + 1);
        try_push(x, y - 1);
    }

    contentBottom = height;
    int maxOpaqueY = -1;
    for (int i = 0; i < width * height; i++)
    {
        if (transparent[i])
        {
            rgba[(size_t)i * 4 + 3] = 0;
        }
        else
        {
            maxOpaqueY = std::max(maxOpaqueY, i / width);
        }
    }
    if (maxOpaqueY >= 0)
    {
        int visualBottomY = maxOpaqueY;
        int minOpaqueForVisualBottom = std::max(20, width / 6);
        for (int y = height - 1; y >= 0; y--)
        {
            int opaqueCount = 0;
            for (int x = 0; x < width; x++)
            {
                if (!transparent[y * width + x])
                    opaqueCount++;
            }
            if (opaqueCount >= minOpaqueForVisualBottom)
            {
                visualBottomY = y;
                break;
            }
        }
        contentBottom = visualBottomY + 1;
    }

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    return texture;
}

static unsigned int create_quad(const float *vertices, unsigned int &vbo, unsigned int &ebo)
{
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    unsigned int vao = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    return vao;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "StamFluidforGames", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    std::filesystem::path root_path(PROJECT_SOURCE_DIR);
    std::string vert_path = (root_path / "shader" / "vshader.glsl").string();
    std::string frag_path = (root_path / "shader" / "fshader.glsl").string();
    Shader ourShader(vert_path.c_str(), frag_path.c_str());

    std::vector<float> densBuffer(SCR_SIZE * SCR_SIZE);

    float smokeVertices[] = {
        -1.0f, -1.0f,    0.0f, 0.0f,
         1.0f, -1.0f,    1.0f, 0.0f,
         1.0f,  1.0f,    1.0f, 1.0f,
        -1.0f,  1.0f,    0.0f, 1.0f,
    };

    int houseWidth = 0;
    int houseHeight = 0;
    int houseContentBottom = 0;
    unsigned int houseTex = load_house_texture(root_path / "asset" / "house.png", houseWidth, houseHeight, houseContentBottom);
    house_bottom_padding_ratio = houseHeight > 0 ? (float)(houseHeight - houseContentBottom) / (float)houseHeight : 0.0f;
    float imageAspect = houseHeight > 0 ? (float)houseWidth / (float)houseHeight : 1.0f;
    float houseNdcHeight = 2.0f * house_scene_height;
    float houseNdcWidth = houseNdcHeight * imageAspect * ((float)SCR_HEIGHT / (float)SCR_WIDTH);
    float houseLeft = -0.5f * houseNdcWidth;
    float houseRight = 0.5f * houseNdcWidth;
    float houseBottom = -1.0f - houseNdcHeight * house_bottom_padding_ratio;
    float houseTop = houseBottom + houseNdcHeight;

    float houseVertices[] = {
        houseLeft,  houseBottom,    0.0f, 1.0f,
        houseRight, houseBottom,    1.0f, 1.0f,
        houseRight, houseTop,       1.0f, 0.0f,
        houseLeft,  houseTop,       0.0f, 0.0f,
    };

    unsigned int smokeVBO = 0, smokeEBO = 0;
    unsigned int smokeVAO = create_quad(smokeVertices, smokeVBO, smokeEBO);
    unsigned int houseVBO = 0, houseEBO = 0;
    unsigned int houseVAO = create_quad(houseVertices, houseVBO, houseEBO);

    unsigned int densTex;
    glGenTextures(1, &densTex);
    glBindTexture(GL_TEXTURE_2D, densTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, SCR_SIZE, SCR_SIZE, 0,
        GL_RED, GL_FLOAT, nullptr);

    ourShader.use();
    ourShader.setInt("uDensTex", 0);
    ourShader.setInt("uHouseTex", 1);
    ourShader.setFloat("uViewportAspect", (float)SCR_WIDTH / (float)SCR_HEIGHT);

    init();

    double fps_last_time = glfwGetTime();
    int fps_frames = 0;

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        update_obstacles(SCR_SIZE);
        // my_dens_step();
        get_from_UI(SCR_SIZE, dens_prev, u_prev, v_prev);
        vel_step(SCR_SIZE, u, v, u_prev, v_prev, visc, dt);
        dens_step(SCR_SIZE, dens, dens_prev, u, v, diff, dt);
        temp_step(SCR_SIZE, temp, temp_prev, u, v, temperature_diff, dt);

        updateDensityBuffer(densBuffer.data(), dens, SCR_SIZE);

        glBindTexture(GL_TEXTURE_2D, densTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_SIZE, SCR_SIZE,
            GL_RED, GL_FLOAT, densBuffer.data());

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ourShader.use();
        ourShader.setInt("uRenderMode", 2);
        glBindVertexArray(smokeVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        if (house_scene_enabled && houseTex != 0)
        {
            ourShader.setInt("uRenderMode", 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, houseTex);
            glBindVertexArray(houseVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        ourShader.setInt("uRenderMode", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densTex);
        glBindVertexArray(smokeVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        fps_frames++;
        double fps_now = glfwGetTime();
        double fps_elapsed = fps_now - fps_last_time;
        if (fps_elapsed >= 1.0)
        {
            std::cout << "[fps] " << (double)fps_frames / fps_elapsed
                << " | sim " << SCR_SIZE << "x" << SCR_SIZE
                << " | window " << SCR_WIDTH << "x" << SCR_HEIGHT
                << " | build " << CMAKE_BUILD_TYPE << std::endl;
            fps_frames = 0;
            fps_last_time = fps_now;
        }

        t += dt;
    }

    glDeleteVertexArrays(1, &smokeVAO);
    glDeleteBuffers(1, &smokeVBO);
    glDeleteBuffers(1, &smokeEBO);
    glDeleteVertexArrays(1, &houseVAO);
    glDeleteBuffers(1, &houseVBO);
    glDeleteBuffers(1, &houseEBO);
    glDeleteTextures(1, &densTex);
    if (houseTex != 0)
        glDeleteTextures(1, &houseTex);

    glfwTerminate();
    return 0;
}
#endif
