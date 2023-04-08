#pragma once
#include "raylib.h"
#include "World.h"
#include "Timer.h"

class Node;
void Traverse(Node* node, const Entity& entity, World& world, bool log = false);

class Node
{
public:
    Node(Enemy& self) : mSelf(self) {}
    virtual Node* Evaluate(const Entity& entity, World& world) = 0;
    virtual bool IsAction() = 0;

protected:
    Enemy& mSelf;
    friend void Traverse(Node* node, const Entity& entity, World& world, bool log);
};

class Condition : public Node
{
public:
    Condition(Enemy& self) : Node(self) {}
    bool IsAction() final { return false; }

    Node* yes = nullptr;
    Node* no = nullptr;
};

class Action : public Node
{
public:
    Action(Enemy& self, Action* fallback = nullptr) : Node(self), mFallback(fallback) {}
    bool IsAction() final { return true; }
    virtual ActionType Type() = 0;

protected:
    Action* mFallback = nullptr;
};

class DetectedCondition : public Condition
{
public:
    DetectedCondition(Enemy& self) : Condition(self) {}
    Node* Evaluate(const Entity& entity, World& world) final;
};

class VisibleCondition : public Condition
{
public:
    VisibleCondition(Enemy& self) : Condition(self) {}
    Node* Evaluate(const Entity& entity, World& world) final;
};

class CloseCombatCondition : public Condition
{
public:
    CloseCombatCondition(Enemy& self) : Condition(self) {}
    Node* Evaluate(const Entity& entity, World& world) final;
};

class RangedCombatCondition : public Condition
{
public:
    RangedCombatCondition(Enemy& self) : Condition(self) {}
    Node* Evaluate(const Entity& entity, World& world) final;
};

class PatrolAction : public Action
{
public:
    PatrolAction(Enemy& self) : Action(self) {}
    Node* Evaluate(const Entity& entity, World& world) final;
    ActionType Type() final { return PATROL; }
};

class FindVisibilityAction : public Action
{
public:
    FindVisibilityAction(Enemy& self, Action* fallback) : Action(self, fallback) {}
    Node* Evaluate(const Entity& entity, World& world) final;
    ActionType Type() final { return FIND_VISIBILITY; }
};

class FindCoverAction : public Action
{
public:
    FindCoverAction(Enemy& self, Action* fallback) : Action(self, fallback) {}
    Node* Evaluate(const Entity& entity, World& world) final;
    ActionType Type() final { return FIND_COVER; }
};

class WaitAction : public Action
{
public:
    WaitAction(Enemy& self, Action* fallback, float duration /*seconds*/) : Action(self, fallback)
    {
        mTimer.duration = duration;
    }

    ActionType Type() final { return WAIT; }
    Node* Evaluate(const Entity& entity, World& world) final;

private:
    Timer mTimer;
};

class SeekAction : public Action
{
public:
    SeekAction(Enemy& self) : Action(self) {}
    Node* Evaluate(const Entity& entity, World& world) final;
    ActionType Type() final { return SEEK; }
};

class FleeAction : public Action
{
public:
    FleeAction(Enemy& self) : Action(self) {}
    Node* Evaluate(const Entity& entity, World& world) final;
    ActionType Type() final { return FLEE; }
};

class ArriveAction : public Action
{
public:
    ArriveAction(Enemy& self) : Action(self) {}
    Node* Evaluate(const Entity& entity, World& world) final;
    ActionType Type() final { return ARRIVE; }
};

class CloseAttackAction : public Action
{
public:
    CloseAttackAction(Enemy& self) : Action(self) {}
    Node* Evaluate(const Entity& entity, World& world) final;
    ActionType Type() final { return CLOSE_ATTACK; }
};

class RangedAttackAction : public Action
{
public:
    RangedAttackAction(Enemy& self) : Action(self) {}
    Node* Evaluate(const Entity& entity, World& world) final;
    ActionType Type() final { return RANGED_ATTACK; }
};
