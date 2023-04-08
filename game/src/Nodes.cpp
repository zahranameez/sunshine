#include "Nodes.h"
#include "Collision.h"
#include "Grid.h"
#include <cassert>
#include <iostream>

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

    size_t& index = mSelf.point;
    index = Distance(mSelf.pos, world.points[index]) <= 100.0f ? ++index % world.points.size() : index;
    mSelf.acc = Arrive(world.points[index], mSelf, mSelf.speed, 200);
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

void Traverse(Node* node, const Entity& entity, World& world, bool log)
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

            if (log)
            {
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
            }
        }
        else
            node = node->Evaluate(entity, world);
    }
}
