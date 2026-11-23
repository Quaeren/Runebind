#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
//#include "orbitcamera.h"
#include "camera.h"
#include "VoxelWorld.hpp"
#include "player.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <chrono>

using namespace std;

// Window dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int fbWidth = SCR_WIDTH;
int fbHeight = SCR_HEIGHT;

// Camera
Camera camera;
float lastX = SCR_WIDTH / 2;
float lastY = SCR_HEIGHT / 2;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void processInput(GLFWwindow* window);

void mouse_callback(GLFWwindow* window, double xPos, double yPos);

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);

unsigned int loadTexture(const char* texturePath, string fileType);

glm::vec3 readMoveInput(GLFWwindow* window, const Camera& camera);

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Runebind", nullptr, nullptr);
    if (window == nullptr) {
        cout << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!glewInit()) {
        cout << "Failed to initialize glew" << endl;
        return -1;
    }

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // load shader and assign textures
    Shader shader("shaders/shader.vs", "shaders/shader.fs");
    shader.use();

    Cave cave;
    VoxelWorld world;

    auto start = chrono::high_resolution_clock::now();

    cave.generateWorld();
    auto caveGenerated = chrono::high_resolution_clock::now();

    world.generateFromSDF(cave);
    auto voxelsGenerated = chrono::high_resolution_clock::now();

    cout << "Caverns: " << cave.getCaverns().size() << ", large: " << cave.getLargeCaverns().size() << endl;

    cout << "Cave structure generation: " << chrono::duration<double>(caveGenerated - start).count() << " seconds" << endl;
    cout << "Voxel generation: " << chrono::duration<double>(voxelsGenerated - start).count() << " seconds" << endl;

    cout << "Voxel count:" << world.getVoxelCount() << endl;
    cout << "Chunk count:" << world.getChunkCount() << endl;
    cout << "Seed: " << seed << endl;

    Player player;
    player.position = glm::vec3(0.0f);
    Mesh playerMesh = makePlayer();

    lastFrame = static_cast<float>(glfwGetTime());

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Time logic pre-frame
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Process any inputs
        processInput(window);

        bool jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        player.facingYaw = camera.yaw;
        player.update(readMoveInput(window, camera), jump, deltaTime, world);
        camera.follow(player.position, deltaTime, [&](const glm::vec3& pos) { return world.isSolidAt(pos); });

        // Render commands
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        shader.setVec3("ambient", glm::vec3(0.35f));
        shader.setInt("lightCount", 0);

        float aspect = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
        shader.setMat4("projection", camera.getProjectionMatrix(aspect));
        shader.setMat4("view", camera.getViewMatrix());

        shader.setBool("useVertexColor", true);
        world.render(shader);

        if (camera.mode == THIRD_PERSON) {
            shader.setBool("useVertexColor", true);
            shader.setVec3("materialColor", glm::vec3(0.9f, 0.3f, 0.2f));

            shader.setMat4("model", player.modelMatrix());
            playerMesh.Draw(shader);
        }

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

    static bool vWasDown = false;
    bool vDown = glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS;
    if (vDown && !vWasDown) { camera.toggleMode(); }
    vWasDown = vDown;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    fbWidth = width;
    fbHeight = std::max(height, 1);
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

    camera.processMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
    camera.processMouseScroll(static_cast<float>(yOffset));
}

glm::vec3 readMoveInput(GLFWwindow* window, const Camera& camera) {
    glm::vec3 wish(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { wish += camera.groundForward(); }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { wish -= camera.groundForward(); }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { wish += camera.groundRight(); }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { wish -= camera.groundRight(); }
    return glm::length(wish) > 0.0f ? glm::normalize(wish) : wish;
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
