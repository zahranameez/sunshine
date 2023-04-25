#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"
#include "Timer.h"

#include <iostream>
using namespace std;

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

bool ResolveCollisions(Circle& circle, const Obstacles& obstacles)
{
    for (const Circle& obstacle : obstacles)
    {
        Vector2 mtv;
        if (CheckCollisionCircles(obstacle, circle, mtv))
        {
            circle.position = circle.position + mtv;
            return true;
        }
    }

    if (circle.position.x < 0.0f) circle.position.x = 0.0f;
    if (circle.position.y < 0.0f) circle.position.y = 0.0f;
    if (circle.position.x > SCREEN_WIDTH) circle.position.x = SCREEN_WIDTH;
    if (circle.position.y > SCREEN_HEIGHT) circle.position.y = SCREEN_HEIGHT;
    return false;
}

void DrawTextureCircle(const Texture& texture, const Circle& circle,
    float rotation/*degrees*/ = 0.0f, Color tint = WHITE)
{
    Rectangle src{ 0.0f, 0.0f, texture.width, texture.height };
    Rectangle dst{ circle.position.x, circle.position.y, circle.radius * 2.0f, circle.radius * 2.0f };
    DrawTexturePro(texture, src, dst, { dst.width * 0.5f, dst.height * 0.5f }, rotation, tint);
}

struct Bullet : Circle
{
    Vector2 direction{};
};

void Move(Bullet& bullet, float bulletSpeed, float dt)
{
    bullet.position = bullet.position + bullet.direction * bulletSpeed * dt;
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sunshine");
    rlImGuiSetup(true);
    InitAudioDevice();

    Sound sound = LoadSound("../game/assets/audio/yay.ogg");
    Music music = LoadMusicStream("../game/assets/audio/ncs_time_leap_aero_chord.mp3");
    bool musicPaused = true;

    Texture texBackground = LoadTexture("../game/assets/textures/galaxy.png");
    Texture texObstacle = LoadTexture("../game/assets/textures/nebula.png");
    Texture texPlayer = LoadTexture("../game/assets/textures/enterprise.png");
    Texture texEnemy = LoadTexture("../game/assets/textures/d7.png");

    Points points = LoadPoints();
    Obstacles obstacles = LoadObstacles();
    
    Circle player{ {}, 50.0f };
    Circle enemy{ {}, 50.0f };
    vector<Bullet> playerBullets;
    vector<Bullet> enemyBullets;
    const float bulletRadius = 10.0f;
    const float bulletSpeed = 500.0f;

    size_t point = 0;
    float t = 0.0f;
    bool enemyAttacking = false;

    // Switch from following path to attacking player every 2.5 seconds
    Timer enemyStateTimer;
    enemyStateTimer.duration = 2.5f;

    // Shoot every 0.05 seconds if in ATTACK state
    Timer enemyAttackCooldown;
    enemyAttackCooldown.duration = 0.05f;

    Timer playerAttackCooldown;
    playerAttackCooldown.duration = 0.25f;

    bool drawColliders = false;
    bool useGUI = false;
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();
        const float playerSpeed = 500.0f * dt;
        if (IsKeyDown(KEY_W))
            player.position.y -= playerSpeed;
        if (IsKeyDown(KEY_S))
            player.position.y += playerSpeed;
        if (IsKeyDown(KEY_A))
            player.position.x -= playerSpeed;
        if (IsKeyDown(KEY_D))
            player.position.x += playerSpeed;
        ResolveCollisions(player, obstacles);
        Vector2 playerDirection = Normalize(GetMousePosition() - player.position);
        if (IsKeyDown(KEY_SPACE))
        {
            playerAttackCooldown.Tick(dt);
            if (playerAttackCooldown.Expired())
            {
                playerAttackCooldown.Reset();

                Bullet bullet;
                bullet.radius = bulletRadius;
                bullet.direction = playerDirection;
                bullet.position = player.position + playerDirection * (player.radius + bullet.radius);
                playerBullets.push_back(bullet);
            }
        }

        enemyStateTimer.Tick(dt);
        if (enemyStateTimer.Expired())
        {
            enemyStateTimer.Reset();
            enemyAttacking = !enemyAttacking;
        }

        Vector2 enemyDirection;
        if (enemyAttacking)
        {
            enemyDirection = Normalize(player.position - enemy.position);
            enemyAttackCooldown.Tick(dt);
            if (enemyAttackCooldown.Expired())
            {
                enemyAttackCooldown.Reset();
                
                Bullet bullet;
                bullet.radius = bulletRadius;
                bullet.direction = Rotate(enemyDirection, Random(-10.0f, 10.0f) * DEG2RAD);
                bullet.position = enemy.position + enemyDirection * (enemy.radius + bullet.radius);
                enemyBullets.push_back(bullet);
            }
        }
        else
        {
            Vector2 currentPoint = points[point];
            Vector2 nextPoint = points[(point + 1) % points.size()];
            enemyDirection = Normalize(nextPoint - currentPoint);
            enemy.position = Lerp(currentPoint, nextPoint, t);
            t += dt;
            if (t > 1.0f)
            {
                t = 0.0f;
                ++point %= points.size();
            }
        }

        for (Bullet& bullet : playerBullets)
            Move(bullet, bulletSpeed, dt);
        for (Bullet& bullet : enemyBullets)
            Move(bullet, bulletSpeed, dt);

        UpdateMusicStream(music);
        if (musicPaused)
            PauseMusicStream(music);
        else
            PlayMusicStream(music);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawTexturePro(texBackground,
            { 0.0f, 0.0f,(float)texBackground.width, (float)texBackground.height },
            { 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_WIDTH }, {}, 0.0f, WHITE);

        if (drawColliders)
        {
            for (const Circle& obstacle : obstacles)
                DrawCircleV(obstacle.position, obstacle.radius, GRAY);
            DrawCircleV(player.position, player.radius, GREEN);
            DrawCircleV(enemy.position, enemy.radius, RED);
        }

        for (const Circle& obstacle : obstacles)
            DrawTextureCircle(texObstacle, obstacle);
        DrawTextureCircle(texPlayer, player, SignedAngle({1.0f, 0.0f}, playerDirection) * RAD2DEG);
        DrawTextureCircle(texEnemy, enemy, SignedAngle({ 1.0f, 0.0f }, enemyDirection) * RAD2DEG);

        for (Bullet& bullet : playerBullets)
            DrawCircleV(bullet.position, bullet.radius, GREEN);
        for (Bullet& bullet : enemyBullets)
            DrawCircleV(bullet.position, bullet.radius, RED);

        if (IsKeyPressed(KEY_GRAVE)) useGUI = !useGUI;
        if (useGUI)
        {
            rlImGuiBegin();
            ImGui::Checkbox("Draw Colliders", &drawColliders);

            if (ImGui::Button("Play Sound"))
                PlaySound(sound);

            if (musicPaused)
            {
                if (ImGui::Button("Play Music"))
                    musicPaused = false;
            }
            else
            {
                if (ImGui::Button("Pause Music"))
                    musicPaused = true;
            }

            if (ImGui::Button("Restart Music"))
                SeekMusicStream(music, 0.0f);

            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    UnloadSound(sound);
    UnloadMusicStream(music);

    UnloadTexture(texEnemy);
    UnloadTexture(texPlayer);
    UnloadTexture(texObstacle);
    UnloadTexture(texBackground);

    CloseAudioDevice();
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
