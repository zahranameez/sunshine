#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

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

    const int gridLength = 80;
    const float tileWidth = screenWidth / gridLength;
    const float tileHeight = screenHeight / gridLength;
    array<Vector2, gridLength * gridLength> visibilityTiles;
    for (size_t i = 0; i < gridLength; i++)
    {
        for (size_t j = 0; j < gridLength; j++)
        {
            visibilityTiles[i * gridLength + j] = { j * tileWidth, i * tileHeight };
        }
    }

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

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Render visibility tiles
        if (useLOS)
        {
            const int alpha = 96;
            for (const Vector2& tilePosition : visibilityTiles)
            {
                Vector2 tileCenter{ tilePosition.x + tileWidth * 0.5f, tilePosition.y + tileHeight * 0.5f };
                bool targetVisible = IsCircleVisible(tileCenter, target.position, target, obstacles);
                bool playerVisible = IsRectangleVisible(tileCenter, playerPosition, playerRec, obstacles);
                Color color;
                if (targetVisible && playerVisible) color = GREEN;
                else if (playerVisible) color = BLUE;
                else if (targetVisible) color = PURPLE;
                else color = BLACK;
                color.a = alpha;
                DrawRectangle(tilePosition.x, tilePosition.y, tileWidth, tileHeight, color);
            }

            DrawCircleV(target.position, target.radius, PURPLE);

            // Legend
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

        // Render geometry
        for (const Rectangle& obstacle : obstacles)
            DrawRectangleRec(obstacle, GREEN);

        // Render labels
        if (usePOI)
        {
            DrawRectangleRec(rectangle, rectangleVisible ? GREEN : RED);
            DrawCircleV(circle.position, circle.radius, circleVisible ? GREEN : RED);

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

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
