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

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Sunshine");
    rlImGuiSetup(true);

    vector<Rectangle> obstacles;
    std::ifstream inFile("../game/assets/data/obstacles.txt");
    while (!inFile.eof())
    {
        Rectangle obstacle;
        inFile >> obstacle.x >> obstacle.y >> obstacle.width >> obstacle.height;
        obstacles.push_back(obstacle);
    }
    inFile.close();

    const size_t gridLength = 80;
    const size_t gridLengthSqr = gridLength * gridLength;
    array<int, gridLengthSqr> visibilityFlags;
    const int tileWidth = screenWidth / gridLength;
    const int tileHeight = screenHeight / gridLength;

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
            for (size_t i = 0; i < gridLengthSqr; i++)
            {
                size_t col = i % gridLength;
                size_t row = i / gridLength;
                Vector2 tilePosition{ col * tileWidth, row * tileHeight };
                Vector2 tileCenter = tilePosition + Vector2{ tileWidth * 0.5f, tileHeight * 0.5f };

                int visibility = VISIBILITY_NONE;
                if (IsRectangleVisible(tileCenter, playerPosition, playerRec, obstacles))
                    visibility |= VISIBILITY_PLAYER;
                if (IsCircleVisible(tileCenter, target.position, target, obstacles))
                    visibility |= VISIBILITY_TARGET;
                visibilityFlags[i] = visibility;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (useLOS)
        {
            const int alpha = 96;

            // Render visibility tiles
            for (size_t i = 0; i < gridLengthSqr; i++)
            {
                size_t col = i % gridLength;
                size_t row = i / gridLength;
                Vector2 tilePosition{ col * tileWidth, row * tileHeight };

                Color color = BLACK;
                switch (visibilityFlags[i])
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
                color.a = alpha;
                DrawRectangle(tilePosition.x, tilePosition.y, tileWidth, tileHeight, color);
            }

            // Render target
            DrawCircleV(target.position, target.radius, PURPLE);

            // Reender legend
            if (useGUI)
            {
                Color both = GREEN;
                Color player = BLUE;
                Color target = PURPLE;
                both.a = player.a = target.a = alpha;
                DrawRectangle(screenWidth - 320, 0, 320, 130, LIGHTGRAY);
                DrawRectangle(screenWidth - 310, 10, 40, 30, both);
                DrawRectangle(screenWidth - 310, 50, 40, 30, player);
                DrawRectangle(screenWidth - 310, 90, 40, 30, target);
                DrawText("<-- Player & Target", screenWidth - 260, 15, fontSize, both);
                DrawText("<-- Player Only", screenWidth - 260, 55, fontSize, player);
                DrawText("<-- Target Only", screenWidth - 260, 95, fontSize, target);
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
