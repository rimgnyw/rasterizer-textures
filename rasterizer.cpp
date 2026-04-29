#include <iostream>
#include <glm/glm.hpp>
#include "SDL2Auxiliary.h"
#include "TestModel.h"
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;
using glm::ivec2;
using glm::mat3;
using glm::vec2;
using glm::vec3;

// ----------------------------------------------------------------------------
// STRUCTURES

struct Pixel {
    int x;
    int y;
    float zinv;
    vec3 pos3d;
    vec2 uv;
};

struct Vertex {
    vec3 position;
    vec2 uv;
};

struct Texture {
    int width;
    int height;
    unsigned char *data;
};

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL2Aux *sdlAux;
int t;
vector<Triangle> triangles;

float focalLength = 687;
vec3 cameraPos(0, 0, -3.5);
mat3 R;
float yaw;

vec3 currentColor;

float depthBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

vec3 lightPos(0, -0.5, -0.7);
vec3 lightPower = 14.f * vec3(1, 1, 1);
vec3 indirectLightPowerPerArea = 0.5f * vec3(1, 1, 1);

vec3 currentNormal;
vec3 currentReflectance;

Texture currentTexture;

// ----------------------------------------------------------------------------
// FUNCTIONS

void Update(void);
void Draw(void);
void DrawPolygon(const vector<vec3> &vertices);

void Interpolate(Pixel a, Pixel b, vector<Pixel> &result);
void ComputePolygonRows(const vector<Pixel> &vertexPixels, vector<Pixel> &leftPixels,
                        vector<Pixel> &rightPixels);
void DrawPolygonRows(const vector<Pixel> &leftPixels, const vector<Pixel> &rightPixels);
void VertexShader(const vec3 &v, Pixel &p);

void PixelShader(const Pixel &p);

void DrawPolygon(const vector<Vertex> &vertices);
void VertexShader(const Vertex &v, Pixel &p);

void LoadImage(const char *filename, Texture &texture);
void FreeImage(unsigned char *data);
int GetImageBufferIndex(int x, int y, int width);
vec3 GetImageColor(const Texture &texture, int index);
int GetImageBufferIndexFromUV(float u, float v, int width, int height);

int main(int argc, char *argv[]) {

    LoadImage("../test.jpg", currentTexture);

    // LoadTestModel(triangles); // Load model

    // Triangle 1
    triangles.push_back(Triangle(vec3(-1, -1, 0), vec3(1, -1, 0), vec3(-1, 1, 0), vec2(0, 0),
                                 vec2(1, 0), vec2(0, 1), vec3(1, 1, 1)));

    // Triangle 2
    triangles.push_back(Triangle(vec3(1, -1, 0), vec3(1, 1, 0), vec3(-1, 1, 0), vec2(1, 0),
                                 vec2(1, 1), vec2(0, 1), vec3(1, 1, 1)));
    sdlAux = new SDL2Aux(SCREEN_WIDTH, SCREEN_HEIGHT);
    t = SDL_GetTicks(); // Set start value for timer.

    while (!sdlAux->quitEvent()) {
        Update();
        Draw();
    }
    sdlAux->saveBMP("screenshot.bmp");
    return 0;
}

void Update(void) {
    // Compute frame time:
    int t2 = SDL_GetTicks();
    float dt = float(t2 - t);
    t = t2;
    cout << "Render time: " << dt << " ms." << endl;

    R[0][0] = glm::cos(yaw);
    R[0][2] = glm::sin(yaw);
    R[2][0] = -glm::sin(yaw);
    R[2][2] = glm::cos(yaw);

    vec3 forward(R[2][0], R[2][1], R[2][2]);

    const Uint8 *keystate = SDL_GetKeyboardState(NULL);
    if (keystate[SDL_SCANCODE_UP]) {
        cameraPos += 0.1f * forward;
    }
    if (keystate[SDL_SCANCODE_DOWN]) {
        cameraPos -= 0.1f * forward;
    }
    if (keystate[SDL_SCANCODE_LEFT]) {
        yaw += 0.05;
    }
    if (keystate[SDL_SCANCODE_RIGHT]) {
        yaw -= 0.05;
    }
    if (keystate[SDL_SCANCODE_W]) {
    }
    if (keystate[SDL_SCANCODE_S]) {
    }
    if (keystate[SDL_SCANCODE_A]) {
    }
    if (keystate[SDL_SCANCODE_D]) {
    }
    if (keystate[SDL_SCANCODE_Q]) {
    }
    if (keystate[SDL_SCANCODE_E]) {
    }
}

