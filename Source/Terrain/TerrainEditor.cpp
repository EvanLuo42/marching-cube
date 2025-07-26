#include "TerrainEditor.h"

#include "imgui.h"

void TerrainEditor::update(float deltaTime) {
    if (newVoxelScale != marchingCube.voxelScale) {
        rebuild();
        marchingCube.voxelScale = newVoxelScale;
    }
}

void TerrainEditor::renderUI() {
    ImGui::Begin("Terrain Editor Settings");
    ImGui::SliderFloat("Voxel Scale", &newVoxelScale, 0.0f, 2.0f);
    ImGui::End();
}

void TerrainEditor::rebuild() {
    meshData = marchingCube.polygonize();
    toggleEdited();
}

const Triangles &TerrainEditor::getMesh() const {
    return meshData;
}
