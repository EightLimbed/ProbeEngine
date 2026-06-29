#include "glad/glad.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_INCLUDES 10

void insertString(const char *orig, const char *to_insert, int pos, char* result) {
    // copy original string up to insertion point
    strncpy(result, orig, pos);
    result[pos] = '\0'; //null terminate;

    // add new substring
    strcat(result, to_insert);
    
    // add remainder of initial string
    strcat(result, orig + pos);
}

char* readFile(const char* fileName) {
    FILE *fp;
    long size = 0;
    char* fileContent;

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
    fileContent = (char*)malloc(size);
    memset(fileContent, '\0', size);
    fread(fileContent, 1, size - 1, fp);
    fclose(fp);
    return fileContent;
}

char* getShaderContent(const char* fileName) {
    char* shaderContent = readFile(fileName);
    //printf("%s",shaderContent);

    // finds header files, and replaces //include line with said code
    
    // finds includes, and stores their locations
    char* target = "//include "; // length of 10
    int indexArray[MAX_INCLUDES]; // 10 max header files
    int indexes = 0;
    // str[i] condition implicitly checks if str[i] != '\0'
    // would use strstr() but want multiple indexes, and strtok would give word instead of character index, hence manually.
    for (int i = 0; shaderContent[i]; i++) {
        char* window = shaderContent+i;
        if (strncmp(target, window, 10)==0) {
            indexArray[indexes]=i;
            indexes++;
            // error handling
            if (indexes > MAX_INCLUDES) {
                printf("Too many includes.");
                return shaderContent;
            }
        }
    }
    // early out
    if (indexes == 0) return shaderContent;

    int insertArray[MAX_INCLUDES];
    char* headerContents[MAX_INCLUDES] = {}; // max of 10 includes again
    // iterates over includes and loads their file paths before adding it to string
    for (int i = 0; i < indexes; i++) {
        char* start = shaderContent+indexArray[i]+10; // gets start of filepath
        char* end = strstr(start, "\n"); // gets end of filepath
        int len = end-start+1;
        // adds end insert part after new line
        insertArray[i]=end-shaderContent+1;

        // gets file path window string
        char window[len]; // +1 for null terminator
        snprintf(window, sizeof(window), "%s", start); // strncpy was not working
        //printf("%s:\n",window);

        // gets file
        headerContents[i] = readFile(window);
        //printf("%s\n", headerContents[i]);
    }

    // SHOULD CHECK FOR DUPLICATE HEADER FILES

    //creates resultant content by inserting
    size_t headerSize = 0;
    for (int i; i< indexes; i++) headerSize+=strlen(headerContents[i]);
    
    // creates space for combined content
    char* resultContent = malloc(strlen(shaderContent)+headerSize+1);
    char* resultContentOld = malloc(strlen(shaderContent)+headerSize+1); // flip flop so memory doesnt break
    //printf("%lld", headerSize);
    snprintf(resultContent, strlen(shaderContent)+1, "%s", shaderContent);
    //printf("%s",resultContent);

    // adds content below

    for (int i = 0; i < indexes; i++) {
        //printf("%s",headerContents[i]);
        insertString(resultContent, headerContents[i], insertArray[i], resultContentOld);
        snprintf(resultContent, strlen(resultContentOld)+1, "%s", resultContentOld);
    }
    //printf("%s",resultContent);
    return resultContent;
    
}

void shaderCompile(GLuint* shaderId, GLenum shaderType, const char* shaderFilePath)
{
    GLint isCompiled = 0;
    // loads shader content
    char* shaderSource = getShaderContent(shaderFilePath); 
    printf("%s",shaderSource);

    // creates shader
    *shaderId = glCreateShader(shaderType);
    if(*shaderId == 0) {
        printf("COULD NOT LOAD SHADER: %s!\n", shaderFilePath);
    }

    // sets shader content
    glShaderSource(*shaderId, 1, (const char**)&shaderSource, NULL);
    glCompileShader(*shaderId);
    glGetShaderiv(*shaderId, GL_COMPILE_STATUS, &isCompiled);

    free(shaderSource); // free memory allocated in previous function.

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