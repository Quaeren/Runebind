#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
//#include "CaveGeneration.hpp"
#include "CaveGenerationV2.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_set>

using namespace std;

// Window dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// World Generation
struct VoxelFace {
    glm::vec3 corners[4];
    glm::vec3 color;
    glm::vec3 normal;
};

VoxelFace faces[] = {
        // Front (Z)
        {
            {
                {-0.5f, -0.5f,  0.5f},
                { 0.5f, -0.5f,  0.5f},
                { 0.5f,  0.5f,  0.5f},
                {-0.5f,  0.5f,  0.5f}
            },
            {1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}
        },
        // Back (-Z)
        {
            {
                { 0.5f, -0.5f, -0.5f},
                {-0.5f, -0.5f, -0.5f},
                {-0.5f,  0.5f, -0.5f},
                { 0.5f,  0.5f, -0.5f}
            },
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f,-1.0f}
        },
        // Right (X)
        {
            {
                { 0.5f, -0.5f,  0.5f},
                { 0.5f, -0.5f, -0.5f},
                { 0.5f,  0.5f, -0.5f},
                { 0.5f,  0.5f,  0.5f}
            },
            {0.0f, 0.0f, 1.0f},
            {1.0f, 0.0f, 0.0f}
        },
        // Left (-X)
        {
            {
                {-0.5f, -0.5f, -0.5f},
                {-0.5f, -0.5f,  0.5f},
                {-0.5f,  0.5f,  0.5f},
                {-0.5f,  0.5f, -0.5f}
            },
            {1.0f, 0.0f, 1.0f},
            {-1.0f, 0.0f, 0.0f}
        },
        // Top (Y)
        {
            {
                {-0.5f,  0.5f,  0.5f},
                { 0.5f,  0.5f,  0.5f},
                { 0.5f,  0.5f, -0.5f},
                {-0.5f,  0.5f, -0.5f}
            },
            {1.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f}
        },
        // Bottom (-Y)
        {
            {
                {-0.5f, -0.5f, -0.5f},
                { 0.5f, -0.5f, -0.5f},
                { 0.5f, -0.5f,  0.5f},
                {-0.5f, -0.5f,  0.5f}
            },
            {0.0f, 1.0f, 1.0f},
            {0.0f,-1.0f, 0.0f}
        }
    };

struct IVec3Hash {
    size_t operator()(const glm::ivec3& v) const noexcept {
        size_t h1 = hash<int>{}(v.x);
        size_t h2 = hash<int>{}(v.y);
        size_t h3 = hash<int>{}(v.z);

        size_t seed = h1;

        seed ^= h2 + 0x9e37779b9 + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e37779b9 + (seed << 6) + (seed >> 2);

        return seed;
    }
};

struct IVec3Equal {
    bool operator()(const glm::ivec3& a, const glm::ivec3 b) const noexcept {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
};

using VoxelSet = unordered_set<glm::ivec3, IVec3Hash, IVec3Equal>;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2;
float lastY = SCR_HEIGHT / 2;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void processInput(GLFWwindow *window);

void mouse_callback(GLFWwindow* window, double xPos, double yPos);

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);

unsigned int loadTexture(const char* texturePath, string fileType);

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    string location;
    unsigned int id;
};



class Mesh {
public:
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures) {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        setupMesh();
    }
    
    void Draw(Shader &shader) {
        for (int i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            string name = "texture" + to_string(i+1);

            shader.setInt(name, i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        glActiveTexture(GL_TEXTURE0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }


private:
    unsigned int VAO, VBO, EBO;
    
    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
};

Mesh createVoxel() {
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    glm::vec2 texCoords[] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    for (int face = 0; face < 6; face++) {
        unsigned int start = vertices.size();

        for (int i = 0; i < 4; i++) {
            Vertex vertex;

            vertex.Position = faces[face].corners[i];
            vertex.Color = faces[face].color;
            vertex.Normal = faces[face].normal;
            vertex.TexCoords = texCoords[i];

            vertices.push_back(vertex);
        }


        indices.push_back(start);
        indices.push_back(start + 1);
        indices.push_back(start + 2);

        indices.push_back(start + 2);
        indices.push_back(start + 3);
        indices.push_back(start);
    }

    return Mesh(vertices, indices, textures);
}

struct World {
    vector<glm::ivec3> locations;
};

bool isVoxelNeighbour(const VoxelSet& voxelSet, const glm::ivec3& location) {
    return voxelSet.find(location) != voxelSet.end();
}

Mesh GenerateMeshFromWorld(World world) {
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    glm::vec2 texCoords[] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    VoxelSet voxelSet;
    voxelSet.reserve(world.locations.size());

    cout << "Generating voxelSet" << endl;
    for (const glm::ivec3 location : world.locations) {
        voxelSet.insert(location);
    }
    cout << "Finished generating voxelSet" << endl;

    for (glm::ivec3 location : world.locations) {
        for (const VoxelFace& face : faces) {
            unsigned int start = vertices.size();
            glm::ivec3 neighbourLocation = location + glm::ivec3(face.normal);
            if(isVoxelNeighbour(voxelSet, neighbourLocation)) {
                continue;
            } else {
                for (int i = 0; i < 4; i++) {
                    Vertex vertex;

                    vertex.Position = face.corners[i] + glm::vec3(location);
                    vertex.Color = face.color;
                    vertex.Normal = face.normal;
                    vertex.TexCoords = texCoords[i];

                    vertices.push_back(vertex);
                }
                indices.push_back(start);
                indices.push_back(start + 1);
                indices.push_back(start + 2);

                indices.push_back(start);
                indices.push_back(start + 2);
                indices.push_back(start + 3);
            }
        }
    }
    return Mesh(vertices, indices, textures);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Runebind", nullptr, nullptr);
    if (window == nullptr) {
        cout << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!glewInit()) {
        cout << "Failed to initialize glew" << endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);


    // load shader and assign textures
    Shader shader("shaders/shader.vs", "shaders/shader.fs");
    shader.use();

    Mesh voxel = createVoxel();
    auto worldHeightMap = generateWorld();
    

    World world;
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            for (int z = 0; z < WORLD_DEPTH; z++) {
                if (worldHeightMap[x][y][z] == 1) {
                    world.locations.push_back(glm::ivec3(x, y, z));
                }
            }
        }
    }

    cout << "Voxel count: " << world.locations.size() << endl;

    Mesh meshWorld = GenerateMeshFromWorld(world);

    cout << "Vertex count: " << meshWorld.vertices.size() << endl;
    cout << "Index count: " << meshWorld.indices.size() << endl;

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Time logic pre-frame
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Process any inputs
        processInput(window);

        // Render commands
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);

        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        meshWorld.Draw(shader);
        

        // Check and call events then swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const float cameraSpeed = 3.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.ProcessKeyboard(UP, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.ProcessKeyboard(DOWN, deltaTime);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
    if (firstMouse) {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }
    

    float xOffset = xPos - lastX;
    float yOffset = lastY - yPos;
    lastX = xPos;
    lastY = yPos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
    camera.ProcessMouseScroll(static_cast<float>(yOffset));
}

unsigned int loadTexture(const char* texturePath, string fileType) {
    unsigned int texture;
    glGenTextures(1, &texture);
    // glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(texturePath, &width, &height, &nrChannels, 0);

    if (data) {
        if (fileType == "jpg") {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else if (fileType == "png") {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            cout << "No file type specified or unknown filetype" << endl;
        }
    } else {
        cout << "Failed to load texture" << endl;
    }
    
    stbi_image_free(data);
    return texture;
}
