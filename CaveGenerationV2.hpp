#ifndef CAVEGENERATIONV2_HPP
#define CAVEGENERATIONV2_HPP

#include <FastNoiseLite/Cpp/FastNoiseLite.h>
#include <array>
#include <ctime>

#define WORLD_WIDTH 100
#define WORLD_HEIGHT 50
#define WORLD_DEPTH 100

float minValue = 1;
float maxValue = -1;

using WorldHeightMap = std::array<std::array<std::array<int, WORLD_DEPTH>, WORLD_HEIGHT>, WORLD_WIDTH>;

WorldHeightMap world{};
FastNoiseLite noise;
int seed = time(nullptr);

void setupNoise() {
    noise.SetSeed(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(0.02f);

    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3);
    noise.SetFractalLacunarity(2.0f);
    noise.SetFractalGain(0.5f);
}

WorldHeightMap generateWorld() {
    setupNoise();
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            for (int z = 0; z < WORLD_DEPTH; z++) {
                float value = noise.GetNoise(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));

                minValue = std::min(minValue, value);
                maxValue = std::max(maxValue, value);

                if (value > 0.0f){
                    world[x][y][z] = 1;                
                } else {
                    world[x][y][z] = 0;
                }
            }   
        }
    }
    std::cout << "MinValue: " << minValue << std::endl;
    std::cout << "MaxValue: " << maxValue << std::endl;

    return world;
}

#endif