void Draw() {
    sdlAux->clearPixels();
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        for (int x = 0; x < SCREEN_WIDTH; ++x)
            depthBuffer[y][x] = 0;

    Texture texture;
    LoadImage("../test.jpg", texture);
    for (int i = 0; i < texture.width; i++) {
        for (int j = 0; j < texture.height; j++) {
            int index = GetImageBufferIndex(i, j, texture.width);
            unsigned char r = texture.data[index];
            unsigned char g = texture.data[index + 1];
            unsigned char b = texture.data[index + 2];
            sdlAux->putPixel(i, j, vec3(r / 255.f, g / 255.f, b / 255.f));
        }
    }
    FreeImage(texture.data);

    for (size_t i = 0; i < triangles.size(); ++i) {
        vector<Vertex> vertices(3);

        currentColor = triangles[i].color;
        vertices[0].position = triangles[i].v0;
        vertices[1].position = triangles[i].v1;
        vertices[2].position = triangles[i].v2;

        vertices[0].uv = triangles[i].uv0;
        vertices[1].uv = triangles[i].uv1;
        vertices[2].uv = triangles[i].uv2;

        currentNormal = triangles[i].normal;
        currentReflectance = triangles[i].color;

        DrawPolygon(vertices);
    }

    sdlAux->render();
}

void VertexShader(const Vertex &v, Pixel &p) {
    vec3 translatedPosition = v.position - cameraPos;
    vec3 rotatedPosition = (translatedPosition)*R;

    p.x = focalLength * rotatedPosition.x / rotatedPosition.z + SCREEN_WIDTH / 2;
    p.y = focalLength * rotatedPosition.y / rotatedPosition.z + SCREEN_HEIGHT / 2;
    p.zinv = 1 / rotatedPosition.z;

    p.pos3d = v.position * p.zinv;
    p.uv = v.uv;
}

void Interpolate(Pixel a, Pixel b, vector<Pixel> &result) {
    int N = result.size();

    float step_x = (b.x - a.x) / float(max(N - 1, 1));
    float step_y = (b.y - a.y) / float(max(N - 1, 1));
    float step_zinv = (b.zinv - a.zinv) / float(max(N - 1, 1));
    vec3 step_pos3d = (b.pos3d - a.pos3d) / float(max(N - 1, 1));
    vec2 step_uv = (b.uv - a.uv) / float(max(N - 1, 1));

    for (int i = 0; i < N; i++) {
        float current_x = a.x + i * step_x;
        float current_y = a.y + i * step_y;
        float current_zinv = a.zinv + i * step_zinv;
        vec3 current_pos3d = a.pos3d + float(i) * step_pos3d;
        vec2 current_uv = a.uv + float(i) * step_uv;

        Pixel p = {current_x, current_y, current_zinv, current_pos3d, current_uv};
        result[i] = p;
    }
}

void ComputePolygonRows(const vector<Pixel> &vertexPixels, vector<Pixel> &leftPixels,
                        vector<Pixel> &rightPixels) {

    int maxY = -numeric_limits<int>::max();
    int minY = numeric_limits<int>::max();
    for (Pixel vertice : vertexPixels) {
        if (vertice.y > maxY)
            maxY = vertice.y;
        if (vertice.y < minY)
            minY = vertice.y;
    }

    int numRows = maxY - minY + 1;

    leftPixels.resize(numRows);
    rightPixels.resize(numRows);

    for (int i = 0; i < numRows; i++) {
        leftPixels[i].x = numeric_limits<int>::max();
        rightPixels[i].x = -numeric_limits<int>::max();
    }

    for (int i = 0; i < 3; i++) {
        Pixel a = vertexPixels[i];
        Pixel b = vertexPixels[(i + 1) % 3];

        int N = max(abs(b.x - a.x), abs(b.y - a.y)) + 1;
        vector<Pixel> edgeLine(N);
        Interpolate(a, b, edgeLine);

        for (Pixel linePoint : edgeLine) {
            int row = linePoint.y - minY;
            if (row < 0 || row >= numRows)
                continue;

            if (linePoint.x < leftPixels[row].x) {
                leftPixels[row] = linePoint;
            }
            if (linePoint.x > rightPixels[row].x) {
                rightPixels[row] = linePoint;
            }
        }
    }
}

