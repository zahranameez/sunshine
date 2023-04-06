#include "rlImGui.h"
#include "Physics.h"
#include "Collision.h"

#include <string>
#include <fstream>
#include <iostream>

using namespace std;

constexpr int GRID_LENGTH = 80;
constexpr int GRID_LENGTH_SQR = GRID_LENGTH * GRID_LENGTH;
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TILE_WIDTH = SCREEN_WIDTH / GRID_LENGTH;
constexpr int TILE_HEIGHT = SCREEN_HEIGHT / GRID_LENGTH;

size_t ScreenToGrid(Vector2 point);
Vector2 GridToScreen(size_t index);

vector<size_t> OverlapTiles(Rectangle rectangle);
vector<size_t> VisibleTiles(Circle target, float sightDistance,
    const Obstacles& obstacles, const vector<size_t>& tiles);

bool IsCollision(Vector2 lineStart, Vector2 lineEnd, const Obstacles& obstacles);
bool ResolveCollisions(Circle& circle, const Obstacles& obstacles);

bool Avoid(Vector2& position, Vector2& direction, float maxRadians, float probeDistance,
    const Obstacles& obstacles);

void SaveObstacles(const Obstacles& obstacles, const char* path = "../game/assets/data/obstacles.txt");
Obstacles LoadObstacles(const char* path = "../game/assets/data/obstacles.txt");

void SavePoints(const Points& points, const char* path = "../game/assets/data/points.txt");
Points LoadPoints(const char* path = "../game/assets/data/points.txt");

void Patrol(const Points& points, Rigidbody& rb, size_t& index, float maxSpeed, float dt, float slowRadius, float pointRadius);

