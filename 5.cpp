#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>

//ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ 
bool moveCamera = true;
float cameraRadius = 6.0f;
float cameraAngle = 0.0f;
float vaseX = 0.0f;
float vaseRotation = 0.0f;
float cameraTargetRadius = 6.0f;
float cameraTargetAngle = 0.0f;
float vaseTargetX = 0.0f;
float vaseTargetRotation = 0.0f;
const float CAMERA_ROT_SPEED = 1.5f;
const float CAMERA_ZOOM_SPEED = 2.0f;
const float VASE_MOVE_SPEED = 1.8f;
const float VASE_ROT_SPEED = 45.0f;
bool keys[1024] = { false };

struct SimpleMesh {
    GLfloat* vertices = nullptr;
    GLsizei vertexCount = 0;
};

//ФУНКЦИЯ СОЗДАНИЯ ПРОСТОГО ПРЯМОУГОЛЬНИКА
void createQuad(SimpleMesh& mesh, float width, float height, bool withNormals = true, bool withTexCoords = true) {
    float halfW = width * 0.5f;
    float halfH = height * 0.5f;

    // Вершины прямоугольника: позиция, нормаль, текстурные координаты
    std::vector<GLfloat> verts = {
        -halfW, -halfH, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
         halfW, -halfH, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
         halfW,  halfH, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,

        -halfW, -halfH, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
         halfW,  halfH, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -halfW,  halfH, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f
    };

    if (!withNormals) {
        std::vector<GLfloat> vertsNoNormals;
        for (size_t i = 0; i < verts.size(); i += 8) {
            vertsNoNormals.push_back(verts[i]);
            vertsNoNormals.push_back(verts[i + 1]);
            vertsNoNormals.push_back(verts[i + 2]);
            vertsNoNormals.push_back(0.0f);
            vertsNoNormals.push_back(0.0f);
            vertsNoNormals.push_back(1.0f);
            vertsNoNormals.push_back(verts[i + 6]);
            vertsNoNormals.push_back(verts[i + 7]);
        }
        verts = vertsNoNormals;
    }

    if (!withTexCoords) {
        for (size_t i = 6; i < verts.size(); i += 8) {
            verts[i] = 0.0f;
            verts[i + 1] = 0.0f;
        }
    }

    mesh.vertexCount = (GLsizei)(verts.size() / 8);
    mesh.vertices = new GLfloat[verts.size()];
    std::copy(verts.begin(), verts.end(), mesh.vertices);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_C) moveCamera = !moveCamera;
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraTargetRadius -= (float)yoffset * 0.5f;
    if (cameraTargetRadius < 1.5f) cameraTargetRadius = 1.5f;
    if (cameraTargetRadius > 20.0f) cameraTargetRadius = 20.0f;
}

// ЗАГРУЗКА OBJ 
bool loadOBJ(const char* path, SimpleMesh& outMesh) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path);
    if (!err.empty()) {
        std::cerr << "Error loading OBJ: " << err << std::endl;
        return false;
    }
    if (!ret) return false;

    std::vector<GLfloat> verts;
    for (auto& shape : shapes) {
        for (auto& idx : shape.mesh.indices) {
            if (idx.vertex_index < 0 || idx.vertex_index >= (int)(attrib.vertices.size() / 3)) continue;
            verts.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
            verts.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
            verts.push_back(attrib.vertices[3 * idx.vertex_index + 2]);

            if (idx.normal_index >= 0 && idx.normal_index < (int)(attrib.normals.size() / 3)) {
                verts.push_back(attrib.normals[3 * idx.normal_index + 0]);
                verts.push_back(attrib.normals[3 * idx.normal_index + 1]);
                verts.push_back(attrib.normals[3 * idx.normal_index + 2]);
            }
            else {
                verts.push_back(0.0f); verts.push_back(0.0f); verts.push_back(1.0f);
            }

            if (idx.texcoord_index >= 0 && idx.texcoord_index < (int)(attrib.texcoords.size() / 2)) {
                verts.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                verts.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
            }
            else {
                verts.push_back(0.0f); verts.push_back(0.0f);
            }
        }
    }

    if (verts.empty()) {
        std::cerr << "No vertices found in OBJ" << std::endl;
        return false;
    }

    outMesh.vertexCount = (GLsizei)(verts.size() / 8);
    outMesh.vertices = new GLfloat[verts.size()];
    std::copy(verts.begin(), verts.end(), outMesh.vertices);
    return true;
}

