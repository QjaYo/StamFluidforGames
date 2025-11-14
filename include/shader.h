#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

class Shader
{
public:
    unsigned int program = 0;
    
    Shader(const char *vertexPath, const char *fragmentPath);
    ~Shader();
    
    void use();
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &anem, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, float value1, float value2, float value3) const;
    void setVec4(const std::string &name, float value1, float value2, float value3, float value4) const;
    
private:
    void checkShaderErrors(unsigned int ID, std::string type);
};
