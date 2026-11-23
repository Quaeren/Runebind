#ifndef VOXELWORD_HPP
#define VOXELWORD_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <FastNoiseLite/Cpp/FastNoiseLite.h>

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <random>
#include <iostream>
#include <vector>
#include <string>

inline const int seed = time(nullptr);
static constexpr int CHUNK_SIZE = 12;



// Random number generator with pre set seed for debugging
inline std::mt19937 rng(seed);
inline float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(rng);
}
inline int randomInt(int min, int max) {
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(rng);
}

inline float sMin(float a, float b, float k) {
    float h = glm::max(k - glm::abs(a-b), 0.0f) / k;
    return glm::min(a, b) - h * h * k * 0.25f;
}

inline float sMax(float a, float b, float k) {
    return -sMin(-a, -b, k);
}

inline int fDivision(int value, int divisor) {
    int result = value / divisor;
    int remainder = value % divisor;   
    if (remainder < 0) {result--;}
    return result;

}


// Function make curved line between two points instead of a normal straight one
inline glm::vec3 catmullRom(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    return 0.5f * (2.0f * p1 + (-p0 + p2)*t + 2.0f * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    std::string location;
    unsigned int id;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    Mesh() {
        setupMesh();
    }

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures) {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        setupMesh();
        uploadMesh();
    }
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    
    void Draw(Shader &shader) {
        for (std::size_t i = 0; i < textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string name = "texture" + std::to_string(i+1);

            shader.setInt(name, i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        glActiveTexture(GL_TEXTURE0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void updateMesh(const std::vector<Vertex>& updatedVertices, const std::vector<unsigned int>& updatedIndices) {
        vertices = updatedVertices;
        indices = updatedIndices;

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);
        glBindVertexArray(0);
    }

    ~Mesh() {
        if (VAO) { glDeleteVertexArrays(1, &VAO); }
        if (VBO) { glDeleteBuffers(1, &VBO); }
        if (EBO) { glDeleteBuffers(1, &EBO); }
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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
    }
    void uploadMesh() {
        updateMesh(vertices, indices);
    }
};

class Cave {
public:
    struct StructurePoint {
        glm::vec3 position;
        float radius;
    };
    struct Cavern {
        glm::vec3 center;
        glm::vec3 radii;

        bool hasLedge = false;
        glm::vec3 ledgeStart{0.0f};
        glm::vec2 ledgeDir{1.0f, 0.0f};
        float ledgeLength = 0.0f;
        float ledgeHalfWidth = 0.0f;
        float ledgeTopY = 0.0f;
    };
    struct LocalShape {
        std::vector<std::pair<const StructurePoint*, const StructurePoint*>> segments;
        std::vector<const Cavern*> caverns;
    };

public:
    const std::vector<StructurePoint>& getMainPath() { return mainPath; }
    const std::vector<Cavern>& getCaverns() {return caverns; }
    const std::vector<std::vector<StructurePoint>>& getSidePaths() {return sidePaths; }
    const std::vector<Cavern>& getLargeCaverns() { return largeCaverns; }
    float getNoiseAmplitude() const { return shapeAmplitude + roughAmplitude; }

    float noiseValue(glm::vec3 point) const {
        float x = point.x, y = point.y, z = point.z;
        warpNoise.DomainWarp(x, y, z);

        float shape = shapeNoise.GetNoise(x, y, z) * shapeAmplitude;
        float rough = roughNoise.GetNoise(x, y, z) * roughAmplitude * 2.0f;
        return shape + rough;
    }

    float tunnelSegmentSDF(glm::vec3 position, const StructurePoint& start, const StructurePoint& end) const {
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

    float ledgeSolidSDF(const glm::vec3& position, const Cavern& cavern) const {
        glm::vec2 relief(position.x - cavern.ledgeStart.x, position.z - cavern.ledgeStart.z);
        float along = glm::dot(relief, cavern.ledgeDir);
        float across = glm::dot(relief, glm::vec2(-cavern.ledgeDir.y, cavern.ledgeDir.x));

        const float behind = 2.0f;
        const float halfHeight = 60.0f;
        float halfLength = (cavern.ledgeLength + behind) * 0.5f;
        float midLength = (cavern.ledgeLength - behind) * 0.5f;

        glm::vec3 q = glm::abs(glm::vec3(along - midLength, position.y - (cavern.ledgeTopY - halfHeight), across)) - glm::vec3(halfLength, halfHeight, cavern.ledgeHalfWidth);
        return glm::length(glm::max(q, 0.0f)) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f);

    }

    float cavernSDF(const glm::vec3& position, const Cavern& cavern) const {
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
        float elipsoid = k0 * (k0 - 1.0f) / k1;
        if (cavern.hasLedge) {
            return sMax(elipsoid, -ledgeSolidSDF(position, cavern), 1.0f);
        }
        return elipsoid;
    }

    void bounds(glm::vec3& minCorner, glm::vec3& maxCorner) const {
        minCorner = glm::vec3(1e30f);
        maxCorner = glm::vec3(-1e30f);
        auto addPath = [&](const std::vector<StructurePoint>& points) {
            for (auto& point : points) {
                minCorner = glm::min(minCorner, point.position - point.radius);
                maxCorner = glm::max(maxCorner, point.position + point.radius);
            }
        };
        addPath(mainPath);
        for (auto& sidePath : sidePaths) { addPath(sidePath); }
        for (auto& cavern : caverns) {
            minCorner = glm::min(minCorner, cavern.center - cavern.radii);
            maxCorner = glm::max(maxCorner, cavern.center + cavern.radii);
        }
        for (auto& largeCavern : largeCaverns) {
            minCorner = glm::min(minCorner, largeCavern.center - largeCavern.radii);
            maxCorner = glm::max(maxCorner, largeCavern.center + largeCavern.radii);            
        }
    }

    LocalShape gather(const glm::vec3& minCorner, const glm::vec3& maxCorner, float margin) const {
        LocalShape output;
        auto addPath = [&](const std::vector<StructurePoint>& path) {
            for (size_t i = 0; i+1 < path.size(); ++i) {
                const auto& a = path[i];
                const auto& b = path[i+1];
                float r = glm::max(a.radius, b.radius) + margin;
                glm::vec3 bmin = glm::min(a.position, b.position) - r;
                glm::vec3 bmax = glm::max(a.position, b.position) + r;
                if (glm::all(glm::lessThanEqual(bmin, maxCorner)) && glm::all(glm::greaterThanEqual(bmax, minCorner))) {
                    output.segments.push_back({&a, &b});
                }
            }
        };
        addPath(mainPath);
        for (auto& sidePath : sidePaths) { addPath(sidePath); }
        for (auto& cavern : caverns) {
            glm::vec3 bmin = cavern.center - cavern.radii - margin;
            glm::vec3 bmax = cavern.center + cavern.radii + margin;
            if (glm::all(glm::lessThanEqual(bmin, maxCorner)) && glm::all(glm::greaterThanEqual(bmax, minCorner))) {
                    output.caverns.push_back(&cavern);
            }
        }
        for (auto& largeCavern : largeCaverns) {
            glm::vec3 bmin = largeCavern.center - largeCavern.radii - margin;
            glm::vec3 bmax = largeCavern.center + largeCavern.radii + margin;
            if (glm::all(glm::lessThanEqual(bmin, maxCorner)) && glm::all(glm::greaterThanEqual(bmax, minCorner))) {
                    output.caverns.push_back(&largeCavern);
            }         
        }        
        return output;
    }

    float localSDF(const glm::vec3& pos, const LocalShape& shape, float k = 3.0f) const {
        float distance = 100'000.0f;
        bool has = false;
        auto combine = [&](float v) {distance = has ? sMin(distance, v, k) : v; has = true; };
        for (auto& [a, b] : shape.segments) { combine(tunnelSegmentSDF(pos, *a, *b)); }
        for (auto* cavern : shape.caverns) { combine(cavernSDF(pos, *cavern)); }
        return distance;
    }

    float noisySDF(const glm::vec3& pos, const LocalShape& shape, float k = 3.0f, float shellThickness=2.0f, float voxelSize=1.0f) const {
        float distance = localSDF(pos, shape, k);
        float reach = getNoiseAmplitude();
        float shellBand = shellThickness + 2.0f + voxelSize;
        if (distance > shellBand + reach || distance < -reach) { return distance; }
        return distance + noiseValue(pos);
    }

private:
    std::vector<StructurePoint> mainPath;
    std::vector<std::vector<StructurePoint>> sidePaths;
    std::vector<Cavern> caverns;
    std::vector<Cavern> largeCaverns;
    float ledgeChance = 0.7f;

    FastNoiseLite shapeNoise;
    FastNoiseLite roughNoise;
    FastNoiseLite warpNoise;

    float shapeAmplitude = 0.8f;
    float roughAmplitude = 0.6f;
    float warpStrength = 4.0f;

    void setupNoise(int seed) {
        shapeNoise.SetSeed(seed);
        shapeNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        shapeNoise.SetFrequency(0.02f);

        roughNoise.SetSeed(seed + 1);
        roughNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        roughNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
        roughNoise.SetFractalOctaves(2);
        roughNoise.SetFractalLacunarity(2.0f);
        roughNoise.SetFractalGain(0.35f);
        roughNoise.SetFrequency(0.15f);

        warpNoise.SetSeed(seed + 2);
        warpNoise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
        warpNoise.SetDomainWarpAmp(warpStrength);
        warpNoise.SetFrequency(0.05f);
    }

    void tryAddLedge(Cavern& cavern, const std::vector<StructurePoint>& points, int index, bool force = false) {
        float prob = randomFloat(0.0f, 1.0f);
        if (!force && prob > ledgeChance) { return; }

        int j = index;
        while (j > 0 && cavernSDF(points[j].position, cavern) < 0.0f) { --j; }

        if (cavernSDF(points[j].position, cavern) < 0.0f) { 
            if (!force) { return; }
            j = index;
            while (j + 1 < (int)points.size() && cavernSDF(points[j].position, cavern) < 0.0f) { return; }
        }

        const StructurePoint& entry = points[j];
        glm::vec2 toCenter(cavern.center.x - entry.position.x, cavern.center.z - entry.position.z);
        float distance = glm::length(toCenter);
        if (distance < 4.0f) { return; } 

        float topY = entry.position.y - entry.radius + 0.25f;
        float cavernFloor = cavern.center.y - cavern.radii.y;
        if (topY - cavernFloor < 8.0f) { return; }

        cavern.hasLedge = true;
        cavern.ledgeStart = entry.position;
        cavern.ledgeDir = toCenter / distance;
        cavern.ledgeLength = distance * randomFloat(0.5f, 0.8f);
        cavern.ledgeHalfWidth = entry.radius + randomFloat(5.0f, 10.0f);
        cavern.ledgeTopY = topY;
    }

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
            if (randomInt(1, 100) <= 1) {
                branchStarts.push_back(point);
            }
        }
        for (const StructurePoint& point : branchStarts) {
            std::vector<StructurePoint> sidePath;
            sidePath.push_back(point);
            glm::vec3 direction = glm::vec3(randomFloat(-0.5, 0.5), randomFloat(-0.1, 0.1), randomFloat(-0.5, 0.5));
            int numberOfSteps = randomInt(20, 60);
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
        for (const auto& sidePath : sidePaths) {
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
        std::cout << "Cavern count: " << cavernCount << std::endl;
        if (points.size() < 3) {
            return;
        }

        for (int i = 0; i < cavernCount; i++) {
            int interval = (points.size() - 2) / cavernCount;
            int approxIndex = 1 + i * interval;
            int randomIndex = randomFloat(-3, 3);
            int index = glm::clamp(approxIndex + randomIndex, 1, static_cast<int>(points.size()-1));
            bool force = (i == 0);

            if (largeCaverns.size() < 3 && randomInt(1, 100) <= 25) {
                glm::vec3 center = points[index].position;
                glm::vec3 radii(randomFloat(20.0f, 40.0f), randomFloat(20.0f, 30.0f), randomFloat(20.0f, 40.0f));
                Cavern cavern{center, radii};
                tryAddLedge(cavern, points, index, force);
                largeCaverns.push_back(cavern);
                continue;
            }

            glm::vec3 center = points[index].position;
            glm::vec3 radii(randomFloat(12.0f, 20.0f), randomFloat(10.0f, 20.0f), randomFloat(12.0f, 20.0f));
            Cavern cavern{center, radii};
            tryAddLedge(cavern, points, index, force);
            caverns.push_back(cavern);
        }
        int ledges = 0;
        for (auto& cavern : caverns) {if (cavern.hasLedge) { ++ledges; }}
        for (auto& largeCavern : largeCaverns) {if (largeCavern.hasLedge) { ++ledges; }}
        std::cout << "ledges: " << ledges << " / " << (caverns.size() + largeCaverns.size()) << std::endl;

    }

public:
    void generateWorld(glm::vec3 origin = {0.0f, 0.0f, 0.0f}, int tunnelLength=100, int segments=2) {
        mainPath.clear();
        caverns.clear();
        largeCaverns.clear();
        sidePaths.clear();
        generateCavePath(origin, glm::vec3(0.5f, -0.2f, 0.0f),tunnelLength);
        generateSpline(segments);
        generateBranches();
        generateSplineBranches(segments);
        generateCaverns(mainPath);
        for (auto sidePath : sidePaths) { generateCaverns(sidePath); }
        setupNoise(seed);
    }
};

struct VoxelFace {
    glm::vec3 corners[4];
    glm::vec3 color;
    glm::vec3 normal;
};

inline VoxelFace faces[] = {
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
    bool operator()(const glm::ivec3& a, const glm::ivec3& b) const noexcept {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
};

inline float hash3(const glm::ivec3 point) {
    uint32_t h = uint32_t(point.x) * 37476139u + uint32_t(point.y) * 668265263u + uint32_t(point.z) * 2147583647u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xFFFFFFu) / float(0x1000000);
}

inline glm::vec3 stoneColor(const glm::ivec3 voxel, float voxelSize) {
    const glm::vec3 grey (0.42f, 0.41f, 0.40f);
    const glm::vec3 brown (0.45f, 0.32f, 0.22f);

    float worldY = voxel.y * voxelSize;
    float band = 0.5f + 0.5f * std::sin(worldY * 0.4f);
    glm::vec3 base = glm::mix(grey, brown, band);

    float speckle = 0.85f + 0.30f * hash3(voxel);
    return base * speckle;

}

class VoxelWorld;

class Chunk {
private:
    glm::ivec3 chunkPosition;
    std::unordered_map<glm::ivec3, int, IVec3Hash, IVec3Equal> localVoxels;
    Mesh mesh;
    bool meshUpdateNeeded = true;

    bool isValidLocalPos(const glm::ivec3& localPos) const {
        return (localPos.x < CHUNK_SIZE && localPos.x >= 0 && localPos.y < CHUNK_SIZE && localPos.y >= 0 && localPos.z < CHUNK_SIZE && localPos.z >= 0);
    }
public:
    Chunk(glm::ivec3 position) : chunkPosition(position) {}

    void addVoxel(glm::ivec3 localPos) {
        if (!isValidLocalPos(localPos)) return;
        localVoxels[localPos] = 1;
        meshUpdateNeeded = true;
    }
    void removeVoxel(glm::ivec3 localPos) {
        if (!isValidLocalPos(localPos)) return;
        if (localVoxels.erase(localPos) > 0) meshUpdateNeeded = true;
    }
    bool checkForVoxel(glm::ivec3 localPos) const { return localVoxels.find(localPos) != localVoxels.end(); }

    void markMeshUpdateNeeded() { meshUpdateNeeded = true; }
    void clearMeshUpdateNeeded() { meshUpdateNeeded = false; }
    
    bool returnMeshUpdateNeeded() const {return meshUpdateNeeded; }
    const glm::ivec3& getPosition() const { return chunkPosition; }
    const std::unordered_map<glm::ivec3, int, IVec3Hash, IVec3Equal>& getLocalVoxels() const { return localVoxels; }
    Mesh& getMesh() { return mesh; }

    void generateMesh(const VoxelWorld& world);
};

class VoxelWorld {
private:
    std::unordered_map<glm::ivec3, Chunk, IVec3Hash, IVec3Equal> chunks;
    glm::vec3 worldOrigin;
    float voxelSize;

    glm::ivec3 worldToVoxel(const glm::vec3& pos) const { return glm::ivec3((pos - worldOrigin) / voxelSize); }
    glm::vec3 voxelToWorld(const glm::ivec3& pos) const { return worldOrigin + glm::vec3(pos) * voxelSize; }

    glm::ivec3 worldToChunk(glm::ivec3 pos) const {
        return {fDivision(pos.x, CHUNK_SIZE), fDivision(pos.y, CHUNK_SIZE), fDivision(pos.z, CHUNK_SIZE)};
    }
    glm::ivec3 worldToLocal(glm::ivec3 pos) const {
        glm::ivec3 chunkPos = worldToChunk(pos);
        return pos - chunkPos * CHUNK_SIZE;
    }

    void markChunkUpdateNeeded(const glm::ivec3& chunkPos) {
        auto it = chunks.find(chunkPos);
        if (it != chunks.end()) it->second.markMeshUpdateNeeded();
    }

    void markNeighbourChunkUpdatedNeeded(const glm::ivec3& pos) {
        glm::ivec3 chunkPos = worldToChunk(pos);
        glm::ivec3 localPos = worldToLocal(pos);

        markChunkUpdateNeeded(chunkPos);
        if (localPos.x == 0) { markChunkUpdateNeeded(chunkPos + glm::ivec3(-1, 0, 0)); }
        if (localPos.x == CHUNK_SIZE - 1) { markChunkUpdateNeeded(chunkPos + glm::ivec3( 1, 0, 0)); }
        if (localPos.y == 0) { markChunkUpdateNeeded(chunkPos + glm::ivec3( 0,-1, 0)); }
        if (localPos.y == CHUNK_SIZE - 1) { markChunkUpdateNeeded(chunkPos + glm::ivec3( 0, 1, 0)); }
        if (localPos.z == 0) { markChunkUpdateNeeded(chunkPos + glm::ivec3( 0, 0,-1)); }
        if (localPos.z == CHUNK_SIZE - 1) { markChunkUpdateNeeded(chunkPos + glm::ivec3( 0, 0, 1)); }
    }

public:
    VoxelWorld(glm::vec3 worldOrigin = glm::vec3(0.0f, 0.0f, 0.0f), float voxelSize = 0.25f) : worldOrigin(worldOrigin), voxelSize(voxelSize) {}

    void addVoxel(glm::ivec3 pos) {
        glm::ivec3 chunkPos = worldToChunk(pos); 
        glm::ivec3 localPos = worldToLocal(pos);

        auto it = chunks.find(chunkPos);
        if (it == chunks.end()) { it = chunks.try_emplace(chunkPos, chunkPos).first; }
        it->second.addVoxel(localPos);
        markNeighbourChunkUpdatedNeeded(pos);
    }
    void removeVoxel(glm::ivec3 pos) {
        glm::ivec3 chunkPos = worldToChunk(pos); 
        glm::ivec3 localPos = worldToLocal(pos);

        auto it = chunks.find(chunkPos);
        if (it == chunks.end()) { return; }
        it->second.removeVoxel(localPos);
        markNeighbourChunkUpdatedNeeded(pos);
    }
    bool checkForVoxel(glm::ivec3 pos) const {
        glm::ivec3 chunkPos = worldToChunk(pos); 
        glm::ivec3 localPos = worldToLocal(pos);

        auto it = chunks.find(chunkPos);
        if (it == chunks.end()) { return false; }
        return it->second.checkForVoxel(localPos);
    }

    float getVoxelSize() const {return voxelSize; }
    std::size_t getVoxelCount() {
        int count = 0;
        for (const auto& [position, chunk] : chunks) {
            count += chunk.getLocalVoxels().size();
        }
        return count;
    }
    std::size_t getChunkCount() {return chunks.size(); }

    void generateFromSDF(Cave& cave, float shellThickness=2.0f) {
        chunks.clear();
        const float blend = 3.0f;
        const float margin = shellThickness + blend + 2.0f + cave.getNoiseAmplitude();

        glm::vec3 minCorner, maxCorner;
        cave.bounds(minCorner, maxCorner);

        std::cout << "Bounds minCorner: " << minCorner.x << " " << minCorner.y << " " << minCorner.z << " maxCorner: " << maxCorner.x << " " << maxCorner.y << " " << maxCorner.z << std::endl;

        glm::ivec3 minChunk = worldToChunk(glm::ivec3(glm::floor((minCorner - margin - worldOrigin) / voxelSize)));
        glm::ivec3 maxChunk = worldToChunk(glm::ivec3(glm::ceil((maxCorner + margin - worldOrigin) / voxelSize)));

        std::cout << "Chunk range: " << minChunk.x << " " << minChunk.y << " " << minChunk.z << " to " << maxChunk.x << " " << maxChunk.y << " " << maxChunk.z << std::endl;

        int visited = 0, nonEmpty = 0, added = 0;
        for (int cx = minChunk.x; cx <= maxChunk.x; ++cx) {
            for (int cy = minChunk.y; cy <= maxChunk.y; ++cy) {
                for (int cz = minChunk.z; cz <= maxChunk.z; ++ cz) {
                    const glm::ivec3 center(cx, cy, cz);

                    glm::vec3 centerMinCorner = worldOrigin + glm::vec3(center * CHUNK_SIZE) * voxelSize;
                    glm::vec3 centerMaxCorner = centerMinCorner + float(CHUNK_SIZE) * voxelSize;
                    ++visited;

                    auto shape = cave.gather(centerMinCorner, centerMaxCorner, margin);
                    if (shape.segments.empty() && shape.caverns.empty()) { continue; }

                    Chunk* chunk = nullptr;

                    for (int x = 0; x < CHUNK_SIZE; ++x) {
                        for (int y = 0; y < CHUNK_SIZE; ++y) {
                            for (int z = 0; z < CHUNK_SIZE; ++z) {
                                glm::ivec3 local(x, y, z);
                                glm::vec3 worldPos = worldOrigin + glm::vec3(center * CHUNK_SIZE + local) * voxelSize;

                                float distance = cave.noisySDF(worldPos, shape, blend, shellThickness, voxelSize);
                                bool isWall = false;

                                if (distance >= 0.0f && distance <= shellThickness + 2.0f) {
                                    for (const auto& face : faces) {
                                        if (cave.noisySDF(worldPos + face.normal * voxelSize, shape, blend, shellThickness, voxelSize) < 0.0f) {
                                            isWall = true;
                                            break;
                                        }
                                    } 
                                }
                                if (isWall) {
                                    if (!chunk) { ++nonEmpty; chunk = &chunks.try_emplace(center, center).first->second; }
                                        chunk->addVoxel(local);
                                        ++added;
                                }
                            }
                        }
                    }
                }
            }
        }
        std::cout << visited << " chunks visited, " << nonEmpty << " with shapes, " << added << " voxels added" << std::endl;
    }

    void render(Shader& shader) {
        for (auto& [position, chunk] : chunks) {
            if (chunk.returnMeshUpdateNeeded()) {
                chunk.generateMesh(*this);
            }
            glm::vec3 chunkWorldPos = worldOrigin + glm::vec3(position * CHUNK_SIZE) * voxelSize;
            glm::mat4 model = glm::translate(glm::mat4(1.0f), chunkWorldPos);
            shader.setMat4("model", model);
            chunk.getMesh().Draw(shader);
        }
    }

    bool overlapsSolid(const glm::vec3& minC, const glm::vec3& maxC) const {
        glm::vec3 a = (minC - worldOrigin) / voxelSize;
        glm::vec3 b = (maxC - worldOrigin) / voxelSize;

        glm::ivec3 low = glm::ivec3(glm::floor(a - 0.5f)) + 1;
        glm::ivec3 high = glm::ivec3(glm::ceil(b + 0.5f)) - 1;

        for (int x = low.x; x <= high.x; ++x) {
            for (int y = low.y; y <= high.y; ++y) {
                for (int z = low.z; z <= high.z; ++z) {
                    if (checkForVoxel({x, y, z})) { return true; }
                }
            }
        }
        return false;
    }

    bool isSolidAt(const glm::vec3& pos) const { return checkForVoxel(glm::ivec3(glm::floor((pos - worldOrigin) / voxelSize + 0.5f))); }
    
    void buildAllMeshes() {
        for (auto& [position, chunk] : chunks) {
            if (chunk.returnMeshUpdateNeeded()) { chunk.generateMesh(*this); }
        }
    }

};

void Chunk::generateMesh(const VoxelWorld& world) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (const auto& [localPos, voxelType] : localVoxels) {
        glm::ivec3 worldPos = chunkPosition * CHUNK_SIZE + localPos;
        
        for (int face = 0; face < 6; face++) {
            unsigned int vertexStart = static_cast<unsigned int>(vertices.size());
            if (world.checkForVoxel(worldPos + glm::ivec3(faces[face].normal))) { 
                continue; 
            } else {
                for (int corner = 0; corner < 4; ++corner) {
                    auto solid = [&](const glm::ivec3& o) { return world.checkForVoxel(worldPos + o) ? 1 : 0; };

                    glm::ivec3 n(faces[face].normal);
                    glm::ivec3 s(glm::sign(faces[face].corners[corner]));

                    glm::ivec3 t1(0), t2(0);
                    int found = 0;
                    for (int a = 0; a < 3; ++a) { if (n[a] == 0) { (found++ == 0 ? t1 : t2)[a] = s[a]; }}

                    int a = solid(n + t1), b = solid(n + t2), c = solid(n + t1 + t2);
                    int ao = (a && b) ? 0 : 3 - (a + b + c);

                    glm::vec3 pos = (glm::vec3(localPos) + faces[face].corners[corner]) * world.getVoxelSize();
                    Vertex vertex;

                    vertex.Position = pos;
                    vertex.Color = stoneColor(worldPos, world.getVoxelSize());
                    vertex.Color *= 0.5f + 0.5f * (ao / 3.0f);
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
        }
    }

    mesh.updateMesh(vertices, indices);
    meshUpdateNeeded = false;
}

#endif