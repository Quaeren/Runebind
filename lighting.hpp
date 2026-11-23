#ifndef LIGHTING_HPP
#define LIGHTING_HPP

#include <glm/glm.hpp>

#include <functional>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "shader.h"

static constexpr int MAX_LIGHTS = 16;
enum class LightType { Point = 0, Spot = 1};

struct Light {
    int id = 0;
    LightType type = LightType::Point;
    
    glm::vec3 position{0.0f};
    glm::vec3 direction{0, -1, 0};
    glm::vec3 color{1.0f, 0.8f, 0.5f};

    float intensity = 1.0f;
    float radius = 14.0f;
    float cutOff = 0.95f, outerCutOff = 0.90f;
    float flicker = 0.0f; // change for torch or fire
    float currentScale = 1.0f;
    bool enabled = true;

    std::function<glm::vec3()> anchor;
    glm::vec3 offset{0.0f};
};

class LightManager {
private:
    std::vector<Light> lights;
    int nextId = 1;
private:
    float flickerNoise(float time, int id) {
        float phase = id * 12.9898f;
        return 0.50f * std::sin(time * 13.0f + phase) + 0.30f * std::sin(time * 7.3f + phase * 1.7f) + 0.20f * std::sin(time * 23.1f + phase * 0.6f);
    }
public:
    int add(Light light) {
        light.id = nextId++;
        lights.push_back(light);
        return light.id;
    }

    void remove(int id) { lights.erase(lights.begin()+ (id-1)); }
    Light* find(int id) { return &lights[id-1]; }

    void attach(int id, std::function<glm::vec3()> fn, glm::vec3 off = {}) {
        lights[id-1].anchor = fn;
        lights[id-1].offset = off;
    }

    void update(float time) {
        for (auto& light : lights) {
            if (light.anchor) { light.position = light.anchor() + light.offset; }
            light.currentScale = 1.0f + light.flicker * flickerNoise(time, light.id);
        }

    }

    void upload(Shader& shader, const glm::vec3& focus) {
        std::vector<const Light*> active;
        active.reserve(lights.size());
        for (const Light& light : lights) {
            if (light.enabled) { active.push_back(&light); }
        }

        auto distSq = [&](const Light* light) {
            glm::vec3 distance = light->position - focus;
            return glm::dot(distance, distance);
        };
        size_t n = std::min<size_t>(active.size(), MAX_LIGHTS);
        std::partial_sort(active.begin(), active.begin() + n, active.end(), [&](const Light* a, const Light* b) { return distSq(a) < distSq(b); });

        for (size_t i = 0; i < n; ++i) {
            const Light& light = *active[i];
            const std::string base = "lights[" + std::to_string(i) + "].";

            glm::vec3 color = light.color * light.intensity * light.currentScale;

            shader.setVec3(base + "position", light.position);
            shader.setVec3(base + "color", color);
            shader.setFloat(base + "radius", light.radius);
            shader.setInt(base + "type", static_cast<int>(light.type));

            if(light.type == LightType::Spot) {
                shader.setVec3(base + "direction", glm::normalize(light.direction));
                shader.setFloat(base + "cutOff", light.cutOff);
                shader.setFloat(base + "outerCutOff", light.outerCutOff);
            }
        }
        shader.setInt("lightCount", static_cast<int>(n));
    }
    
};

#endif