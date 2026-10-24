#include <cstdlib>
#include <vector>
#include <queue>
#include <algorithm>
#include <climits>
#include <iostream>

#include <FastNoiseLite/Cpp/FastNoiseLite.h>

using namespace std;

#define MAP_WIDTH 100
#define MAP_HEIGHT 100

int map[MAP_HEIGHT][MAP_WIDTH];
int mapBuffer[MAP_HEIGHT][MAP_WIDTH];
float heightMap[MAP_HEIGHT][MAP_WIDTH];
// int seed = time(nullptr);
int seed = 1;

FastNoiseLite noise;

struct Point {
    int y;
    int x;
};

struct TerrainRegion {
    Point center;
    float baseHeight;
    float noiseStrength;
    float noiseScale;
    int terrainSeed;

    TerrainRegion(Point c, float bH, float nSt, float nSc, int tS) {
        center = c;
        baseHeight = bH;
        noiseStrength = nSt;
        noiseScale = nSc;
        terrainSeed = tS;
    }
};

void setupNoise() {
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(0.01f);
    noise.SetSeed(seed);
}

void displayMap() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (map[y][x] == 1) {
                cout << "#";
            } else {
                cout << ".";
            }
        }
        cout << endl;
    }
}

void displayHeightMap() {
    const char* gradient = " .:-=+*#%@";
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (heightMap[y][x] == -1) {
                cout << 'X';
            } else {
                int index = static_cast<int>(heightMap[y][x] * 9);
                cout << gradient[index];
            }
        }
        cout << endl;
    }
}

void genMap() {
    srand(seed);

    // Generate initial map
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if ((rand() % 101) < 49) {
                map[y][x] = 1;
                mapBuffer[y][x] = 1;
            } else {
                map[y][x] = 0;
                mapBuffer[y][x] = 0;
            }
        }
    }

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (y == 0 || y == (MAP_HEIGHT - 1) || x == 0 || x == (MAP_HEIGHT - 1)) {
                map[y][x] = 1;
            }
        }
    }
    
}

int checkSurround(int py, int px, int check=1, int gridSize=3) {
    int count = 0;
    int itMod = gridSize -1; // modifier for modifying the iterator
    int startMod = gridSize - itMod; // modifier for modifying the starting coordinates

    for (int y = py-startMod; y < py+itMod; y++) {
        for (int x = px-startMod; x < px+itMod; x++) {
            if (!(y == py && x == px)) {
                if (y >= 0 && y < MAP_HEIGHT && x >= 0 && x < MAP_WIDTH) {
                    if (map[y][x] == check) {
                        count++;
                    }
                }
            }
        }
    }

    return count;
}

void progressGenWall() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (checkSurround(y, x) >= 5) {
                mapBuffer[y][x] = 1;
            } else if (checkSurround(y, x) < 4) {
                mapBuffer[y][x] = 0;
            }
        }
    }

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map[y][x] = mapBuffer[y][x];
        }
    }
}

void progressGenBoth() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (checkSurround(y, x) >= 5 || checkSurround(y, x, 0) <= 3) {
                mapBuffer[y][x] = 1;
            } else if (checkSurround(y, x) < 4) {
                mapBuffer[y][x] = 0;
            }
        }
    }

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map[y][x] = mapBuffer[y][x];
        }
    }
}

void caveGenCycle() {
    genMap();
    for (int i=1; i<=4; i++) {
        progressGenBoth();
    }
    for (int i=1; i<=3; i++) {
        progressGenWall();
    }
}

vector<vector<Point>> findRegions() {
    bool visited[MAP_HEIGHT][MAP_WIDTH] = {};
    vector<vector<Point>> regions;

    int dy[] = {-1, 1, 0, 0};
    int dx[] = {0, 0, -1, 1};

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {

            if (map[y][x] == 1 || visited[y][x]) {
                continue;
            }

            vector<Point> region;
            queue<Point> pending;

            pending.push({y, x});
            visited[y][x] = true;

            while (!pending.empty()) {
                Point current = pending.front();
                pending.pop();

                region.push_back(current);

                for (int d = 0; d < 4; d++) {
                    int ny = current.y + dy[d];
                    int nx = current.x + dx[d];

                    if (ny < 0 || ny >= MAP_HEIGHT || nx < 0 || nx >= MAP_WIDTH) {
                        continue;
                    } 

                    if (map[ny][nx] == 0 && !visited[ny][nx]) {
                        visited[ny][nx] = true;
                        pending.push({ny, nx});
                    }
                }
            }
            regions.push_back(region);
        }
    }
    return regions;
}

