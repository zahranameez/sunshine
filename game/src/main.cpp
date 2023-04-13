#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"
#include <string>

using namespace std;

struct Projectile
{
    Vector2 position{};
    Vector2 direction{};

    Vector2 velocity{};
    float speed = 0.0f;
};

struct Timer
{
    float elapsed = 0.0f;
    float duration = 0.0f;
    bool Expired() { return elapsed >= duration; }
    void Reset() { elapsed = 0.0f; }
    void Tick(float dt) { elapsed += dt; }
};

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

    std::vector<Projectile> projectiles;
    Timer projectileTimer;
    projectileTimer.duration = 0.25f;

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
            if (projectileTimer.Expired())
            {
                projectileTimer.Reset();
                Projectile projectile;
                projectile.position = playerPosition;
                projectile.direction = playerDirection;
                projectile.speed = 100.0f;
                projectile.velocity = projectile.direction * projectile.speed;
                projectiles.push_back(projectile);
            }
        }

        projectileTimer.Tick(dt);

        playerPosition = GetMousePosition();
        playerDirection = Direction(playerRotation * DEG2RAD);

        for (int i = 0; i < projectiles.size(); i++)
            projectiles[i].position = projectiles[i].position + projectiles[i].velocity * dt;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectanglePro({ playerPosition.x, playerPosition.y, recWidth, recHeight },
            { recWidth * 0.5f, recHeight * 0.5f }, playerRotation, BLUE);

        for (int i = 0; i < projectiles.size(); i++)
        {
            const Projectile& projectile = projectiles[i];
            const float projectileRotation = SignedAngle(originDirection, projectile.direction) * RAD2DEG;
            DrawRectanglePro({ projectile.position.x, projectile.position.y, recWidth, recHeight },
                { recWidth * 0.5f, recHeight * 0.5f }, projectileRotation, ORANGE);
        }

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