void DrawPolygonRows(const vector<Pixel> &leftPixels, const vector<Pixel> &rightPixels) {
    for (int i = 0; i < leftPixels.size(); i++) {
        Pixel leftPoint = leftPixels[i];
        Pixel rightPoint = rightPixels[i];

        int x0 = leftPoint.x;
        int x1 = rightPoint.x;
        int y = leftPoint.y;

        if (y < 0 || y >= SCREEN_HEIGHT)
            continue;

        if (x0 > x1)
            swap(x0, x1);

        int N = x1 - x0 + 1;

        vector<Pixel> result(N);
        Interpolate(leftPoint, rightPoint, result);

        for (Pixel p : result) {
            if (p.x < 0 || p.x >= SCREEN_WIDTH)
                continue;

            p.pos3d /= p.zinv;
            PixelShader(p);
        }
    }
}

void DrawPolygon(const vector<Vertex> &vertices) {
    int V = vertices.size();

    vector<Pixel> vertexPixels(V);

    for (int i = 0; i < V; ++i)
        VertexShader(vertices[i], vertexPixels[i]);

    vector<Pixel> leftPixels;
    vector<Pixel> rightPixels;

    ComputePolygonRows(vertexPixels, leftPixels, rightPixels);

    DrawPolygonRows(leftPixels, rightPixels);
}

void PixelShader(const Pixel &p) {
    int x = p.x;
    int y = p.y;

    vec3 lightDirection = lightPos - p.pos3d;
    float radius = glm::length(lightDirection);
    float area = 4 * 3.14 * radius * radius;
    vec3 n_hat = currentNormal;
    vec3 r_hat = glm::normalize(lightDirection);

    vec3 D = lightPower * max(0.f, glm::dot(n_hat, r_hat)) / area;

    // vec3 illumnination = currentReflectance * (D + indirectLightPowerPerArea);
    vec3 illumnination =
        GetImageColor(currentTexture,
                      GetImageBufferIndexFromUV(p.uv.x, p.uv.y, currentTexture.width,
                                                currentTexture.height)) *
        (D + indirectLightPowerPerArea);

    if (p.zinv > depthBuffer[y][x]) {
        depthBuffer[y][x] = p.zinv;
        sdlAux->putPixel(x, y, illumnination);
    }
}

void LoadImage(const char *filename, Texture &texture) {
    texture.data = stbi_load(filename, &texture.width, &texture.height, NULL, 3);
    if (texture.data == NULL) {
        cerr << "Error loading texture file " << filename << endl;
        exit(1);
    }
}

void FreeImage(unsigned char *data) { stbi_image_free(data); }

int GetImageBufferIndex(int x, int y, int width) { return (y * width + x) * 3; }

int GetImageBufferIndexFromUV(float u, float v, int width, int height) {
    int x = u * (width - 1);
    int y = v * (height - 1);

    return (y * width + x) * 3;
}

vec3 GetImageColor(const Texture &texture, int index) {
    unsigned char r = texture.data[index];
    unsigned char g = texture.data[index + 1];
    unsigned char b = texture.data[index + 2];
    return vec3(r / 255.f, g / 255.f, b / 255.f);
}
// int width, height, channels;

// unsigned char *img = stbi_load("test.png", &width, &height, &channels, 0);

// if (img == NULL) {
//     printf("Failed to load image\n");
//     return 1;
// }

// printf("Loaded image!\n");
// printf("Width: %d, Height: %d, Channels: %d\n", width, height, channels);

// stbi_image_free(img);
// return 0;
