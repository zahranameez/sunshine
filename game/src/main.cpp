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

//#define LOG_DECISIONS false // <-- Logging decisions is overkill cause multiple logs will be made each frame
#define LOG_ACTIONS true

size_t ScreenToGrid(Vector2 point);
Vector2 GridToScreen(size_t index);

vector<size_t> OverlapTiles(Rectangle rectangle);
vector<size_t> VisibleTiles(Circle target, float detectionRadius, const Obstacles& obstacles, const vector<size_t>& tiles);

void SaveObstacles(const Obstacles& obstacles, const char* path = "../game/assets/data/obstacles.txt");
Obstacles LoadObstacles(const char* path = "../game/assets/data/obstacles.txt");

void SavePoints(const Points& points, const char* path = "../game/assets/data/points.txt");
Points LoadPoints(const char* path = "../game/assets/data/points.txt");

bool IsCollision(Vector2 lineStart, Vector2 lineEnd, const Obstacles& obstacles);
bool ResolveCollisions(Vector2& point, float radius, const Obstacles& obstacles);

Vector2 Avoid(const Rigidbody& rb, float probeLength, float dt, const Obstacles& obstacles);
Vector2 Patrol(const Points& points, const Rigidbody& rb, size_t& index, float maxSpeed, float slowRadius, float pointRadius);

struct Timer
{
    float elapsed = 0.0f;
    float duration = 0.0f;

    float Percent() { return elapsed / duration; }
    bool Expired() { return elapsed >= duration; }
    void Reset() { elapsed = 0.0f; }
    void Tick(float dt) { elapsed += dt; }
};

struct Entity : public Rigidbody
{
    string name;
    float radius = 0.0f;
    Circle Collider() const { return { pos, radius }; }
};

struct Enemy : public Entity
{
    float speed = 0.0f;
    size_t point = 0;

    float detectionRadius = 0.0f;
    float combatRadius = 0.0f;
    float probeLength = 0.0f;

    // Used to ensure we don't do stuff like fire projectiles every frame of the attack action
    enum ActionType
    {
        PATROL,
        FIND_VISIBILITY,
        FIND_COVER,
        SEEK,
        FLEE,
        ARRIVE,
        CLOSE_ATTACK,
        RANGED_ATTACK,
        WAIT,
    } prev = WAIT, curr = WAIT;
};

struct Player : public Entity {};

struct World
{
    Points points;
    Obstacles obstacles;
    vector<Entity> projectiles;
};

class Node
{
public:
    Node(Enemy& self) : mSelf(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) = 0;

protected:
    Enemy& mSelf;
};

class Condition : public Node
{
public:
    Condition(Enemy& self) : Node(self) {}
    Node* yes = nullptr;
    Node* no = nullptr;
};

class Action : public Node
{
public:
    Action(Enemy& self, Action* fallback = nullptr) : Node(self), mFallback(fallback) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        // nullptr because an action node is a leaf!
        return nullptr;
    }
    
protected:
    Action* mFallback = nullptr;

    void Log(const string& message)
    {
#if LOG_ACTIONS
        cout << message << endl;
#endif
    }
};

class DetectedCondition : public Condition
{
public:
    DetectedCondition(Enemy& self) : Condition(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        return DistanceSqr(mSelf.pos, entity.pos) <= mSelf.detectionRadius * mSelf.detectionRadius ? yes : no;
    }
};

class VisibleCondition : public Condition
{
public:
    VisibleCondition(Enemy& self) : Condition(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        // Doesn't take direction/FoV into account. An omniscient AI leads to better gameplay in this case!
        Vector2 detectionEnd = mSelf.pos + Normalize(entity.pos - mSelf.pos) * mSelf.detectionRadius;
        return IsCircleVisible(mSelf.pos, detectionEnd, entity.Collider(), world.obstacles) ? yes : no;
    }
};

class CloseCombatCondition : public Condition
{
public:
    CloseCombatCondition(Enemy& self) : Condition(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        // CCE attacks player if within combat radius
        return DistanceSqr(mSelf.pos, entity.pos) <= mSelf.combatRadius * mSelf.combatRadius ? yes : no;
    }
};

class RangedCombatCondition : public Condition
{
public:
    RangedCombatCondition(Enemy& self) : Condition(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        // RCE attacks player if NOT within radius
        return DistanceSqr(mSelf.pos, entity.pos) >= mSelf.combatRadius * mSelf.combatRadius ? yes : no;
    }
};

class PatrolAction : public Action
{
public:
    PatrolAction(Enemy& self) : Action(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        mSelf.acc = Patrol(world.points, mSelf, mSelf.point, mSelf.speed, 200.0f, 100.0f);
        Log(mSelf.name + " patrolling");
        return nullptr;
    }
};

