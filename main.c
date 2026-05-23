#include "spine/Skeleton.h"
#include "spine/SkeletonBinary.h"
#include "spine/VertexAttachment.h"
#include <spine/spine.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
// #include <winscard.h>
#include "stb_image.h"
typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint shader;
} Renderer;

typedef struct {
    float x;
    float y;
    float u;
    float v;
    float r;
    float g;
    float b;
    float a;
} Vertex;

typedef struct {
    spSkeleton* skeleton;
    spAnimationStateData* stateData;
    spAnimationState* state;
} SpineObject;

const char* animations[100];
int animationCount = 0;
SpineObject spine;
void load_spine(char* spine_name) {
    char path_atlas[20], path_skel[20];
    sprintf(path_atlas, "data/%s/%s_spr.atlas", spine_name, spine_name);
    sprintf(path_skel, "data/%s/%s_spr.skel", spine_name, spine_name);
    spAtlas* atlas = spAtlas_createFromFile(path_atlas, 0);

    spAtlasPage* page = atlas->pages;

    spSkeletonBinary* binary = spSkeletonBinary_create(atlas);
    spSkeletonData* skeletonData = spSkeletonBinary_readSkeletonDataFile(binary, path_skel);
    spine.skeleton =
        spSkeleton_create(skeletonData);
    spine.stateData =
        spAnimationStateData_create(skeletonData);
    spine.state = spAnimationState_create(spine.stateData);

    for (int i = 0;
         i < skeletonData->animationsCount;
         i++) {
             animations[i] = skeletonData->animations[i]->name;
             animationCount++;
    }
    spAnimationState_setAnimationByName(spine.state, 0, "Idle_01", 1);
}

void update_animation(float delta) {
    spAnimationState_update(spine.state, delta);
    spAnimationState_apply(spine.state, spine.skeleton);
    spSkeleton_updateWorldTransform(spine.skeleton);
}

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", path);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    char* buffer = malloc(size + 1);
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        printf("Shader compile error:\n%s\n", log);
    }
    return shader;
}
GLuint create_shader_program() {
    char* vertexShaderSource = read_file("shaders/spine.vert");
    char* fragmentShaderSource = read_file("shaders/spine.frag");
    GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vertexShaderSource );
    GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fragmentShaderSource );
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        printf("Shader link error:\n%s\n", log);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}
void spine_render(Renderer *renderer, spSkeleton* skeleton) {
    glUseProgram(renderer->shader);
    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ebo);
    Vertex vertices[8192];
    unsigned short indices[16384];
    int vertexCount = 0;
    int indexCount = 0;
    for (int i = 0; i < skeleton->slotsCount; i++) {
        spSlot* slot = skeleton->drawOrder[i];
        if (!slot->attachment) continue;
        float r = skeleton->color.r * slot->color.r;
        float g = skeleton->color.g * slot->color.g;
        float b = skeleton->color.b * slot->color.b;
        float a = skeleton->color.a * slot->color.a;
        if (slot->attachment->type == SP_ATTACHMENT_REGION) {
            spRegionAttachment* region =
                (spRegionAttachment*)slot->attachment;
            float worldVertices[8];
            spRegionAttachment_computeWorldVertices(region, slot->bone, worldVertices, 0, 2);
            float* uvs = region->uvs;
            Vertex vertices[4];
            for (int j = 0; j < 4; j++) {
                vertices[j].x = worldVertices[j * 2];
                vertices[j].y = worldVertices[j * 2 + 1];
                vertices[j].u = uvs[j * 2];
                vertices[j].v = uvs[j * 2 + 1];
                vertices[j].r = r;
                vertices[j].g = g;
                vertices[j].b = b;
                vertices[j].a = a;
            }
            unsigned short indices[6] = {
                0, 1, 2,
                2, 3, 0
            };
            spAtlasRegion* atlasRegion = (spAtlasRegion*)region->rendererObject;
            GLuint texture = (GLuint)(uintptr_t) atlasRegion->page->rendererObject;
            glBindTexture(GL_TEXTURE_2D, texture);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
        } else if (slot->attachment->type == SP_ATTACHMENT_MESH) {
            spMeshAttachment* mesh = (spMeshAttachment*)slot->attachment;
            int worldVerticesLength = mesh->super.worldVerticesLength;
            float worldVertices[8192];
            spVertexAttachment_computeWorldVertices((spVertexAttachment*)mesh, slot, 0, worldVerticesLength, worldVertices, 0, 2);
            int numVertices = worldVerticesLength / 2;
            Vertex vertices[8192];
            for (int j = 0; j < numVertices; j++) {
                vertices[j].x = worldVertices[j * 2];
                vertices[j].y = worldVertices[j * 2 + 1];
                vertices[j].u = mesh->uvs[j * 2];
                vertices[j].v = mesh->uvs[j * 2 + 1];
                vertices[j].r = r;
                vertices[j].g = g;
                vertices[j].b = b;
                vertices[j].a = a;
            }
            unsigned short indices[8192];
            for (int j = 0; j < mesh->trianglesCount; j++) {
                indices[j] = mesh->triangles[j];
            }
            spAtlasRegion* atlasRegion = (spAtlasRegion*)mesh->rendererObject;
            GLuint texture = (GLuint)(uintptr_t) atlasRegion->page->rendererObject;
            glBindTexture(GL_TEXTURE_2D, texture);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, vertices, GL_DYNAMIC_DRAW);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * mesh->trianglesCount, indices, GL_DYNAMIC_DRAW);
            glDrawElements(GL_TRIANGLES, mesh->trianglesCount, GL_UNSIGNED_SHORT, 0);
        }
    }

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(Vertex) * vertexCount,
        vertices,
        GL_DYNAMIC_DRAW
    );

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(unsigned short) * indexCount,
        indices,
        GL_DYNAMIC_DRAW
    );

    glDrawElements(
        GL_TRIANGLES,
        indexCount,
        GL_UNSIGNED_SHORT,
        0
    );
}

