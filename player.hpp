#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "VoxelWorld.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

inline Mesh makePlayer() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int face = 0; face < 6; face++) {
        unsigned int vertexStart =
            static_cast<unsigned int>(vertices.size());

        for (int corner = 0; corner < 4; ++corner) {
            Vertex vertex;

            vertex.Position = faces[face].corners[corner];
            vertex.Color = faces[face].color;
            vertex.Normal = faces[face].normal;
            vertex.TexCoords = {(corner == 1 || corner == 2) ? 1.0f : 0.0f, (corner >= 2) ? 1.0f : 0.0f};
            vertices.push_back(vertex);
        }

        indices.push_back(vertexStart);
        indices.push_back(vertexStart + 1);
        indices.push_back(vertexStart + 2);

        indices.push_back(vertexStart + 2);
        indices.push_back(vertexStart + 3);
        indices.push_back(vertexStart);
    }

    return Mesh(vertices, indices, {});
}

class Player {
public:
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec3 size{0.8f, 1.2f, 0.8f};
    bool grounded = false;

    float moveSpeed = 6.0f;
    float gravity = 25.0f;
    float jumpSeed = 8.0f;
    float maxFallSpeed = 40.0f;

    float facingYaw = -90.0f;

    glm::vec3 halfExtents() const { return size * 0.5f; }

    glm::mat4 modelMatrix() const { 
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::rotate(model, glm::radians(90.0f - facingYaw), glm::vec3(0, 1, 0));
        return glm::scale(model, size); 
    }

    void update(const glm::vec3& wishDir, bool jump, float dt, const VoxelWorld& world) {
        dt = std::min(dt, 0.05f);

        velocity.x = wishDir.x * moveSpeed;
        velocity.z = wishDir.z * moveSpeed;
        velocity.y = std::max(velocity.y - gravity * dt, -maxFallSpeed);

        if (jump && grounded) { velocity.y = jumpSeed; }

        move(velocity * dt, world);
    }
private:
    void move(glm::vec3 delta, const VoxelWorld& world) {
        grounded = false;
        const float maxStep = world.getVoxelSize() * 0.5f;

        for (int axis = 0; axis < 3; ++axis) {
            int steps = std::max (1, static_cast<int>(std::ceil(std::abs(delta[axis]) / maxStep)));
            float stepDelta = delta[axis] / steps;

            for (int i = 0; i < steps; ++i) {
                position[axis] += stepDelta;
                if (world.overlapsSolid(position - halfExtents(), position + halfExtents())) {
                    position[axis] -= stepDelta;
                    if (axis == 1 && delta.y < 0.0f) { grounded = true; }
                    velocity[axis] = 0.0f;
                    break;
                }
            }
        }
    }
};

#endif