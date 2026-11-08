//#ifndef CAVEGENERATIONV3_HPP
//#define CAVEGENERATIONV3_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SFML/Graphics.hpp>

#include "camera.h"
#include "shader.h"

#include <iostream>
#include <vector>
#include <random>
#include <cmath>
using namespace std;

// Window dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2;
float lastY = SCR_HEIGHT / 2;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct TubeMesh {
    vector<glm::vec3> vertices;
    vector<unsigned int> indices;
};

mt19937 rng(1);
float randomFloat(float min, float max) {
    uniform_real_distribution<float> distribution(min, max);
    return distribution(rng);
}

struct StructurePoint {
    glm::vec3 position;
    float radius;
};

struct Cavern {
    glm::vec3 center;
    glm::vec3 radii;
};

struct CaveSystem {
    vector<StructurePoint> mainPath;
    vector<Cavern> caverns;
};

vector<glm::vec3> generateRing(glm::vec3 center, float radius, glm::vec3 axisU, glm::vec3 axisV, int segments=8) {
    vector<glm::vec3> vertices;

    for (size_t i = 0; i < segments; i++) {
        float angle = (2.0f * glm::pi<float>() * i)/segments;
        glm::vec3 position = center + radius * (cos(angle) * axisU + sin(angle) * axisV);
        vertices.push_back(position);
    }
    return vertices;
}

void connectRings(TubeMesh& mesh, unsigned int ringStart, unsigned int nextRingStart, int segments=8) {
    for (int i = 0; i < segments; i++) {
        unsigned int a = ringStart+i;
        unsigned int b = ringStart+ (i+1) % segments;
        unsigned int c = nextRingStart+i;
        unsigned int d = nextRingStart + (i+1) % segments;

        mesh.indices.push_back(a);
        mesh.indices.push_back(b);
        mesh.indices.push_back(c);

        mesh.indices.push_back(b);
        mesh.indices.push_back(d);
        mesh.indices.push_back(c);
    }
}

glm::vec3 calculateTangent(const vector<StructurePoint>& points, size_t i) {
    glm::vec3 tangent;
    
    if (i == 0) {
        tangent = glm::normalize(points[i+1].position - points[i].position);
    } else if (i == points.size() - 1) {
        tangent = glm::normalize(points[i].position - points[i-1].position);
    } else {
        tangent = glm::normalize(points[i+1].position - points[i-1].position);
    }

    float tangentSquared = glm::dot(tangent, tangent);

    if (tangentSquared < 0.000001f && i < points.size() - 1) {
        tangent = points[i+1].position - points[i].position;
        tangentSquared = glm::dot(tangent, tangent);
    } 
    if (tangentSquared < 0.000001f && i > 0) {
        tangent = points[i].position - points[i-1].position;
        tangentSquared = glm::dot(tangent, tangent);
    }
    if (tangentSquared < 0.000001f) {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }

    return glm::normalize(tangent);

}

