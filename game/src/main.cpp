#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include <string>
#include <fstream>
#include <iostream>

using namespace std;

constexpr int GRID_LENGTH = 80;
constexpr int GRID_LENGTH_SQR = GRID_LENGTH * GRID_LENGTH;
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TILE_WIDTH = SCREEN_WIDTH / GRID_LENGTH;
constexpr int TILE_HEIGHT = SCREEN_HEIGHT / GRID_LENGTH;

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

vector<size_t> OverlapTiles(Rectangle rectangle)
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

vector<size_t> VisibleTiles(Circle target, float sightDistance,
    const Obstacles& obstacles, const vector<size_t>& tiles)
{
    vector<size_t> visibilityTiles;
    visibilityTiles.reserve(tiles.size());
    for (size_t i : tiles)
    {
        Vector2 tileCenter = GridToScreen(i) + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };
        Vector2 tileEnd = tileCenter + Normalize(target.position - tileCenter) * sightDistance;
        if (IsCircleVisible(tileCenter, tileEnd, target, obstacles)) visibilityTiles.push_back(i);
    }
    return visibilityTiles;
}

bool IsCollision(Vector2 lineStart, Vector2 lineEnd, const Obstacles& obstacles)
{
    for (const Circle& obstacle : obstacles)
    {
        if (CheckCollisionLineCircle(lineStart, lineEnd, obstacle))
            return true;
    }
    return false;
}

bool Avoid(Vector2& position, Vector2& direction, float maxRadians, float probeDistance, const Obstacles& obstacles)
{
    const float near = 15.0f * DEG2RAD;
    const float far = 45.0f * DEG2RAD;
    Vector2 nearLeft = position + Rotate(direction, -near) * probeDistance;
    if (IsCollision(position, nearLeft, obstacles))
    {
        direction = Rotate(direction, maxRadians);
        return true;
    }
    Vector2 nearRight = position + Rotate(direction, near) * probeDistance;
    if (IsCollision(position, nearRight, obstacles))
    {
        direction = Rotate(direction, -maxRadians);
        return true;
    }
    // These do more harm than good
    //Vector2 farLeft = position + Rotate(direction, -far) * probeDistance;
    //if (IsCollision(position, farLeft, obstacles))
    //{
    //    direction = Rotate(direction, maxRadians);
    //    return true;
    //}
    //Vector2 farRight = position + Rotate(direction, far) * probeDistance;
    //if (IsCollision(position, farRight, obstacles))
    //{
    //    direction = Rotate(direction, -maxRadians);
    //    return true;
    //}

    return false;
}

Vector2 RotateTowards(Vector2 from, Vector2 to, float maxRadians)
{
    float deltaRadians = LineAngle(from, to);
    return Rotate(from, fminf(deltaRadians, maxRadians) * Sign(Cross(from, to)));
}

int main(void)
{
    Obstacles obstacles;
    std::ifstream inFile("../game/assets/data/obstacles.txt");
    while (!inFile.eof())
    {
        Rectangle rectangle;
        inFile >> rectangle.x >> rectangle.y >> rectangle.width >> rectangle.height;
        obstacles.push_back(From(rectangle));
    }
    inFile.close();

    Circle player;
    player.radius = 30.0f;
    float playerRotation = 0.0f;

    Circle cce{ { 1000.0f, 250.0f }, 50.0f };
    Circle rce{ { 1000.0f, 650.0f }, 50.0f };
    Vector2 cceDirection{ -1.0f, 0.0f };
    Vector2 rceDirection{ -1.0f, 0.0f };
    Rigidbody cceBody;
    Rigidbody rceBody;
    float enemySightDistance = 300.0f;
    float enemyProbeDistance = 100.0f;
    const float enemySpeed = 500.0f;
    const float enemyRotationSpeed = 100.0f;

    const Color playerColor = GREEN;
    const Color cceColor = BLUE;
    const Color rceColor = VIOLET;
    const Color background = RAYWHITE;

    bool useDebug = true;
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

        player.position = GetMousePosition();
        const Vector2 playerDirection = Direction(playerRotation * DEG2RAD);
        const Vector2 playerEnd = player.position + playerDirection * 500.0f;

        const float enemyRotationDelta = enemyRotationSpeed * dt * DEG2RAD;
        if (Avoid(cce.position, cceDirection, enemyRotationDelta, enemyProbeDistance, obstacles))
        {
            ApplySeek(cce.position + cceDirection * enemySpeed, cce.position, cceBody, enemySpeed, dt);
        }
        else
        {
            ApplyArrive(player.position, cce.position, cceBody, enemySpeed, dt);
            cceDirection = RotateTowards(cceDirection, Normalize(player.position - cce.position), enemyRotationDelta);
        }
        
        vector<size_t> cceOverlapTiles = OverlapTiles(From(cce));
        vector<size_t> rceOverlapTiles = OverlapTiles(From(rce));
        vector<size_t> cceVisibleTiles = VisibleTiles(player, enemySightDistance, obstacles, cceOverlapTiles);
        vector<size_t> rceVisibleTiles = VisibleTiles(player, enemySightDistance, obstacles, rceOverlapTiles);

        vector<Vector2> intersections;
        for (const Circle& obstacle : obstacles)
        {
            Vector2 poi;
            if (CheckCollisionLineCircle(player.position, playerEnd, obstacle, poi))
                intersections.push_back(poi);
        }

        BeginDrawing();
        ClearBackground(background);

        // Render debug
        if (useDebug)
        {
            for (size_t i : cceOverlapTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, DARKBLUE);

            for (size_t i : cceVisibleTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, SKYBLUE);

            for (size_t i : rceOverlapTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, DARKPURPLE);

            for (size_t i : rceVisibleTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, PURPLE);
        }

        // Render entities
        DrawCircle(cce, cceColor);
        DrawCircle(rce, rceColor);
        DrawCircle(player, playerColor);
        DrawLineV(player.position, playerEnd, playerColor);
        DrawLineV(cce.position, cce.position + cceDirection * enemySightDistance, cceColor);
        DrawLineV(cce.position, cce.position + Rotate(cceDirection, -15.0f * DEG2RAD) * enemyProbeDistance, cceColor);
        DrawLineV(cce.position, cce.position + Rotate(cceDirection,  15.0f * DEG2RAD) * enemyProbeDistance, cceColor);
        
        // Render obstacle intersections
        Vector2 obstaclesPoi;
        if (NearestIntersection(player.position, playerEnd, obstacles, obstaclesPoi))
            DrawCircleV(obstaclesPoi, 10.0f, playerColor);

        // Render obstacles
        for (const Circle& obstacle : obstacles)
            DrawCircle(obstacle, GRAY);
        
        // Render GUI
        if (IsKeyPressed(KEY_GRAVE)) useGUI = !useGUI;
        if (useGUI)
        {
            rlImGuiBegin();
            ImGui::Checkbox("Use heatmap", &useDebug);
            ImGui::SliderFloat2("CCE Position", (float*)&cce.position, 0.0f, 1200.0f);
            ImGui::SliderFloat2("RCE Position", (float*)&rce.position, 0.0f, 1200.0f);
            ImGui::SliderFloat("Sight Distance", &enemySightDistance, 0.0f, 1500.0f);
            ImGui::SliderFloat("Probe Distance", &enemyProbeDistance, 0.0f, 1500.0f);
            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