void freeMesh(SimpleMesh& mesh) {
    delete[] mesh.vertices;
    mesh.vertices = nullptr;
    mesh.vertexCount = 0;
}

// ЗАГРУЗКА ТЕКСТУРЫ 
GLuint loadTexture(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

//КОМПИЛЯЦИЯ ШЕЙДЕРА 
GLuint createShader(const char* src, GLenum type) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        std::cerr << "Shader compile error: " << log << std::endl;
        return 0;
    }
    return s;
}

// ПЛАВНОСТЬ 
float smoothLerp(float current, float target, float speed, float dt) {
    float diff = target - current;
    if (std::abs(diff) > 0.001f) {
        return current + diff * std::min(speed * dt, 1.0f);
    }
    return target;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Натюрморт с комнатой", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { glfwTerminate(); return -1; }
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.9f, 0.9f, 0.95f, 1.0f);

    //ЗАГРУЗКА МОДЕЛЕЙ 
    SimpleMesh vase, table, flower, carpet;
    if (!loadOBJ("vase.obj", vase)) { std::cerr << "Failed to load vase.obj\n"; return -1; }
    if (!loadOBJ("table.obj", table)) { std::cerr << "Failed to load table.obj\n"; return -1; }
    if (!loadOBJ("flower.obj", flower)) { std::cerr << "Failed to load flower.obj\n"; return -1; }
    if (!loadOBJ("carpet.obj", carpet)) {
        std::cerr << "Warning: carpet.obj not found, skipping carpet.\n";
    }

    //  СОЗДАНИЕ СТЕН И ПОЛА
    SimpleMesh floorMesh;
    SimpleMesh wallFrontMesh;
    SimpleMesh wallBackMesh;
    SimpleMesh wallLeftMesh;
    SimpleMesh wallRightMesh;

    createQuad(floorMesh, 20.0f, 20.0f);

    createQuad(wallBackMesh, 20.0f, 5.0f);

    createQuad(wallFrontMesh, 20.0f, 5.0f);

    createQuad(wallLeftMesh, 20.0f, 5.0f);

    createQuad(wallRightMesh, 20.0f, 5.0f);

    stbi_set_flip_vertically_on_load(true);

    //ЗАГРУЗКА ТЕКСТУР
    GLuint vaseTexture = loadTexture("vase_diffuse.jpg");
    GLuint tableTexture = loadTexture("wood_texture.jpg");
    GLuint flowerTexture = loadTexture("flower_diffuse.png");
    GLuint carpetTexture = loadTexture("carpet_texture.jpg");
    GLuint floorTexture = loadTexture("floor_texture.jpg");
    if (!floorTexture) floorTexture = tableTexture;
    GLuint wallTexture = loadTexture("wall_texture.jpg");
    if (!wallTexture) wallTexture = tableTexture;

    // VAO/VBO
    int baseModels = 3; 
    int extraModels = (carpet.vertexCount ? 1 : 0);
    int roomModels = 5;
    int totalModels = baseModels + extraModels + roomModels;

    GLuint* VAOs = new GLuint[totalModels];
    GLuint* VBOs = new GLuint[totalModels];
    glGenVertexArrays(totalModels, VAOs);
    glGenBuffers(totalModels, VBOs);

    auto setupVAO = [&](int i, const SimpleMesh& mesh) {
        glBindVertexArray(VAOs[i]);
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[i]);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertexCount * 8 * sizeof(GLfloat), mesh.vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        };

    int idx = 0;
    setupVAO(idx++, vase);
    setupVAO(idx++, table);
    setupVAO(idx++, flower);

    int carpetIdx = -1;
    if (carpet.vertexCount) {
        carpetIdx = idx;
        setupVAO(idx++, carpet);
    }

    // Индексы для комнаты
    int floorIdx = idx++;
    int wallBackIdx = idx++;
    int wallFrontIdx = idx++;
    int wallLeftIdx = idx++;
    int wallRightIdx = idx++;

    setupVAO(floorIdx, floorMesh);
    setupVAO(wallBackIdx, wallBackMesh);
    setupVAO(wallFrontIdx, wallFrontMesh);
    setupVAO(wallLeftIdx, wallLeftMesh);
    setupVAO(wallRightIdx, wallRightMesh);

    // ШЕЙДЕРЫ
    const char* vsSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    const char* fsSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform sampler2D texture_diffuse;
