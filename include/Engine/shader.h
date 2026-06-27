#include "glad/glad.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* getShaderContent(const char* fileName) {
    FILE *fp;
    long size = 0;
    char* shaderContent;

    // file size or something idrk
    fp = fopen(fileName, "rb");
    if(fp == NULL) {
        return "";
    }
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp) + 1;
    fclose(fp);

    // reads file content
    fp = fopen(fileName, "r");
    shaderContent = (char*)malloc(size);
    memset(shaderContent, '\0', size);
    fread(shaderContent, 1, size - 1, fp);
    fclose(fp);
    return shaderContent;
}

void shaderCompile(GLuint* shaderId, GLenum shaderType, const char* shaderFilePath)
{
    GLint isCompiled = 0;
    // loads shader content
    const char* shaderSource = getShaderContent(shaderFilePath); 

    // creates shader
    *shaderId = glCreateShader(shaderType);
    if(*shaderId == 0) {
        printf("COULD NOT LOAD SHADER: %s!\n", shaderFilePath);
    }

    // sets shader content
    glShaderSource(*shaderId, 1, (const char**)&shaderSource, NULL);
    glCompileShader(*shaderId);
    glGetShaderiv(*shaderId, GL_COMPILE_STATUS, &isCompiled);

    // error handling
    if(isCompiled == GL_FALSE) { // give better messages eventually
        printf("Shader Compiler Error: %s\n", shaderFilePath);
        glDeleteShader(*shaderId);
        return;
    }
}

GLuint linkShaders(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    // error handling lowkey probably don't work so just don't do things wrong.
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, 1024, NULL, infoLog);

        printf("Shader compilation failed:\n%s\n", infoLog);
        //glDeleteShader(program);
    }
    //glUseProgram(program);
    return program;
}

GLuint linkComputeShader(GLuint computeShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        printf("Compute program link failed:\n%s\n", infoLog);
    }

    return program;
}