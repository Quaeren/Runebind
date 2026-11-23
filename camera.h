#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

enum CameraMode {
    FIRST_PERSON,
    THIRD_PERSON
};

const float YAW = -90.0f;
const float PITCH = 15.0f;
const float SPEED = 10.0f;
const float SENSITIVIY = 0.1f;
const float FOV = 70.0f;

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;
    float fov;

    CameraMode mode = THIRD_PERSON;
    float eyeHeight = 0.35f;
    float thirdPersonDistance = 4.0f;
    float minDistance = 1.5f;
    float maxDistance = 12.0f;
    float collisionMargin = 0.3f;
    float zoomOutSpeed = 8.0f;
    float nearPlane = 0.05f;
    float farPlane = 500.0f;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED), mouseSensitivity(SENSITIVIY), fov(FOV) {
        this->position = position;
        this->worldUp = up;
        this->yaw = yaw;
        this->pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() { return glm::lookAt(position, position + front, up); }
    glm::mat4 getProjectionMatrix(float aspect) const { return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane); }

    glm::vec3 groundForward() const { return glm::normalize(glm::vec3(front.x, 0.0f, front.z)); }
    glm::vec3 groundRight() const { return right; }

    void toggleMode() {
        if (mode == FIRST_PERSON) {
            mode = THIRD_PERSON;
            currentDistance = 0.0f;
        } else {
            mode = FIRST_PERSON;
        }
        pitch = std::min(pitch, maxPitch());
        updateCameraVectors();
    }

    template <typename SolidFn>
    void follow(const glm::vec3& playerPos, float deltaTime, SolidFn isSolid) {
        glm::vec3 pivot = playerPos + glm::vec3(0.0f, eyeHeight, 0.0f);

        if (mode == FIRST_PERSON) {
            position = pivot;
            return;
        }

        glm::vec3 back = -front;

        float allowed = thirdPersonDistance;
        const float step = 0.1f;
        for (float t = step; t <= thirdPersonDistance; t += step) {
            if (isSolid(pivot + back * t)) {
                allowed = std::max(t - collisionMargin, 0.0f);
                break;
            }
        }

        if (allowed < currentDistance) { currentDistance = allowed; }
        else {currentDistance = std::min(allowed, currentDistance + zoomOutSpeed * deltaTime); }

        position = pivot + back * currentDistance;
    }

    void processKeyboard(CameraMovement direction, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        switch (direction) {
            case FORWARD:
                position += front * velocity;
            case BACKWARD:
                position -= front * velocity;
            case RIGHT:
                position += right * velocity;
            case LEFT:
                position -= right * velocity;
            case UP: 
                position += up * velocity;
            case DOWN:
                position -= up * velocity;
        }
    }

    void processMouseMovement(float xOffset, float yOffset, GLboolean constrainPitch = true) {
        xOffset *= mouseSensitivity;
        yOffset *= mouseSensitivity;

        yaw += xOffset;
        pitch += yOffset;

        if (constrainPitch) { pitch = glm::clamp(pitch, -89.0f, maxPitch()); }
        updateCameraVectors();
    }

    void processMouseScroll(float yOffset) {
        if (mode == THIRD_PERSON) { thirdPersonDistance = glm::clamp(thirdPersonDistance - yOffset * 0.5f, minDistance, maxDistance); }
        else { fov = glm::clamp(fov - yOffset, 20.0f, 90.0f); }
    }

private:
    float currentDistance = 4.0f;

    float maxPitch() const { return mode == THIRD_PERSON ? 60.0f : 89.0f; }

    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->front = glm::normalize(front);

        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }
};

#endif