uniform float materialShininess;
uniform float materialSpecularStrength;
uniform float materialEmissive;
void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    // Ambient
    vec3 ambient = 0.1 * texture(texture_diffuse, TexCoord).rgb;
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * texture(texture_diffuse, TexCoord).rgb;
    // Specular (Phong)
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = materialSpecularStrength * spec * lightColor;
    // Emissive
    vec3 emissive = materialEmissive * texture(texture_diffuse, TexCoord).rgb;
    vec3 result = ambient + diffuse + specular + emissive;
    FragColor = vec4(result, 1.0);
}
)";

    GLuint vs = createShader(vsSrc, GL_VERTEX_SHADER);
    GLuint fs = createShader(fsSrc, GL_FRAGMENT_SHADER);
    GLuint shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    glUseProgram(shader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint modelLoc = glGetUniformLocation(shader, "model");
    GLint viewLoc = glGetUniformLocation(shader, "view");
    GLint projLoc = glGetUniformLocation(shader, "projection");
    GLint lightPosLoc = glGetUniformLocation(shader, "lightPos");
    GLint viewPosLoc = glGetUniformLocation(shader, "viewPos");
    GLint lightColorLoc = glGetUniformLocation(shader, "lightColor");
    GLint texLoc = glGetUniformLocation(shader, "texture_diffuse");
    GLint shininessLoc = glGetUniformLocation(shader, "materialShininess");
    GLint specularStrengthLoc = glGetUniformLocation(shader, "materialSpecularStrength");
    GLint emissiveLoc = glGetUniformLocation(shader, "materialEmissive");

    glm::mat4 proj = glm::perspective(glm::radians(35.0f), 1024.0f / 768.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

    //ИСТОЧНИК СВЕТА
    glm::vec3 lightPos(-4.0f, 5.0f, 3.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 0.9f);
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    std::cout << "=== УПРАВЛЕНИЕ ===\n";
    std::cout << "C: change (camera / vase)\n";
    std::cout << "A/D: movement / orbit\n";
    std::cout << "Q/E: move vase\n";
    std::cout << "scroll: scale\n";
    std::cout << "ESC: exit\n";

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = (float)(currentTime - lastTime);
        lastTime = currentTime;
        if (dt > 0.1f) dt = 0.1f;

        // Управление
        if (moveCamera) {
            if (keys[GLFW_KEY_A]) cameraTargetAngle += CAMERA_ROT_SPEED * dt;
            if (keys[GLFW_KEY_D]) cameraTargetAngle -= CAMERA_ROT_SPEED * dt;
        }
        else {
            if (keys[GLFW_KEY_A]) vaseTargetX -= VASE_MOVE_SPEED * dt;
            if (keys[GLFW_KEY_D]) vaseTargetX += VASE_MOVE_SPEED * dt;
            if (keys[GLFW_KEY_Q]) vaseTargetRotation += VASE_ROT_SPEED * dt;
            if (keys[GLFW_KEY_E]) vaseTargetRotation -= VASE_ROT_SPEED * dt;
        }
        vaseTargetX = std::max(-1.2f, std::min(1.2f, vaseTargetX));

        // Плавность
        cameraAngle = smoothLerp(cameraAngle, cameraTargetAngle, CAMERA_ROT_SPEED, dt);
        cameraRadius = smoothLerp(cameraRadius, cameraTargetRadius, CAMERA_ZOOM_SPEED, dt);
        vaseX = smoothLerp(vaseX, vaseTargetX, VASE_MOVE_SPEED, dt);
        vaseRotation = smoothLerp(vaseRotation, vaseTargetRotation, VASE_ROT_SPEED, dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Камера
        glm::vec3 camPos(
            cameraRadius * sin(cameraAngle),
            2.0f,
            cameraRadius * cos(cameraAngle)
        );
        glm::vec3 lookAtPoint(vaseX, 0.6f, 0.0f);
        glm::mat4 view = glm::lookAt(camPos, lookAtPoint, glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(viewPosLoc, 1, &camPos[0]);

        // ПОЛ
        glm::mat4 floorModel = glm::mat4(1.0f);
        floorModel = glm::translate(floorModel, glm::vec3(0.0f, -1.0f, 0.0f));
        floorModel = glm::rotate(floorModel, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        floorModel = glm::scale(floorModel, glm::vec3(1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(floorModel));
        glUniform1f(shininessLoc, 5.0f);
        glUniform1f(specularStrengthLoc, 0.05f);
        glUniform1f(emissiveLoc, 0.0f);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glBindVertexArray(VAOs[floorIdx]);
        glDrawArrays(GL_TRIANGLES, 0, floorMesh.vertexCount);

        // СТЕНЫ
        glm::mat4 wallBackModel = glm::mat4(1.0f);
        wallBackModel = glm::translate(wallBackModel, glm::vec3(0.0f, 1.5f, -10.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallBackModel));
        glUniform1f(shininessLoc, 2.0f);
        glUniform1f(specularStrengthLoc, 0.01f);
        glBindTexture(GL_TEXTURE_2D, wallTexture);
        glBindVertexArray(VAOs[wallBackIdx]);
        glDrawArrays(GL_TRIANGLES, 0, wallBackMesh.vertexCount);

        glm::mat4 wallFrontModel = glm::mat4(1.0f);
        wallFrontModel = glm::translate(wallFrontModel, glm::vec3(0.0f, 1.5f, 10.0f));
        wallFrontModel = glm::rotate(wallFrontModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallFrontModel));
        glBindVertexArray(VAOs[wallFrontIdx]);
        glDrawArrays(GL_TRIANGLES, 0, wallFrontMesh.vertexCount);

        glm::mat4 wallLeftModel = glm::mat4(1.0f);
        wallLeftModel = glm::translate(wallLeftModel, glm::vec3(-10.0f, 1.5f, 0.0f));
        wallLeftModel = glm::rotate(wallLeftModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallLeftModel));
        glBindVertexArray(VAOs[wallLeftIdx]);
        glDrawArrays(GL_TRIANGLES, 0, wallLeftMesh.vertexCount);

        glm::mat4 wallRightModel = glm::mat4(1.0f);
        wallRightModel = glm::translate(wallRightModel, glm::vec3(10.0f, 1.5f, 0.0f));
        wallRightModel = glm::rotate(wallRightModel, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(wallRightModel));
        glBindVertexArray(VAOs[wallRightIdx]);
        glDrawArrays(GL_TRIANGLES, 0, wallRightMesh.vertexCount);

        //КОВЁР
        if (carpet.vertexCount) {
            glm::mat4 carpetModel = glm::mat4(1.0f);
            carpetModel = glm::translate(carpetModel, glm::vec3(0.0f, -0.75f, 0.0f));
            carpetModel = glm::scale(carpetModel, glm::vec3(0.03f, 0.03f, 0.01f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(carpetModel));
            glUniform1f(shininessLoc, 5.0f);
            glUniform1f(specularStrengthLoc, 0.05f);
            glUniform1f(emissiveLoc, 0.0f);
            glBindTexture(GL_TEXTURE_2D, carpetTexture);
            glBindVertexArray(VAOs[carpetIdx]);
            glDrawArrays(GL_TRIANGLES, 0, carpet.vertexCount);
        }

        // СТОЛ (глянцевый лак)
        glm::mat4 tableModel = glm::mat4(1.0f);
        tableModel = glm::translate(tableModel, glm::vec3(0.0f, -0.7f, 0.0f));
        tableModel = glm::scale(tableModel, glm::vec3(0.8f, 0.6f, 0.8f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(tableModel));
        glUniform1f(shininessLoc, 128.0f);
        glUniform1f(specularStrengthLoc, 0.85f);
        glUniform1f(emissiveLoc, 0.0f);
        glBindTexture(GL_TEXTURE_2D, tableTexture);
        glBindVertexArray(VAOs[1]);
        glDrawArrays(GL_TRIANGLES, 0, table.vertexCount);

        //ВАЗА (матовая глина) 
        glm::mat4 vaseModel = glm::mat4(1.0f);
        vaseModel = glm::translate(vaseModel, glm::vec3(vaseX, 0.2f, 0.0f));
        vaseModel = glm::rotate(vaseModel, glm::radians(-90.0f), glm::vec3(0.5f, 0.0f, 0.0f));
        vaseModel = glm::rotate(vaseModel, glm::radians(vaseRotation), glm::vec3(0.0f, 0.0f, 1.0f));
        vaseModel = glm::scale(vaseModel, glm::vec3(0.015f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(vaseModel));
        glUniform1f(shininessLoc, 8.0f);
        glUniform1f(specularStrengthLoc, 0.012f);
        glUniform1f(emissiveLoc, 0.0f);
        glBindTexture(GL_TEXTURE_2D, vaseTexture);
        glBindVertexArray(VAOs[0]);
        glDrawArrays(GL_TRIANGLES, 0, vase.vertexCount);

        // ЦВЕТОК 
        glm::mat4 flowerModel = glm::mat4(1.0f);
        flowerModel = glm::translate(flowerModel, glm::vec3(vaseX, 1.35f, 0.0f));
        float sway = sin(glfwGetTime() * 2.0f) * 3.0f;
        flowerModel = glm::rotate(flowerModel, glm::radians(sway), glm::vec3(0.0f, 1.0f, 0.0f));
        flowerModel = glm::scale(flowerModel, glm::vec3(0.35f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(flowerModel));
        glUniform1f(shininessLoc, 10.0f);
        glUniform1f(specularStrengthLoc, 0.2f);
        glUniform1f(emissiveLoc, 0.0f);
        glBindTexture(GL_TEXTURE_2D, flowerTexture);
        glBindVertexArray(VAOs[2]);
        glDrawArrays(GL_TRIANGLES, 0, flower.vertexCount);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ОЧИСТКА
    freeMesh(vase);
    freeMesh(table);
    freeMesh(flower);
    if (carpet.vertexCount) freeMesh(carpet);
    freeMesh(floorMesh);
    freeMesh(wallFrontMesh);
    freeMesh(wallBackMesh);
    freeMesh(wallLeftMesh);
    freeMesh(wallRightMesh);

    glDeleteVertexArrays(totalModels, VAOs);
    glDeleteBuffers(totalModels, VBOs);
    delete[] VAOs;
    delete[] VBOs;
    glDeleteProgram(shader);

    glDeleteTextures(1, &vaseTexture);
    glDeleteTextures(1, &tableTexture);
    glDeleteTextures(1, &flowerTexture);
    if (carpet.vertexCount) glDeleteTextures(1, &carpetTexture);
    glDeleteTextures(1, &floorTexture);
    glDeleteTextures(1, &wallTexture);

    glfwTerminate();
    return 0;
}