#ifndef ORBITCAMERA_H
#define ORBITCAMERA_H

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

class OrbitCamera {
public:
    glm::vec3 target{0.0f};
    float yaw = 45.0f;
    float targetYaw = 45.0f;
    float pitch = glm::degrees(std::atan(1/std::sqrt(2)));
    float distance = 40.0f;
    
    float orthoSize = 18.0f;
    float minOrthoSize = 5.0f;
    float maxOrthoSize = 25.0f;
    float frontClip = 2.0f;
    float backDepth = 80.0f;

    float orbitSpeed = 90.0f;
    float yawSmoothing = 12.0f;
    float followSmoothing = 10.0f;
    float zoomStep = 1.0f;

    glm::vec3 position() const {
        float pos = glm::radians(pitch), height = glm::radians(yaw);
        return target + distance * glm::vec3(std::cos(pos) * std::sin(height), std::sin(pos), std::cos(pos) * std::cos(height));
    }

    glm::mat4 view() const {
        return glm::lookAt(position(), target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 projection(float aspect) const {
        float nearPos = std::max(distance - frontClip, 0.1f);
        float farPos = distance + backDepth;
        return glm::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, nearPos, farPos);
    }

    glm::vec3 viewDirection() const { return glm::normalize(target - position()); }

    glm::vec3 groundForward() const {
        float y = glm::radians(yaw);
        return -glm::vec3(std::sin(y), 0.0f, std::cos(y));
    }

    glm::vec3 groundRight() const {
        return glm::cross(groundForward(), glm::vec3(0.0f, 1.0f, 0.0f));
    }


    void orbit(float dir, float dt) {
        targetYaw += dir * orbitSpeed * dt;
    }

    void snapOrbit(float stepDegrees) {
        targetYaw = std::round(targetYaw / stepDegrees) * stepDegrees + stepDegrees;
    }

    void zoom(float scrollY) {
        orthoSize = glm::clamp(orthoSize - scrollY * zoomStep, minOrthoSize, maxOrthoSize);
    }


    void update(const glm::vec3& playerPos, float dt) {
        float tf = 1.0f - std::exp(-followSmoothing * dt);
        target = glm::mix(target, playerPos, tf);

        float ty = 1.0f - std::exp(-yawSmoothing * dt);
        yaw += (targetYaw - yaw) * ty;
    }

    void snapTo(const glm::vec3& pos) {
        target = pos;
        yaw = targetYaw;
    }
};

#endif