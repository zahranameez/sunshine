#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include "Grid.h"
#include "World.h"
#include "Nodes.h"
#include <iostream>

using namespace std;

enum Screen
{
    WIN,
    LOSE,
    GAME
};

Vector2 Avoid(const Rigidbody& rb, float probeLength, float dt, const Obstacles& obstacles)
{
    // Test obstacles against probe
    auto obstacleDetected = [&](float angle) -> bool
    {
        for (const Circle& obstacle : obstacles)
        {
            Vector2 probeEnd = rb.pos + Rotate(Normalize(rb.vel), angle * DEG2RAD) * probeLength;
            if (CheckCollisionLineCircle(rb.pos, probeEnd, obstacle))
                return true;
        }
        return false;
    };

    // Solve for acceleration that will change linear velocity to point angular speed radians away from oncoming obstacle
    auto avoid = [&](float angle) -> Vector2
    {
        const Vector2 vf = Rotate(Normalize(rb.vel), rb.angularSpeed * dt * Sign(-angle)) * Length(rb.vel);
        return Acceleration(rb.vel, vf, dt);
    };

    // Return avoidance acceleration, otherwise return no acceleration since there's no oncoming obstacles
    if (obstacleDetected(-15.0f)) return avoid(-15.0f);
    if (obstacleDetected(-30.0f)) return avoid(-30.0f);
    if (obstacleDetected( 15.0f)) return avoid( 15.0f);
    if (obstacleDetected( 30.0f)) return avoid( 30.0f);
    return {};
}

bool ResolveCollisions(Entity& entity, const Obstacles& obstacles)
{
    for (const Circle& obstacle : obstacles)
    {
        Vector2 mtv;
        if (CheckCollisionCircles(obstacle, entity.Collider(), mtv))
        {
            entity.pos = entity.pos + mtv;
            return true;
        }
    }

    if (entity.pos.x < 0.0f) entity.pos.x = 0.0f;
    if (entity.pos.y < 0.0f) entity.pos.y = 0.0f;
    if (entity.pos.x > SCREEN_WIDTH) entity.pos.x = SCREEN_WIDTH;
    if (entity.pos.y > SCREEN_HEIGHT) entity.pos.y = SCREEN_HEIGHT;
    return false;
}

void DrawHealthBar(const Entity& entity, Color background = DARKGRAY, Color foreground = RED)
{
    const float healthBarWidth = 150.0f;
    const float healthBarHeight = 20.0f;
    const float x = entity.pos.x - healthBarWidth * 0.5f;
    const float y = entity.pos.y - (entity.radius + 30.0f);
    DrawRectangle(x, y, healthBarWidth, healthBarHeight, background);
    DrawRectangle(x, y, healthBarWidth * entity.HealthPercent(), healthBarHeight, foreground);
}

void DrawTextCentered(const char* text, Rectangle rec, int fontSize, Color color)
{
    DrawText(text,
        rec.x + rec.width * 0.5f - MeasureText(text, fontSize) * 0.5f,
        rec.y + rec.height * 0.5f - fontSize * 0.5f,
        fontSize, color);
}