class FindVisibilityAction : public Action
{
public:
    FindVisibilityAction(Enemy& self, Action* fallback) :
        Action(self, fallback) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        // Seek nearest enemy-visible tile
        vector<size_t> visibleTiles = VisibleTiles(entity.Collider(), mSelf.detectionRadius,
            world.obstacles, OverlapTiles(From({ mSelf.pos, mSelf.detectionRadius })));
        
        // Ensure points aren't too close to obstacles
        Points visiblePoints(visibleTiles.size());
        for (size_t i = 0; i < visiblePoints.size(); i++)
            visiblePoints[i] = GridToScreen(visibleTiles[i]) + Vector2{TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f};

        if (!visiblePoints.empty())
        {
            Log(mSelf.name + " finding visibility to " + entity.name);
            mSelf.acc = Seek(NearestPoint(mSelf.pos, visiblePoints), mSelf, mSelf.speed);
            return nullptr;
        }

        assert(mFallback != nullptr);
        Log(mSelf.name + " falling back");
        return mFallback->Evaluate(entity, world);
    }
};

class FindCoverAction : public Action
{
public:
    FindCoverAction(Enemy& self, Action* fallback) : Action(self, fallback) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        Log(mSelf.name + " finding cover from " + entity.name);
        return nullptr;
    }
};

class WaitAction : public Action
{
public:
    WaitAction(Enemy& self, Action* fallback, float duration /*seconds*/) : Action(self, fallback)
    {
        mTimer.duration = duration;
    }

    virtual Node* Evaluate(const Entity& entity, World& world) override
    {

        mTimer.Tick(GetFrameTime());
        Log(mSelf.name + " waiting: " + to_string(mTimer.Percent() * 100.0f) + "%");
        return nullptr;
    }

private:
    Timer mTimer;
};

class SeekAction : public Action
{
public:
    SeekAction(Enemy& self) : Action(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        mSelf.acc = Seek(entity.pos, mSelf, mSelf.speed);
        Log(mSelf.name + " seeking to " + entity.name);
        return nullptr;
    }
};

class FleeAction : public Action
{
public:
    FleeAction(Enemy& self) : Action(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        mSelf.acc = Negate(Seek(entity.pos, mSelf, mSelf.speed));
        Log(mSelf.name + " fleeing from " + entity.name);
        return nullptr;
    }
};

class ArriveAction : public Action
{
public:
    ArriveAction(Enemy& self) : Action(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        mSelf.acc = Arrive(entity.pos, mSelf, mSelf.speed, 100.0f, 5.0f);
        Log(mSelf.name + " arriving at " + entity.name);
        return nullptr;
    }
};

class CloseAttackAction : public Action
{
public:
    CloseAttackAction(Enemy& self) : Action(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) override
    {
        mSelf.acc = Arrive(entity.pos, mSelf, mSelf.speed, 100.0f, 5.0f);
        Log(mSelf.name + " attacking " + entity.name);
        return nullptr;
    }
};

void Traverse(Node* node, const Entity& entity, World& world)
{
    while (node != nullptr)
        node = node->Evaluate(entity, world);
}

