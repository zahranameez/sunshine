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

int main(void)
{
    Tiles tiles;
    for (size_t i = 0; i < GRID_LENGTH_SQR; i++)
    {
        size_t col = i % GRID_LENGTH;
        size_t row = i / GRID_LENGTH;
        tiles.position[i] = { float(col * TILE_WIDTH), float(row * TILE_HEIGHT) };
    }

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
    const float playerHeight = 40.0f;

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
        const Rectangle playerRec{ playerPosition.x, playerPosition.y, playerWidth, playerHeight };

        Polygon playerPolygon = FromRectangle(playerWidth, playerHeight);
        TransformPolygon(playerPolygon, playerPosition, playerRotation * DEG2RAD);

        cceProximityTiles.clear();
        rceProximityTiles.clear();
        cceVisibilityTiles.clear();
        rceVisibilityTiles.clear();

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
        for (size_t i : cceProximityTiles)
        {
            tiles.flags[i] |= TILE_PROXIMITY_CCE;
            Vector2 tileCenter = tiles.position[i] + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };

            if (IsPolygonVisible(tileCenter, playerPosition, playerPolygon, polygons))
            {
                tiles.flags[i] |= TILE_VISIBILITY_CCE;
                cceVisibilityTiles.push_back(i);
            }
        }

        // Narrow phase rce
        for (size_t i : rceProximityTiles)
        {
            tiles.flags[i] |= TILE_PROXIMITY_RCE;
            Vector2 tileCenter = tiles.position[i] + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };

            if (IsPolygonVisible(tileCenter, playerPosition, playerPolygon, polygons))
            {
                tiles.flags[i] |= TILE_VISIBILITY_RCE;
                rceVisibilityTiles.push_back(i);
            }
        }

        vector<Vector2> cceIntersections;
        vector<Vector2> rceIntersections;
        if (IsPolygonVisible(ccePosition, playerPosition, playerPolygon, polygons))
            cceIntersections = CheckIntersectionLinePolygon(ccePosition, playerPosition, playerPolygon);
        if (IsPolygonVisible(rcePosition, playerPosition, playerPolygon, polygons))
            rceIntersections = CheckIntersectionLinePolygon(rcePosition, playerPosition, playerPolygon);

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

        // Render enemy-player intersections
        for (Vector2 poi : cceIntersections)
            DrawCircleV(poi, 10.0f, cceColor);
        for (Vector2 poi : rceIntersections)
            DrawCircleV(poi, 10.0f, rceColor);

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
