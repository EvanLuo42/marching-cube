#pragma once
#include "glm/vec3.hpp"

struct BlinnPhongVariables {
    glm::vec3 lightPos;
    float shininess = 1;
};