int main(void)
{
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
    cce.angularSpeed = DEG2RAD * 100.0f;
    cce.point = 0;
    cce.speed = 300.0f;
    cce.radius = 50.0f;
    cce.detectionRadius = 300.0f;
    cce.probeLength = 100.0f;
    cce.combatRadius = 100.0f;
    cce.name = "Close-combat enemy";

    Enemy rce;
    rce.pos = { 10.0f, 10.0f };
    rce.dir = { 1.0f, 0.0f };
    rce.angularSpeed = DEG2RAD * 100.0f;
    rce.point = 0;
    rce.speed = 300.0f;
    rce.radius = 50.0f;
    rce.detectionRadius = 300.0f;
    rce.probeLength = 100.0f;
    rce.combatRadius = 400.0f;
    rce.name = "Ranged-combat enemy";

    DetectedCondition cceIsPlayerDetected(cce);
    VisibleCondition cceIsPlayerVisible(cce);
    CloseCombatCondition cceIsPlayerCombat(cce);

    PatrolAction ccePatrol(cce);
    FindVisibilityAction cceFindVisibility(cce, &ccePatrol);
    ArriveAction cceArrive(cce);
    CloseAttackAction cceAttack(cce);

    Node* cceRoot = &cceIsPlayerDetected;
    cceIsPlayerDetected.no = &ccePatrol;
    cceIsPlayerDetected.yes = &cceIsPlayerVisible;
    cceIsPlayerVisible.no = &cceFindVisibility;
    cceIsPlayerVisible.yes = &cceIsPlayerCombat;
    cceIsPlayerCombat.no = &cceArrive;
    cceIsPlayerCombat.yes = &cceAttack;

    const Color background = RAYWHITE;
    const Color playerColor = { 0, 228, 48, 128 };          // GREEN

    const Color cceColor = { 0, 121, 241, 128 };            // BLUE
    const Color cceOverlapColor = { 0, 82, 172, 128 };      // DARKBLUE
    const Color cceVisibleColor = { 102, 191, 255, 128 };   // SKYBLUE

    const Color rceColor = { 135, 60, 190, 128 };           // VIOLET
    const Color rceOverlapColor = { 112, 31, 126, 128 };    // DARKPURPLE
    const Color rceVisibleColor = { 200, 122, 255, 128 };   // PURPLE

    bool useDebug = true;
    bool useGUI = false;
    bool showPoints = false;
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sunshine");
    rlImGuiSetup(true);
    SetTargetFPS(60);
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
        cce.acc = cce.acc + Avoid(cce, cce.probeLength, dt, world.obstacles);
        Integrate(cce, dt);

        rce.acc = Patrol(world.points, rce, rce.point, rce.speed, 200.0f, 100.0f);
        rce.acc = rce.acc + Avoid(rce, rce.probeLength, dt, world.obstacles);
        Integrate(rce, dt);

        bool playerCollision = ResolveCollisions(player.pos, player.radius, world.obstacles);
        bool cceCollision = ResolveCollisions(cce.pos, cce.radius, world.obstacles);
        bool rceCollision = ResolveCollisions(rce.pos, rce.radius, world.obstacles);

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

        // Render debug
        if (useDebug)
        {
            Rectangle cceOverlapRec = From({ cce.pos, cce.detectionRadius });
            vector<size_t> cceVisibleTiles =
                VisibleTiles(player.Collider(), cce.detectionRadius, world.obstacles, OverlapTiles(cceOverlapRec));

            Rectangle rceOverlapRec = From({ rce.pos, rce.detectionRadius });
            vector<size_t> rceVisibleTiles =
                VisibleTiles(player.Collider(), rce.detectionRadius, world.obstacles, OverlapTiles(rceOverlapRec));
                
            DrawRectangleRec(rceOverlapRec, rceOverlapColor);
            for (size_t i : cceVisibleTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, cceVisibleColor);

            DrawRectangleRec(cceOverlapRec, cceOverlapColor);
            for (size_t i : rceVisibleTiles)
                DrawRectangleV(GridToScreen(i), { TILE_WIDTH, TILE_HEIGHT }, rceVisibleColor);
        }

        // Render entities
        DrawCircle(cce.Collider(), cceCollision ? RED : cceColor);
        DrawCircle(rce.Collider(), rceCollision ? RED : rceColor);
        DrawCircle(player.Collider(), playerCollision ? RED : playerColor);
        DrawLineEx(cce.pos, cce.pos + cce.dir * cce.detectionRadius, 10.0f, cceColor);
        DrawLineEx(rce.pos, rce.pos + rce.dir * rce.detectionRadius, 10.0f, rceColor);
        DrawLineEx(player.pos, playerEnd, 10.0f, playerIntersection ? RED : playerColor);

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
            DrawCircle(obstacle, GRAY);

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
    if (rectangle.x < 0.0f) rectangle.x = 0.0f;
    if (rectangle.y < 0.0f) rectangle.y = 0.0f;
    if (rectangle.x + rectangle.width > SCREEN_WIDTH) rectangle.x = SCREEN_WIDTH - rectangle.width;
    if (rectangle.y + rectangle.height > SCREEN_HEIGHT) rectangle.y = SCREEN_HEIGHT - rectangle.height;

    const size_t colMin = rectangle.x / TILE_WIDTH;
    const size_t rowMin = rectangle.y / TILE_HEIGHT;
    const size_t colMax = (rectangle.x + rectangle.width) / TILE_WIDTH;
    const size_t rowMax = (rectangle.y + rectangle.height) / TILE_HEIGHT;

    vector<size_t> indices;
    indices.reserve((colMax - colMin) * (rowMax - rowMin));
    for (size_t row = rowMin; row < rowMax; row++)
    {
        for (size_t col = colMin; col < colMax; col++)
        {
            indices.push_back(row * GRID_LENGTH + col);
        }
    }
    return indices;
}

