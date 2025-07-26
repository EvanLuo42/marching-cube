#include "MarchingCube.h"

#include "MarchingTables.h"

void MarchingCube::generateDensitySphere(const glm::vec3 center, const float radius, const float density) {
    FOREACH_VOXEL(x, y, z) {
        auto position = glm::vec3{x, y, z} * voxelScale;
        if (const auto distance = glm::distance(center, position); distance < radius) {
            voxelGrid[x][y][z].density = density * (1.0f - distance / radius);
        }
    }
}

Triangles MarchingCube::polygonize() const {
    Triangles result;
    uint32_t indexOffset = 0;

    FOREACH_VOXEL_1(x, y, z) {
        const glm::vec3 basePos = glm::vec3(x, y, z) * voxelScale;

        const glm::vec3 cubePos[8] = {
                basePos + glm::vec3(0, 0, 0) * voxelScale, basePos + glm::vec3(1, 0, 0) * voxelScale,
                basePos + glm::vec3(1, 0, 1) * voxelScale, basePos + glm::vec3(0, 0, 1) * voxelScale,
                basePos + glm::vec3(0, 1, 0) * voxelScale, basePos + glm::vec3(1, 1, 0) * voxelScale,
                basePos + glm::vec3(1, 1, 1) * voxelScale, basePos + glm::vec3(0, 1, 1) * voxelScale,
        };

        const float cubeVal[8] = {voxelGrid[x][y][z].density,
                                  voxelGrid[x + 1][y][z].density,
                                  voxelGrid[x + 1][y][z + 1].density,
                                  voxelGrid[x][y][z + 1].density,
                                  voxelGrid[x][y + 1][z].density,
                                  voxelGrid[x + 1][y + 1][z].density,
                                  voxelGrid[x + 1][y + 1][z + 1].density,
                                  voxelGrid[x][y + 1][z + 1].density};

        const auto cubeIndex = computeCubeIndex(cubeVal);
        if (edgeTable[cubeIndex] == 0)
            continue;

        glm::vec3 edgeVertex[12];
        if (edgeTable[cubeIndex] & 1)
            edgeVertex[0] = VERT(0, 1);
        if (edgeTable[cubeIndex] & 2)
            edgeVertex[1] = VERT(1, 2);
        if (edgeTable[cubeIndex] & 4)
            edgeVertex[2] = VERT(2, 3);
        if (edgeTable[cubeIndex] & 8)
            edgeVertex[3] = VERT(3, 0);
        if (edgeTable[cubeIndex] & 16)
            edgeVertex[4] = VERT(4, 5);
        if (edgeTable[cubeIndex] & 32)
            edgeVertex[5] = VERT(5, 6);
        if (edgeTable[cubeIndex] & 64)
            edgeVertex[6] = VERT(6, 7);
        if (edgeTable[cubeIndex] & 128)
            edgeVertex[7] = VERT(7, 4);
        if (edgeTable[cubeIndex] & 256)
            edgeVertex[8] = VERT(0, 4);
        if (edgeTable[cubeIndex] & 512)
            edgeVertex[9] = VERT(1, 5);
        if (edgeTable[cubeIndex] & 1024)
            edgeVertex[10] = VERT(2, 6);
        if (edgeTable[cubeIndex] & 2048)
            edgeVertex[11] = VERT(3, 7);

        for (int i = 0; triTable[cubeIndex][i] != -1; i += 3) {
            glm::vec3 p0 = edgeVertex[triTable[cubeIndex][i + 0]];
            glm::vec3 p1 = edgeVertex[triTable[cubeIndex][i + 1]];
            glm::vec3 p2 = edgeVertex[triTable[cubeIndex][i + 2]];

            const glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));

            result.vertices.push_back({p0, generateUV(p0), normal});
            result.vertices.push_back({p1, generateUV(p1), normal});
            result.vertices.push_back({p2, generateUV(p2), normal});
            result.indices.push_back(indexOffset++);
            result.indices.push_back(indexOffset++);
            result.indices.push_back(indexOffset++);
        }
    }

    return result;
}

uint8_t MarchingCube::computeCubeIndex(const float densities[8]) const {
    uint8_t index = 0;
    for (int i = 0; i < 8; ++i)
        if (densities[i] < isoLevel)
            index |= 1 << i;
    return index;
}

glm::vec3 MarchingCube::interpolateVertex(const float iso, const glm::vec3 p1, const glm::vec3 p2, const float val1,
                                          const float val2) {
    if (std::abs(iso - val1) < 1e-6f)
        return p1;
    if (std::abs(iso - val2) < 1e-6f)
        return p2;
    if (std::abs(val1 - val2) < 1e-6f)
        return p1;
    const float t = (iso - val1) / (val2 - val1);
    return glm::mix(p1, p2, t);
}

glm::vec2 MarchingCube::generateUV(const glm::vec3 &pos, const float uvScale) {
    return glm::vec2(pos.x, pos.z) * uvScale;
}
