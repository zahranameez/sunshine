#include "rlImGui.h"
#include "Math.h"
#include <string>
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

struct Rigidbody
{
    Vector2 position;
    Vector2 direction;
    float linearSpeed;
    float angularSpeed;
};

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sunshine");
    SetTargetFPS(60);

    Rigidbody body;
    body.position = { 100.0f, 80.0f };
    body.direction = { 1.0f, 0.0f };
    body.linearSpeed = 250.0f;
    body.angularSpeed = 100.0f;

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        if (IsKeyDown(KEY_W))
            body.position = body.position + body.direction * body.linearSpeed * dt;
        if (IsKeyDown(KEY_S))
            body.position = body.position - body.direction * body.linearSpeed * dt;
        if (IsKeyDown(KEY_E))
            body.direction = Rotate(body.direction, body.angularSpeed * dt * DEG2RAD);
        if (IsKeyDown(KEY_Q))
            body.direction = Rotate(body.direction, -body.angularSpeed * dt * DEG2RAD);

        float radians1 = atan2(body.direction.y, body.direction.x);
        float radians2 = SignedAngle({ 1.0f, 0.0f }, body.direction);
        
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectanglePro({ body.position.x, body.position.y, 60.f, 40.0f }, { 30.f, 20.0f },
            SignedAngle({ 1.0f, 0.0f }, body.direction) * RAD2DEG, RED);
        DrawLineV(body.position, body.position + body.direction * 300.0f, BLACK);
        DrawText(std::to_string(radians1 * RAD2DEG).c_str(), 10, 10, 20, BLACK);
        DrawText(std::to_string(radians2 * RAD2DEG).c_str(), 10, 30, 20, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}