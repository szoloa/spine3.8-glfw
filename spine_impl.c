#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <spine/Atlas.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <glad/glad.h>

char* _spUtil_readFile(const char* path, int* length) {
    FILE* file = fopen(path, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    int size = (int)ftell(file);
    rewind(file);

    char* data = (char*)malloc(size);
    fread(data, 1, size, file);
    fclose(file);

    *length = size;
    return data;
}

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path) {
    int width, height, channels;

    unsigned char* pixels =
        stbi_load(path, &width, &height, &channels, 4);
    if (!pixels) {
        printf("Failed to load image: %s\n", path);
        return;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );
    printf("Loading texture: %s\n", path);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    self->rendererObject = (void*)(uintptr_t)tex;

    self->width = width;
    self->height = height;

    stbi_image_free(pixels);
}

void _spAtlasPage_disposeTexture(spAtlasPage* self) {
    GLuint tex = (GLuint)(uintptr_t)self->rendererObject;

    glDeleteTextures(1, &tex);
}