int main(void)
{
    Obstacles obstacles = LoadObstacles();
    Points points = LoadPoints();
    size_t point = 0;

    Circle player{ {}, 60.0f };
    Vector2 playerDirection{ 1.0f, 0.0f };
    const float playerRotationSpeed = 100.0f;

    float enemySightDistance = 300.0f;
    float enemyProbeDistance = 100.0f;
    const float enemyRadius = 50.0f;
    const float enemySpeed = 300.0f;
    const float enemyRotationSpeed = 200.0f;
    Rigidbody cce;
    Rigidbody rce;
    cce.pos = { 1000.0f, 250.0f };
    rce.pos = { 10.0f, 10.0f };
    cce.dir = { -1.0f, 0.0f };
    rce.dir = { 1.0f, 0.0f };
    cce.angularSpeed = rce.angularSpeed = enemyRotationSpeed;

    const Color playerColor = GREEN;
    const Color cceColor = BLUE;
    const Color rceColor = VIOLET;
    const Color background = RAYWHITE;

    bool useDebug = true;
    bool useGUI = false;
    bool showPoints = false;
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sunshine");
    rlImGuiSetup(true);
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();
        const float playerRotationDelta = playerRotationSpeed * dt * DEG2RAD;

        // Update player information
        if (IsKeyDown(KEY_E))
            playerDirection = Rotate(playerDirection, playerRotationDelta);
        if (IsKeyDown(KEY_Q))
            playerDirection = Rotate(playerDirection, -playerRotationDelta);

        player.position = GetMousePosition();
        const Vector2 playerEnd = player.position + playerDirection * 500.0f;

        Patrol(points, rce, point, enemySpeed, dt, 200.0f, 100.0f);
        cce.acc = Arrive(player.position, cce, enemySpeed, 100.0f, 5.0f);
        Integrate(cce, dt);

        Circle cceCircle{ cce.pos, enemyRadius };
        Circle rceCircle{ rce.pos, enemyRadius };

        bool playerCollision = ResolveCollisions(player, obstacles);
        bool cceCollision = ResolveCollisions(cceCircle, obstacles);
        bool rceCollision = ResolveCollisions(rceCircle, obstacles);

        vector<size_t> cceOverlapTiles = OverlapTiles(From(cceCircle));
        vector<size_t> rceOverlapTiles = OverlapTiles(From(rceCircle));
        vector<size_t> cceVisibleTiles = VisibleTiles(player, enemySightDistance, obstacles, cceOverlapTiles);
        vector<size_t> rceVisibleTiles = VisibleTiles(player, enemySightDistance, obstacles, rceOverlapTiles);

        vector<Vector2> intersections;
        for (const Circle& obstacle : obstacles)
        {
            Vector2 poi;
            if (CheckCollisionLineCircle(player.position, playerEnd, obstacle, poi))
                intersections.push_back(poi);
        }
        bool playerIntersection = !intersections.empty();

        BeginDrawing();
        ClearBackground(background);

        // Render debug
        if (useDebug)
        {
            for (size_t i : cceOverlapTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, DARKBLUE);

            for (size_t i : cceVisibleTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, SKYBLUE);

            for (size_t i : rceOverlapTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, DARKPURPLE);

            for (size_t i : rceVisibleTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, PURPLE);
        }

        // Render entities
        DrawCircle(cceCircle, cceCollision ? RED : cceColor);
        DrawCircle(rceCircle, rceCollision ? RED : rceColor);
        DrawCircle(player, playerCollision ? RED : playerColor);
        DrawLineV(player.position, playerEnd, playerIntersection ? RED : playerColor);
        DrawLineV(cce.pos, cce.pos + cce.dir * enemySightDistance, cceColor);
        DrawLineV(rce.pos, rce.pos + rce.dir * enemySightDistance, rceColor);

        // Render obstacle intersections
        Vector2 obstaclesPoi;
        if (NearestIntersection(player.position, playerEnd, obstacles, obstaclesPoi))
            DrawCircleV(obstaclesPoi, 10.0f, playerIntersection ? RED : playerColor);

        // Render obstacles
        for (const Circle& obstacle : obstacles)
            DrawCircle(obstacle, GRAY);

        // Render points
        for (size_t i = 0; i < points.size(); i++)
        {
            const Vector2& p0 = points[i];
            const Vector2& p1 = points[(i + 1) % points.size()];
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
            ImGui::SliderFloat("Sight Distance", &enemySightDistance, 0.0f, 1500.0f);
            ImGui::SliderFloat("Probe Distance", &enemyProbeDistance, 0.0f, 1500.0f);
            
            ImGui::Separator();
            if (ImGui::Button("Save Obstacles"))
                SaveObstacles(obstacles);
            if (ImGui::Button("Add Obstacle"))
                obstacles.push_back({ {}, 10.0f });
            if (ImGui::Button("Remove Obstacle"))
                obstacles.pop_back();
            for (size_t i = 0; i < obstacles.size(); i++)
                ImGui::SliderFloat3(string("Obstacle " + to_string(i + 1)).c_str(), (float*)&obstacles[i], 0.0f, 1200.0f);

            ImGui::Separator();
            if (ImGui::Button("Save Points"))
                SavePoints(points);
            if (ImGui::Button("Add Point"))
                points.push_back({ {}, 10.0f });
            if (ImGui::Button("Remove Point"))
                points.pop_back();
            for (size_t i = 0; i < points.size(); i++)
                ImGui::SliderFloat2(string("Point " + to_string(i + 1)).c_str(), (float*)&points[i], 0.0f, 1200.0f);

            rlImGuiEnd();
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}

size_t ScreenToGrid(Vector2 point)
{
    size_t col = point.x / TILE_WIDTH;
    size_t row = point.y / TILE_HEIGHT;
    return row * GRID_LENGTH + col;
}

Vector2 GridToScreen(size_t index)
{
    size_t col = index % GRID_LENGTH;
    size_t row = index / GRID_LENGTH;
    return { float(col * TILE_WIDTH), float(row * TILE_HEIGHT) };
}

vector<size_t> OverlapTiles(Rectangle rectangle)
{
    const size_t colMin = rectangle.x / TILE_WIDTH;
    const size_t colMax = ((rectangle.x + rectangle.width) / TILE_WIDTH) > GRID_LENGTH ?
        GRID_LENGTH : (rectangle.x + rectangle.width) / TILE_WIDTH;
    const size_t rowMin = rectangle.y / TILE_HEIGHT;
    const size_t rowMax = (rectangle.y + rectangle.height) / TILE_HEIGHT;
    const size_t width = colMax - colMin;
    const size_t height = rowMax - rowMin;

    vector<size_t> indices;
    indices.reserve(width * height);
    for (size_t row = rowMin; row < rowMax; row++)
    {
        for (size_t col = colMin; col < colMax; col++)
        {
            indices.push_back(row * GRID_LENGTH + col);
        }
    }
    return indices;
}

vector<size_t> VisibleTiles(Circle target, float sightDistance,
    const Obstacles& obstacles, const vector<size_t>& tiles)
{
    vector<size_t> visibilityTiles;
    visibilityTiles.reserve(tiles.size());
    for (size_t i : tiles)
    {
        Vector2 tileCenter = GridToScreen(i) + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };
        Vector2 tileEnd = tileCenter + Normalize(target.position - tileCenter) * sightDistance;
        if (IsCircleVisible(tileCenter, tileEnd, target, obstacles)) visibilityTiles.push_back(i);
    }
    return visibilityTiles;
}

