#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

using namespace std;

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Sunshine");

    Circle testCircle{ {500.0f, 500.0f}, 50.0f };
    Circle playerCircle{ {0.0f, 0.0f}, 50.0f };

    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        playerCircle.position = GetMousePosition();

        Vector2 mtv;
        bool circleCollision = CheckCollisionCircles(playerCircle, testCircle, mtv);
        Color circleColor = circleCollision ? RED : GREEN;

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawCircleV(playerCircle.position, 50.0f, circleColor);
        DrawCircleV(testCircle.position, 50.0f, circleColor);

        if (circleCollision)
        {
            DrawLineEx(playerCircle.position, playerCircle.position + mtv, 5, BLACK);
            testCircle.position = testCircle.position + mtv;
        }

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
