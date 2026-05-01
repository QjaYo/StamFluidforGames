#define activator
#ifdef activator

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "callback.h"
#include "shader.h"
#include "util.h"
#include "global.h"
#include "simulation.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //if (apple user)

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

    //register callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    //initialize shader (실행 위치 무관하게 동작하도록 절대 경로 사용)
    std::filesystem::path root_path(PROJECT_SOURCE_DIR);
    std::string vert_path = (root_path / "shader" / "vshader.glsl").string();
    std::string frag_path = (root_path / "shader" / "fshader.glsl").string();
    Shader ourShader(vert_path.c_str(), frag_path.c_str());

    // 매 프레임 채워서 GPU 텍스처로 업로드할 inner cells 밀도 버퍼 (CPU 측 스테이징)
    std::vector<float> densBuffer(SCR_SIZE * SCR_SIZE);

    // ─── 화면 전체를 덮는 quad (정점 4개 + 인덱스 6개) ───
    float quadVertices[] = {
        // aPos (NDC)    // aTexCoord
        -1.0f, -1.0f,    0.0f, 0.0f,   // 0: 좌하
         1.0f, -1.0f,    1.0f, 0.0f,   // 1: 우하
         1.0f,  1.0f,    1.0f, 1.0f,   // 2: 우상
        -1.0f,  1.0f,    0.0f, 1.0f,   // 3: 좌상
    };
    unsigned int quadIndices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    // location=0 → aPos (vec2), location=1 → aTexCoord (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

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

    init();

    //renderloop
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        update_velocity_field();

        my_dens_step();
        //dens_step(SCR_SIZE, dens, dens_prev, u, v, diff, dt);

        updateDensityBuffer(densBuffer.data(), dens, SCR_SIZE);

        // 시뮬 결과를 텍스처에 업로드
        glBindTexture(GL_TEXTURE_2D, densTex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCR_SIZE, SCR_SIZE,
            GL_RED, GL_FLOAT, densBuffer.data());

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ourShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densTex);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        t += dt;
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &densTex);

    glfwTerminate();
    return 0;
}
#endif
