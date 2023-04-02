#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

#define VISIBILITY_NONE 0
#define VISIBILITY_PLAYER 1
#define VISIBILITY_TARGET 2

constexpr int GRID_LENGTH = 80;
constexpr int GRID_LENGTH_SQR = GRID_LENGTH * GRID_LENGTH;
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TILE_WIDTH = SCREEN_WIDTH / GRID_LENGTH;
constexpr int TILE_HEIGHT = SCREEN_HEIGHT / GRID_LENGTH;

struct Tiles
{
    array<Vector2, GRID_LENGTH_SQR> position{};
    array<int, GRID_LENGTH_SQR> visibility{};
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

    float playerRotation = 0.0f;
    const float playerWidth = 60.0f;
    const float playerHeight = 40.0f;
    const float playerRange = 1000.0f;
    const float playerRotationSpeed = 100.0f;

    const char* recText = "Nearest to Rectangle";
    const char* circleText = "Nearest to Circle";
    const char* poiText = "Nearest Intersection";
    const int fontSize = 20;
    const int recTextWidth = MeasureText(recText, fontSize);
    const int circleTextWidth = MeasureText(circleText, fontSize);
    const int poiTextWidth = MeasureText(poiText, fontSize);

    const Rectangle rectangle{ 1000.0f, 500.0f, 160.0f, 90.0f };
    const Circle circle{ { 1000.0f, 250.0f }, 50.0f };
    const Circle target{ {100.0f, 600.0f}, 50.0f };

    bool usePOI = false; // show nearest point of intersection, nearest circle, and nearest rectangle points
    bool useLOS = false; // show if player & target, only player, only target, or nothing is visible
    bool useGUI = false;
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sunshine");
    rlImGuiSetup(true);
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        if (IsKeyDown(KEY_E))
            playerRotation += playerRotationSpeed * dt;
        if (IsKeyDown(KEY_Q))
            playerRotation -= playerRotationSpeed * dt;

        const Vector2 playerPosition = GetMousePosition();
        const Vector2 playerDirection = Direction(playerRotation * DEG2RAD);
        const Vector2 playerEnd = playerPosition + playerDirection * playerRange;
        const Rectangle playerRec{ playerPosition.x, playerPosition.y, playerWidth, playerHeight };

        const Vector2 nearestRecPoint = NearestPoint(playerPosition, playerEnd,
            { rectangle.x + rectangle.width * 0.5f, rectangle.y + rectangle.height * 0.5f });
        const Vector2 nearestCirclePoint = NearestPoint(playerPosition, playerEnd, circle.position);
        Vector2 poi;

        const bool collision = NearestIntersection(playerPosition, playerEnd, obstacles, poi);
        const bool rectangleVisible = IsRectangleVisible(playerPosition, playerEnd, rectangle, obstacles);
        const bool circleVisible = IsCircleVisible(playerPosition, playerEnd, circle, obstacles);
        const Color recMarkerColor = rectangleVisible ? DARKGREEN : RED;
        const Color circleMarkerColor =  circleVisible ? DARKGREEN : RED;

        if (useLOS)
        {
            for (size_t i = 0; i < GRID_LENGTH_SQR; i++)
            {
                Vector2 tileCenter = tiles.position[i] + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };
                tiles.visibility[i] = VISIBILITY_NONE;
                if (IsRectangleVisible(tileCenter, playerPosition, playerRec, obstacles))
                    tiles.visibility[i] |= VISIBILITY_PLAYER;
                if (IsCircleVisible(tileCenter, target.position, target, obstacles))
                    tiles.visibility[i] |= VISIBILITY_TARGET;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (useLOS)
        {
            // Render visibility tiles
            const int tileAlpha = 96;
            for (size_t i = 0; i < GRID_LENGTH_SQR; i++)
            {
                Color color = BLACK;
                switch (tiles.visibility[i])
                {
                case VISIBILITY_PLAYER | VISIBILITY_TARGET:
                    color = GREEN;
                    break;
                case VISIBILITY_PLAYER:
                    color = BLUE;
                    break;
                case VISIBILITY_TARGET:
                    color = PURPLE;
                    break;
                }
                color.a = tileAlpha;
                DrawRectangleV(tiles.position[i], { TILE_WIDTH, TILE_HEIGHT }, color);
            }

            // Render target
            DrawCircleV(target.position, target.radius, PURPLE);

            // Render legend
            if (useGUI)
            {
                Color both = GREEN;
                Color player = BLUE;
                Color target = PURPLE;
                both.a = player.a = target.a = tileAlpha;
                DrawRectangle(SCREEN_WIDTH - 320, 0, 320, 130, LIGHTGRAY);
                DrawRectangle(SCREEN_WIDTH - 310, 10, 40, 30, both);
                DrawRectangle(SCREEN_WIDTH - 310, 50, 40, 30, player);
                DrawRectangle(SCREEN_WIDTH - 310, 90, 40, 30, target);
                DrawText("<-- Player & Target", SCREEN_WIDTH - 260, 15, fontSize, both);
                DrawText("<-- Player Only", SCREEN_WIDTH - 260, 55, fontSize, player);
                DrawText("<-- Target Only", SCREEN_WIDTH - 260, 95, fontSize, target);
            }
        }

        // Render player
        DrawRectanglePro(playerRec, { playerWidth * 0.5f, playerHeight * 0.5f }, playerRotation, BLUE);
        DrawLine(playerPosition.x, playerPosition.y, playerEnd.x, playerEnd.y, BLUE);
        DrawCircleV(playerPosition, 10.0f, BLACK);

        // Render obstacles (occluders)
        for (const Rectangle& obstacle : obstacles)
            DrawRectangleRec(obstacle, GREEN);

        if (usePOI)
        {
            // Render collision shapes
            DrawRectangleRec(rectangle, rectangleVisible ? GREEN : RED);
            DrawCircleV(circle.position, circle.radius, circleVisible ? GREEN : RED);

            // Render labels & markers
            DrawText(circleText, nearestCirclePoint.x - circleTextWidth * 0.5f, nearestCirclePoint.y - fontSize * 2, fontSize, BLACK);
            DrawCircleV(nearestRecPoint, 10.0f, recMarkerColor);
            DrawText(recText, nearestRecPoint.x - recTextWidth * 0.5f, nearestRecPoint.y - fontSize * 2, fontSize, BLACK);
            DrawCircleV(nearestCirclePoint, 10.0f, circleMarkerColor);
            if (collision)
            {
                DrawText(poiText, poi.x - poiTextWidth * 0.5f, poi.y - fontSize * 2, fontSize, BLACK);
                DrawCircleV(poi, 10.0f, DARKGREEN);
            }
        }
        
        // Render GUI
        if (IsKeyPressed(KEY_GRAVE)) useGUI = !useGUI;
        if (useGUI)
        {
            rlImGuiBegin();
            ImGui::Checkbox("POI", &usePOI);
            ImGui::Checkbox("LOS", &useLOS);
            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