vector<size_t> VisibleTiles(Circle target, float detectionRadius,
    const Obstacles& obstacles, const vector<size_t>& tiles)
{
    vector<size_t> visibilityTiles;
    visibilityTiles.reserve(tiles.size());
    for (size_t i : tiles)
    {
        Vector2 tileCenter = GridToScreen(i) + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };
        Vector2 tileEnd = tileCenter + Normalize(target.position - tileCenter) * detectionRadius;
        if (IsCircleVisible(tileCenter, tileEnd, target, obstacles)) visibilityTiles.push_back(i);
    }
    return visibilityTiles;
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

bool IsCollision(Vector2 lineStart, Vector2 lineEnd, const Obstacles& obstacles)
{
    for (const Circle& obstacle : obstacles)
    {
        if (CheckCollisionLineCircle(lineStart, lineEnd, obstacle))
            return true;
    }
    return false;
}

bool ResolveCollisions(Vector2& position, float radius, const Obstacles& obstacles)
{
    for (const Circle& obstacle : obstacles)
    {
        Vector2 mtv;
        if (CheckCollisionCircles(obstacle, {position, radius}, mtv))
        {
            position = position + mtv;
            return true;
        }
    }
    return false;
}

Vector2 Avoid(const Rigidbody& rb, float probeLength, float dt, const Obstacles& obstacles)
{
    auto avoid = [&](float angle, Vector2& acc) -> bool
    {
        if (IsCollision(rb.pos, rb.pos + Rotate(Normalize(rb.vel), angle * DEG2RAD) * probeLength, obstacles))
        {
            const Vector2 vf = Rotate(Normalize(rb.vel), rb.angularSpeed * dt * Sign(-angle)) * Length(rb.vel);
            acc = Acceleration(rb.vel, vf, dt);
            return true;
        }
        return false;
    };

    Vector2 acc{};
    if (avoid(-15.0f, acc)) return acc;
    if (avoid( 15.0f, acc)) return acc;
    if (avoid(-30.0f, acc)) return acc;
    if (avoid( 30.0f, acc)) return acc;
    return acc;
}

Vector2 Patrol(const Points& points, const Rigidbody& rb, size_t& index, float maxSpeed, float slowRadius, float pointRadius)
{
    index = Distance(rb.pos, points[index]) <= pointRadius ? ++index % points.size() : index;
    return Arrive(points[index], rb, maxSpeed, slowRadius);
}

// Sooooooo much less code thant the decision tree...
// Perhaps give this to students and ask them to implement this as a decision tree for future assignment 3?
/*
void Melee(Rigidbody& enemy, EnemyData& enemyData, const Rigidbody& player, const PlayerData& playerData,
    const Points& points, const Obstacles& obstacles)
{
    const Circle playerCircle{ player.pos, playerData.radius };

    // Player detected?
    if (CheckCollisionCircles({ enemy.pos, enemyData.sightDistance }, playerCircle))
    {
        // Player visible? (No FoV check or rotate till in FoV because this is complicated enough already)...
        if (IsCircleVisible(enemy.pos, enemy.pos + Normalize(player.pos - enemy.pos) * enemyData.sightDistance,
            playerCircle, obstacles))
        {
            // Within combat distance?
            if (CheckCollisionCircles({ enemy.pos, enemyData.combatDistance }, playerCircle))
            {
                // Close attack (must still call arrive)
                enemy.acc = Arrive(player.pos, enemy, enemyData.speed, 100.0f, 5.0f);
                printf("Melee attacking player\n");
            }
            else
            {
                // Seek player
                enemy.acc = Arrive(player.pos, enemy, enemyData.speed, 100.0f, 5.0f);
                printf("Melee seeking player\n");
            }
        }
        else
        {
            // Seek nearest visible tile
            Rectangle area = From({ enemy.pos, enemyData.radius });
            vector<size_t> overlap = OverlapTiles(area);
            vector<size_t> visible = VisibleTiles(playerCircle, enemyData.sightDistance, obstacles, overlap);
            Points points(visible.size());
            for (size_t i = 0; i < points.size(); i++)
                points[i] = GridToScreen(visible[i]);
            if (!points.empty())
                enemy.acc = Seek(NearestPoint(enemy.pos, points), enemy, enemyData.speed);
            printf("Melee seeking visibility\n");
        }
    }
    else
    {
        enemy.acc = Patrol(points, enemy, enemyData.point, enemyData.speed, 200.0f, 100.0f);
        printf("Melee patrolling\n");
    }
}

void Ranged(Rigidbody& enemy, EnemyData& enemyData, const Rigidbody& player, const PlayerData& playerData,
    const Points& points, const Obstacles& obstacles)
{

}
*/