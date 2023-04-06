#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

using namespace std;

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Sunshine");

    const float recWidth = 60.0f;
    const float recHeight = 40.0f;

    const int fontSize = 20;
    const char* greeting = "Hello World!";
    const int textWidth = MeasureText(greeting, fontSize);

    float playerRotation = 0.0f;
    const float playerRotationSpeed = 100.0f;   // 100 degrees per second
    const float playerDistance = 500.0f;

    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        if (IsKeyDown(KEY_Q))
            playerRotation -= playerRotationSpeed * dt;
        if (IsKeyDown(KEY_E))
            playerRotation += playerRotationSpeed * dt;

        Vector2 playerPosition = GetMousePosition();
        Vector2 playerDirection = Direction(playerRotation * DEG2RAD);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectanglePro({ playerPosition.x, playerPosition.y, recWidth, recHeight },
            { recWidth * 0.5f, recHeight * 0.5f }, playerRotation, BLUE);

        DrawRectangle(0, 0, recWidth, recHeight, RED);
        DrawRectangle(screenWidth - recWidth, 0, recWidth, recHeight, ORANGE);
        DrawRectangle(0, screenHeight - recHeight, recWidth, recHeight, YELLOW);
        DrawRectangle(screenWidth - recWidth, screenHeight - recHeight, recWidth, recHeight, GREEN);
        
        DrawLineV(playerPosition, playerPosition + playerDirection * playerDistance, BLUE);
        
        EndDrawing();
    }

    CloseWindow();
    return 0;
}