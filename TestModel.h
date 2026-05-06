#ifndef TEST_MODEL_CORNEL_BOX_H
#define TEST_MODEL_CORNEL_BOX_H

// Defines a simple test model: The Cornel Box

#include <glm/glm.hpp>
#include <vector>

// Used to describe a triangular surface:
class Triangle {
  public:
    glm::vec3 v0;
    glm::vec3 v1;
    glm::vec3 v2;
    glm::vec2 uv0;
    glm::vec2 uv1;
    glm::vec2 uv2;
    glm::vec3 normal;
    glm::vec3 color;

    Triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2,
             glm::vec3 color)
        : v0(v0), v1(v1), v2(v2), uv0(uv0), uv1(uv1), uv2(uv2), color(color) {
        ComputeNormal();
    }
    // Triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 color)
    //     : v0(v0), v1(v1), v2(v2), color(color) {
    //     ComputeNormal();
    // }

    void ComputeNormal() {
        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;
        normal = glm::normalize(glm::cross(e2, e1));
    }
};
// Helper for adding a square (two triangles) to the model
void AddSquare(glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 D, glm::vec3 color,
               std::vector<Triangle> &triangles) {
    glm::vec2 uvB(0, 1);
    glm::vec2 uvA(1, 1);
    glm::vec2 uvD(0, 0);
    glm::vec2 uvC(1, 0);

    triangles.emplace_back(A, B, C, uvA, uvB, uvC, color);
    triangles.emplace_back(B, D, C, uvB, uvD, uvC, color);
};

// Loads the Cornell Box. It is scaled to fill the volume:
// -1 <= x <= +1
// -1 <= y <= +1
// -1 <= z <= +1
void LoadTestModel(std::vector<Triangle> &triangles) {
    using glm::vec3;

    // Defines colors:
    vec3 red(0.75f, 0.15f, 0.15f);
    vec3 yellow(0.75f, 0.75f, 0.15f);
    vec3 green(0.15f, 0.75f, 0.15f);
    vec3 cyan(0.15f, 0.75f, 0.75f);
    vec3 blue(0.15f, 0.15f, 0.75f);
    vec3 purple(0.75f, 0.15f, 0.75f);
    vec3 white(0.75f, 0.75f, 0.75f);

    triangles.clear();

    // ---------------------------------------------------------------------------
    // Room

    float L = 555; // Length of Cornell Box side.

    vec3 A(L, 0, 0);
    vec3 B(0, 0, 0);
    vec3 C(L, 0, L);
    vec3 D(0, 0, L);

    vec3 E(L, L, 0);
    vec3 F(0, L, 0);
    vec3 G(L, L, L);
    vec3 H(0, L, L);

    // Floor
    AddSquare(C, D, A, B, green, triangles);

    // Ceiling
    AddSquare(E, F, G, H, cyan, triangles);

    // Back wall
    AddSquare(D, C, H, G, white, triangles);

    // Left wall
    AddSquare(B, D, F, H, purple, triangles);

    // Right wall
    AddSquare(C, A, G, E, yellow, triangles);

    // ---------------------------------------------------------------------------
    // Short block

    A = vec3(290, 0, 114);
    B = vec3(130, 0, 65);
    C = vec3(240, 0, 272);
    D = vec3(82, 0, 225);

    E = vec3(290, 165, 114);
    F = vec3(130, 165, 65);
    G = vec3(240, 165, 272);
    H = vec3(82, 165, 225);

    // Front
    AddSquare(A, B, E, F, red, triangles);

    // Front
    AddSquare(B, D, F, H, red, triangles);

    // BACK
    AddSquare(D, C, H, G, red, triangles);

    // LEFT
    AddSquare(C, A, G, E, red, triangles);

    // TOP
    AddSquare(F, E, H, G, red, triangles);

    // ----------------------------------------------
    // Scale to the volume [-1,1]^3

    for (size_t i = 0; i < triangles.size(); ++i) {
        triangles[i].v0 *= 2 / L;
        triangles[i].v1 *= 2 / L;
        triangles[i].v2 *= 2 / L;

        triangles[i].v0 -= vec3(1, 1, 1);
        triangles[i].v1 -= vec3(1, 1, 1);
        triangles[i].v2 -= vec3(1, 1, 1);

        triangles[i].v0.x *= -1;
        triangles[i].v1.x *= -1;
        triangles[i].v2.x *= -1;

        triangles[i].v0.y *= -1;
        triangles[i].v1.y *= -1;
        triangles[i].v2.y *= -1;

        triangles[i].ComputeNormal();
    }
}

#endif