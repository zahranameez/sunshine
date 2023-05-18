#include "rlImGui.h"
#include "Math.h"
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

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectanglePro({ body.position.x, body.position.y, 60.f, 40.0f }, { 30.f, 20.0f },
            SignedAngle({ 1.0f, 0.0f }, body.direction) * RAD2DEG, RED);
        DrawLineV(body.position, body.position + body.direction * 300.0f, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}