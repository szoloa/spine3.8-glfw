#include "spine/SkeletonBinary.h"
#include "spine/VertexAttachment.h"
#include <spine/spine.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "stb_image.h"

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
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint shader;
} Renderer;

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

    const char* vertexShaderSource =
        "#version 330 core\n"
        "\n"
        "layout(location = 0) in vec2 aPos;\n"
        "layout(location = 1) in vec2 aUV;\n"
        "layout(location = 2) in vec4 aColor;\n"
        "\n"
        "out vec2 vUV;\n"
        "out vec4 vColor;\n"
        "\n"
        "void main() {\n"
        "\n"
        "    float x = (aPos.x / 1600.0) - 0;\n"
        "    float y = 0.0 - (aPos.y / 1600.0);\n"
        "\n"
        "    gl_Position = vec4(x, y, 0.0, 1.0);\n"
        "\n"
        "    vUV = aUV;\n"
        "    vColor = aColor;\n"
        "}\n";

    const char* fragmentShaderSource =
        "#version 330 core\n"
        "\n"
        "in vec2 vUV;\n"
        "in vec4 vColor;\n"
        "\n"
        "uniform sampler2D tex;\n"
        "\n"
        "out vec4 FragColor;\n"
        "\n"
        "void main() {\n"
        "    FragColor = texture(tex, vUV) * vColor;\n"
        "}\n";

    GLuint vertexShader =
        compile_shader(
            GL_VERTEX_SHADER,
            vertexShaderSource
        );

    GLuint fragmentShader =
        compile_shader(
            GL_FRAGMENT_SHADER,
            fragmentShaderSource
        );

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    GLint success;

    glGetProgramiv(program,
                   GL_LINK_STATUS,
                   &success);

    if (!success) {
        char log[512];

        glGetProgramInfoLog(program,
                            512,
                            NULL,
                            log);

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

        if (!slot->attachment)
            continue;

        float r = skeleton->color.r *
                  slot->color.r;
        float g = skeleton->color.g *
                  slot->color.g;
        float b = skeleton->color.b *
                  slot->color.b;
        float a = skeleton->color.a *
                  slot->color.a;


        if (slot->attachment->type == SP_ATTACHMENT_REGION) {

            spRegionAttachment* region =
                (spRegionAttachment*)slot->attachment;

            float worldVertices[8];

            spRegionAttachment_computeWorldVertices(
                region,
                slot->bone,
                worldVertices,
                0,
                2
            );

            float* uvs = region->uvs;

            Vertex vertices[4];

            for (int j = 0; j < 4; j++) {

                vertices[j].x =
                    worldVertices[j * 2];

                vertices[j].y =
                    worldVertices[j * 2 + 1];

                vertices[j].u =
                    uvs[j * 2];

                vertices[j].v =
                    uvs[j * 2 + 1];

                vertices[j].r = r;
                vertices[j].g = g;
                vertices[j].b = b;
                vertices[j].a = a;
            }

            unsigned short indices[6] = {
                0, 1, 2,
                2, 3, 0
            };

            spAtlasRegion* atlasRegion =
                (spAtlasRegion*)region->rendererObject;

            GLuint texture =
                (GLuint)(uintptr_t)
                atlasRegion->page->rendererObject;

            glBindTexture(GL_TEXTURE_2D, texture);

            glBufferData(
                GL_ARRAY_BUFFER,
                sizeof(vertices),
                vertices,
                GL_DYNAMIC_DRAW
            );

            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                sizeof(indices),
                indices,
                GL_DYNAMIC_DRAW
            );

            glDrawElements(
                GL_TRIANGLES,
                6,
                GL_UNSIGNED_SHORT,
                0
            );
        }

        else if (slot->attachment->type == SP_ATTACHMENT_MESH) {

            spMeshAttachment* mesh =
                (spMeshAttachment*)slot->attachment;

            int worldVerticesLength =
                mesh->super.worldVerticesLength;

            float worldVertices[8192];

            spVertexAttachment_computeWorldVertices(
                (spVertexAttachment*)mesh,
                slot,
                0,
                worldVerticesLength,
                worldVertices,
                0,
                2
            );

            int numVertices =
                worldVerticesLength / 2;

            Vertex vertices[8192];

            for (int j = 0; j < numVertices; j++) {

                vertices[j].x =
                    worldVertices[j * 2];

                vertices[j].y =
                    worldVertices[j * 2 + 1];

                vertices[j].u =
                    mesh->uvs[j * 2];

                vertices[j].v =
                    mesh->uvs[j * 2 + 1];

                vertices[j].r = r;
                vertices[j].g = g;
                vertices[j].b = b;
                vertices[j].a = a;
            }

            unsigned short indices[8192];

            for (int j = 0;
                 j < mesh->trianglesCount;
                 j++) {

                indices[j] =
                    mesh->triangles[j];
            }

            spAtlasRegion* atlasRegion =
                (spAtlasRegion*)mesh->rendererObject;

            GLuint texture =
                (GLuint)(uintptr_t)
                atlasRegion->page->rendererObject;

            glBindTexture(GL_TEXTURE_2D, texture);

            glBufferData(
                GL_ARRAY_BUFFER,
                sizeof(Vertex) * numVertices,
                vertices,
                GL_DYNAMIC_DRAW
            );

            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                sizeof(unsigned short) *
                mesh->trianglesCount,
                indices,
                GL_DYNAMIC_DRAW
            );

            glDrawElements(
                GL_TRIANGLES,
                mesh->trianglesCount,
                GL_UNSIGNED_SHORT,
                0
            );
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

