#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

using namespace std;

// Angle between two normalized vectors
// We want to use directions instead of angles to represent rotations
// because angles can exceed the range of [0, 360] which may cause problems!
RMAPI float SignedAngle(Vector2 from, Vector2 to)
{
    float angle = LineAngle(from, to);
    float sign = (from.x * to.y - from.y * to.x) < 0.0f ? -1.0f : 1.0f;
    return angle * sign;
}

void DrawCapsule(Capsule capsule, Color color)
{
    Vector2 top = capsule.position + capsule.direction * capsule.halfHeight;
    Vector2 bot = capsule.position - capsule.direction * capsule.halfHeight;
    DrawCircleV(top, capsule.radius, color);
    DrawCircleV(bot, capsule.radius, color);

    // Render edges
    Vector2 perp{ capsule.direction.y, -capsule.direction.x };

    Vector2 leftStart = bot + perp * capsule.radius;
    Vector2 leftEnd = leftStart + capsule.direction * capsule.halfHeight * 2.0f;
    Vector2 rightStart = bot - perp * capsule.radius;
    Vector2 rightEnd = rightStart + capsule.direction * capsule.halfHeight * 2.0f;
    DrawLineEx(leftStart, leftEnd, 5.0f, color);
    DrawLineEx(rightStart, rightEnd, 5.0f, color);

    Rectangle rec{ capsule.position.x, capsule.position.y,
    capsule.radius * 2.0f, capsule.halfHeight * 2.0f };
    
    DrawRectanglePro(rec, { capsule.radius, capsule.halfHeight },
        SignedAngle({ 1.0, 0.0f }, capsule.direction) * RAD2DEG, color);
}

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Sunshine");

    Capsule test{ {500.0f, 500.0f}, {1.0f, 0.0f}, 50.0f, 50.0f };
    Capsule player{ {0.0f, 0.0f}, {1.0f, 0.0f}, 50.0f, 50.0f };

    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        player.position = GetMousePosition();

        Vector2 mtv;
        bool collision = CheckCollisionCapsules(player, test, mtv);
        Color color = collision ? RED : GREEN;

        float angularSpeed = 100.0f * GetFrameTime(); // 100 degrees per second

        if (IsKeyDown(KEY_Q))
            player.direction = Rotate(player.direction, -angularSpeed * DEG2RAD);
        if (IsKeyDown(KEY_E))
            player.direction = Rotate(player.direction, angularSpeed * DEG2RAD);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawCapsule(test, color);
        DrawCapsule(player, color);

        if (collision)
        {
            DrawLineEx(player.position, player.position + mtv, 5, BLACK);
            test.position = test.position + mtv;
        }

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
