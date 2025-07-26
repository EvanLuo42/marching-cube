#pragma once
#include <vector>

#include "../Render/Vertex.h"
#include "Voxel.h"

struct Triangles {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};

class MarchingCube {
public:
    constexpr static int gridX = 64;
    constexpr static int gridY = 16;
    constexpr static int gridZ = 64;

    float isoLevel = 0.5f;
    float voxelScale = 0.25f;
    std::vector<std::vector<std::vector<Voxel>>> voxelGrid;

    MarchingCube() {
        voxelGrid.resize(gridX, std::vector(
        gridY, std::vector(gridZ, Voxel{0.0f})));
    }

    void generateDensitySphere(glm::vec3 center, float radius, float density);
    [[nodiscard]] Triangles polygonize() const;

private:
    uint8_t computeCubeIndex(const float densities[8]) const;
    static glm::vec3 interpolateVertex(float iso, glm::vec3 p1, glm::vec3 p2, float val1, float val2);

    static glm::vec2 generateUV(const glm::vec3 &pos, float uvScale = 1.0f);
};

#define FOREACH_VOXEL(X, Y, Z) \
    for (int X = 0; X < MarchingCube::gridX; ++X) \
    for (int Y = 0; Y < MarchingCube::gridY; ++Y) \
    for (int Z = 0; Z < MarchingCube::gridZ; ++Z)

#define FOREACH_VOXEL_1(X, Y, Z) \
    for (int X = 0; X < MarchingCube::gridX - 1; ++X) \
    for (int Y = 0; Y < MarchingCube::gridY - 1; ++Y) \
    for (int Z = 0; Z < MarchingCube::gridZ - 1; ++Z)

#define VERT(i, j) interpolateVertex(isoLevel, cubePos[i], cubePos[j], cubeVal[i], cubeVal[j])