int main(void)
{
    InitAudioDevice();
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sunshine");
    rlImGuiSetup(true);
    SetTargetFPS(60);

    Sound enemyDeath1Sound = LoadSound("../game/assets/audio/ownage.mp3");
    Sound enemyDeath2Sound = LoadSound("../game/assets/audio/doublekill.mp3");
    Sound playerDeathSound = LoadSound("../game/assets/audio/headshot.mp3");
    Sound playerHitSound = LoadSound("../game/assets/audio/impact1.wav");
    Sound enemyHitSound = LoadSound("../game/assets/audio/impact2.wav");
    Sound cceAttackSound = LoadSound("../game/assets/audio/shotgun.wav");
    Sound rceAttackSound = LoadSound("../game/assets/audio/sniper.wav");
    Sound playerAttackSound = LoadSound("../game/assets/audio/rifle.wav");
    SetSoundVolume(cceAttackSound, 0.5f);
    SetSoundVolume(playerAttackSound, 0.5f);

    Texture playerTex = LoadTexture("../game/assets/textures/enterprise.png");
    Texture enemyTex = LoadTexture("../game/assets/textures/d7.png");
    Texture nebulaTex = LoadTexture("../game/assets/textures/nebula.png");
    Texture galaxyTex = LoadTexture("../game/assets/textures/galaxy.png");

    Rectangle playerSrc = { 0.0f, 0.0f, playerTex.width, playerTex.height };
    Rectangle enemySrc = { 0.0f, 0.0f, enemyTex.width, enemyTex.height };
    Rectangle nebulaSrc = { 0.0f, 0.0f, nebulaTex.width, nebulaTex.height };
    Rectangle galaxySrc = { 0.0f, 0.0f, galaxyTex.width, galaxyTex.height };

    World world;
    world.points = LoadPoints();
    world.obstacles = LoadObstacles();

    Enemy cce;
    cce.pos = { SCREEN_WIDTH * 0.9f, SCREEN_HEIGHT * 0.1f };
    cce.dir = { -1.0f, 0.0f };
    cce.angularSpeed = DEG2RAD * 200.0f;
    cce.point = 5;
    cce.speed = 500.0f;
    cce.radius = 50.0f;
    cce.detectionRadius = 400.0f;
    cce.probeLength = 100.0f;
    cce.combatRadius = 300.0f;
    cce.name = "Close-combat enemy";

    Enemy rce;
    rce.pos = { SCREEN_WIDTH * 0.1f, SCREEN_HEIGHT * 0.1f };
    rce.dir = { 1.0f, 0.0f };
    rce.angularSpeed = DEG2RAD * 100.0f;
    rce.point = 0;
    rce.speed = 250.0f;
    rce.radius = 50.0f;
    rce.detectionRadius = 600.0f;
    rce.probeLength = 100.0f;
    rce.combatRadius = 400.0f;
    rce.name = "Ranged-combat enemy";

    // CCE conditions
    DetectedCondition cceIsPlayerDetected(cce);
    VisibleCondition cceIsPlayerVisible(cce);
    CloseCombatCondition cceIsPlayerCombat(cce);

    // CCE actions
    PatrolAction ccePatrol(cce);
    FindVisibilityAction cceFindVisibility(cce, &ccePatrol);
    ArriveAction cceArrive(cce);
    CloseAttackAction cceAttack(cce, &cceArrive, &cceAttackSound);

    // CCE tree
    Node* cceRoot = &cceIsPlayerDetected;
    cceIsPlayerDetected.no = &ccePatrol;
    cceIsPlayerDetected.yes = &cceIsPlayerVisible;
    cceIsPlayerVisible.no = &cceFindVisibility;
    cceIsPlayerVisible.yes = &cceIsPlayerCombat;
    cceIsPlayerCombat.no = &cceArrive;
    cceIsPlayerCombat.yes = &cceAttack;

    // RCE decisions
    DetectedCondition rceIsPlayerDetected(rce);
    VisibleCondition rceIsPlayerVisible(rce);
    RangedCombatCondition rceIsPlayerCombat(rce);

    // RCE actions
    PatrolAction rcePatrol(rce);
    FindVisibilityAction rceFindVisibility(rce, &rcePatrol);
    FleeAction rceFlee(rce);
    RangedAttackAction rceAttack (rce, &rcePatrol, &rceAttackSound);

    // Need something like an interrupt if I'm to implement something like this elegantly...
    // Interrupt would have to tick previous state timer while not relinquishing control until current condition is met.
    //FindCoverAction rceFindCover(rce, &rcePatrol);
    //RangedAttackAction rceAttack (rce, &rceFindCover, &sniperSound);

    // RCE tree
    Node* rceRoot = &rceIsPlayerDetected;
    rceIsPlayerDetected.no = &rcePatrol;
    rceIsPlayerDetected.yes = &rceIsPlayerVisible;
    rceIsPlayerVisible.no = &rceFindVisibility;
    rceIsPlayerVisible.yes = &rceIsPlayerCombat;
    rceIsPlayerCombat.no = &rceFlee;
    rceIsPlayerCombat.yes = &rceAttack;

    Player player;
    player.pos = { SCREEN_WIDTH * 0.8f, SCREEN_HEIGHT * 0.8f };
    player.radius = 60.0f;
    player.dir = { 1.0f, 0.0f };
    player.angularSpeed = 250.0f;
    player.name = "Player";
    const float playerSpeed = 500.0f;

    Timer playerAttackTimer;
    playerAttackTimer.duration = 0.20f;
    playerAttackTimer.elapsed = playerAttackTimer.duration;

    const Color background = RAYWHITE;
    const Color playerColor = { 0, 228, 48, 128 };          // GREEN

    const Color cceColor = { 0, 121, 241, 128 };            // BLUE
    const Color cceOverlapColor = { 0, 82, 172, 128 };      // DARKBLUE
    const Color cceVisibleColor = { 102, 191, 255, 128 };   // SKYBLUE

    const Color rceColor = { 135, 60, 190, 128 };           // VIOLET
    const Color rceOverlapColor = { 200, 122, 255, 128 };   // PURPLE
    const Color rceVisibleColor = { 255, 0, 255, 128 };     // MAGENTA

    Screen screen = Screen::GAME;
    size_t enemiesDead = 0;

    bool useGUI = false;
    bool useDebug = false;
    
    bool showPoints = false;
    bool showObstacles = false;
    bool showEntities = false;
    bool showIntersection = false;
    bool showAvoidance = false;

    bool cceShowTiles = false;
    bool rceShowTiles = false;
    bool cceEnabled = true;
    bool rceEnabled = true;

    while (!WindowShouldClose())
    {
        // First goto statement in 7 years (2016 when I started uni). Legitimately clearner than wrapping update in if :'D
        if (screen != Screen::GAME)
            goto DRAW;

        const float dt = GetFrameTime();
        const float playerPositionDelta = playerSpeed * dt;
        const float playerRotationDelta = player.angularSpeed * dt * DEG2RAD;
        player.dir = RotateTowards(player.dir, Normalize(GetMousePosition() - player.pos), playerRotationDelta);
        playerAttackTimer.Tick(dt);

        // Update player information
        if (IsKeyDown(KEY_W))
            player.pos = player.pos + player.dir * playerPositionDelta;
        if (IsKeyDown(KEY_S))
            player.pos = player.pos - player.dir * playerPositionDelta;
        if (IsKeyDown(KEY_D))
            player.pos = player.pos + Rotate(player.dir, 90.0f * DEG2RAD) * playerPositionDelta;
        if (IsKeyDown(KEY_A))
            player.pos = player.pos - Rotate(player.dir, 90.0f * DEG2RAD) * playerPositionDelta;
        if (IsKeyDown(KEY_SPACE))
        {
            if (playerAttackTimer.Expired())
            {
                playerAttackTimer.Reset();
                PlaySound(playerAttackSound);

                Projectile projectile;
                projectile.type = Projectile::PLAYER;
                projectile.dir = Rotate(player.dir, Random(-10.0f, 10.0f) * DEG2RAD);
                projectile.radius = 20.0f;
                projectile.pos = player.pos + projectile.dir * (player.radius + projectile.radius);
                projectile.vel = projectile.dir * 500.0f;
                projectile.acc = projectile.dir * 1000.0f;
                projectile.damage = 5.0f;
                world.projectiles.push_back(std::move(projectile));
            }
        }

        if (cceEnabled)
        {
            Traverse(cceRoot, player, world);
            cce.acc = cce.acc + Avoid(cce, cce.probeLength, dt, world.obstacles);
            Integrate(cce, dt);
        }
        
        if (rceEnabled)
        {
            Traverse(rceRoot, player, world);
            rce.acc = rce.acc + Avoid(rce, rce.probeLength, dt, world.obstacles);
            Integrate(rce, dt);
        }

        for (Projectile& projectile : world.projectiles)
            Integrate(projectile, dt);

        size_t prevEnemiesDead = enemiesDead;
        world.projectiles.erase(
            remove_if(world.projectiles.begin(), world.projectiles.end(),
                [&player, &cce, &rce, &world, &playerDeathSound, &playerHitSound, &enemyHitSound, &cceEnabled, &rceEnabled, &enemiesDead]
                (const Projectile& projectile) -> bool
                {
                    if (CheckCollisionCircles(player.Collider(), projectile.Collider()))
                    {
                        if (projectile.type == Projectile::ENEMY)
                        {
                            if (player.health > 0.0f && player.health - projectile.damage <= 0.0f)
                                PlaySound(playerDeathSound);

                            player.health -= projectile.damage;
                            PlaySound(playerHitSound);
                        }
                        return true;
                    }

                    if (cceEnabled)
                    {
                        if (CheckCollisionCircles(cce.Collider(), projectile.Collider()))
                        {
                            if (projectile.type == Projectile::PLAYER)
                            {
                                cce.health -= projectile.damage;
                                PlaySound(enemyHitSound);
                                cceEnabled = cce.health > 0.0f;
                                if (!cceEnabled) ++enemiesDead;
                            }
                            return true;
                        }
                    }
                    
                    if (rceEnabled)
                    {
                        if (CheckCollisionCircles(rce.Collider(), projectile.Collider()))
                        {
                            if (projectile.type == Projectile::PLAYER)
                            {
                                rce.health -= projectile.damage;
                                PlaySound(enemyHitSound);
                                rceEnabled = rce.health > 0.0f;
                                if (!rceEnabled) ++enemiesDead;
                            }
                            return true;
                        }
                    }

                    for (const Circle& obstacle : world.obstacles)
                    {
                        if (CheckCollisionCircles(obstacle, projectile.Collider()))
                            return true;
                    }

                    return !CheckCollisionPointRec(projectile.pos, SCREEN_REC);
                }
            ),
        world.projectiles.end());

        if (prevEnemiesDead != enemiesDead)
        {
            if (enemiesDead == 1)
                PlaySound(enemyDeath1Sound);
            else if (enemiesDead == 2)
                PlaySound(enemyDeath2Sound);
        }
        
DRAW:
        bool cceCollision = ResolveCollisions(cce, world.obstacles);
        bool rceCollision = ResolveCollisions(rce, world.obstacles);
        bool playerCollision = ResolveCollisions(player, world.obstacles);

        if (player.health <= 0.0f) screen = Screen::LOSE;
        else if (cce.health <= 0.0f && rce.health <= 0.0f) screen = Screen::WIN;

        BeginDrawing();
        ClearBackground(background);
        DrawTexturePro(galaxyTex, galaxySrc, SCREEN_REC, {}, 0.0f, WHITE);

        if (screen != Screen::GAME)
        {
            if (screen == Screen::WIN)
            {
                DrawRectangleRec(SCREEN_REC, GREEN);
                DrawTextCentered("You win :)", SCREEN_REC, 30, WHITE);
            }
            else
            {
                DrawRectangleRec(SCREEN_REC, RED);
                DrawTextCentered("You lose :(", SCREEN_REC, 30, BLACK);
            }
            EndDrawing();
            continue;
        }

        if (useDebug)
        {
            if (cceShowTiles)
            {
                Rectangle cceOverlapRec = From({ cce.pos, cce.detectionRadius });
                vector<size_t> cceVisibleTiles =
                    VisibleTiles(player.Collider(), cce.detectionRadius, world.obstacles, OverlapTiles(cceOverlapRec));

                DrawRectangleRec(cceOverlapRec, cceOverlapColor);
                for (size_t i : cceVisibleTiles)
                    DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, cceVisibleColor);
            }
            
            if (rceShowTiles)
            {
                Rectangle rceOverlapRec = From({ rce.pos, rce.detectionRadius });
                vector<size_t> rceVisibleTiles =
                    VisibleTiles(player.Collider(), rce.detectionRadius, world.obstacles, OverlapTiles(rceOverlapRec));

                DrawRectangleRec(rceOverlapRec, rceOverlapColor);
                for (size_t i : rceVisibleTiles)
                    DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, rceVisibleColor);
            }

            if (showPoints)
            {
                for (size_t i = 0; i < world.points.size(); i++)
                {
                    const Vector2& p0 = world.points[i];
                    const Vector2& p1 = world.points[(i + 1) % world.points.size()];
                    DrawLineV(p0, p1, GRAY);
                    DrawCircle(p0.x, p0.y, 5.0f, LIGHTGRAY);
                }
            }

            if (showObstacles)
            {
                for (const Circle& obstacle : world.obstacles)
                    DrawCircleV(obstacle.position, obstacle.radius, GRAY);
            }

            if (showEntities)
            {
                DrawCircleV(cce.pos, cce.radius, cceCollision ? RED : cceColor);
                DrawCircleV(rce.pos, cce.radius, rceCollision ? RED : rceColor);
                DrawLineEx(cce.pos, cce.pos + cce.dir * cce.detectionRadius, 10.0f, cceColor);
                DrawLineEx(rce.pos, rce.pos + rce.dir * rce.detectionRadius, 10.0f, rceColor);
                DrawCircleV(player.pos, player.radius, playerCollision ? RED : playerColor);
            }

            if (showIntersection)
            {
                Vector2 obstaclesPoi;
                if (NearestIntersection(player.pos, player.pos + player.dir * 1500.0f, world.obstacles, obstaclesPoi))
                    DrawCircleV(obstaclesPoi, 10.0f, RED);
            }

            if (showAvoidance)
            {
                auto drawProbe = [](const Enemy& enemy, float angle, Color color)
                {
                    DrawLineEx(enemy.pos, enemy.pos + Rotate(Normalize(enemy.vel), angle * DEG2RAD) * enemy.probeLength,
                        5.0f, color);
                };

                drawProbe(cce, -30.0f, cceColor);
                drawProbe(cce, -15.0f, cceColor);
                drawProbe(cce,  15.0f, cceColor);
                drawProbe(cce,  30.0f, cceColor);

                drawProbe(rce, -30.0f, rceColor);
                drawProbe(rce, -15.0f, rceColor);
                drawProbe(rce,  15.0f, rceColor);
                drawProbe(rce,  30.0f, rceColor);
            }
        }

        for (const Projectile& projectile : world.projectiles)
            DrawCircleV(projectile.pos, projectile.radius, projectile.type == Projectile::ENEMY ? RED : GREEN);

        for (const Circle& obstacle : world.obstacles)
            DrawTexturePro(nebulaTex, nebulaSrc, From(obstacle), {}, 0.0f, WHITE);

        auto drawEntity = [](const Entity& entity, const Texture& tex, const Rectangle& src, Color tint = WHITE)
        {
            const Rectangle dst{ entity.pos.x, entity.pos.y, entity.radius * 2.0f, entity.radius * 2.0f };
            DrawTexturePro(tex, src, dst, { dst.width * 0.5f, dst.height * 0.5f },
                SignedAngle({ 1.0f, 0.0f }, entity.dir) * RAD2DEG, tint);
        };

        if (cceEnabled)
        {
            drawEntity(cce, enemyTex, enemySrc, YELLOW);
            DrawHealthBar(cce);
        }

        if (rceEnabled)
        {
            drawEntity(rce, enemyTex, enemySrc, PINK);
            DrawHealthBar(rce);
        }

        drawEntity(player, playerTex, enemySrc);
        DrawLineEx(player.pos, player.pos + player.dir * 250.0f, 10.0f, playerColor);
        DrawHealthBar(player);
        
        if (IsKeyPressed(KEY_GRAVE)) useGUI = !useGUI;
        if (useGUI)
        {
            rlImGuiBegin();
            ImGui::Checkbox("Use Debug", &useDebug);

            ImGui::Checkbox("Show Points", &showPoints);
            ImGui::Checkbox("Show Obstacles", &showObstacles);
            ImGui::Checkbox("Show Entities", &showEntities);
            ImGui::Checkbox("Show Intersections", &showIntersection);
            ImGui::Checkbox("Show Avoidance", &showAvoidance);

            ImGui::Checkbox("Show CCE Tiles", &cceShowTiles);
            ImGui::Checkbox("Show RCE Tiles", &rceShowTiles);
            ImGui::Checkbox("Toggle CCE", &cceEnabled);
            ImGui::Checkbox("Toggle RCE", &rceEnabled);

            ImGui::SliderFloat2("CCE Position", (float*)&cce.pos, 0.0f, 1200.0f);
            ImGui::SliderFloat2("RCE Position", (float*)&rce.pos, 0.0f, 1200.0f);
            ImGui::SliderFloat("CCE Detection Radius", &cce.detectionRadius, 0.0f, 1500.0f);
            ImGui::SliderFloat("RCE Detection Radius", &rce.detectionRadius, 0.0f, 1500.0f);
            ImGui::SliderFloat("CCE Probe Length", &cce.probeLength, 0.0f, 250.0f);
            ImGui::SliderFloat("RCE Probe Length", &rce.probeLength, 0.0f, 250.0f);
            
            if (showPoints)
            {
                ImGui::Separator();
                if (ImGui::Button("Save Points"))
                    SavePoints(world.points);
                if (ImGui::Button("Add Point"))
                    world.points.push_back({ {}, 10.0f });
                if (ImGui::Button("Remove Point"))
                    world.points.pop_back();
                for (size_t i = 0; i < world.points.size(); i++)
                    ImGui::SliderFloat2(string("Point " + to_string(i + 1)).c_str(),
                        (float*)&world.points[i], 0.0f, 1200.0f);
            }

            if (showObstacles)
            {
                ImGui::Separator();
                if (ImGui::Button("Save Obstacles"))
                    SaveObstacles(world.obstacles);
                if (ImGui::Button("Add Obstacle"))
                    world.obstacles.push_back({ {}, 10.0f });
                if (ImGui::Button("Remove Obstacle"))
                    world.obstacles.pop_back();
                for (size_t i = 0; i < world.obstacles.size(); i++)
                    ImGui::SliderFloat3(string("Obstacle " + to_string(i + 1)).c_str(),
                        (float*)&world.obstacles[i], 0.0f, 1200.0f);
            }
            
            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    UnloadTexture(galaxyTex);
    UnloadTexture(nebulaTex);
    UnloadTexture(enemyTex);
    UnloadTexture(playerTex);
    
    UnloadSound(playerAttackSound);
    UnloadSound(rceAttackSound);
    UnloadSound(cceAttackSound);
    UnloadSound(enemyHitSound);
    UnloadSound(playerHitSound);
    UnloadSound(playerDeathSound);
    UnloadSound(enemyDeath2Sound);
    UnloadSound(enemyDeath1Sound);
    CloseAudioDevice();

    return 0;
}
