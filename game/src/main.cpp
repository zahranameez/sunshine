#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"
#include <string>

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

    float playerRotation = -45.0f;
    const float playerRotationSpeed = 100.0f;   // 100 degrees per second
    const float playerDistance = 500.0f;

    const Vector2 originDirection = Direction(0.0f);

    Vector2 playerPosition{};
    Vector2 playerDirection{};

    Vector2 projectilePosition{ -100.0f, 0.0f };
    Vector2 projectileDirection{};
    
    Vector2 projectileVelocity;

    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();
        if (IsKeyDown(KEY_Q))
            playerRotation -= playerRotationSpeed * dt;
        if (IsKeyDown(KEY_E))
            playerRotation += playerRotationSpeed * dt;
        if (IsKeyDown(KEY_SPACE))
        {
            projectilePosition = playerPosition;
            projectileDirection = playerDirection;
        }

        playerPosition = GetMousePosition();
        playerDirection = Direction(playerRotation * DEG2RAD);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectanglePro({ playerPosition.x, playerPosition.y, recWidth, recHeight },
            { recWidth * 0.5f, recHeight * 0.5f }, playerRotation, BLUE);

        const float projectileRotation = SignedAngle(originDirection, projectileDirection) * RAD2DEG;
        DrawRectanglePro({ projectilePosition.x, projectilePosition.y, recWidth, recHeight },
            { recWidth * 0.5f, recHeight * 0.5f }, projectileRotation, ORANGE);

        //DrawRectangle(0, 0, recWidth, recHeight, RED);
        DrawRectangle(screenWidth - recWidth, 0, recWidth, recHeight, ORANGE);
        DrawRectangle(0, screenHeight - recHeight, recWidth, recHeight, YELLOW);
        DrawRectangle(screenWidth - recWidth, screenHeight - recHeight, recWidth, recHeight, GREEN);
        DrawLineV(playerPosition, playerPosition + playerDirection * playerDistance, BLUE);
        
        string angle = "Angle: " + to_string(fmodf(fabs(playerRotation + 360.0f), 360.0f));
        string dir = "Direction: [" + to_string(playerDirection.x) + ", " +
            to_string(playerDirection.y) + "]";
        DrawText(angle.c_str(), 10, 10, fontSize, RED);
        DrawText(dir.c_str(), 10, 10 + fontSize, fontSize, RED);
        
        EndDrawing();
    }

    CloseWindow();
    return 0;
}