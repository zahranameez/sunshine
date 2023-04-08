#include "Nodes.h"
#include "Collision.h"
#include "Grid.h"
#include <cassert>
#include <iostream>
#define LOG_ACTIONS true

Node* DetectedCondition::Evaluate(const Entity& entity, World& world)
{
    return DistanceSqr(mSelf.pos, entity.pos) <= mSelf.detectionRadius * mSelf.detectionRadius ? yes : no;
}

Node* VisibleCondition::Evaluate(const Entity& entity, World& world)
{
    // Doesn't take direction/FoV into account. An omniscient AI leads to better gameplay in this case!
    Vector2 detectionEnd = mSelf.pos + Normalize(entity.pos - mSelf.pos) * mSelf.detectionRadius;
    return IsCircleVisible(mSelf.pos, detectionEnd, entity.Collider(), world.obstacles) ? yes : no;
}

Node* CloseCombatCondition::Evaluate(const Entity& entity, World& world)
{
    // CCE attacks player if within combat radius
    return DistanceSqr(mSelf.pos, entity.pos) <= mSelf.combatRadius * mSelf.combatRadius ? yes : no;
}

Node* RangedCombatCondition::Evaluate(const Entity& entity, World& world)
{
    // RCE attacks player if NOT within radius
    return DistanceSqr(mSelf.pos, entity.pos) >= mSelf.combatRadius * mSelf.combatRadius ? yes : no;
}

Node* PatrolAction::Evaluate(const Entity& entity, World& world)
{
    // Find nearest waypoint
    if (mSelf.prev != PATROL)
    {
        size_t min = 0;
        for (size_t i = 1; i < world.points.size(); i++)
        {
            if (DistanceSqr(mSelf.pos, world.points[i]) < DistanceSqr(mSelf.pos, world.points[min]))
                min = i;
        }
        mSelf.point = min;
    }
    mSelf.acc = Patrol(world.points, mSelf, mSelf.point, mSelf.speed, 200.0f, 100.0f);
    return nullptr;
}

Node* FindVisibilityAction::Evaluate(const Entity& entity, World& world)
{
    // Seek nearest enemy-visible tile
    std::vector<size_t> visibleTiles = VisibleTiles(entity.Collider(), mSelf.detectionRadius,
        world.obstacles, OverlapTiles(From({ mSelf.pos, mSelf.detectionRadius })));

    // Ensure points aren't too close to obstacles
    Points visiblePoints(visibleTiles.size());
    for (size_t i = 0; i < visiblePoints.size(); i++)
        visiblePoints[i] = GridToScreen(visibleTiles[i]) + Vector2{ TILE_WIDTH * 0.5f, TILE_HEIGHT * 0.5f };

    if (!visiblePoints.empty())
    {
        mSelf.acc = Seek(NearestPoint(mSelf.pos, visiblePoints), mSelf, mSelf.speed);
        return nullptr;
    }

    assert(mFallback != nullptr);
    return mFallback->Evaluate(entity, world);
}

Node* FindCoverAction::Evaluate(const Entity& entity, World& world)
{
    return nullptr;
}

Node* WaitAction::Evaluate(const Entity& entity, World& world)
{
    return nullptr;
}

Node* SeekAction::Evaluate(const Entity& entity, World& world)
{
    mSelf.acc = Seek(entity.pos, mSelf, mSelf.speed);
    return nullptr;
}

Node* FleeAction::Evaluate(const Entity& entity, World& world)
{
    mSelf.acc = Negate(Seek(entity.pos, mSelf, mSelf.speed));
    return nullptr;
}

Node* ArriveAction::Evaluate(const Entity& entity, World& world)
{
    mSelf.acc = Arrive(entity.pos, mSelf, mSelf.speed, 100.0f, 5.0f);
    return nullptr;
}

Node* CloseAttackAction::Evaluate(const Entity& entity, World& world)
{
    mSelf.acc = Arrive(entity.pos, mSelf, mSelf.speed, 100.0f, 5.0f);
    return nullptr;
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
    if (avoid(15.0f, acc)) return acc;
    if (avoid(-30.0f, acc)) return acc;
    if (avoid(30.0f, acc)) return acc;
    return acc;
}

Vector2 Patrol(const Points& points, const Rigidbody& rb, size_t& index, float maxSpeed, float slowRadius, float pointRadius)
{
    index = Distance(rb.pos, points[index]) <= pointRadius ? ++index % points.size() : index;
    return Arrive(points[index], rb, maxSpeed, slowRadius);
}

void Traverse(Node* node, const Entity& entity, World& world)
{
    using std::cout;
    using std::endl;

    while (node != nullptr)
    {
        if (node->IsAction())
        {
            Action* action = dynamic_cast<Action*>(node);
            ActionType type = action->Type();
            Enemy& self = action->mSelf;

            self.curr = type;
            node = node->Evaluate(entity, world);
            self.prev = type;

#if LOG_ACTIONS
            switch (type)
            {
            case PATROL:
                cout << self.name + " patrolling" << endl;
                break;

            case FIND_VISIBILITY:
                cout << self.name + " finding visibility to " + entity.name << endl;
                break;

            case FIND_COVER:
                cout << self.name + " finding cover from " + entity.name << endl;
                break;

            case SEEK:
                cout << self.name + " seeking to " + entity.name << endl;
                break;

            case FLEE:
                cout << self.name + " fleeing from " + entity.name << endl;
                break;

            case ARRIVE:
                cout << self.name + " arriving at " + entity.name << endl;
                break;

            case CLOSE_ATTACK:
                cout << self.name + " attacking " + entity.name << endl;
                break;

            case RANGED_ATTACK:
                cout << self.name + " attacking " + entity.name << endl;
                break;

            case WAIT:
                cout << self.name + " waiting. . ." << endl;
                break;
            }
#endif
        }
        else
            node = node->Evaluate(entity, world);
    }
}
