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

int main(void)
{
    Tiles tiles;
    for (size_t i = 0; i < GRID_LENGTH_SQR; i++)
    {
        size_t col = i % GRID_LENGTH;
        size_t row = i / GRID_LENGTH;
        tiles.position[i] = { float(col * TILE_WIDTH), float(row * TILE_HEIGHT) };
    }

    vector<Rectangle> obstacles;
    std::ifstream inFile("../game/assets/data/obstacles.txt");
    while (!inFile.eof())
    {
        Rectangle obstacle;
        inFile >> obstacle.x >> obstacle.y >> obstacle.width >> obstacle.height;
        obstacles.push_back(obstacle);
    }
    inFile.close();

    Circle player{ {0.0f, 0.0f}, 50.0f };
    Circle cce{ {1000.0f, 650.0f}, 50.0f };
    Circle rce{ {1000.0f, 250.0f}, 50.0f };
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
        player.position = GetMousePosition();

        cceProximityTiles.clear();
        rceProximityTiles.clear();
        cceVisibilityTiles.clear();
        rceVisibilityTiles.clear();

        // Broad phase
        for (size_t i = 0; i < GRID_LENGTH_SQR; i++)
        {
            tiles.flags[i] = TILE_FLAGS_NONE;
            Vector2 tileCenter = tiles.position[i] + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };

            if (CheckCollisionPointCircle(tileCenter, { cce.position, enemySensorRadius }))
                cceProximityTiles.push_back(i);

            if (CheckCollisionPointCircle(tileCenter, { rce.position, enemySensorRadius }))
                rceProximityTiles.push_back(i);
        }

        // Narrow phase cce
        for (size_t i : cceProximityTiles)
        {
            tiles.flags[i] |= TILE_PROXIMITY_CCE;
            Vector2 tileCenter = tiles.position[i] + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };

            if (IsCircleVisible(tileCenter, player.position, player, obstacles))
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

            if (IsCircleVisible(tileCenter, player.position, player, obstacles))
            {
                tiles.flags[i] |= TILE_VISIBILITY_RCE;
                rceVisibilityTiles.push_back(i);
            }
        }

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
        DrawCircle(cce, cceColor);
        DrawCircle(rce, rceColor);
        DrawCircle(player, playerColor);

        // Render obstacles (occluders)
        for (const Rectangle& obstacle : obstacles)
            DrawRectangleRec(obstacle, GRAY);
        
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
