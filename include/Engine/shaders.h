#pragma once
#include "glad/glad.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Engine/types.h>

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

// saves a file.
void saveFile(char *data, char *path) {
    FILE* fptr;

    // opens file for binary writing
    fptr = fopen(path, "wb");
    fwrite(data, strlen(data), 1, fptr);
    fclose(fptr);
    return;
}

char* getShaderContent(const char* fileName) {
    char* shaderContent = readFile(fileName);

    // finds header files, and on //include line adds said code
    
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
    int sizeArray[MAX_INCLUDES] = {0};
    char* headerContents[MAX_INCLUDES] = {}; // max of 10 includes again
    // iterates over includes and loads their file paths before adding it to string
    for (int i = 0; i < indexes; i++) {
        char* start = shaderContent+indexArray[i]+10; // gets start of filepath
        char* end = strstr(start, "\n"); // gets end of filepath
        if (!end) end = shaderContent + strlen(shaderContent); // if no newline before end of file. 
        int len = end-start+1;
        // adds end insert part after new line
        insertArray[i]=end-shaderContent+1;

        // gets file path window string
        char window[len]; // +1 for null terminator
        snprintf(window, sizeof(window), "%s", start); // strncpy was not working
        //printf("%s:\n",window);

        // gets file
        headerContents[i] = readFile(window);
        // saves size of file to offset next index. Offset because first goes in first place
        sizeArray[i] = strlen(headerContents[i]);
        //printf("%s\n", headerContents[i]);
    }

    // SHOULD CHECK FOR DUPLICATE HEADER FILES

    //creates resultant content by inserting
    size_t headerSize = 0;
    for (int i=0; i< indexes; i++) headerSize+=strlen(headerContents[i]);
    
    // creates space for combined content
    char* resultContent = malloc(strlen(shaderContent)+headerSize+1);
    char* resultContentOld = malloc(strlen(shaderContent)+headerSize+1); // flip flop so memory doesnt break
    //printf("%lld", headerSize);
    snprintf(resultContent, strlen(shaderContent)+1, "%s", shaderContent);
    //printf("%s",resultContent);

    // adds content below
    int sizeSum = 0; // offset based on sizes of old inserts
    for (int i = 0; i < indexes; i++) {
        //printf("%s",headerContents[i]);
        insertString(resultContent, headerContents[i], insertArray[i]+sizeSum, resultContentOld);
        snprintf(resultContent, strlen(resultContentOld)+1, "%s", resultContentOld);

        // increment size sum
        sizeSum+=sizeArray[i];
    }

    // free memory
    for (int i = 0; i < indexes; i++) free(headerContents[i]);
    free(shaderContent);
    free(resultContentOld);

    return resultContent;  
}


void shaderCompile(GLuint* shaderId, GLenum shaderType, const char* shaderFilePath) {
    GLint isCompiled = 0;
    // loads shader content
    char* shaderSource = getShaderContent(shaderFilePath); 
    //printf("%s",shaderSource);

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
    if(isCompiled == GL_FALSE) {

        char infoLog[1024];
        glGetShaderInfoLog(*shaderId, sizeof(infoLog), NULL, infoLog);

        printf("Shader Compiler Error: %s\n%s\n", shaderFilePath, infoLog);
        glDeleteShader(*shaderId);
        *shaderId = 0;   // important
        return;
    }
}

GLuint linkShaders(GLuint vertexShader, GLuint fragmentShader) {
    if (vertexShader == 0 || fragmentShader == 0) {
        printf("Not linking because shader compilation failed.\n\n");
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
        printf("Shader program link failed:\n%s\n", infoLog);
    }

    return program;
}

GLuint linkComputeShader(GLuint computeShader) {
    if (computeShader == 0) {
        printf("Not linking because shader compilation failed.\n\n");
        return 0;
    }

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
    //glUseProgram(program);
    return program;
}

void shaderSetFloat(GLuint ID, const char* name, float value) {
    glUseProgram(ID);
    glUniform1f(glGetUniformLocation(ID, name), value);
}

void shaderSetUint(GLuint ID, const char* name, unsigned int value) {
    glUseProgram(ID);
    glUniform1ui(glGetUniformLocation(ID, name), value);
}

void shaderSetInt(GLuint ID, const char* name, int value) {
    glUseProgram(ID);
    glUniform1i(glGetUniformLocation(ID, name), value);
}

void shaderSetVec3(GLuint ID, const char* name, vec3 value) {
    glUseProgram(ID);
    glUniform3f(glGetUniformLocation(ID, name), value.x, value.y, value.z);
}

// initializes buffer at ID
void createSSBO(GLuint ID, size_t size, int index) {
  glGenBuffers(1, &ID);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);
  glBufferData(GL_SHADER_STORAGE_BUFFER, size, NULL, GL_DYNAMIC_DRAW); // could use glBufferStorage, data initializes all 0s
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, ID);
}

void* createAndPersistentlyMapSSBO(GLuint ID, size_t size, int index) {
    // maps chunk buffer
    glGenBuffers(1, &ID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ID);

    // define required flags for immutable storage
    GLbitfield storageFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

    // allocate memory
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, NULL, storageFlags);

    // bind buffer to index
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, ID);

    // retrieve cpu pointer
    GLbitfield mapFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    return glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, mapFlags);
}

// creates gpu fence sync object
void createFenceGPU() {
    GLsync gpuSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    GLenum waitStatus = glClientWaitSync(gpuSync, GL_SYNC_FLUSH_COMMANDS_BIT, 5000000000); // 5 second timeout
    if (waitStatus == GL_ALREADY_SIGNALED || waitStatus == GL_CONDITION_SATISFIED) {
        return;
    } else {
        printf("GPU sync error.");
    }
}