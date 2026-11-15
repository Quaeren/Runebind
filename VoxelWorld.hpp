#ifndef VOXELWORD_HPP
#define VOXELWORD_HPP

#include <glm/glm.hpp>

#include <unordered_set>
#include <functional>
#include <random>
#include <iostream>
#include <vector>

int seed = time(nullptr);

// Random number generator with pre set seed for debugging
std::mt19937 rng(seed);
float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(rng);
}
int randomInt(int min, int max) {
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(rng);
}

float sMin(float a, float b, float k) {
    float h = glm::max(k - glm::abs(a-b), 0.0f) / k;
    return glm::min(a, b) - h * h * k * 0.25f;
}


// Function make curved line between two points instead of a normal straight one
glm::vec3 catmullRom(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    return 0.5f * (2.0f * p1 + (-p0 + p2)*t + 2.0f * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

class Cave {
public:
    struct StructurePoint {
        glm::vec3 position;
        float radius;
    };
    struct Cavern {
        glm::vec3 center;
        glm::vec3 radii;
    };

private:
    std::vector<StructurePoint> mainPath;
    std::vector<std::vector<StructurePoint>> sidePaths;
    std::vector<Cavern> caverns;
    std::vector<Cavern> largeCaverns;

    void generateCavePath(glm::vec3 startPos, glm::vec3 startDirection, int numberOfSteps) {
        glm::vec3 position = startPos;
        glm::vec3 direction = glm::normalize(startDirection);
        float radius = 4.0f;

        const float stepLength = 4.0f;

        const float directionVariability = 0.15f;

        for (int i = 0; i < numberOfSteps; i++) {
            mainPath.push_back({position, radius});

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
    }
    void generateSpline(int subdivisions=8) {
        std::vector<StructurePoint> splinePoints;

        if (mainPath.size() < 2 || subdivisions < 1) {
            return;
        }

        for (int i = 0; i < mainPath.size() - 1; i++) {
            glm::vec3 p0 = mainPath[std::max(i - 1, 0)].position;
            glm::vec3 p1 = mainPath[i].position;
            glm::vec3 p2 = mainPath[i+1].position;
            glm::vec3 p3 = mainPath[std::min(i + 2, (int)mainPath.size() - 1)].position;

            float r1 = mainPath[i].radius;
            float r2 = mainPath[i + 1].radius;

            for (int j = 0; j < subdivisions; j++) {
                float t = (float)j / subdivisions;
                glm::vec3 position = catmullRom(p0, p1, p2, p3, t);
                float radius = glm::mix(r1, r2, t);
                splinePoints.push_back({position, radius});
            }
        }
        splinePoints.push_back(mainPath.back());
        mainPath = splinePoints;
    }
    void generateBranches() {
        const float directionVariability = 0.15f;
        float stepLength = 4.0f;
        std::vector<StructurePoint> branchStarts;

        for (const StructurePoint& point : mainPath) {
            if (randomInt(1, 100) <= 5) {
                branchStarts.push_back(point);
            }
        }
        for (const StructurePoint& point : branchStarts) {
            std::vector<StructurePoint> sidePath;
            sidePath.push_back(point);
            glm::vec3 direction = glm::vec3(randomFloat(-0.5, 0.5), randomFloat(-0.1, 0.1), randomFloat(-0.5, 0.5));
            int numberOfSteps = randomInt(20, 80);
            glm::vec3 position = point.position;
            float radius = 4.0f;

            for (int i = 0; i < numberOfSteps; i++) {
                glm::vec3 randomChange(
                    randomFloat(-directionVariability, directionVariability),
                    randomFloat(-directionVariability, directionVariability),
                    randomFloat(-directionVariability, directionVariability)
                );
                float radiusChange = randomFloat(-0.3f, 0.3f);

                direction += randomChange;
                direction = glm::normalize(direction);

                position += direction * stepLength;
                radius += radiusChange;
                radius = glm::clamp(radius, 2.0f, 6.0f);
                sidePath.push_back({position, radius});
            }
            sidePaths.push_back(sidePath);
        }
    }
    void generateSplineBranches(int subdivisions=8) {
        std::vector<std::vector<StructurePoint>> splineSidePaths;
        for (auto sidePath : sidePaths) {
            std::vector<StructurePoint> splineSidePath;
            if (sidePath.size() < 2 || subdivisions < 2) {
                continue;
            }
            for (int i = 0; i < sidePath.size() - 1; i++) {
                glm::vec3 p0 = sidePath[std::max(i - 1, 0)].position;
                glm::vec3 p1 = sidePath[i].position;
                glm::vec3 p2 = sidePath[i+1].position;
                glm::vec3 p3 = sidePath[std::min(i + 2, (int)sidePath.size() - 1)].position;

                float r1 = sidePath[i].radius;
                float r2 = sidePath[i + 1].radius;

                for (int j = 0; j < subdivisions; j++) {
                    float t = (float)j / subdivisions;
                    glm::vec3 position = catmullRom(p0, p1, p2, p3, t);
                    float radius = glm::mix(r1, r2, t);
                    splineSidePath.push_back({position, radius});
                }
            }
            splineSidePath.push_back(sidePath.back());
            splineSidePaths.push_back(splineSidePath);
        }
        sidePaths = splineSidePaths;
    }
    void generateCaverns(std::vector<StructurePoint> points) {
        int cavernCount = static_cast<int>(randomFloat(2, 6));
        
        if (mainPath.size() < 3) {
            return;
        }

        for (int i = 0; i < cavernCount; i++) {
            int interval = (points.size() - 2) / cavernCount;
            int approxIndex = 1 + i * interval;
            int randomIndex = static_cast<float>(randomFloat(-3.0f, 3.0f));
            int index = glm::clamp(approxIndex + randomIndex, 1, static_cast<int>(points.size()-1));

            if (largeCaverns.size() < 2 || randomInt(1, 100) <= 5) {
                glm::vec3 center = points[index].position;
                glm::vec3 radii(randomFloat(20.0f, 40.0f), randomFloat(20.0f, 30.0f), randomFloat(20.0f, 40.0f));
                largeCaverns.push_back({center, radii});
                continue;
            }

            glm::vec3 center = points[index].position;
            glm::vec3 radii(randomFloat(8.0f, 16.0f), randomFloat(5.0f, 10.0f), randomFloat(8.0f, 16.0f));

            caverns.push_back({center, radii});
        }
    }

public:
    float tunnelSegmentSDF(glm::vec3 position, const StructurePoint& start, const StructurePoint& end) {
        glm::vec3 segment = end.position - start.position;
        float lengthSquared = glm::dot(segment, segment);
        float t = 0.0f;

        if(lengthSquared >= 0.000001f) {
            t = glm::dot((position - start.position), (segment / lengthSquared));
        }

        t = glm::clamp(t, 0.0f, 1.0f);
        glm::vec3 closestPoint = start.position + t * segment;
        float distance = glm::length(position - closestPoint);
        float radius = glm::mix(start.radius, end.radius, t);
        return distance - radius;
    }
    float cavernSDF(const glm::vec3& position, const Cavern& cavern) {
        glm::vec3 relative = position - cavern.center;
        glm::vec3 radii = cavern.radii;

        if(radii.x <= 0.0f || radii.y <= 0.0f || radii.z <= 0.0f) {
            return 100000.0f;
        }

        float k0 = glm::length(relative / radii);
        float k1 = glm::length(relative / (radii * radii));

        if (k1 < 0.000001f) {
            return -glm::min(radii.x, glm::min(radii.y, radii.z));
        }
        return k0 * (k0 - 1.0f) / k1;
    }
    float caveSDF(glm::vec3& location, float blendstrength = 3.0f) {
        bool hasShape = false;
        float distance = 0.0f;

        for (size_t i = 0; i + 1 < mainPath.size(); i++) {
            float segmentDistance = tunnelSegmentSDF(location, mainPath[i], mainPath[i + 1]);

            if (!hasShape) {
                distance = segmentDistance;
                hasShape = true;
            } else {
                distance = sMin(distance, segmentDistance, blendstrength);
            }
        }

        for (const auto& sidePath : sidePaths) {
            for (size_t i = 0; i+1 < sidePath.size(); i++) {
                float segmentDistance = tunnelSegmentSDF(location, sidePath[i], sidePath[i + 1]);

                if (!hasShape) {
                    distance = segmentDistance;
                    hasShape = true;
                } else {
                    distance = sMin(distance, segmentDistance, blendstrength);
                }
            }
        }

        for (const auto& cavern : caverns) {
            float cavernDistance = cavernSDF(location, cavern);

            if (!hasShape) {
                distance = cavernDistance;
                hasShape = true;
            } else {
                distance = sMin(distance, cavernDistance, blendstrength);
            }
        }

        for (const auto& largeCavern : largeCaverns) {
            float largeCavernDistance = cavernSDF(location, largeCavern);

            if(!hasShape) {
                distance = largeCavernDistance;
                hasShape = true;
            } else {
                distance = sMin(distance, largeCavernDistance, blendstrength);
            }
        }

        return hasShape ? distance : 100000.0f;
    }

    void generateWorld(glm::vec3 origin = {0.0f, 0.0f, 0.0f}, int tunnelLength=50, int segments=2) {
        mainPath.clear();
        caverns.clear();
        sidePaths.clear();
        generateCavePath(origin, glm::vec3(0.5f, -0.2f, 0.0f),tunnelLength);
        generateSpline(segments);
        generateBranches();
        generateSplineBranches(segments);
        generateCaverns(mainPath);
        for (auto sidePath : sidePaths) { generateCaverns(sidePath); }
    }

    const std::vector<StructurePoint>& getMainPath() { return mainPath; }
    const std::vector<Cavern>& getCaverns() {return caverns; }
    const std::vector<std::vector<StructurePoint>>& getSidePaths() {return sidePaths; }

    
};

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
        size_t h1 = std::hash<int>{}(v.x);
        size_t h2 = std::hash<int>{}(v.y);
        size_t h3 = std::hash<int>{}(v.z);

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

using VoxelSet = std::unordered_set<glm::ivec3, IVec3Hash, IVec3Equal>;

class VoxelWorld {
private:
    VoxelSet voxelSet;
    
    glm::vec3 worldOrigin;
    float voxelSize;

    glm::ivec3 worldToVoxel(const glm::vec3& pos) const { return glm::ivec3((pos - worldOrigin) / voxelSize); }
    glm::vec3 voxelToWorld(const glm::ivec3& pos) const { return worldOrigin + glm::vec3(pos) * voxelSize; }

public:
    VoxelWorld(glm::vec3 worldOrigin = glm::vec3(0.0f, 0.0f, 0.0f), float voxelSize = 1.0f) : worldOrigin(worldOrigin), voxelSize(voxelSize) {}

    void addVoxel(glm::ivec3 position) { voxelSet.insert(position); }
    void removeVoxel(glm::ivec3 position) { voxelSet.erase(position); }
    bool checkForVoxel(glm::ivec3 position) const { return voxelSet.find(position) != voxelSet.end(); }

    void generateTunnelSegment(Cave& cave, const Cave::StructurePoint& start, const Cave::StructurePoint& end, float shellThickness) {
        float radius = glm::max(start.radius, end.radius);
        glm::vec3 paddingVec(radius + shellThickness + 1.0f);
        
        glm::vec3 minBounds = glm::min(start.position, end.position) - paddingVec;
        glm::vec3 maxBounds = glm::max(start.position, end.position) + paddingVec;

        glm::ivec3 minVoxel = glm::ivec3(glm::floor((minBounds - worldOrigin) / voxelSize));
        glm::ivec3 maxVoxel = glm::ivec3(glm::ceil((maxBounds - worldOrigin) / voxelSize));

        for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
            for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
                for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
                    glm::ivec3 voxelPos(x, y, z);
                    glm::vec3 worldPos = worldOrigin + glm::vec3(voxelPos) * voxelSize;
                    if (cave.caveSDF(worldPos) >= 0.0f) { addVoxel(voxelPos); }
                }
            }
        }
        
    }
    void generateCavern(Cave& cave, const Cave::Cavern& cavern, float shellThickness) {
        glm::vec3 padding(shellThickness + 1.0f);

        glm::vec3 minBounds = cavern.center - cavern.radii - padding;
        glm::vec3 maxBounds = cavern.center + cavern.radii + padding;
        glm::ivec3 minVoxel = glm::ivec3(glm::floor((minBounds - worldOrigin) / voxelSize));
        glm::ivec3 maxVoxel = glm::ivec3(glm::ceil((maxBounds - worldOrigin) / voxelSize));

        for (int x = minVoxel.x; x <= maxVoxel.x; ++x) {
            for (int y = minVoxel.y; y <= maxVoxel.y; ++y) {
                for (int z = minVoxel.z; z <= maxVoxel.z; ++z) {
                    glm::ivec3 voxelPos(x, y, z);
                    glm::vec3 worldPos = worldOrigin + glm::vec3(voxelPos) * voxelSize;
                    if (cave.caveSDF(worldPos) >= 0.0f) { addVoxel(voxelPos); }
                }
            }
        }
    }

    const float& getVoxelSize() const {return voxelSize; }
    const float getVoxelSetSize() const {return voxelSet.size(); }
    const VoxelSet& getVoxels() const { return voxelSet; }

    void generateFromSDF(Cave& cave, float shellThickness=0.0f) {
        voxelSet.clear();

        const auto& path = cave.getMainPath();
        const auto sidePaths = cave.getSidePaths();
        const auto& caverns = cave.getCaverns();

        for (size_t i = 0; i + 1 < path.size(); ++i) {
            generateTunnelSegment(cave, path[i], path[i+1], shellThickness);
        }
        for (auto sidePath : sidePaths) {
            for (size_t i = 0; i + 1 < sidePath.size(); ++i) {
                generateTunnelSegment(cave, sidePath[i], sidePath[i+1], shellThickness);
            }
        }
        for (const auto& cavern : caverns) {
            generateCavern(cave, cavern, shellThickness);
        }
    }

};

#endif