#include "glad/glad.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void insertString(const char *orig, const char *to_insert, int pos, char *result) {
    // copy original string up to insertion point
    strncpy(result, orig, pos);
    result[pos] = '\0'; //null terminate;
    
    // add new substring
    strcat(result, to_insert);
    
    // add remainder of initial string
    strcat(result, orig + pos);
}

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

    // finds header files, and replaces //include line with said code
    
    // finds includes, and stores their locations
    char* target = "//include "; // length of 10
    int indexArray[10]; // 10 max header files
    int indexes = 0;
    // str[i] condition implicitly checks if str[i] != '\0'
    // would use strstr() but want multiple indexes, and strtok would give word instead of character index, hence manually.
    for (int i = 0; shaderContent[i]; i++) {
        char* window = shaderContent+i;
        if (strncmp(target, window, 10)==0) {
            indexArray[indexes]=i;
            indexes++;
        }
    }
    // early out
    if (indexes == 0) return shaderContent;

    // iterates over includes and loads their file paths before adding it to string
    for (int i = 0; i < indexes; i++) {
        char* start = shaderContent+indexArray[i]+10; // gets start of filepath
        char* end = strstr(start, "\n"); // gets end of filepath

        // gets file path window string
        char window[end-start];
        strncpy(window, start, end-start);
        printf("%s\n", window);
    }
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