int dragging = 0;

float scale = 1.0f;
double lastMouseX = 0;
double lastMouseY = 0;
int currentAnimation = 0;

int create_window(GLFWwindow* window, Renderer* renderer ,char* spine_name, char* animation) {
    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);
    glGenBuffers(1, &renderer->ebo);
    renderer->shader = create_shader_program();
    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->ebo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 4));
    glEnableVertexAttribArray(2);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,
                GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(renderer->shader);
    glUniform2f(
        glGetUniformLocation(renderer->shader, "screenSize"),
        2000.0,
        1500.0
    );
    float lastTime = glfwGetTime(), now, delta;
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        now = glfwGetTime();
        delta = now - lastTime;
        lastTime = now;
        if (dragging) {
            double mouseX;
            double mouseY;
            glfwGetCursorPos(
                window,
                &mouseX,
                &mouseY
            );
            double dx = mouseX - lastMouseX;
            double dy = mouseY - lastMouseY;
            spine.skeleton->x += dx;
            spine.skeleton->y -= dy;
            lastMouseX = mouseX;
            lastMouseY = mouseY;
        }
        spine.skeleton->scaleX = scale;
        spine.skeleton->scaleY = scale;
        update_animation(delta);
        spine_render(renderer, spine.skeleton);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

void scroll_callback(
    GLFWwindow* window,
    double xoffset,
    double yoffset
) {
    scale += yoffset * 0.1f;
    if (scale < 0.1f)
        scale = 0.1f;
}

void mouse_button_callback(
    GLFWwindow* window,
    int button,
    int action,
    int mods
) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            dragging = 1;
            glfwGetCursorPos(
                window,
                &lastMouseX,
                &lastMouseY
            );
        }
        else if (action == GLFW_RELEASE) {
            dragging = 0;
        }
    }
}

void key_callback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods
) {
    if (action != GLFW_PRESS)
        return;
    if (key == GLFW_KEY_RIGHT) {
        currentAnimation++;
        if (currentAnimation >= animationCount)
            currentAnimation = 0;
        spAnimationState_setAnimationByName(
                spine.state,
                1,
                animations[
                    currentAnimation
                ],
                1
        );
    }
    if (key == GLFW_KEY_LEFT) {
        currentAnimation--;
        if (currentAnimation < 0)
            currentAnimation =
                animationCount - 1;
        spAnimationState_setAnimationByName(
                        spine.state,
                        1,
                        animations[
                            currentAnimation
                        ],
                        1
                );
    }
    printf("%s\n", animations[
                currentAnimation
            ]);
}

int main(int argc, char *argv[]) {
    GLFWwindow* window;
    Renderer renderer;
    char* character = argv[1], *animation = argv[2];

    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,
                    GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(
         GLFW_TRANSPARENT_FRAMEBUFFER,
         GLFW_TRUE
     );
    window = glfwCreateWindow(2000, 1500, "Spine", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(
        window,
        mouse_button_callback
    );
    glfwSetScrollCallback(
        window,
        scroll_callback
    );
    glfwSetKeyCallback(
        window,
        key_callback
    );
    // VERY IMPORTANT
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return 1;
    }
    load_spine(character);
    spAnimationState_setAnimationByName(
        spine.state,
        1,
        animations[
            currentAnimation
        ],
        1
    );
    printf("%s\n", animations[
                currentAnimation
            ]);
    create_window(window, &renderer, character, animation);
    return 0;
}
