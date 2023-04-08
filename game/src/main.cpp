#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include "Grid.h"
#include "World.h"
#include "Nodes.h"
#include <iostream>

using namespace std;

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
    return false;
}

void RenderHealthBar(const Entity& entity, Color background = DARKGRAY, Color foreground = RED)
{
    const float healthBarWidth = 150.0f;
    const float healthBarHeight = 20.0f;
    const float x = entity.pos.x - healthBarWidth * 0.5f;
    const float y = entity.pos.y - (entity.radius + 30.0f);
    DrawRectangle(x, y, healthBarWidth, healthBarHeight, background);
    DrawRectangle(x, y, healthBarWidth * entity.HealthPercent(), healthBarHeight, foreground);
}

void CenterText(const char* text, Rectangle rec, int fontSize, Color color)
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

    Sound shotgunSound = LoadSound("../game/assets/audio/shotgun.wav");
    Sound sniperSound = LoadSound("../game/assets/audio/sniper.wav");

    World world;
    world.obstacles = LoadObstacles();
    world.points = LoadPoints();

    Player player;
    player.radius = 60.0f;
    player.dir = { 1.0f, 0.0f };
    player.angularSpeed = 100.0f;
    player.name = "Player";

    Enemy cce;
    cce.pos = { 1000.0f, 250.0f };
    cce.dir = { -1.0f, 0.0f };
    cce.angularSpeed = DEG2RAD * 200.0f;
    cce.point = 0;
    cce.speed = 500.0f;
    cce.radius = 50.0f;
    cce.detectionRadius = 300.0f;
    cce.probeLength = 100.0f;
    cce.combatRadius = 300.0f;
    cce.name = "Close-combat enemy";

    Enemy rce;
    rce.pos = { 10.0f, 10.0f };
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
    CloseAttackAction cceAttack(cce, shotgunSound);

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
    RangedAttackAction rceAttack (rce, sniperSound);

    // RCE tree
    Node* rceRoot = &rceIsPlayerDetected;
    rceIsPlayerDetected.no = &rcePatrol;
    rceIsPlayerDetected.yes = &rceIsPlayerVisible;
    rceIsPlayerVisible.no = &rceFindVisibility;
    rceIsPlayerVisible.yes = &rceIsPlayerCombat;
    rceIsPlayerCombat.no = &rceFlee;
    rceIsPlayerCombat.yes = &rceAttack;

    // Turn off rapid-fire shotgun while I make RCE xD
    //cceRoot = &ccePatrol;

    const Color background = RAYWHITE;
    const Color playerColor = { 0, 228, 48, 128 };          // GREEN

    const Color cceColor = { 0, 121, 241, 128 };            // BLUE
    const Color cceOverlapColor = { 0, 82, 172, 128 };      // DARKBLUE
    const Color cceVisibleColor = { 102, 191, 255, 128 };   // SKYBLUE

    const Color rceColor = { 135, 60, 190, 128 };           // VIOLET
    const Color rceOverlapColor = { 200, 122, 255, 128 };   // PURPLE
    const Color rceVisibleColor = { 255, 0, 255, 128 };     // MAGENTA

    bool useDebug = true;
    bool useGUI = false;
    bool showPoints = false;
    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        // Update player information
        if (IsKeyDown(KEY_E))
            player.dir = Rotate(player.dir, player.angularSpeed * dt * DEG2RAD);
        if (IsKeyDown(KEY_Q))
            player.dir = Rotate(player.dir, -player.angularSpeed * dt * DEG2RAD);

        player.pos = GetMousePosition();
        const Vector2 playerEnd = player.pos + player.dir * 500.0f;

        Traverse(cceRoot, player, world);
        //cce.acc = cce.acc + Avoid(cce, cce.probeLength, dt, world.obstacles);
        Integrate(cce, dt);

        Traverse(rceRoot, player, world);
        //rce.acc = rce.acc + Avoid(rce, rce.probeLength, dt, world.obstacles);
        Integrate(rce, dt);

        for (Projectile& projectile : world.projectiles)
            Integrate(projectile, dt);

        world.projectiles.erase(
            remove_if(world.projectiles.begin(), world.projectiles.end(),
                [&player, &world](const Projectile& projectile) -> bool
                {
                    if (CheckCollisionCircles(player.Collider(), projectile.Collider()))
                        return true;

                    for (const Circle& obstacle : world.obstacles)
                    {
                        if (CheckCollisionCircles(obstacle, projectile.Collider()))
                            return true;
                    }

                    return !CheckCollisionPointRec(projectile.pos, SCREEN_REC);
                }
            ),
        world.projectiles.end());

        bool playerCollision = ResolveCollisions(player, world.obstacles);
        bool cceCollision = ResolveCollisions(cce, world.obstacles);
        bool rceCollision = ResolveCollisions(rce, world.obstacles);

        vector<Vector2> intersections;
        for (const Circle& obstacle : world.obstacles)
        {
            Vector2 poi;
            if (CheckCollisionLineCircle(player.pos, playerEnd, obstacle, poi))
                intersections.push_back(poi);
        }
        bool playerIntersection = !intersections.empty();

        BeginDrawing();
        ClearBackground(background);

        // Cheap implementation of game screens (note that the update code is still running)
        auto endScreen = [](const char* message, Color background, Color foreground)
        {
            DrawRectangleRec(SCREEN_REC, background);
            CenterText(message, SCREEN_REC, 30, foreground);
            EndDrawing();
        };

        if (player.health <= 0.0f)
        {
            endScreen("You lost :(", RED, BLACK);
            continue;
        }

        if (cce.health <= 0.0f && rce.health <= 0.0f)
        {
            endScreen("You won :)", GREEN, WHITE);
            continue;
        }

        // Render debug
        if (useDebug)
        {
            Rectangle cceOverlapRec = From({ cce.pos, cce.detectionRadius });
            vector<size_t> cceVisibleTiles =
                VisibleTiles(player.Collider(), cce.detectionRadius, world.obstacles, OverlapTiles(cceOverlapRec));

            Rectangle rceOverlapRec = From({ rce.pos, rce.detectionRadius });
            vector<size_t> rceVisibleTiles =
                VisibleTiles(player.Collider(), rce.detectionRadius, world.obstacles, OverlapTiles(rceOverlapRec));
                
            //DrawRectangleRec(cceOverlapRec, cceOverlapColor);
            //for (size_t i : cceVisibleTiles)
            //    DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, cceVisibleColor);
            //
            //DrawRectangleRec(rceOverlapRec, rceOverlapColor);
            //for (size_t i : rceVisibleTiles)
            //    DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, rceVisibleColor);
        }

        // Render entities
        DrawCircleV(cce.pos, cce.radius, cceCollision ? RED : cceColor);
        DrawCircleV(rce.pos, cce.radius, rceCollision ? RED : rceColor);
        DrawCircleV(player.pos, player.radius, playerCollision ? RED : playerColor);
        DrawLineEx(cce.pos, cce.pos + cce.dir * cce.detectionRadius, 10.0f, cceColor);
        DrawLineEx(rce.pos, rce.pos + rce.dir * rce.detectionRadius, 10.0f, rceColor);
        DrawLineEx(player.pos, playerEnd, 10.0f, playerIntersection ? RED : playerColor);
        for (const Projectile& projectile : world.projectiles)
            DrawCircleV(projectile.pos, projectile.radius, RED);

        // Health bars
        RenderHealthBar(cce);
        RenderHealthBar(rce);
        RenderHealthBar(player);

        // Avoidance lines
        DrawLineEx(cce.pos, cce.pos + Rotate(Normalize(cce.vel), -30.0f * DEG2RAD) * cce.probeLength, 5.0f, cceColor);
        DrawLineEx(cce.pos, cce.pos + Rotate(Normalize(cce.vel), -15.0f * DEG2RAD) * cce.probeLength, 5.0f, cceColor);
        DrawLineEx(cce.pos, cce.pos + Rotate(Normalize(cce.vel),  15.0f * DEG2RAD) * cce.probeLength, 5.0f, cceColor);
        DrawLineEx(cce.pos, cce.pos + Rotate(Normalize(cce.vel),  30.0f * DEG2RAD) * cce.probeLength, 5.0f, cceColor);
        DrawLineEx(rce.pos, rce.pos + Rotate(Normalize(rce.vel), -30.0f * DEG2RAD) * rce.probeLength, 5.0f, rceColor);
        DrawLineEx(rce.pos, rce.pos + Rotate(Normalize(rce.vel), -15.0f * DEG2RAD) * rce.probeLength, 5.0f, rceColor);
        DrawLineEx(rce.pos, rce.pos + Rotate(Normalize(rce.vel),  15.0f * DEG2RAD) * rce.probeLength, 5.0f, rceColor);
        DrawLineEx(rce.pos, rce.pos + Rotate(Normalize(rce.vel),  30.0f * DEG2RAD) * rce.probeLength, 5.0f, rceColor);

        // Render obstacle intersections
        Vector2 obstaclesPoi;
        if (NearestIntersection(player.pos, playerEnd, world.obstacles, obstaclesPoi))
            DrawCircleV(obstaclesPoi, 10.0f, playerIntersection ? RED : playerColor);

        // Render obstacles
        for (const Circle& obstacle : world.obstacles)
            DrawCircleV(obstacle.position, obstacle.radius, GRAY);

        // Render points
        for (size_t i = 0; i < world.points.size(); i++)
        {
            const Vector2& p0 = world.points[i];
            const Vector2& p1 = world.points[(i + 1) % world.points.size()];
            DrawLineV(p0, p1, GRAY);
            DrawCircle(p0.x, p0.y, 5.0f, LIGHTGRAY);
        }
        
        // Render GUI
        if (IsKeyPressed(KEY_GRAVE)) useGUI = !useGUI;
        if (useGUI)
        {
            rlImGuiBegin();
            ImGui::Checkbox("Use heatmap", &useDebug);
            ImGui::SliderFloat2("CCE Position", (float*)&cce.pos, 0.0f, 1200.0f);
            ImGui::SliderFloat2("RCE Position", (float*)&rce.pos, 0.0f, 1200.0f);
            ImGui::SliderFloat("CCE Detection Radius", &cce.detectionRadius, 0.0f, 1500.0f);
            ImGui::SliderFloat("RCE Detection Radius", &rce.detectionRadius, 0.0f, 1500.0f);
            ImGui::SliderFloat("CCE Probe Length", &cce.probeLength, 0.0f, 250.0f);
            ImGui::SliderFloat("RCE Probe Length", &rce.probeLength, 0.0f, 250.0f);
            
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

            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    UnloadSound(sniperSound);
    UnloadSound(shotgunSound);
    CloseAudioDevice();

    return 0;
}