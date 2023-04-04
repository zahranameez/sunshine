#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

#define TILE_FLAGS_NONE 0
#define TILE_VISIBILITY_CCE 1
#define TILE_VISIBILITY_RCE 2
#define TILE_PROXIMITY_CCE 4
#define TILE_PROXIMITY_RCE 8

constexpr int GRID_LENGTH = 80;
constexpr int GRID_LENGTH_SQR = GRID_LENGTH * GRID_LENGTH;
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TILE_WIDTH = SCREEN_WIDTH / GRID_LENGTH;
constexpr int TILE_HEIGHT = SCREEN_HEIGHT / GRID_LENGTH;

struct Tiles
{
    array<Vector2, GRID_LENGTH_SQR> position{};
    array<int, GRID_LENGTH_SQR> flags{};
};

void TransformPolygon(Polygon& polygon, Vector2 translation, float rotation)
{
    for (Vector2& point : polygon)
        point = Rotate(point, rotation) + translation;
}

Polygon FromRectangle(int width, int height)
{
    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;

    Polygon polygon(4);
    polygon[0] = { -halfWidth, -halfHeight };   // top left
    polygon[1] = { halfWidth, -halfHeight };    // top right
    polygon[2] = { halfWidth, halfHeight };     // bot right
    polygon[3] = { -halfWidth, halfHeight };    // bot left

    return polygon;
}

void NarrowPhase(Vector2 position, const Polygon& polygon,
    const vector<Polygon>& polygons, Tiles& tiles,
    const vector<size_t>& proximityTiles, vector<size_t>& visibilityTiles,
    int proximityFlag, int visibilityFlag)
{
    for (size_t i : proximityTiles)
    {
        tiles.flags[i] |= proximityFlag;
        Vector2 tileCenter = tiles.position[i] + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };

        bool visible = IsPointVisible(tileCenter, position, polygons);
        // These checks are too costly. Visibility is guaranteed if line from enemy to player center.
        //for (Vector2 point : polygon)
        //    visible = visible || IsCircleVisible(tileCenter, point, { point, 5.0f }, polygons);
        //visible = visible || IsPolygonVisible(tileCenter, position, polygon, polygons);

        if (visible)
        {
            tiles.flags[i] |= visibilityFlag;
            visibilityTiles.push_back(i);
        }
    }
}

size_t ScreenToGrid(Vector2 point)
{
    size_t col = point.x / TILE_WIDTH;
    size_t row = point.y / TILE_HEIGHT;
    return row * GRID_LENGTH + col;
}

Vector2 GridToScreen(size_t index)
{
    size_t col = index % GRID_LENGTH;
    size_t row = index / GRID_LENGTH;
    return { float(col * TILE_WIDTH), float(row * TILE_HEIGHT) };
}

// Efficient
vector<size_t> GridIndices(Rectangle rectangle)
{
    const size_t colMin = rectangle.x / TILE_WIDTH;
    const size_t colMax = (rectangle.x + rectangle.width) / TILE_WIDTH;
    const size_t rowMin = rectangle.y / TILE_HEIGHT;
    const size_t rowMax = (rectangle.y + rectangle.height) / TILE_HEIGHT;
    const size_t width = colMax - colMin;
    const size_t height = rowMax - rowMin;

    vector<size_t> indices;
    indices.reserve(width * height);
    for (size_t row = rowMin; row < rowMax; row++)
    {
        for (size_t col = colMin; col < colMax; col++)
        {
            indices.push_back(row * GRID_LENGTH + col);
        }
    }
    return indices;
}

// Inefficient
vector<size_t> GridIndices(Circle circle)
{
    float length = circle.radius + circle.radius;
    Rectangle rec{ circle.position.x - circle.radius, circle.position.y - circle.radius, length, length };
    vector<size_t> recIndices = GridIndices(rec);
    vector<size_t> circleIndices;
    circleIndices.reserve(recIndices.size());
    for (size_t i : recIndices)
    {
        Vector2 tileCenter = GridToScreen(i) + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };
        if (CheckCollisionPointCircle(tileCenter, circle)) circleIndices.push_back(i);
    }
    return circleIndices;
}

