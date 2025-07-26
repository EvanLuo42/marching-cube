#pragma once
#include "MarchingCube.h"

class TerrainEditor {
public:
    TerrainEditor() {
        marchingCube.generateDensitySphere(glm::vec3(0, 0, 0), 6.0f, 1.0f);
        rebuild();
    }

    void update(float deltaTime);
    void renderUI();
    void rebuild();
    [[nodiscard]] const Triangles &getMesh() const;

    void toggleEdited() { edited = !edited; }

    [[nodiscard]] bool isEdited() const {
        return edited;
    }

private:
    MarchingCube marchingCube;
    Triangles meshData;
    bool edited = false;

    float newVoxelScale = 0.25f;
};
