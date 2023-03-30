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

    float playerRotation = 0.0f;
    const float playerWidth = 60.0f;
    const float playerHeight = 40.0f;
    const float playerRange = 1000.0f;
    const float playerRotationSpeed = 100.0f;

    const char* recText = "Nearest to Rectangle";
    const char* circleText = "Nearest to Circle";
    const char* poiText = "Nearest Intersection";
    const int fontSize = 10;
    const int recTextWidth = MeasureText(recText, fontSize);
    const int circleTextWidth = MeasureText(circleText, fontSize);
    const int poiTextWidth = MeasureText(poiText, fontSize);

    const Rectangle rectangle{ 1000.0f, 500.0f, 160.0f, 90.0f };
    const Circle circle{ { 1000.0f, 250.0f }, 50.0f };

    bool demoGUI = false;
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

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Render player
        DrawRectanglePro(playerRec, { playerWidth * 0.5f, playerHeight * 0.5f }, playerRotation, PURPLE);
        DrawLine(playerPosition.x, playerPosition.y, playerEnd.x, playerEnd.y, BLUE);
        DrawCircleV(playerPosition, 10.0f, BLUE);

        // Render geometry
        for (const Rectangle& obstacle : obstacles)
            DrawRectangleRec(obstacle, GREEN);
        DrawRectangleRec(rectangle, rectangleVisible ? GREEN : RED);
        DrawCircleV(circle.position, circle.radius, circleVisible ? GREEN : RED);

        // Render labels
        DrawText(circleText, nearestCirclePoint.x - circleTextWidth * 0.5f, nearestCirclePoint.y - fontSize * 2, fontSize, BLUE);
        DrawCircleV(nearestRecPoint, 10.0f, BLUE);
        DrawText(recText, nearestRecPoint.x - recTextWidth * 0.5f, nearestRecPoint.y - fontSize * 2, fontSize, BLUE);
        DrawCircleV(nearestCirclePoint, 10.0f, BLUE);
        if (collision)
        {
            DrawText(poiText, poi.x - poiTextWidth * 0.5f, poi.y - fontSize * 2, fontSize, BLUE);
            DrawCircleV(poi, 10.0f, BLUE);
        }

        // Render GUI
        if (IsKeyPressed(KEY_GRAVE)) demoGUI = !demoGUI;
        if (demoGUI)
        {
            rlImGuiBegin();
            ImGui::ShowDemoWindow(nullptr);
            rlImGuiEnd();
        }

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
