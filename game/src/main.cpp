#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

constexpr int GRID_LENGTH = 80;
constexpr int GRID_LENGTH_SQR = GRID_LENGTH * GRID_LENGTH;
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TILE_WIDTH = SCREEN_WIDTH / GRID_LENGTH;
constexpr int TILE_HEIGHT = SCREEN_HEIGHT / GRID_LENGTH;

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

vector<size_t> VisibilityIndices(Circle player, const vector<Polygon>& obstacles, const vector<size_t>& proximityTiles)
{
    vector<size_t> visibilityTiles;
    visibilityTiles.reserve(proximityTiles.size());
    for (size_t i : proximityTiles)
    {
        Vector2 tileCenter = GridToScreen(i) + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };
        if (IsCircleVisible(tileCenter, player.position, player, obstacles)) visibilityTiles.push_back(i);
    }
    return visibilityTiles;
}

int main(void)
{
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
    const float playerRadius = 30.0f;

    Vector2 ccePosition{ 1000.0f, 250.0f };
    Vector2 rcePosition{ 1000.0f, 650.0f };
    const float enemyRadius = 50.0f;

    const Color playerColor = GREEN;
    const Color cceColor = BLUE;
    const Color rceColor = VIOLET;
    const Color background = RAYWHITE;

    bool useHeatmap = true;
    bool useGUI = false;
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sunshine");
    rlImGuiSetup(true);
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        // Update player information
        float dt = GetFrameTime();
        if (IsKeyDown(KEY_E))
            playerRotation += 100.0f * dt;
        if (IsKeyDown(KEY_Q))
            playerRotation -= 100.0f * dt;

        playerPosition = GetMousePosition();
        const Vector2 playerDirection = Direction(playerRotation * DEG2RAD);
        const Vector2 playerEnd = playerPosition + playerDirection * 500.0f;
        const Circle playerCircle{ playerPosition, playerRadius };
        Polygon playerPolygon = FromRectangle(playerRadius * 2.0f, playerRadius * 2.0f);
        TransformPolygon(playerPolygon, playerPosition, playerRotation * DEG2RAD);

        // Update enemy information
        Rectangle cceRec{ ccePosition.x - enemyRadius, ccePosition.y - enemyRadius,
            enemyRadius * 2.0f, enemyRadius * 2.0f };
        Rectangle rceRec{ rcePosition.x - enemyRadius, rcePosition.y - enemyRadius,
            enemyRadius * 2.0f, enemyRadius * 2.0f };
        vector<size_t> cceProximityTiles = GridIndices(cceRec);
        vector<size_t> rceProximityTiles = GridIndices(rceRec);
        vector<size_t> cceVisibilityTiles = VisibilityIndices(playerCircle, polygons, cceProximityTiles);
        vector<size_t> rceVisibilityTiles = VisibilityIndices(playerCircle, polygons, rceProximityTiles);

        BeginDrawing();
        ClearBackground(background);

        // Render debug
        if (useHeatmap)
        {
            const Color cceProximity = DARKBLUE;
            const Color cceVisibiliy = SKYBLUE;
            const Color rceProximity = DARKPURPLE;
            const Color rceVisibiliy = PURPLE;
            // No need for blending, just render enemies after heatmap.
            //cceProximity.a = cceVisibiliy.a = rceProximity.a = rceVisibiliy.a = 128;
            for (size_t i : cceProximityTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, cceProximity);

            for (size_t i : cceVisibilityTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, cceVisibiliy);

            for (size_t i : rceProximityTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, rceProximity);

            for (size_t i : rceVisibilityTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, rceVisibiliy);
        }

        // Render entities
        DrawCircleV(ccePosition, enemyRadius, cceColor);
        DrawCircleV(rcePosition, enemyRadius, rceColor);
        DrawLineV(playerPosition, playerEnd, playerColor);
        DrawPolygon(playerPolygon, playerColor);

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
            ImGui::SliderFloat2("CCE Position", &ccePosition.x, 0.0f, 1200.0f);
            ImGui::SliderFloat2("RCE Position", &rcePosition.x, 0.0f, 1200.0f);
            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