TubeMesh generateTube(const vector<StructurePoint>& points, int segments=8) {
    TubeMesh mesh;

    if (points.size() < 2 || segments < 3) {
        return mesh;
    }

    glm::vec3 tangent = calculateTangent(points, 0);

    glm::vec3 helper(0.0f, -1.0f, 0.0f);

    if (abs(glm::dot(tangent, helper)) > 0.99f) {
        helper = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 axisU = glm::normalize(glm::cross(tangent, helper));
    glm::vec3 axisV = glm::normalize(glm::cross(tangent, axisU));

    vector<glm::vec3> firstRing = generateRing(points[0].position, points[0].radius, axisU, axisV, segments);
    for (const auto& vertex : firstRing) {
        mesh.vertices.push_back(vertex);
    }

    for (size_t i = 1; i < points.size(); i++) {

        glm::vec3 tangent = calculateTangent(points, i);

        glm::vec3 axisTempU = axisU - glm::dot(axisU, tangent) * tangent;
        float axisTempUSquared = glm::dot(axisTempU, axisTempU);

        if (axisTempUSquared < 0.000001f) {
            glm::vec3 fallback(0.0f, 1.0f, 0.0f);
            if (abs(glm::dot(tangent, fallback)) > 0.99f) {
                fallback = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            axisTempU = glm::cross(tangent, fallback);
        }

        axisTempU = glm::normalize(axisTempU);

        glm::vec3 axisTempV = glm::cross(tangent, axisTempU);
        axisTempV = glm::normalize(axisTempV);

        vector<glm::vec3> ring = generateRing(points[i].position, points[i].radius, axisTempU, axisTempV, segments);
        unsigned int ringStart = static_cast<unsigned int>(mesh.vertices.size());
        for (const auto& vertex : ring) {
            mesh.vertices.push_back(vertex);
        }

        unsigned int prevRingStart = ringStart - segments;
        connectRings(mesh, prevRingStart, ringStart, segments);

        axisU = axisTempU;
        axisV = axisTempV;
    }
    return mesh;
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

glm::vec3 interpolate(glm::vec3 a, glm::vec3 b, float t) {
    t = glm::clamp(t, 0.0f, 1.0f);
    return a + t * (b - a);
}

glm::vec3 catmullRom(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    return 0.5f * (2.0f * p1 + (-p0 + p2)*t + 2.0f * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

vector<StructurePoint> generateSpline(const vector<StructurePoint>& controlPoints, int subdivisions=8) {
    vector<StructurePoint> splinePoints;

    if (controlPoints.size() < 2 || subdivisions < 1) {
        return controlPoints;
    }

    for (int i = 0; i < controlPoints.size() - 1; i++) {
        glm::vec3 p0 = controlPoints[max(i - 1, 0)].position;
        glm::vec3 p1 = controlPoints[i].position;
        glm::vec3 p2 = controlPoints[i+1].position;
        glm::vec3 p3 = controlPoints[min(i + 2, (int)controlPoints.size() - 1)].position;

        float r1 = controlPoints[i].radius;
        float r2 = controlPoints[i + 1].radius;

        for (int j = 0; j < subdivisions; j++) {
            float t = (float)j / subdivisions;
            glm::vec3 position = catmullRom(p0, p1, p2, p3, t);
            float radius = glm::mix(r1, r2, t);
            splinePoints.push_back({position, radius});
        }
    }
    splinePoints.push_back(controlPoints.back());
    return splinePoints;
}

vector<StructurePoint> generateCavePath(glm::vec3 startPos, glm::vec3 startDirection, int numberOfSteps) {
    vector<StructurePoint> points;

    glm::vec3 position = startPos;
    glm::vec3 direction = glm::normalize(startDirection);
    float radius = 4.0f;

    const float stepLength = 4.0f;

    const float directionVariability = 0.15f;

    for (int i = 0; i < numberOfSteps; i++) {
        points.push_back({position, radius});

        glm::vec3 randomChange(
            randomFloat(-directionVariability, directionVariability),
            randomFloat(-directionVariability, directionVariability),
            randomFloat(-directionVariability, directionVariability)
        );
        float radiusChange = randomFloat(-0.5f, 0.5f);

        direction += randomChange;
        direction = glm::normalize(direction);

        position += direction * stepLength;
        radius += radiusChange;
        radius = glm::clamp(radius, 2.0f, 6.0f);
    }

    return points;
}

CaveSystem generateCaveStructure();

void convertCaveIntoVoxels(const CaveSystem& cave);

void generateWorldMeshes();

// void generateWorld() {
//    CaveSystem cave = generateCaveStructure();
//
//    convertCaveIntoVoxels(cave);
//    
//    generateWorldMeshes();
//}




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

    vector<StructurePoint> basicPath = generateCavePath(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, -0.5f, 0.0f), 40);
    vector<StructurePoint> smoothPath = generateSpline(basicPath, 2);

    TubeMesh tube = generateTube(smoothPath, 16);
    vector<glm::vec3> vertices = tube.vertices;
    vector<unsigned int> indices = tube.indices;

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 *sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // load shader and assign textures
    Shader shader("shaders/shaderTest.vs", "shaders/shaderTest.fs");
    shader.use();

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

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        shader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        shader.setMat4("projection", projection);

        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);

        glBindVertexArray(VAO);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Check and call events then swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


//#endif