bool IsCollision(Vector2 lineStart, Vector2 lineEnd, const Obstacles& obstacles)
{
    for (const Circle& obstacle : obstacles)
    {
        if (CheckCollisionLineCircle(lineStart, lineEnd, obstacle))
            return true;
    }
    return false;
}

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
    return false;
}

bool Avoid(Vector2& position, Vector2& direction, float maxRadians, float probeDistance, const Obstacles& obstacles)
{
    const float near = 15.0f * DEG2RAD;
    const float far = 30.0f * DEG2RAD;
    const Vector2 nearLeft = position + Rotate(direction, -near) * probeDistance;
    const Vector2 nearRight = position + Rotate(direction, near) * probeDistance;
    const Vector2 farLeft = position + Rotate(direction, -far) * probeDistance;
    const Vector2 farRight = position + Rotate(direction, far) * probeDistance;

    if (IsCollision(position, nearLeft, obstacles))
    {
        direction = Rotate(direction, maxRadians);
        return true;
    }

    if (IsCollision(position, nearRight, obstacles))
    {
        direction = Rotate(direction, -maxRadians);
        return true;
    }

    if (IsCollision(position, farLeft, obstacles))
    {
        direction = Rotate(direction, maxRadians);
        return true;
    }

    if (IsCollision(position, farRight, obstacles))
    {
        direction = Rotate(direction, -maxRadians);
        return true;
    }

    return false;
}

void SaveObstacles(const Obstacles& obstacles, const char* path)
{
    ofstream file(path, ios::out | ios::trunc);
    for (const Circle& obstacle : obstacles)
        file << obstacle.position.x << " " << obstacle.position.y << " " << obstacle.radius << endl;
    file.close();
}

Obstacles LoadObstacles(const char* path)
{
    Obstacles obstacles;
    ifstream file(path);
    while (!file.eof())
    {
        Circle obstacle;
        file >> obstacle.position.x >> obstacle.position.y >> obstacle.radius;
        obstacles.push_back(std::move(obstacle));
    }
    file.close();
    return obstacles;
}

void SavePoints(const Points& points, const char* path)
{
    ofstream file(path, ios::out | ios::trunc);
    for (const Vector2& point : points)
        file << point.x << " " << point.y << endl;
    file.close();
}

Points LoadPoints(const char* path)
{
    Points points;
    ifstream file(path);
    while (!file.eof())
    {
        Vector2 point;
        file >> point.x >> point.y;
        points.push_back(std::move(point));
    }
    file.close();
    return points;
}

void Patrol(const Points& points, Rigidbody& rb, size_t& index, float maxSpeed, float dt, float slowRadius, float pointRadius)
{
    const Vector2& target = points[index];
    rb.acc = Arrive(target, rb, maxSpeed, slowRadius);
    Integrate(rb, dt);
    index = Distance(rb.pos, target) <= pointRadius ? ++index % points.size() : index;
}
