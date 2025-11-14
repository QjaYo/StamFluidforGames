#define activator
#ifdef activator

#include <iostream>
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
    
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "StamFluidforGames", NULL, NULL);
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
    
    //initialize shader
    Shader ourShader("shader/vshader.glsl", "shader/fshader.glsl");
    
    //set up buffers
    std::vector<float> vertices(SCR_SIZE*SCR_SIZE * (3*2));
    
    //generate buffers
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    //bind vertex array
    glBindVertexArray(VAO);
    
    //upload vertex data to GPU
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW); //(copy data to vertex buffer) & (upload data to GPU via VBO)
    
    //link vertex attributes (VBO ID is also stored in VAO by glVertexAttribPointer)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    //unbind vertex array for saftey
    glBindVertexArray(0);
    
    init();
    
    //renderloop
    while(!glfwWindowShouldClose(window))
    {
        processInput(window);
        
        
        my_dens_step();
        //dens_step(SCR_SIZE, dens, dens_prev, u, v, diff, dt);
        
        updateVertices(vertices.data(), dens, SCR_SIZE);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ourShader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, SCR_SIZE*SCR_SIZE);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    
    glfwTerminate();
    return 0;
}
#endif