int create_window(GLFWwindow* window, Renderer* renderer, spSkeleton* skeleton) {
    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);
    glGenBuffers(1, &renderer->ebo);

    renderer->shader =
        create_shader_program();

    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER,
                 renderer->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                 renderer->ebo);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)0
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)(sizeof(float) * 2)
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        (void*)(sizeof(float) * 4)
    );

    glEnableVertexAttribArray(2);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,
                GL_ONE_MINUS_SRC_ALPHA);
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        spine_render(renderer, skeleton);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}



int main(){
    Renderer renderer;

    GLFWwindow* window;
    if (!glfwInit()) {
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,
                   GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(
        800,
        600,
        "Spine",
        NULL,
        NULL
    );

    if (!window) {
        printf("Failed to create window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    // VERY IMPORTANT
    if (!gladLoadGLLoader(
            (GLADloadproc)glfwGetProcAddress)) {

        printf("Failed to initialize GLAD\n");
        return 1;
    }

    spAtlas* atlas =
        spAtlas_createFromFile("data/aris_spr.atlas", 0);
    if (!atlas) {
        printf("Atlas load failed\n");
        return 1;
    }
    spAtlasPage* page = atlas->pages;
    while (page) {
        printf("Atlas page: %s\n", page->name);

        page = page->next;
    }
    spSkeletonBinary* binary =
        spSkeletonBinary_create(atlas);
    spSkeletonData* skeletonData =
        spSkeletonBinary_readSkeletonDataFile(
            binary,
            "data/aris_spr.skel"
        );
    if (!skeletonData) {
        printf("Spine load failed: %s\n", binary->error ? binary->error : "unknown");
        return 0;
    }
    printf("Bones: %d\n",
           skeletonData->bonesCount);

    printf("Slots: %d\n",
           skeletonData->slotsCount);

    printf("Animations: %d\n",
           skeletonData->animationsCount);

    printf("Skins: %d\n",
           skeletonData->skinsCount);
    spSkeleton* skeleton =
        spSkeleton_create(skeletonData);
    spAnimationStateData* stateData =
        spAnimationStateData_create(skeletonData);

    spAnimationState* state =
        spAnimationState_create(stateData);
    for (int i = 0;
         i < skeletonData->animationsCount;
         i++) {

        spAnimation* anim =
            skeletonData->animations[i];

        printf(
            "Animation[%d]: %s\n",
            i,
            anim->name
        );
    }
    spAnimationState_setAnimationByName(
        state,
        0,
        "00",
        1
    );
    float delta = 1.0f / 60.0f;

    spAnimationState_update(state, delta);
    spAnimationState_apply(state, skeleton);

    spSkeleton_updateWorldTransform(skeleton);
    for (int i = 0;
         i < skeleton->slotsCount;
         i++) {

        spSlot* slot =
            skeleton->slots[i];

        printf(
            "Slot[%d]: %s\n",
            i,
            slot->data->name
        );

        if (slot->attachment) {

            printf(
                "  Attachment: %s\n",
                slot->attachment->name
            );

            printf(
                "  Type: %d\n",
                slot->attachment->type
            );
        }
        else {
            printf("  No attachment\n");
        }
    }
    for (int i = 0;
         i < skeleton->bonesCount;
         i++) {

        spBone* bone =
            skeleton->bones[i];

        printf(
            "%s world: %f %f\n",
            bone->data->name,
            bone->worldX,
            bone->worldY
        );
    }

    create_window(window, &renderer, skeleton);
    return 0;
}
