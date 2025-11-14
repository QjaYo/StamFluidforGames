#include "shader.h"

Shader::Shader(const char *vertexPath, const char *fragmentPath)
{
    //glEnable(GL_PROGRAM_POINT_SIZE);
    
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        
        vShaderFile.close();
        fShaderFile.close();
        
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure &e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        std::cout << "ERROR::!!!!!!!" << e.what() << std::endl;
    }
    
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    
    unsigned int vShader, fShader;
    
    vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vShaderCode, NULL);
    glCompileShader(vShader);
    checkShaderErrors(vShader, "VERTEX_SHADER");
    
    fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fShaderCode, NULL);
    glCompileShader(fShader);
    checkShaderErrors(fShader, "FRAGMENT_SHADER");
    
    program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);
    checkShaderErrors(program, "PROGRAM");
    
    glDeleteShader(vShader);
    glDeleteShader(fShader);
};

Shader::~Shader()
{
    if (program != 0)
    {
        glDeleteProgram(program);
        program = 0;
    }
}

void Shader::use()
{
    glUseProgram(program);
}

void Shader::setBool(const std::string &name, bool value) const
{
    int location = glGetUniformLocation(program, name.c_str());
    glUniform1i(location, (int) value);
}

void Shader::setInt(const std::string &name, int value) const
{
    int location = glGetUniformLocation(program, name.c_str());
    glUniform1i(location, value);
}

void Shader::setFloat(const std::string &name, float value) const
{
    int location = glGetUniformLocation(program, name.c_str());
    glUniform1f(location, value);
}

void Shader::setVec3(const std::string &name, float value1, float value2, float value3) const
{
    int location = glGetUniformLocation(program, name.c_str());
    glUniform3f(location, value1, value2, value3);
}

void Shader::setVec4(const std::string &name, float value1, float value2, float value3, float value4) const
{
    int location = glGetUniformLocation(program, name.c_str());
    glUniform4f(location, value1, value2, value3, value4);
}


void Shader::checkShaderErrors(unsigned int ID, std::string type)
{
    int success;
    char infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(ID, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(ID, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else
    {
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(ID, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}