vector<vector<Point>> connectRegions() {
    vector<vector<Point>> tunnelPaths; 

    while (true) {
        vector<vector<Point>> regions = findRegions();

        if (regions.size() <= 1) {
            break;
        }

        int largest = 0;

        for (int i = 1; i < regions.size(); i++) {
            if (regions[i].size() > regions[largest].size()) {
                largest = i;
            }
        }

        int smallestDistance = INT_MAX;
        Point start = {};
        Point target = {};

        for (int i = 0; i < regions.size(); i++) {
            if (i == largest) {
                continue;
            }

            for (const Point& a : regions[largest]) {
                for (const Point& b : regions[i]) {
                    int distance = abs(a.x - b.x) + abs(a.y - b.y);

                    if (distance < smallestDistance) {
                        smallestDistance = distance;
                        start = a;
                        target = b;
                    }
                }
            }
        }

        Point current = start;
        map[current.y][current.x] = 0;
        vector<Point> tunnelPath;
        tunnelPath.push_back(start);

        while (current.x != target.x || current.y != target.y) {
            bool canMoveX = current.x != target.x;
            bool canMoveY = current.y != target.y;

            bool moveX;

            if (canMoveX && canMoveY) {
                moveX = rand() % 2 == 0;
            } else {
                moveX = canMoveX;
            }

            if (moveX) {
                if (target.x > current.x) {
                    current.x++;
                } else {
                    current.x--;
                }
            } else {
                if (target.y > current.y) {
                    current.y++;
                } else {
                    current.y--;
                }
            }

            map[current.y][current.x] = 0;
            tunnelPath.push_back(current);
        }
        tunnelPaths.push_back(tunnelPath);
    }
    return tunnelPaths;
}

vector<TerrainRegion> genTerrainRegions() {
    vector<vector<Point>> regions = findRegions();
    vector<TerrainRegion> terrainRegions;
    setupNoise();

    for (int i = 0; i < regions.size(); i++) {
        int totalX = 0;
        int totalY = 0;

        for (const Point& a : regions[i]) {
            totalX += a.x;
            totalY += a.y;
        }

        int avgX = totalX / regions[i].size();
        int avgY = totalY / regions[i].size();

        float baseHeight = 1.0f - (static_cast<float>(avgY) / (MAP_HEIGHT - 1));
        terrainRegions.push_back({{avgY, avgX}, baseHeight, 0.05f, 0.04f, seed+i});
    }
    return terrainRegions;
}

void genHeightMap() {
    vector<TerrainRegion> terrainRegions = genTerrainRegions();
    vector<vector<Point>> regions = findRegions();

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (map[y][x] == 1) {
                heightMap[y][x] = -1;
                continue;
            }
        }
    }

    for (size_t i = 0; i < regions.size(); i++) {
        const TerrainRegion& terrain = terrainRegions[i];
        FastNoiseLite noise;

        noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        noise.SetFrequency(terrain.noiseScale);
        noise.SetSeed(terrain.terrainSeed);

        for (const Point& point : regions[i]) {
            float noiseValue = noise.GetNoise(static_cast<float>(point.x - terrain.center.x), static_cast<float>(point.y - terrain.center.y));
            float height = terrain.baseHeight + noiseValue * terrain.noiseStrength;

            heightMap[point.y][point.x] = clamp(height, 0.0f, 1.0f);
        }
    }

}

void genTunnelHeights(vector<vector<Point>> tunnels) {
    vector<vector<Point>> tunnelPaths = tunnels;
    for (vector<Point> tunnelPath : tunnelPaths) {
        float startHeight = heightMap[tunnelPath[0].y][tunnelPath[0].x];
        float endHeight = heightMap[tunnelPath[tunnelPath.size() - 1].y][tunnelPath[tunnelPath.size() - 1].x];
        
        for (int i = 0; i < tunnelPath.size(); i++) {
            float t;
            if ((tunnelPath.size() - 1) == 0) {
                t = 0.0f;
            } else {
                t = static_cast<float>(i) / (tunnelPath.size() - 1);
            }

            float height = startHeight + t * (endHeight - startHeight);

            heightMap[tunnelPath[i].y][tunnelPath[i].x] = height;
        }
    }

}

void genMaps() {
    caveGenCycle();
    genHeightMap();
    vector<vector<Point>> tunnelPaths = connectRegions();
    genTunnelHeights(tunnelPaths);

}

int main() {
    genMaps();

    displayMap();
    cout << "\n\nHeight Map:\n" << endl;
    displayHeightMap();    
    return 0;
}