int main(void)
{
    Tiles tiles;
    for (size_t i = 0; i < GRID_LENGTH_SQR; i++)
        tiles.position[i] = GridToScreen(i);

    vector<Polygon> polygons;
    std::ifstream inFile("../game/assets/data/obstacles.txt");
    while (!inFile.eof())
    {
        Rectangle obstacle;
        inFile >> obstacle.x >> obstacle.y >> obstacle.width >> obstacle.height;
        Vector2 position{ obstacle.x + obstacle.width * 0.5f, obstacle.y + obstacle.height * 0.5f };
        Polygon polygon = FromRectangle(obstacle.width, obstacle.height);
        TransformPolygon(polygon, position, 0.0f);
        polygons.push_back(std::move(polygon));
    }
    inFile.close();

    Vector2 playerPosition = Vector2Zero();
    float playerRotation = 0.0f;
    const float playerWidth = 60.0f;
    const float playerHeight = 60.0f;
    const float pointSize = 5.0f;

    // Enemy rotation not in use since they do not yet move
    //float cceRotation = 45.0f;
    //float rceRotation = 45.0f;
    Vector2 ccePosition{ 1000.0f, 250.0f };
    Vector2 rcePosition{ 1000.0f, 650.0f };
    const float enemyRenderRadius = 50.0f;
    const float enemySensorRadius = 100.0f;

    const Color playerColor = ORANGE;
    const Color cceColor = RED;
    const Color rceColor = BLUE;
    const Color background = RAYWHITE;

    // Pre-allocating to reduce lag
    vector<size_t> cceProximityTiles(GRID_LENGTH_SQR);
    vector<size_t> rceProximityTiles(GRID_LENGTH_SQR);
    vector<size_t> cceVisibilityTiles(GRID_LENGTH_SQR);
    vector<size_t> rceVisibilityTiles(GRID_LENGTH_SQR);

    bool useHeatmap = true;
    bool useGUI = false;
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sunshine");
    rlImGuiSetup(true);
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        if (IsKeyDown(KEY_E))
            playerRotation += 100.0f * dt;
        if (IsKeyDown(KEY_Q))
            playerRotation -= 100.0f * dt;

        playerPosition = GetMousePosition();
        const Vector2 playerDirection = Direction(playerRotation * DEG2RAD);
        const Vector2 playerEnd = playerPosition + playerDirection * 500.0f;
        const Rectangle playerRec
        {
            playerPosition.x - playerWidth * 0.5f, playerPosition.y - playerHeight * 0.5f,
            playerWidth, playerHeight
        };

        Polygon playerPolygon = FromRectangle(playerWidth, playerHeight);
        TransformPolygon(playerPolygon, playerPosition, playerRotation * DEG2RAD);

        cceProximityTiles.clear();
        rceProximityTiles.clear();
        cceVisibilityTiles.clear();
        rceVisibilityTiles.clear();

        vector<size_t> playerRecIndices = GridIndices(playerRec);
        vector<size_t> playerCircleIndices = GridIndices({ playerPosition, 30.0f});

        // Broad phase
        for (size_t i = 0; i < GRID_LENGTH_SQR; i++)
        {
            tiles.flags[i] = TILE_FLAGS_NONE;
            Vector2 tileCenter = tiles.position[i] + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };

            if (CheckCollisionPointCircle(tileCenter, { ccePosition, enemySensorRadius }))
                cceProximityTiles.push_back(i);

            if (CheckCollisionPointCircle(tileCenter, { rcePosition, enemySensorRadius }))
                rceProximityTiles.push_back(i);
        }

        // Narrow phase cce
        NarrowPhase(playerPosition, playerPolygon, polygons, tiles,
            cceProximityTiles, cceVisibilityTiles, TILE_PROXIMITY_CCE, TILE_VISIBILITY_CCE);

        // Narrow phase rce
        NarrowPhase(playerPosition, playerPolygon, polygons, tiles,
            rceProximityTiles, rceVisibilityTiles, TILE_PROXIMITY_RCE, TILE_VISIBILITY_RCE);

        BeginDrawing();
        ClearBackground(background);

        // Render tiles
        if (useHeatmap)
        {
            for (size_t i = 0; i < GRID_LENGTH_SQR; i++)
            {
                Color color = BLACK;
                const int flag = tiles.flags[i];
                const unsigned char intensity = 64;

                // Increment color based on flags; cce = red, rce = blue.
                // (Not using else because I want visibility tiles to be 2x the brightness of proximity tiles).
                if (flag & TILE_PROXIMITY_CCE) color.r += intensity;
                if (flag & TILE_PROXIMITY_RCE) color.b += intensity;
                if (flag & TILE_VISIBILITY_CCE) color.r += intensity;
                if (flag & TILE_VISIBILITY_RCE) color.b += intensity;
                if (flag == TILE_FLAGS_NONE) color = background;
                DrawRectangleV(tiles.position[i], { TILE_WIDTH, TILE_HEIGHT }, color);
            }
        }

        // Render entities
        DrawCircleV(ccePosition, enemyRenderRadius, cceColor);
        DrawCircleV(rcePosition, enemyRenderRadius, rceColor);
        DrawLineV(playerPosition, playerEnd, playerColor);
        DrawPolygon(playerPolygon, playerColor);
        for (size_t i : playerRecIndices)
            DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, RED);
        for (size_t i : playerCircleIndices)
            DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, BLUE);

        // Render map intersections
        Vector2 levelPoi;
        if (NearestIntersection(playerPosition, playerEnd, polygons, levelPoi))
            DrawCircleV(levelPoi, 10.0f, GRAY);

        // Render map
        for (const Polygon& polygon : polygons)
            DrawPolygon(polygon, GRAY);
        
        // Render GUI
        if (IsKeyPressed(KEY_GRAVE)) useGUI = !useGUI;
        if (useGUI)
        {
            rlImGuiBegin();
            ImGui::Checkbox("Use heatmap", &useHeatmap);
            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
